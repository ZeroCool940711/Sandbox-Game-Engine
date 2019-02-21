/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "network_app.hpp"

#include "cstdmf/memory_tracker.hpp"
#include "network/packet_filter.hpp"

#include "test_mangle_interfaces.hpp"

#include <map>


extern BWTimingMethod g_timingMethod;
//extern bool g_shouldWritePid;

namespace
{

// -----------------------------------------------------------------------------
// Section:Configuration options
// -----------------------------------------------------------------------------

// How much to mangle a packet
const uint32 DEFAULT_MANGLE_PER_PACKET_PERCENT = 10;

// The number of packets to mangle
const uint32 DEFAULT_MANGLE_NUM_PACKETS_PERCENT = 50;

// The number of bundles that each client should send
const uint32 DEFAULT_NUM_BUNDLES_TO_SEND = 1000;

// Each bundle is a random size in the following range
const int DEFAULT_MIN_BUNDLE_SIZE = 1000;
const int DEFAULT_MAX_BUNDLE_SIZE = 5000;

// How often the server app checks whether all client apps are done.
const int SERVER_TICK_PERIOD = 100000; // 100 ms

// How often the client should check to send to the server
const int CLIENT_TICK_PERIOD = 1000; // 1ms

// Number of client processes that create INTERNAL channels
const int NUM_INTERNAL_CLIENTS = 0;

// Number of client processes that create EXTERNAL channels
const int NUM_EXTERNAL_CLIENTS = 10;

// The number of seconds the test should run before it is considered a failure
// with a timeout.
const int MAX_RUNNING_TIME = 120; // 2 minutes

} // namespace anonymous

// -----------------------------------------------------------------------------
// Section: MangleFilter
// -----------------------------------------------------------------------------

/**
 *  Mangling packet filter.
 */
class MangleFilter : public Mercury::PacketFilter
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param perPacketPercent Indicates what percentage to mangle a packet.
	 *		This percentage (on average) of bytes will be set to a random byte.
	 *	@param numPacketsPercent Indicates what percentage of packets should be
	 *		mangled.
	 */
	MangleFilter( int perPacketPercent, int numPacketsPercent ) :
		Mercury::PacketFilter(),
		perPacketPercent_( perPacketPercent ),
		numPacketsPercent_( numPacketsPercent )
	{
	}

	virtual Mercury::Reason recv( Mercury::Nub & nub,
			const Mercury::Address & addr, Mercury::Packet * pPacket );

private:
	int perPacketPercent_;
	int numPacketsPercent_;
};

typedef SmartPointer< MangleFilter > MangleFilterPtr;


/**
 *	This method processes a received packet. It randomly decides whether each
 *	packet should be mangled and, if so, it mangles a certain percentage of
 *	the bytes in the packet.
 */
Mercury::Reason MangleFilter::recv( Mercury::Nub & nub,
		const Mercury::Address & addr, Mercury::Packet * pPacket )
{
	if ((rand() % 100) < numPacketsPercent_)
	{
		for (int i = 0; i < pPacket->totalSize(); ++i)
		{
			if (rand() % 100 < perPacketPercent_)
			{
				pPacket->data()[i] = char( rand() );
			}
		}
	}

	return this->PacketFilter::recv( nub, addr, pPacket );
}


// -----------------------------------------------------------------------------
// Section: MangleServerApp
// -----------------------------------------------------------------------------

/**
 *	This class is used to receive bundles from the client applications. A
 *	certain amount of the received packets are mangled.
 */
class MangleServerApp : public NetworkApp
{
public:
	MangleServerApp() :
		NetworkApp(),
		pTestCase_( NULL ),
		hasTimedOut_( false ),
		latestEndTime_( 0 )
	{
		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		MangleServerInterface::registerWithNub( nub_ );
		nub_.setIrregularChannelsResendPeriod( 0.1f );
		nub_.isExternal( true );
	}

	void init( MultiProcTestCase & testCase )
	{
		pTestCase_ = & testCase;
	}

	~MangleServerApp();

	virtual int handleTimeout( Mercury::TimerID id, void * arg )
	{
		// Run until all client application have finished
		if (!pTestCase_->hasRunningChildren())
		{
			nub_.breakProcessing();
		}
		else if (timestamp() > latestEndTime_)
		{
			nub_.breakProcessing();
			hasTimedOut_ = true;
		}

		return 0;
	}

