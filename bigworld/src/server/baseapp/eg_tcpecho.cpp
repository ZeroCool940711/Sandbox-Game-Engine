/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 * 	This file contains an example of how to do non-blocking socket I/O using
 *  Mercury. To see this example in action:
 * 		1. Add this file into baseapp's Makefile and rebuild baseapp.
 * 		2. Start the server.
 * 		3. Start the eg_tcpechoserver.py TCP server.
 * 		4. Telnet to baseapp's Python console and run
 * 		   "BigWorld.tcpEcho( <ip_addr>, <port>, <message>[, <callback>] )",
 * 		   where the <ip_addr> and <port> is the IP address and port of the
 * 		   TCP server and <message> is a message you want to send to the
 * 		   server. e.g.
 * 				BigWorld.tcpEcho( "10.40.3.37", 9999, "Hello World!" )
 *		   You can also optionally specify a callback to be called when we
 * 		   receive the response from the server. The callback should take two
 * 		   parameters: isError, message. When isError is true, then the message
 * 		   is an error message. When isError is false, then the message is the
 * 		   response from the server.
 * 	The code in this example will open a connection to the TCP server.
 *  Wait for the connection to establish. Send the message to the TCP server.
 *  Wait for the response and print out the response. This example shows how
 * 	it is possible to wait without blocking.
 *
 * 	Although this code is currently specific to baseapp, it can be easily
 *	adapted to work in other server components. The dependency on baseapp
 * 	is mainly because of the need to access the global Mercury::Nub. The
 * 	way to access this global is similar but not identical on all server
 * 	components.
 */

#include "Python.h"		// See http://docs.python.org/api/includes.html

#include "network/mercury.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

#include "baseapp.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

/**
 *	This class is a simple example of how to use registerFileDescriptor and
 * 	registerWriteFileDescriptor.
 */
class TCPEcho : public Mercury::InputNotificationHandler
{
public:
	TCPEcho() :
		readyToWrite_( false ),
		pRecieveBuf_( NULL ),
		numReceived_( 0 )
	{
	}
	virtual ~TCPEcho()
	{
		delete [] pRecieveBuf_;
	}

	void init( const std::string & ip, uint16 port, const std::string & msg,
		PyObjectPtr pCallback )
	{
		endpoint_.socket( SOCK_STREAM );
		endpoint_.setnonblocking( true );
		u_int32_t address;
		if (endpoint_.convertAddress( (char *)ip.c_str(), address ) < 0)
		{
			ERROR_MSG( "TCPEcho::init: Could not convert address '%s'\n",
					ip.c_str() );
			delete this;
			return;
		}

		endpoint_.connect( htons( port ), address );
		BaseApp::instance().intNub().registerWriteFileDescriptor( endpoint_,
				this );
		msg_ = msg;
		pRecieveBuf_ = new char[ msg.size() + 1 ];
		pRecieveBuf_[ msg.size() ] = '\0';

		pCallback_ = pCallback;
	}

protected:
	virtual int handleInputNotification( int fd )
	{
		INFO_MSG( "handleInputNotification: readyToWrite_ = %d\n",
					int(readyToWrite_) );
		if (!readyToWrite_)
		{
			readyToWrite_ = true;
			int sendSize = endpoint_.send( msg_.data(), msg_.size() );

			BaseApp::instance().intNub().deregisterWriteFileDescriptor( endpoint_ );

			if (sendSize < 0)
			{
				std::stringstream errorMsg;
				errorMsg << "Failed to send (" << strerror( errno ) << ")";
				this->doCallback( true, errorMsg.str() );
				delete this;
			}
			else
			{
				BaseApp::instance().intNub().registerFileDescriptor( endpoint_, this );
			}
		}
		else
		{
			int size = endpoint_.recv( pRecieveBuf_ + numReceived_,
					msg_.size() - numReceived_ );

			if (size <= 0)
			{
				if (size < 0)
				{
					std::stringstream errorMsg;
					errorMsg << "Failed to recv (" << strerror( errno ) << ")";
					this->doCallback( true, errorMsg.str() );
				}
				else
				{
					this->doCallback( false, std::string( pRecieveBuf_, numReceived_ ) );
				}

				BaseApp::instance().intNub().deregisterFileDescriptor( endpoint_ );
				delete this;
				return 0;
			}

			numReceived_ += size;

			if (numReceived_ == msg_.size())
			{
				this->doCallback( false, std::string( pRecieveBuf_ ) );
				BaseApp::instance().intNub().deregisterFileDescriptor( endpoint_ );

				delete this;
				return 0;
			}
		}

		return 0;
	}

	void doCallback( bool isError, const std::string& msg )
	{
		if (pCallback_)
		{
			// Script::call() decrements reference count of pCallback_
			Py_INCREF( pCallback_.getObject() );
			Script::call( pCallback_.getObject(),
					Py_BuildValue( "(Os)", isError ? Py_True : Py_False,
									msg.c_str() ),
					"TCPEcho callback", /*okIfFnNull:*/false );
		}
		else
		{
			if (isError)
				ERROR_MSG( "TCPEcho: %s\n", msg.c_str() );
			else
				INFO_MSG( "TCPEcho: %s\n", msg.c_str() );
		}
	}

private:
	bool readyToWrite_;
	Endpoint endpoint_;
	std::string msg_;
	char * pRecieveBuf_;
	uint numReceived_;
	PyObjectPtr pCallback_;
};

/**
 *	This function implements the Python BigWorld.tcpEcho function.
 */
bool tcpEcho( const std::string & ip, uint16 port, const std::string & msg,
		PyObjectPtr pCallback )
{
	TCPEcho * pEcho = new TCPEcho;
	pEcho->init( ip, port, msg, pCallback );
	return true;
}
PY_AUTO_MODULE_FUNCTION( RETOK, tcpEcho,
		ARG( std::string, ARG( uint16, ARG( std::string,
			OPTARG( PyObjectPtr, NULL, END ) ) ) ), BigWorld )
