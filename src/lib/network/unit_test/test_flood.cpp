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

#include "unit_test_lib/multi_proc_test_case.hpp"

#include "cstdmf/timestamp.hpp"
#include "cstdmf/memory_tracker.hpp"

#include "network/interfaces.hpp"
#include "network/nub.hpp"
#include "network/packet_filter.hpp"
#include "network/unit_test/network_app.hpp"

#include "test_flood_interfaces.hpp"

#include <map>
#include <vector>
#include <set>

#include <string.h>


// -----------------------------------------------------------------------------
// Section: Test constants
// -----------------------------------------------------------------------------

/**
 *	How many ticks/iterations clients will run for, (i.e. how many msg1
 *	messages from the client to the server to send)
 */
static const int NUM_ITERATIONS = 100;

/**
 *	The size of the message payloads between the client and server (msg1 and
 *	msg2).
 */
static const uint PAYLOAD_SIZE = 64 * 1024;


/**
 *	The tick period in microseconds.
 */
static const long TICK_PERIOD = 100000L;

// -----------------------------------------------------------------------------
// Section: FloodServerApp
// -----------------------------------------------------------------------------


/**
 *	The flood server application.
 */
class FloodServerApp : public NetworkApp
{

public:
	FloodServerApp( unsigned payloadSizeBytes,
		unsigned long maxRunTimeMicros );
	~FloodServerApp();

	virtual int run();


	void connect( const Mercury::Address & srcAddr,
			const FloodServerInterface::connectArgs & args );

