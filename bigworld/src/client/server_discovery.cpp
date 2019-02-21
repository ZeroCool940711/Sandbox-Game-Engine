/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"
#include "server_discovery.hpp"

#include "network/endpoint.hpp"
#include "network/machine_guard.hpp"
#include "network/portmap.hpp"

#include "connection_control.hpp"
#include "common/servconn.hpp"
#include "common/login_interface.hpp"


DECLARE_DEBUG_COMPONENT2( "Server", 0 )

// -----------------------------------------------------------------------------
// Section: ProbeReplyHandler
// -----------------------------------------------------------------------------

class ProbeReplyHandler : public Mercury::ReplyMessageHandler
{
public:
	ProbeReplyHandler( ServerDiscovery * pSD, const Mercury::Address & addr ) :
		sd_( pSD ),
		addr_( addr )
 		{}

	virtual void handleMessage( const Mercury::Address & /*source*/,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * /*arg*/ )
	{
		BW_GUARD;
		ServerDiscovery::DetailsPtr d;
		for (uint i = 0; (d=sd_->details( i )); i++)
			if (d->ip_ == htonl(addr_.ip) && d->port_ == htons(addr_.port))
				break;
		if (d)
		{
			std::string key, value;
			while (header.length > 0)
			{
				data >> key >> value;
				header.length -= key.length() + value.length() + 2;
				if (key == PROBE_KEY_HOST_NAME)
					{ d->hostName_ = value; }
				else if (key == PROBE_KEY_OWNER_NAME)
					{ d->ownerName_ = value;	d->ownerValid_ = true; }
				else if (key == PROBE_KEY_USERS_COUNT)
					{ d->usersCount_ = atoi( value.c_str() );
						d->usersCountValid_ = true; }
				else if (key == PROBE_KEY_UNIVERSE_NAME)
					{ d->universeName_ = value;	d->universeValid_ = true; }
				else if (key == PROBE_KEY_SPACE_NAME)
					{ d->spaceName_ = value;	d->spaceValid_ = true; }
			}

			if (sd_->changeNotifier())
			{
				Py_INCREF( sd_->changeNotifier() );
				Script::call( sd_->changeNotifier(), PyTuple_New(0),
					"ServerDiscovery changeNotifier in probe: " );
			}
		}
		else
		{
			data.retrieve( header.length );
			ERROR_MSG( "ServerDiscovery: "
				"Got probe reply from %s but no details found!\n",
				static_cast<char*>( addr_ ) );
		}
		delete this;
	}

	/**
	 * 	This method is called by Mercury when the request fails. The
	 * 	normal reason for this happening is a timeout.
	 *
	 * 	@param exception	The reason for failure.
	 * 	@param arg			The user-defined data associated with the request.
	 */
	virtual void handleException( const Mercury::NubException & /*exception*/,
		void * /*arg*/ )
	{
		BW_GUARD;
		delete this;
	}

private:
	SmartPointer<ServerDiscovery>	sd_;
	Mercury::Address				addr_;
};

// -----------------------------------------------------------------------------
// Section: ServerDiscovery
// -----------------------------------------------------------------------------

int ServerDiscovery_token = 0;

/*~ attribute ServerDiscovery servers
 *  This is a list of ServerDiscovery::Details objects which contain status
 *  information for each of the servers that the ServerDiscovery object has
 *  found.
 *  @type Read-only list of ServerDiscovery objects
 */
/*~ attribute ServerDiscovery searching
 *  This attribute can be used to initiate a search for BigWorld servers on
 *  the LAN. When this is set to 1 it initiates a search, and when it is set to
 *  0 it stops the current search and clears the servers attribute.
 *  @type Integer. Default is 0.
 */
/*~ attribute ServerDiscovery changeNotifier
 *  The changeNotifier attribute is a Python function object which will be
 *  called every time a ServerDiscovery::Details object is created or updated
 *  in the servers attribute list. It is called with no arguments.
 *  @type Callback Function (python function object, or an instance of a python
 *  class that implements the __call__ interface). Default is None.
 */
PY_TYPEOBJECT( ServerDiscovery )

PY_BEGIN_ATTRIBUTES( ServerDiscovery )
	PY_ATTRIBUTE( servers )
	PY_ATTRIBUTE( searching )
	PY_ATTRIBUTE( changeNotifier )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( ServerDiscovery )
PY_END_METHODS()

#pragma warning (disable:4355)

/**
 *	Constructor.
 */
ServerDiscovery::ServerDiscovery( PyTypePlus * pType ) :
	PyObjectPlus( pType ),
	pEP_( NULL ),
	detailsHolder_( details_, this, false ),
	searching_( false )
{
	BW_GUARD;
	MainLoopTasks::root().add( this, "Device/ServerDiscovery", ">App", NULL );
}


/**
 *	Destructor.
 */
ServerDiscovery::~ServerDiscovery()
{
	BW_GUARD;
	this->searching( false );
	MainLoopTasks::root().del( this, "Device/ServerDiscovery" );
}

/**
 *	MainLoopTask fini method
 */
