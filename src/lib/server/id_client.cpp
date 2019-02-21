/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "id_client.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/timestamp.hpp"

#include "network/blocking_reply_handler.hpp"
#include "network/bundle.hpp"
#include "network/channel.hpp"

DECLARE_DEBUG_COMPONENT( 0 );

namespace
{
	// Some random cap so that desired size doesn't grow to some ridiculous number
	const size_t MAX_AUTO_INCREMENT_DESIRED_SIZE = 65535;
}

/**
 *	Constructor.
 */
IDClient::IDClient() :
	pChannel_( NULL ),

	highSize_( 0 ),
	desiredSize_( 0 ),
	lowSize_( 0 ),
	criticallyLowSize_( 0 ),

	pendingRequest_( false ),
	inEmergency_( false ),
	pGetMoreMethod_( NULL ),
	pPutBackMethod_( NULL )
{
}


/**
 *	This method initialises the IDClient.
 */
bool IDClient::init(
		Mercury::Channel * pChannel,
		const Mercury::InterfaceElement& getMoreMethod,
		const Mercury::InterfaceElement& putBackMethod,
		size_t criticallyLowSize,
		size_t lowSize,
		size_t desiredSize,
		size_t highSize )
{
	pChannel_ = pChannel;

	pGetMoreMethod_ = &getMoreMethod;
	pPutBackMethod_ = &putBackMethod;
	criticallyLowSize_ = criticallyLowSize;
	lowSize_ = lowSize;
	desiredSize_ = desiredSize;
	highSize_ = highSize;
	inEmergency_ = true;
	pendingRequest_ = false;

	bool isSorted =
		(criticallyLowSize_ < lowSize_) &&
		(lowSize_ < desiredSize_) &&
		(desiredSize_ < highSize_);

	if (!isSorted)
	{
		ERROR_MSG( "IDClient::init: \n"
					"Configuration options must be in increasing order!\n"
					"\tcriticallyLowSize %zu\n"
					"\tlowSize %zu\n"
					"\tdesiredSize %zu\n"
					"\thighSize %zu\n",
				criticallyLowSize,
				lowSize,
				desiredSize,
				highSize );
	}

	return this->getMoreIDsBlocking() && isSorted;
}


/**
 *	Constructor. This stores an id and the time that it is ready to return to
 *	the id pool.
 */
IDClient::LockedID::LockedID( EntityID id )
{
	id_ = id;
	// TODO: put this in bw.xml
	unlockTime_ = timestamp() + uint64(10) * stampsPerSecond();
}


/**
 *	This method accepts an id that is no longer owned by an entity. This allows
 *	the id to later be recycled.
 */
void IDClient::putUsedID( EntityID id )
{
#ifdef MF_ID_RECYCLING
	// TODO: Turn this on again. This is done because recycling can crash the
	// cell. If a reference is kept to the entity, it tries to bring it back to
	// life.
	lockedIDs_.push( id );
#endif
	this->performUpdates( false );
}


/**
 *	This method allocates a new id. If an id could not be allocated, 0 is
 *	returned.
 */
EntityID IDClient::getID()
{
	if (readyIDs_.empty())
	{
		// ARGH there's no ID's left... hurry up and get some!!!
		this->performUpdates( true );
		if (readyIDs_.empty())
		{
			ERROR_MSG("IDClient::getID: no id's left (really bad)\n");
			// return zero... what else can we do?
			return 0;
		}
	}
	EntityID id = readyIDs_.front();
	readyIDs_.pop();
	this->performUpdates( false );
	return id;
}


/**
 *	This method returns this brokers ready ids to the parent.
 */
void IDClient::returnIDs()
{
	// TODO: Is this safe to return all of the locked ids here? Don't we need to
	// make sure that they have expired?
	while (lockedIDs_.size())
	{
		readyIDs_.push( lockedIDs_.front().id_ );
		lockedIDs_.pop();
	}

	if (pChannel_ != NULL)
	{
		Mercury::Bundle & bundle = pChannel_->bundle();
		bundle.startMessage( *pPutBackMethod_ );
		this->placeIDsOntoStream( readyIDs_.size(), readyIDs_, bundle );
		pChannel_->send();
	}
}


/**
 *	This method allows the broker a chance to request extra ids if necessary or
 *	to return excess ids to its parent.
 */