	void disconnect( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void msg1( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	int handleTimeout( Mercury::TimerID id, void * /*arg*/ );

	/**
	 *	Return the number of msg1 messages we have received from our clients.
	 */
	unsigned getMsg1Count() const { return msg1Count_; }

	/**
	 *	Singleton instance accessor.
	 */
	static FloodServerApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:

	typedef std::map<Mercury::Address, Mercury::Channel *> Channels;
	Channels			channels_;

	typedef std::map<Mercury::Address, unsigned> ChannelCount;
	ChannelCount		channelCounts_;

	uint 				msg1Count_;
	unsigned 			payloadSize_;
	unsigned long 		maxRunTimeMicros_;

	Mercury::TimerID 	watchTimerID_;

	static FloodServerApp * s_pInstance;
};


/** Singleton instance pointer. */
FloodServerApp * FloodServerApp::s_pInstance = NULL;

/**
 *	Constructor.
 *
 *	@param payloadSizeBytes		the length of the payload to send to clients
 *	@param maxRunTimeMicros		the maximum time to run the test for before
 *								aborting and asserting test failure
 */
FloodServerApp::FloodServerApp( unsigned payloadSizeBytes,
			unsigned long maxRunTimeMicros ):
		NetworkApp(),
		channels_(),
		channelCounts_(),
		msg1Count_( 0 ),
		payloadSize_( payloadSizeBytes ),
		maxRunTimeMicros_( maxRunTimeMicros )
{
	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;

	FloodServerInterface::registerWithNub( this->nub() );
}


/**
 *	Destructor.
 */
FloodServerApp::~FloodServerApp()
{
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;
}


/**
 *	App run function.
 */
int FloodServerApp::run()
{
	INFO_MSG( "FloodServerApp(%d)::run: started\n", getpid() );

	this->startTimer( TICK_PERIOD );

	watchTimerID_ = this->nub().registerTimer( maxRunTimeMicros_, this, NULL );

	bool done = false;
	while (!done)
	{
		try
		{
			this->nub().processContinuously();
			done = true;
		}
		catch (Mercury::NubExceptionWithAddress & ea)
		{
			Mercury::Address addr;
			ea.getAddress( addr );

			// was this one for one of channel addresses?
			Channels::iterator iChannel = channels_.find( addr );
			if (iChannel != channels_.end())
			{
				ERROR_MSG( "FloodServerApp(%d): Condemning channel to %s "
						"due to exception: %s\n",
					getpid(), addr.c_str(),
					Mercury::reasonToString( ea.reason() ) );

				iChannel->second->condemn();
				channels_.erase( iChannel );
			}
			else
			{
				// unknown channel
				ERROR_MSG( "FloodServerApp(%d)::run: "
						"got nub exception for address %s: %s\n",
					getpid(), addr.c_str(),
					Mercury::reasonToString( ea.reason() ) );
			}


		}
		catch (Mercury::NubException & e)
		{
			ERROR_MSG( "FloodServerApp(%d)::run: got nub exception: %s\n",
				getpid(), Mercury::reasonToString( e.reason() ) );
		}
	}

	// If the test has already failed, bail now
	if (this->hasFailed())
	{
		return 1;
	}

	TRACE_MSG( "FloodServerApp(%d)::run: "
		"Processing until channels empty\n", getpid() );
	this->nub().processUntilChannelsEmpty();

	// condemn all the channels (we pass ownership to the nub)
	Channels::iterator iChannel = channels_.begin();
	while (iChannel != channels_.end())
	{
		if (!iChannel->second->isCondemned())
		{
			// we should already have condemned it in disconnect
			WARNING_MSG( "FloodServerApp(%d): Condemning channel to %s\n",
				getpid(), iChannel->first.c_str() );
			iChannel->second->condemn();
		}
		++iChannel;
	}

	INFO_MSG( "FloodServerApp(%d)::run: finished (#msg1 = %d)\n",
		getpid(), msg1Count_ );
	return 0;
}


/**
 *	Timeout handler. If the watch timer (configured with maxRunTimeMicros
 *	parameter at construction) goes off, then abort test and assert test
 *	failure. Otherwise, it is a tick timer and so send all connected clients a
 *	msg2 message.
 */
int FloodServerApp::handleTimeout( Mercury::TimerID timerID, void * /*arg*/ )
{
	if (timerID == watchTimerID_)
	{
		// Our watchdog timer has expired
		ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
				"Max run time (%.1fs) is up\n",
			getpid(), maxRunTimeMicros_ / 1000000.f );

		NETWORK_APP_FAIL_RET( "Timed out", 1 );
	}

	// Send to clients that we know are still connected
	Channels::iterator iter = channels_.begin();

	while (iter != channels_.end())
	{
		Channels::iterator oldIter = iter++;
		Mercury::Channel & channel = *oldIter->second;
		Mercury::Bundle & bundle = channel.bundle();

		bundle.startMessage( FloodClientInterface::msg2 );

		// put the length-prefixed payload on to msg2 bundle
		char buf[1024];
		memset( buf, 'b', 1024 );
		bundle << payloadSize_;
		for (unsigned i = 0; i < payloadSize_; i += 1024)
		{
			bundle.addBlob( buf, MF_MIN( payloadSize_ - i, (unsigned)1024 ) );
		}

		// try and send
		Mercury::Reason reason = Mercury::REASON_SUCCESS;
		try
		{
			channel.send();
		}
		catch (Mercury::NubException & e)
		{
			ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
					"exception when sending msg2 to %s:%s\n",
				getpid(), channel.c_str(),
				Mercury::reasonToString( e.reason() ) );

			reason = e.reason();
		}

		// if we failed to send, condemn the channel
		if (reason != Mercury::REASON_SUCCESS)
		{
			ERROR_MSG( "FloodServerApp(%d)::handleTimeout: "
				"Disconnecting because could not send msg2 to %s: %s\n",
				getpid(), channel.c_str(),
				Mercury::reasonToString( reason ) );

			// condemn this channel, remove later after loop
			channel.condemn();
			channels_.erase( oldIter );
		}
	}

	return 0;
}


// -----------------------------------------------------------------------------
// Section: FloodServerApp Message Handlers
// -----------------------------------------------------------------------------

/**
 *	Clients send this message to establish a channel.
 */
