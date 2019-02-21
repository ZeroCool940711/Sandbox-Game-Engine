/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef ACK_CELL_APP_DEATH_HELPER_HPP
#define ACK_CELL_APP_DEATH_HELPER_HPP

#include "entity.hpp"
#include "network/interfaces.hpp"
#include "network/basictypes.hpp"

class CellApp;

/**
 *  Helper class to decide when it's OK to ack a CellApp death back to the
 *  CellAppMgr.  We can't do this until:
 *
 *  - All the reals on this app have informed their bases of their addresses.
 *  - All emergencySetCurrentCell messages have been delivered and replied to.
 *
 *  We require both of these things so that all lost reals will be restored, and
 *  so that reals that weren't lost won't be restored.
 *
 *  This class polls for a finished state instead of using a nice callback-based
 *  design because there is no callback mechanism for detecting resolution of
 *  'critical' state on a Channel.  The reason that mechanism is not provided is
 *  that it is too much trouble to implement the streaming of those callbacks
 *  when Channels are offloaded.
 *
 *  TODO: Using requests/replies to check that emergencySetCurrentCell messages
 *  have been delivered is overkill.  Either use anonymous channels and mark
 *  them as critical, or implement a new Mercury mechanism for detecting the
 *  successful delivery of a Bundle (i.e. callback when all packets in a Bundle
 *  have been acked).
 *
 *  This object deletes itself.
 */
class AckCellAppDeathHelper :
	public Mercury::TimerExpiryHandler,
	public Mercury::ReplyMessageHandler
{
public:
	AckCellAppDeathHelper( const Mercury::Address & deadAddr );

	~AckCellAppDeathHelper()
	{
		--s_numInstances_;
	}

	virtual int handleTimeout( Mercury::TimerID id, void * arg );

	virtual void handleMessage( const Mercury::Address & srcAddr,
		Mercury::UnpackedMessageHeader & header,
		BinaryIStream & data,
		void * arg );

	virtual void handleException( const Mercury::NubException & exc, void * );


	/**
	 *  This method adds a real in a critical state to the helper's collection.
	 */
	void addCriticalEntity( Entity * pEntity )
	{
		recentOnloads_.insert( pEntity );
	}


	/**
	 *  This method adds a dead real's ID to the collection.  You should call
	 *  this for every emergencySetCurrentCell message sent.
	 */
	void addBadGhost()
	{
		++numBadGhosts_;
	}


	void startTimer();

private:
	void checkFinished();

	/// Reference to the CellApp instance, for convenience.
	CellApp & app_;

	/// A collection of entities whose channels were in a critical state at the
	/// time the CellApp death notification arrived.
	typedef std::set< EntityPtr > RecentOnloads;
	RecentOnloads recentOnloads_;

	/// The address of the dead app.
	Mercury::Address deadAddr_;

	/// How fast we check this (in microseconds).
	int checkPeriod_;

	/// The number of emergencySetCurrentCell messages we sent.
	int numBadGhosts_;

	/// Our timer ID with the nub.
	Mercury::TimerID timerID_;

	/// When we started the timer
	uint64	startTime_;

	/// Static instance counter
	static int s_numInstances_;
};


#endif // ACK_CELL_APP_DEATH_HELPER_HPP