	virtual int run()
	{
		INFO_MSG( "MangleServerApp(%d)::run: started\n", getpid() );

		latestEndTime_ = timestamp() + MAX_RUNNING_TIME * stampsPerSecond();

		this->startTimer( SERVER_TICK_PERIOD );
		this->NetworkApp::run();

		INFO_MSG( "MangleServerApp(%d)::run: finished\n", getpid() );
		return 0;
	}

	// ---- Properties ----
	bool hasTimedOut() const	{ return hasTimedOut_; }

	// ---- Message Handlers ----

	void connect( const Mercury::Address & srcAddr,
			const MangleServerInterface::connectArgs & args );

	void msg1( const Mercury::Address & srcAddr,
			const MangleServerInterface::msg1Args & args );

	void msg2( const Mercury::Address & srcAddr,
			const MangleServerInterface::msg2Args & args );

	// ---- Statics ----

	static MangleServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	// typedef std::map< Mercury::Address, Mercury::Channel * > Channels;
	// Channels channels_;
	MultiProcTestCase * pTestCase_;

	// Whether the run ended with a timeout
	bool hasTimedOut_;

	// The time after which we consider the test has timed out
	uint64 latestEndTime_;

	static MangleServerApp * s_pInstance;
};

MangleServerApp * MangleServerApp::s_pInstance = NULL;


/**
 *	Destructor.
 */
MangleServerApp::~MangleServerApp()
{
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;
}