void FloodServerApp::connect( const Mercury::Address & srcAddr,
		const FloodServerInterface::connectArgs & args )
{
	TRACE_MSG( "FloodServerApp(%d)::connect( %s ): traits = %d\n",
		getpid(), srcAddr.c_str(), args.traits );

	if (this->nub().findChannel( srcAddr ))
	{
		// we may already have one - the client spams connect until it gets
		// connectAck
		TRACE_MSG( "FloodServerApp(%d)::connect(%s): already have channel\n",
			getpid(), srcAddr.c_str() );
		return;
	}

	Mercury::Channel * pChannel =
		new Mercury::Channel( this->nub(), srcAddr, args.traits );

	channels_[ srcAddr ] = pChannel;
	channelCounts_[ srcAddr ] = 0;

	// send the connect ack
	pChannel->bundle().startMessage( FloodClientInterface::connectAck, true );
	pChannel->send();
}


/**
 *	Clients disconnect after they have sent a certain number of msg1 messages
 *	to the server. The channel is expected to be destroyed.
 */
void FloodServerApp::disconnect( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	TRACE_MSG( "FloodServerApp(%d)::disconnect( %s )\n",
		getpid(), srcAddr.c_str() );

	Channels::iterator iter = channels_.find( srcAddr );

	if (iter != channels_.end())
	{
		iter->second->condemn();
		channels_.erase( iter );
	}
	else
	{
		ERROR_MSG( "FloodServerApp(%d)::disconnect( %s ): "
			"unknown address\n",
			getpid(), srcAddr.c_str() );
	}

	if (!channels_.size())
	{
		TRACE_MSG( "FloodServerApp(%d)::disconnect: no more clients\n",
			getpid() );
		this->nub().breakProcessing();
	}
}


/**
 *	Clients send a certain number of msg1 messages to the server. It contains a
 *	4-byte length prefix and the message payload proper.
 */
void FloodServerApp::msg1( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & /*header*/,
		BinaryIStream & data )
{
	uint seq;
	uint lenBytes;

	data >> seq >> lenBytes;

	TRACE_MSG( "FloodServerApp(%d)::msg1 (%s): seq=%u (len=%u)\n",
			getpid(), srcAddr.c_str(), seq, lenBytes );

	const char * buf =
		reinterpret_cast<const char*>( data.retrieve( lenBytes ) );

	Channels::iterator iChannel = channels_.find( srcAddr );
	if (iChannel == channels_.end())
	{
		ERROR_MSG( "FloodServerApp(%d)::msg1: no channel exists for %s\n",
			getpid(), srcAddr.c_str() );
	}

	// some checks
	NETWORK_APP_ASSERT_WITH_MESSAGE( lenBytes == PAYLOAD_SIZE,
		"Payload length does not match" );

	NETWORK_APP_ASSERT_WITH_MESSAGE( iChannel != channels_.end(),
		"Channel not found" );

	NETWORK_APP_ASSERT_WITH_MESSAGE( channelCounts_[srcAddr] == seq,
		"Got bad sequence number" );


	// verify buf
	const char * pChar = buf;
	while ((unsigned)(pChar - buf) < lenBytes )
	{
		NETWORK_APP_ASSERT_WITH_MESSAGE( *(pChar++) == 'a',
			"Client gave us invalid data" );
	}

	// all is OK

	++msg1Count_;
	++channelCounts_[srcAddr];

	// ack it back to the client
	Mercury::Channel * pChannel = iChannel->second;
	pChannel->bundle().startMessage( FloodClientInterface::msg1Ack, true );
	pChannel->bundle() << seq;
	pChannel->send();
}

// -----------------------------------------------------------------------------
// Section: FloodServerApp struct message handler
// -----------------------------------------------------------------------------
/**
 *	Class for struct-style Mercury message handler objects for FloodServerApp.
 */
