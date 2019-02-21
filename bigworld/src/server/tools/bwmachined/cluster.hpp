/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef CLUSTER_HPP
#define CLUSTER_HPP

#include "cstdmf/time_queue.hpp"
#include "cstdmf/timestamp.hpp"
#include "network/endpoint.hpp"
#include "network/machine_guard.hpp"
#include <set>

class BWMachined;

class Cluster
{
public:
	Cluster( BWMachined &machined );

	void chooseBuddy();

	friend class BWMachined;
	typedef std::set< uint32 > Addresses;

protected:

	class ClusterTimeoutHandler : public TimeQueueHandler
	{
	public:
		ClusterTimeoutHandler( Cluster &cluster ) :
			cluster_( cluster ), id_( 0 ) {}

		void onRelease( TimeQueueId id, void *pUser ) {}
		void addTimer();
		TimeQueueId id() const { return id_; }
		void cancel();

		virtual TimeQueue::TimeStamp delay() const = 0;
		virtual TimeQueue::TimeStamp interval() const { return 0; }

	protected:
		Cluster &cluster_;
		TimeQueueId id_;
	};

	// Timeout handler for triggering cluster flood checks
	class FloodTriggerHandler : public ClusterTimeoutHandler
	{
	public:
		static const TimeQueue::TimeStamp AVERAGE_INTERVAL = 2000;

		FloodTriggerHandler( Cluster &cluster ) :
			ClusterTimeoutHandler( cluster ) {}
		void handleTimeout( TimeQueueId id, void *pUser );
		TimeQueue::TimeStamp delay() const;
	};

	// Timeout handler for polling the entire cluster
	class FloodReplyHandler : public ClusterTimeoutHandler
	{
	public:
		static const int MAX_RETRIES = 2;

		FloodReplyHandler( Cluster &cluster );
		void handleTimeout( TimeQueueId id, void *pUser );
		inline void markReceived( uint32 addr ) { replied_.insert( addr ); }
		uint16 seq() const { return wmm_.seq(); }
		void sendBroadcast();

		TimeQueue::TimeStamp delay() const;
		TimeQueue::TimeStamp interval() const { return this->delay(); }

	private:
		Addresses replied_;
		int tries_;
		WholeMachineMessage wmm_;
	};

	// Timeout handler for birth messages.  Basically, when we are born people will
	// start replying to us telling us how many other machines are on the network.
	// We must become aware of this many machines within a reasonable timeframe or
	// we will send out the request again
	class BirthReplyHandler : public ClusterTimeoutHandler
	{
	public:
		BirthReplyHandler( Cluster &cluster ) :
			ClusterTimeoutHandler( cluster ) {}
		void addTimer();
		void handleTimeout( TimeQueueId id, void *pUser );
		void markReceived( uint32 addr, uint32 count );

		TimeQueue::TimeStamp delay() const;

	protected:
		uint32 toldSize_;
	};

	BWMachined &machined_;

	// Set of ip addresses of known machines
	Addresses machines_;

	uint32 ownAddr_;
	uint32 buddyAddr_;

	FloodTriggerHandler floodTriggerHandler_;
	FloodReplyHandler *pFloodReplyHandler_;
	BirthReplyHandler birthHandler_;
};

#endif // CLUSTER_HPP