/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ServerMangleStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (MangleServerApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ServerMangleStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(MangleServerApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: MangleServerApp Message Handlers
// -----------------------------------------------------------------------------

// Magic data sent in messages
const uint32 CONNECT_MAGIC = 0x0708090a;
const uint32 MSG1_MAGIC = 0x01020304;
const uint8 MSG2_MAGIC = 0x06;

/**
 *	This method handles a message from the client informing us to connect.
 */
void MangleServerApp::connect( const Mercury::Address & srcAddr,
		const MangleServerInterface::connectArgs & args )
{
	// If the magic number does not match, this method has probably been called
	// because of a corrupted packet.
	if (args.magic != CONNECT_MAGIC)
	{
		WARNING_MSG( "MangleServerApp::connect: Incorrect magic number %x != %x\n",
				args.magic, CONNECT_MAGIC );
		return;
	}

	INFO_MSG( "MangleServerApp::connect( %s ): traits = %d\n",
		srcAddr.c_str(), args.traits );

	if (nub_.findChannel( srcAddr ))
	{
		DEBUG_MSG( "MangleServerApp::connect: Already have channel\n" );
	}
	else
	{
		Mercury::Channel * pChannel =
			new Mercury::Channel( nub_, srcAddr, args.traits, 1.0,
				new MangleFilter( DEFAULT_MANGLE_PER_PACKET_PERCENT,
					DEFAULT_MANGLE_NUM_PACKETS_PERCENT ) );

		// Since we don't store a reference to this channel anywhere, we need to
		// mark it as anonymous so that the Nub will take ownership of it and
		// destroy it for us.
		pChannel->isAnonymous( true );

		Mercury::Bundle & bundle = pChannel->bundle();
		MangleClientInterface::ackConnectArgs & args =
			MangleClientInterface::ackConnectArgs::start( bundle );
		args.magic = CONNECT_MAGIC;
		pChannel->send();
	}
}


/**
 *	This method handles msg1 messages.
 */
void MangleServerApp::msg1( const Mercury::Address & srcAddr,
		const MangleServerInterface::msg1Args & args )
{
#if 0
	static int total = 0;
	DEBUG_MSG( "MangleServerApp::msg1: Got message from %s/%d\n",
			srcAddr.c_str(), ++total );
#endif
}


/**
 *	This method handles msg2 messages.
 */
void MangleServerApp::msg2( const Mercury::Address & srcAddr,
		const MangleServerInterface::msg2Args & args )
{
#if 0
	static int total = 0;
	DEBUG_MSG( "MangleServerApp::msg2: Got message from %s/%d\n",
			srcAddr.c_str(), ++total );
#endif
}

// -----------------------------------------------------------------------------
// Section: MangleClientApp
// -----------------------------------------------------------------------------

/**
 *	This class is used to send information to server application. Instances of
 *	this class run inside spawned child processes.
 */
class MangleClientApp : public NetworkApp
{
public:

	MangleClientApp( const Mercury::Address & dstAddr,
			Mercury::Channel::Traits channelTraits = Mercury::Channel::INTERNAL,
			uint32 numBundlesToSend = DEFAULT_NUM_BUNDLES_TO_SEND,
			int minPacketSize = DEFAULT_MIN_BUNDLE_SIZE,
			int maxPacketSize = DEFAULT_MAX_BUNDLE_SIZE ) :
		NetworkApp(),
		outSeq_( 0 ),
		numBundlesToSend_( numBundlesToSend ),
		pChannel_( new Mercury::Channel( nub_, dstAddr, channelTraits ) ),
		hasFinishedCleanly_( false ),
		minPacketSize_( minPacketSize ),
		maxPacketSize_( maxPacketSize ),
		isConnectionEstablished_( false )
	{
		MF_ASSERT( minPacketSize_ < maxPacketSize_ );

		pChannel_->isIrregular( true );

		TRACE_MSG( "MangleClientApp::MangleClientApp: server is at %s\n",
				dstAddr.c_str() );
		// Dodgy singleton code
		MF_ASSERT( s_pInstance == NULL );
		s_pInstance = this;

		MangleClientInterface::registerWithNub( nub_ );
	}

	~MangleClientApp()
	{
		INFO_MSG( "MangleClientApp(%d)::~MangleClientApp: %p\n",
				getpid(), this );
		pChannel_->destroy();
		MF_ASSERT( s_pInstance == this );
		s_pInstance = NULL;
	}

	virtual int run()
	{
		INFO_MSG( "MangleClientApp(%d)::run: Starting\n", getpid() );

		this->startTest();

		this->nub().processUntilBreak();
		this->nub().processUntilChannelsEmpty();
		INFO_MSG( "MangleClientApp(%d)::run: Finished\n", getpid() );

		return hasFinishedCleanly_ ? 0 : -1;
	}

	void startTest();
	void sendConnect();

	int handleTimeout( Mercury::TimerID id, void * arg );

	// ---- Message Handlers ----
	void ackConnect( const Mercury::Address & srcAddr,
			const MangleClientInterface::ackConnectArgs & args );

	static MangleClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	uint32 outSeq_;
	unsigned numBundlesToSend_;
	Mercury::Channel * pChannel_;
	bool hasFinishedCleanly_;

	int minPacketSize_;
	int maxPacketSize_;

	bool isConnectionEstablished_;

	void addMsg1( Mercury::Bundle & bundle );
	void addMsg2( Mercury::Bundle & bundle );

	static MangleClientApp * s_pInstance;
};

MangleClientApp * MangleClientApp::s_pInstance = NULL;


/**
 *	Class for struct-style Mercury message handler objects.
 */
template <class ARGS> class ClientMangleStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (MangleClientApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ClientMangleStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(MangleClientApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};


/**
 *	This method is called to start this application.
 */
void MangleClientApp::startTest()
{
	const uint tickRate = CLIENT_TICK_PERIOD; // 1 ms
	INFO_MSG( "MangleClientApp(%d)::startTest: starting timer\n", getpid() );
	this->startTimer( tickRate );
}


/**
 *	This method sends a connect message to the server.
 */
void MangleClientApp::sendConnect()
{
	Mercury::Bundle bundle;
	MangleServerInterface::connectArgs & args =
		MangleServerInterface::connectArgs::start( bundle,
				Mercury::RELIABLE_NO );

	args.traits =
		pChannel_->isExternal() ?
			Mercury::Channel::EXTERNAL : Mercury::Channel::INTERNAL;
	args.magic = CONNECT_MAGIC;

	nub_.send( pChannel_->addr(), bundle );
}


/**
 *	This method adds a msg1 message to the bundle.
 */
void MangleClientApp::addMsg1( Mercury::Bundle & bundle )
{
	MangleServerInterface::msg1Args & args =
		MangleServerInterface::msg1Args::start( pChannel_->bundle() );

	args.magic = MSG1_MAGIC;
}


/**
 *	This method adds a msg2 message to the bundle.
 */
void MangleClientApp::addMsg2( Mercury::Bundle & bundle )
{
	MangleServerInterface::msg2Args & args =
		MangleServerInterface::msg2Args::start( pChannel_->bundle() );

	args.magic = MSG2_MAGIC;
}


/**
 *	The client sends to the server based on a timer.
 */
int MangleClientApp::handleTimeout( Mercury::TimerID id, void * arg )
{
	if (!isConnectionEstablished_)
	{
		this->sendConnect();
		return 0;
	}

	// If the send window is fairly full, it is too early to send more data.
	if (pChannel_->sendWindowUsage() * 5 > pChannel_->windowSize())
	{
#if 0
		DEBUG_MSG( "MangleClientApp::handleTimeout: sendWindowUsage = %d\n",
				pChannel_->sendWindowUsage() );
#endif
		return 0;
	}

	int desiredPacketSize =
		minPacketSize_ + rand() % (maxPacketSize_ - minPacketSize_);

	while (pChannel_->bundle().size() < desiredPacketSize)
	{
		const int NUM_CHOICES = 2;

		switch (rand() % NUM_CHOICES)
		{
		case 0:
			this->addMsg1( pChannel_->bundle() );
			break;

		case 1:
			this->addMsg2( pChannel_->bundle() );
			break;

		// TODO: Add a variable length message

		default:
			MF_ASSERT( 0 );
		}
	}

	if (--numBundlesToSend_ == 0)
	{
		hasFinishedCleanly_ = true;
		this->nub().breakProcessing();
		this->stopTimer();
	}

	// DEBUG_MSG( "MangleClientApp::handleTimeout: %d left.\n", numBundlesToSend_ );
	pChannel_->send();

	return 0;
}


/**
 *	This method handles a message from the server indicating that it has
 *	accepted the connection.
 */
void MangleClientApp::ackConnect( const Mercury::Address & srcAddr,
		const MangleClientInterface::ackConnectArgs & args )
{
	isConnectionEstablished_ = true;
}

struct Fixture
{
	Fixture()
	{
#ifdef MF_SERVER
		// Initialise stampsPerSecond and timing method
		setenv( "BW_TIMING_METHOD", "gettimeofday", 1 );
		stampsPerSecond();
#endif

		// g_shouldWritePid = true;
		DebugFilter::instance().hasDevelopmentAssertions( false );
		originalOutputFilter_ = DebugFilter::instance().filterThreshold();

		DebugFilter::instance().filterThreshold( MESSAGE_PRIORITY_CRITICAL );
	}

	~Fixture()
	{
		// TODO: Restore to original state, not assume as true.
		DebugFilter::instance().hasDevelopmentAssertions( true );
		DebugFilter::instance().filterThreshold( originalOutputFilter_ );
		// g_shouldWritePid = false;
	}

	int originalOutputFilter_;
};

TEST_F( Fixture, timingMethod )
{
	ASSERT_WITH_MESSAGE( g_timingMethod == GET_TIME_OF_DAY_TIMING_METHOD,
		"Incorrect timing method. "
			"Set environment variable BW_TIMING_METHOD to 'gettimeofday'\n" );
}

MEMTRACKER_DECLARE( TestMangle_mangledChannel, "TestMangle_mangledChannel",
	0 );

TEST_F( Fixture, mangledChannel )
{
	MEMTRACKER_SCOPED( TestMangle_mangledChannel );

	MangleServerApp serverApp;
	MultiProcTestCase mp( serverApp );
	serverApp.init( mp );

	for (int i = 0; i < NUM_INTERNAL_CLIENTS; ++i)
	{
		mp.runChild(
				new MangleClientApp( serverApp.nub().address(),
				Mercury::Channel::INTERNAL ) );
	}

	for (int i = 0; i < NUM_EXTERNAL_CLIENTS; ++i)
	{
		mp.runChild(
				new MangleClientApp( serverApp.nub().address(),
				Mercury::Channel::EXTERNAL ) );
	}

	MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( mp );

	CHECK( !serverApp.hasTimedOut() );

}

#define DEFINE_SERVER_HERE
#include "test_mangle_interfaces.hpp"


// test_mangle.cpp