void IDClient::performUpdates( bool isEmergency )
{
	isEmergency = isEmergency || readyIDs_.size() < criticallyLowSize_;

	// readjust our limits if we were faced with an emergency, to try and
	// ensure it doesn't happen again
	if (isEmergency && !inEmergency_)
	{
		if (desiredSize_ < MAX_AUTO_INCREMENT_DESIRED_SIZE/2)
		{
			highSize_ *= 2;
			desiredSize_ *= 2;
			lowSize_ *= 2;
			TRACE_MSG( "IDClient::performUpdates: "
						"increased sizes to (%zu, %zu, %zu)\n",
					lowSize_, desiredSize_, highSize_ );
		}
		else
		{
			// Adjust to maximum size
			float growRatio =
					float(MAX_AUTO_INCREMENT_DESIRED_SIZE)/desiredSize_;
			highSize_ = int( growRatio * highSize_ );
			desiredSize_ = MAX_AUTO_INCREMENT_DESIRED_SIZE;
			lowSize_ = int( growRatio * lowSize_ );
			WARNING_MSG( "IDClient::performUpdates: Sizes are at maximum "
					"(%zu, %zu, %zu). Please slow down entity creation.\n",
					lowSize_, desiredSize_, highSize_ );
		}
		inEmergency_ = true;
	}

#if MF_ID_RECYCLING
	uint64 now = timestamp();

	// go through and unlock any id's that are ready to be reused
	while (lockedIDs_.size() &&
			(lockedIDs_.front().unlockTime_ < now || isEmergency))
	{
		readyIDs_.push( lockedIDs_.front().id_ );
		lockedIDs_.pop();
		isEmergency = false;
	}

	// have we more readyID's than we need?
	if (readyIDs_.size() > highSize_ && !inEmergency_)
		this->putBackIDs();
	// have we fewer readyID's than we need?
	else
#endif
	if (((readyIDs_.size() < lowSize_) || inEmergency_) &&
			!pendingRequest_)
	{
		this->getMoreIDs();
	}
}


/**
 *	This method sends some ids back to the parent.
 */
void IDClient::putBackIDs()
{
#ifdef MF_ID_RECYCLING
	if ((pChannel_ != NULL) && (readyIDs_.size() > desiredSize_))
	{
		Mercury::Bundle & bundle = pChannel_->bundle();
		bundle.startMessage( *pPutBackMethod_ );
		int numIDs = readyIDs_.size() - desiredSize_;
		this->placeIDsOntoStream( numIDs, readyIDs_, bundle );
		pChannel_->send();
	}
#else
	ERROR_MSG( "IDClient::putBackIDs: Recycling has been disabled\n" );
#endif
}


/**
 *	This method requests more IDs from the parent broker. It tries to get the
 *	number of IDs to the desired size in an asychronous way.
 */
void IDClient::getMoreIDs()
{
	this->getMoreIDs( this );
}


/**
 *	This method requests more IDs from the parent broker. It tries to get the
 *	number of IDs to the desired size in an sychronous way.
 */
bool IDClient::getMoreIDsBlocking()
{
	Mercury::BlockingReplyHandler handler( pChannel_->nub(), this );
	this->getMoreIDs( &handler );
	return handler.waitForReply( pChannel_ ) == Mercury::REASON_SUCCESS;
}


/**
 *	This method requests more IDs from the parent broker. It tries to get the
 *	number of IDs to the desired size.
 */
void IDClient::getMoreIDs( Mercury::ReplyMessageHandler * pHandler )
{
	if (!pChannel_)
	{
		INFO_MSG( "IDClient::getMoreIDs: No server yet.\n" );
		return;
	}

	MF_ASSERT( !pendingRequest_ );
	if ((pChannel_ != NULL) && (readyIDs_.size() < desiredSize_))
	{
		Mercury::Bundle & bundle = pChannel_->bundle();

		bundle.startRequest( *pGetMoreMethod_, pHandler );

		int numIDs = desiredSize_ - readyIDs_.size();
		bundle << numIDs;
		pChannel_->send();
		pendingRequest_ = true;
	}
}


/**
 *	This method handles a message that delivers extra ids to this broker.
 */
void IDClient::handleMessage( const Mercury::Address& source,
		Mercury::UnpackedMessageHeader& header, BinaryIStream& data,
		void * arg )
{
	MF_ASSERT( pendingRequest_ );
	size_t oldSize = readyIDs_.size();
	this->retrieveIDsFromStream( readyIDs_, data );
	INFO_MSG( "IDClient::handleMessage: "
				"Number of ids increased from %zd to %zu\n",
			oldSize, readyIDs_.size() );

	pendingRequest_ = false;
	inEmergency_ = false;
	this->performUpdates( false );
}


/**
 *	This method handles the exception if a request for more ids could not be
 *	satisfied.
 */
void IDClient::handleException( const Mercury::NubException& exception,
		void * arg )
{
	MF_ASSERT( pendingRequest_ );
	ERROR_MSG( "IDClient::handleException: failed to fetch more ID's\n" );
	pendingRequest_ = false;
	this->performUpdates( false );
}


/**
 *	This method adds ids from the input queue to the input stream.
 *
 *	@param n	The number of ids to add.
 *	@param stk	The queue to take the ids from.
 *	@param stream	The stream to add the ids to.
 */
void IDClient::placeIDsOntoStream( size_t n, IDQueue& stk,
		BinaryOStream& stream )
{
	MF_ASSERT( n <= stk.size() );
	while (n--)
	{
		stream << stk.front();
		stk.pop();
	}
}


/**
 *	This method reads IDs off this input stream and adds them to the input
 *	queue.
 */
void IDClient::retrieveIDsFromStream( IDQueue& stk,
		BinaryIStream& stream )
{
	while (stream.remainingLength())
	{
		EntityID id;
		stream >> id;
		stk.push( id );
	}
}

// id_client.cpp