void ServerDiscovery::fini()
{
	BW_GUARD;
	// Note: This method is not normally called - we are destructed along
	// with all the other python stuff, in the Script group.
	this->searching( false );
}

/**
 *	MainLoopTask tick method
 */
void ServerDiscovery::tick( float /*dTime*/ )
{
	BW_GUARD;
	// do stuff with pEP_;
	if (!searching_) return;
	MF_ASSERT_DEV( pEP_ != NULL );

	unsigned detCount = details_.size();
	while (this->recv());	// receive messages
	if (details_.size() > detCount && changeNotifier_)
	{
		Py_INCREF( &*changeNotifier_ );
		Script::call( &*changeNotifier_, PyTuple_New(0),
			"ServerDiscovery changeNotifier: " );
	}
}

/**
 *	Searching set method
 */
void ServerDiscovery::searching( bool go )
{
	BW_GUARD;
	if (searching_ == go) return;
	searching_ = go;

	if (!searching_)
	{
		details_.clear();
		delete pEP_;
		pEP_ = NULL;
	}
	else
	{
		pEP_ = new Endpoint();
		pEP_->socket( SOCK_DGRAM );
		pEP_->bind();
		pEP_->setbroadcast( true );
		pEP_->setnonblocking( true );
		if (!pEP_->good())
		{
			delete pEP_;
			pEP_ = NULL;
			searching_ = false;
			ERROR_MSG( "ServerDiscovery: Could not make endpoint\n" );
		}
		else
		{
			this->sendFindMGM();
		}
	}
}


/**
 *	This private method sends a 'find' machine guard message.
 */
void ServerDiscovery::sendFindMGM()
{
	BW_GUARD;
	ProcessStatsMessage psm;
	psm.param_ = psm.PARAM_USE_CATEGORY | psm.PARAM_USE_NAME;
	psm.category_ = psm.SERVER_COMPONENT;
	psm.name_ = "LoginInterface";
	psm.sendto( *pEP_, htons( PORT_MACHINED ) );
}

/**
 *	This private method recevies an outstanding packet.  It will return true
 *  if it is able to read any data off the wire, whether or not that data is
 *  useful.
 */
bool ServerDiscovery::recv()
{
	BW_GUARD;
	char recvbuf[ MGMPacket::MAX_SIZE ];
	uint32 srcaddr;
	int len = pEP_->recvfrom( recvbuf, sizeof( recvbuf ), NULL, &(u_int32_t&)srcaddr );
	if (len <= 0) return false;

	MemoryIStream is( recvbuf, len );
	MGMPacket packet( is );
	if (is.error())
	{
		ERROR_MSG( "ServerDiscovery::recv: Garbage MGM received\n" );
		return true;
	}

	for (unsigned i=0; i < packet.messages_.size(); i++)
	{
		ProcessStatsMessage &psm =
			static_cast< ProcessStatsMessage& >( *packet.messages_[i] );

		// Verify message type
		if (psm.message_ != psm.PROCESS_STATS_MESSAGE)
		{
			ERROR_MSG( "ServerDiscovery::recv: Unexpected message type %d\n",
				psm.message_ );
			continue;
		}

		// pid == 0 means no process found
		if (psm.pid_ == 0)
			continue;

		// ok, now parse the mgm
		DetailsPtr det( new Details(), true );
		det->ip_ = ntohl( srcaddr );
		det->port_ = ntohs( psm.port_ );
		det->uid_ = psm.uid_;
		det->ownerValid_ = false;
		det->usersCountValid_ = false;
		det->universeValid_ = false;
		det->spaceValid_ = false;

		// now fire off requests for more info
		Mercury::Address probeAddr( srcaddr, psm.port_ );
		Mercury::Bundle b;

		b.startRequest( LoginInterface::probe,
			new ProbeReplyHandler( this, probeAddr ),
			NULL,
			Mercury::DEFAULT_REQUEST_TIMEOUT,
			Mercury::RELIABLE_NO );

		// Set this to be a internal nub so that we can receive once-off reliable 
		// messages for the probe replies. The nub will be switched back to an 
		// external nub on the next log on attempt.
		ConnectionControl::serverConnection()->nub().isExternal( false );
		ConnectionControl::serverConnection()->nub().send( probeAddr, b );

		details_.push_back( det );
	}

	return true;
}


/**
 *	Python get attribute method
 */