template <class ARGS> class ServerFloodStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (FloodServerApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ServerFloodStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(FloodServerApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};

// -----------------------------------------------------------------------------
// Section: FloodClientApp
// -----------------------------------------------------------------------------

class FloodClientApp : public NetworkApp
{
public:
	FloodClientApp( const Mercury::Address & dstAddr,
			unsigned payloadSizeBytes,
			unsigned numIterations );

	virtual ~FloodClientApp();

	virtual int run();

	void connectAck( const Mercury::Address & addr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & data );

	void msg1Ack( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void msg2( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data );

	void startTest();

	int handleTimeout( Mercury::TimerID id, void * arg );

	static FloodClientApp & instance()
	{
		MF_ASSERT( s_pInstance != NULL );
		return *s_pInstance;
	}

private:
	void connect();

private:
	Mercury::Channel *	pChannel_;
	Mercury::Address	serverAddr_;
	unsigned 			payloadSize_;
	uint32 				outSeq_;
	unsigned 			numIterations_;
	unsigned			nextExpectedAck_;
	int 				errorStatus_;
	bool				hasConnected_;
	int					connectTriesLeft_;

	static FloodClientApp * s_pInstance;
};

/** Singleton instance. */
FloodClientApp * FloodClientApp::s_pInstance = NULL;


/**
 *	Constructor.
 *
 *	@param dstAddr				the server address
 *	@param payloadSizeBytes		the size of the payload to send to the server
 *								(msg1)
 *	@param numIterations		how many msg1 messages to send to the server
 */
FloodClientApp::FloodClientApp( const Mercury::Address & dstAddr,
			unsigned payloadSizeBytes,
			unsigned numIterations ):
		NetworkApp(),
		pChannel_( NULL ),
		serverAddr_( dstAddr ),
		payloadSize_( payloadSizeBytes ),
		outSeq_( 0 ),
		numIterations_( numIterations ),
		nextExpectedAck_( 0 ),
		errorStatus_( 0 ),
		hasConnected_( false ),
		connectTriesLeft_( 5 )
{
	INFO_MSG( "FloodClientApp(%p)::ClientApp: server is at %s\n",
			this, dstAddr.c_str() );

	// Dodgy singleton code
	MF_ASSERT( s_pInstance == NULL );
	s_pInstance = this;

	FloodClientInterface::registerWithNub( this->nub() );
}


/**
 *	Destructor.
 */
FloodClientApp::~FloodClientApp()
{
	INFO_MSG( "FloodClientApp(%d)::~ClientApp\n", getpid() );
	MF_ASSERT( s_pInstance == this );
	s_pInstance = NULL;

	if (pChannel_)
	{
		// condemning channels passes ownership to the nub
		pChannel_->condemn();
	}
}


/**
 *	App run function.
 */
int FloodClientApp::run()
{
	INFO_MSG( "FloodClientApp(%d)::run: Starting\n", getpid() );

	this->startTimer( TICK_PERIOD );

	try
	{
		this->nub().processContinuously();
	}
	catch (Mercury::NubException & ne)
	{
		ERROR_MSG( "FloodClientApp(%d)::run: exiting due to exception: %s",
			getpid(), Mercury::reasonToString( ne.reason() ) );
		errorStatus_ = 1;
	}

	if (!errorStatus_)
	{
		TRACE_MSG( "FloodClientApp(%d): processing until channels empty\n",
			getpid() );
		this->nub().processUntilChannelsEmpty();
	}
	else
	{
		ERROR_MSG( "FloodClientApp(%d)::run: exiting with %d\n",
			getpid(), errorStatus_ );
	}

	return errorStatus_;
}


/**
 *	Attempt to connect to the server.
 */
void FloodClientApp::connect()
{
	if (!--connectTriesLeft_)
	{
		ERROR_MSG( "FloodClientApp(%d)::connect: Out of connect retries\n",
			getpid() );
		errorStatus_ = 1;
		this->nub().breakProcessing();
		return;
	}

	// send connect unreliably as we don't have a channel yet
	Mercury::Bundle bundle;
	FloodServerInterface::connectArgs & args =
		FloodServerInterface::connectArgs::start( bundle,
			Mercury::RELIABLE_NO );

	args.traits = Mercury::Channel::EXTERNAL;

	this->nub().send( serverAddr_, bundle );

	TRACE_MSG( "FloodClientApp(%d)::connect: Sent connect\n", getpid() );

	if (!pChannel_)
	{
		pChannel_ = new Mercury::Channel( this->nub(), serverAddr_,
			Mercury::Channel::EXTERNAL );
	}

}


/**
 *	Timeout handler. This serves to send the msg1 message to the server every
 *	tick.
 */
int FloodClientApp::handleTimeout( Mercury::TimerID id, void * arg )
{
	//TRACE_MSG( "FloodClientApp(%d)::handleTimeout: Sending %d\n",
	//		getpid(), outSeq_ );

	if (!hasConnected_)
	{
		this->connect();
		return 0;
	}

	Mercury::Bundle & bundle = pChannel_->bundle();

	int outSeq = outSeq_++;

	bundle.startMessage( FloodServerInterface::msg1, true );

	// sequence number
	bundle << outSeq;
	// payload length
	bundle << payloadSize_;
	// payload data
	char buf[1024];
	memset( buf, 'a', 1024 );
	for (unsigned i = 0; i < payloadSize_; i += 1024)
	{
		bundle.addBlob( buf, MF_MIN( payloadSize_ - i, (unsigned)1024 ) );
	}

	if (outSeq_ == numIterations_)
	{
		// we've sent enough payloads, stop ticking
		this->stopTimer();
	}

	pChannel_->send();

	return 0;
}

// -----------------------------------------------------------------------------
// Section: Message handlers
// -----------------------------------------------------------------------------

/**
 *	Server sends this message to acknowledge receipt of a connect message.
 */
void FloodClientApp::connectAck( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	TRACE_MSG( "FloodClientApp(%d)::connectAck: connect handshake done\n",
		getpid() );
	hasConnected_ = true;
}


/**
 *	Server sends this message to acknowledge receipt of msg1's that we send it.
 */
void FloodClientApp::msg1Ack( const Mercury::Address & addr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	uint seq;
	data >> seq;
	TRACE_MSG( "FloodClientApp(%d)::msg1Ack: seq=%u\n", getpid(), seq );

	if (seq != nextExpectedAck_)
	{
		ERROR_MSG( "FloodClientApp(%d)::msg1Ack: "
				"got ack for unexpected sequence number %u (expecting %u)\n",
			getpid(), seq, nextExpectedAck_ );
		errorStatus_ = 1;
		this->nub().breakProcessing();
		return;
	}
	++nextExpectedAck_;

	if (seq == numIterations_ - 1)
	{
		// we have the last msg1 ack
		TRACE_MSG( "FloodClientApp(%d)::msg1Ack: we're done, disconnecting\n",
			getpid() );
		pChannel_->bundle().startMessage( FloodServerInterface::disconnect,
			true );
		pChannel_->send();

		// start shutting down
		this->nub().breakProcessing();
	}
}


/**
 *	Receive a test message from the server containing a large payload. A 4-byte
 *	length prefix is sent, followed by the payload proper (the length is
 *	configured at FloodServerApp's construction time).
 */
void FloodClientApp::msg2( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data )
{
	static int numMsg2 = 0;
	unsigned lenBytes;
	data >> lenBytes;

	TRACE_MSG( "FloodClientApp(%d)::msg2(%s): #%d (len=%d)\n",
			getpid(), srcAddr.c_str(), numMsg2++, lenBytes );

	const char * buf =
		reinterpret_cast<const char*>( data.retrieve( lenBytes ) );


	if (lenBytes != PAYLOAD_SIZE)
	{
		ERROR_MSG( "FloodClientApp(%d)::msg2: invalid payload size=%u\n",
			getpid(), lenBytes );
		errorStatus_ = 1;
		this->nub().breakProcessing();
		return;
	}

	// verify the data
	const char * pChar = buf;
	while ((unsigned)(pChar - buf) < lenBytes)
	{
		if (*pChar != 'b')
		{
			ERROR_MSG( "FloodClientApp(%d)::msg2: "
				"invalid message payload, index %td char=%d\n",
				getpid(), pChar - buf, *pChar );
			errorStatus_ = 1;
			this->nub().breakProcessing();
			return;
		}
		++pChar;
	}

	if (data.remainingLength())
	{
		ERROR_MSG( "FloodClientApp(%d)::msg2: "
				"message data has remaining length of %d\n",
			getpid(), data.remainingLength() );
		errorStatus_ = 1;
		this->nub().breakProcessing();
	}
}


/**
 *	Class for struct-style Mercury message handler objects for FloodClientApp.
 */
template <class ARGS> class ClientStructMessageHandler :
	public Mercury::InputMessageHandler
{
public:
	typedef void (FloodClientApp::*Handler)( const Mercury::Address & srcAddr,
			const ARGS & args );

	ClientStructMessageHandler( Handler handler ) :
		handler_( handler )
	{}

private:
	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header, BinaryIStream & data )
	{
		ARGS * pArgs = (ARGS*)data.retrieve( sizeof(ARGS) );
		(FloodClientApp::instance().*handler_)( srcAddr, *pArgs );
	}

	Handler handler_;
};

/**
 *	Template class for handling variable length messages.
 */
template <class OBJECT_TYPE>
class VarLenMessageHandler : public Mercury::InputMessageHandler
{
	public:
		/**
		 *	This type is the function pointer type that handles the incoming
		 *	message.
		 */
		typedef void (OBJECT_TYPE::*Handler)( const Mercury::Address & srcAddr,
			Mercury::UnpackedMessageHeader & header,
			BinaryIStream & stream );

		/**
		 *	Constructor.
		 */
		VarLenMessageHandler( Handler handler ) : handler_( handler ) {}

	private:
		// Override
		virtual void handleMessage( const Mercury::Address & srcAddr,
				Mercury::UnpackedMessageHeader & header,
				BinaryIStream & data )
		{
			OBJECT_TYPE * pObject = &OBJECT_TYPE::instance();

			if (pObject != NULL)
			{
				(pObject ->*handler_)( srcAddr, header, data );
			}
			else
			{
				ERROR_MSG( "VarLenMessageHandler::handleMessage: "
					"Could not find object\n" );
			}
		}

		Handler handler_;
};


// -----------------------------------------------------------------------------
// Section: Tests
// -----------------------------------------------------------------------------


/**
 *	Test that the timing method is gettimeofday.
 */
TEST( TestFlood_testTimingMethod )
{
	ASSERT_WITH_MESSAGE( g_timingMethod == GET_TIME_OF_DAY_TIMING_METHOD,
		"Incorrect timing method. "
		"Set environment variable BW_TIMING_METHOD to 'gettimeofday'\n" );
}

MEMTRACKER_DECLARE( TestFlood_testFlood, "TestFlood_testFlood",
	0 );

TEST( TestFlood_testFlood )
{
	MEMTRACKER_SCOPED( TestFlood_testFlood );

	const unsigned numChildren = 1;

	const float maxRunTimeSeconds = NUM_ITERATIONS *
		(TICK_PERIOD / 1000000.) * 3.0;

	TRACE_MSG( "testFlood: numChildren = %d, payload = %d\n",
		1, PAYLOAD_SIZE );

	// Create server and child processes
	FloodServerApp serverApp( PAYLOAD_SIZE,
		(unsigned long)(maxRunTimeSeconds * 1000000L) );

	MultiProcTestCase multiProcTestCase( serverApp );

	for (unsigned i=0; i < numChildren; i++)
	{
		multiProcTestCase.runChild(
			new FloodClientApp( serverApp.nub().address(),
				PAYLOAD_SIZE, NUM_ITERATIONS ) );
	}

	// Run all processes and wait for them to terminate.
	MULTI_PROC_TEST_CASE_WAIT_FOR_CHILDREN( multiProcTestCase );

	INFO_MSG( "testFlood: got %u count, expecting %u\n",
		serverApp.getMsg1Count(),
		numChildren * NUM_ITERATIONS );

	ASSERT_WITH_MESSAGE(
		serverApp.getMsg1Count() >= numChildren * NUM_ITERATIONS,
		"Failed to receive all messages that were expected" );

	ASSERT_WITH_MESSAGE(
		serverApp.getMsg1Count() == numChildren * NUM_ITERATIONS,
		"Received more messages than were expected" );
}

#define DEFINE_SERVER_HERE
#include "test_flood_interfaces.hpp"


// test_flood.cpp