PyObject * ServerDiscovery::pyGetAttribute( const char * attr )
{
	BW_GUARD;
	PY_GETATTR_STD();
	return this->PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Python set attribute method
 */
int ServerDiscovery::pySetAttribute( const char * attr, PyObject * value )
{
	BW_GUARD;
	PY_SETATTR_STD();
	return this->PyObjectPlus::pySetAttribute( attr, value );
}

/*~ attribute BigWorld serverDiscovery
 *  This is an instance of the ServerDiscovery class. It can be used to locate
 *  BigWorld servers on a local area network and obtain information regarding
 *  their current status. This is the only instance of ServerDiscovery which
 *  can be accessed via Python script.
 *  @type ServerDiscovery
 */

// Add ourselves to the BigWorld module
PY_MODULE_ATTRIBUTE( BigWorld, serverDiscovery, new ServerDiscovery() )


// -----------------------------------------------------------------------------
// Section: ServerDiscovery::Details
// -----------------------------------------------------------------------------

/*~ attribute ServerDiscoveryDetails hostName
 *  This is the name of the server computer, as returned by a call to the
 *  standard getHostName function.
 *  @type Read-only String
 */
/*~ attribute ServerDiscoveryDetails ip
 *  This is the IP address of the server, expressed as a single integer. If the
 *  IP address is considered to be like a base 256 number, then this would be
 *  its decimal representation. For example, the IP address 10.11.12.42 would
 *  be represented here as 168496170, which is
 *  10 * 256^3 + 11 * 256^2 + 12 * 256^1 + 42 * 256^0.
 *  @type Read-only Integer
 */
/*~ attribute ServerDiscoveryDetails port
 *  This is an integer equal to the port on which the server is listening.
 *  @type Read-only Integer
 */
/*~ attribute ServerDiscoveryDetails uid
 *  This integer is the user identification number with which the server was
 *  launched. This can be used to differentiate between multiple servers
 *  which may be running on the same machine.
 *  @type Read-only Integer
 */
/*~ attribute ServerDiscoveryDetails ownerName
 *  This is the name of the user who launched the server. If the server is
 *  running Windows then this would be the value returned by the GetLogin()
 *  function, otherwise it would be the value returned by GetUserName().
 *  @type Read-only String
 */
/*~ attribute ServerDiscoveryDetails usersCount
 *  This integer represents the total number of attempts that have been made to
 *  connect to the server (by any client).
 *  @type Read-only Integer
 */
/*~ attribute ServerDiscoveryDetails universeName
 *  This string is the name of the universe currently playing on the server.
 *  @type Read-only String
 */
/*~ attribute ServerDiscoveryDetails spaceName
 *  This string is the name of the space currently playing on the server.
 *  @type Read-only String
 */
 /*~ attribute ServerDiscoveryDetails serverString
 *  This is a server string of the form <ip>:<port>, suitable to be passed into
 *	BigWorld.connect()
 *  @type Read-only String
 */
PY_TYPEOBJECT( ServerDiscovery::Details )

PY_BEGIN_ATTRIBUTES( ServerDiscovery::Details )
	PY_ATTRIBUTE( hostName )
	PY_ATTRIBUTE( ip )
	PY_ATTRIBUTE( port )
	PY_ATTRIBUTE( uid )
	PY_ATTRIBUTE( ownerName )
	PY_ATTRIBUTE( usersCount )
	PY_ATTRIBUTE( universeName )
	PY_ATTRIBUTE( spaceName )
	PY_ATTRIBUTE( serverString )
PY_END_ATTRIBUTES()

PY_BEGIN_METHODS( ServerDiscovery::Details )
PY_END_METHODS()

PY_SCRIPT_CONVERTERS( ServerDiscovery::Details )


/**
 *	Constructor
 */
ServerDiscovery::Details::Details( PyTypePlus * pType )
	:
	PyObjectPlus( pType ),
	hostName_(),
	ip_( 0 ),
	port_( 0 ),
	uid_( 0 ),
	ownerValid_( 0 ),
	usersCountValid_( 0 ),
	universeValid_( 0 ),
	spaceValid_( 0 ),
	ownerName_(),
	usersCount_( 0 ),
	universeName_(),
	spaceName_()
{

}


/**
 *	Python get attribute method
 */
PyObject * ServerDiscovery::Details::pyGetAttribute( const char * attr )
{
	BW_GUARD;
	PY_GETATTR_STD();
	return this->PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Python set attribute method
 */
int ServerDiscovery::Details::pySetAttribute( const char * attr, PyObject * value )
{
	BW_GUARD;
	PY_SETATTR_STD();
	return this->PyObjectPlus::pySetAttribute( attr, value );
}


/**
 *	Returns a string representation of the server details.
 */
PyObject * ServerDiscovery::Details::pyRepr()
{
	BW_GUARD;
	char itoaBuffer[65];

	PyObject * result = PyString_FromFormat( 
		"server:'%s' uid:'%d' ownerName:%s usersCount:%s universeName:%s spaceName:%s", 
		this->getServerString().c_str(),
		uid_,
		ownerValid_ ? ("'" + ownerName_ + "'").c_str() : "None",
		usersCountValid_ ?  _itoa( usersCount_, itoaBuffer, 10 ) : "None",
		universeValid_ ? ("'" + universeName_ + "'").c_str() : "None",
		spaceValid_ ? ("'" + spaceName_ + "'").c_str() : "None" );

	return result;
}


/**
 *	Returns a server string \<ip\>:\<port\> suitable to be passed to
 *	BigWorld.connect()
 *
 *	@return	server string
 */
std::string ServerDiscovery::Details::getServerString() const
{
	BW_GUARD;
	Mercury::Address address( htonl( ip_ ), htons( port_ ) );

	return std::string( address.c_str() );
}




// server_discovery.cpp
