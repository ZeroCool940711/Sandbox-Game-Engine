/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef COMMON_MACHINE_GUARD_HPP
#define COMMON_MACHINE_GUARD_HPP

#include <stdio.h>
#include <string.h>

#include "cstdmf/debug.hpp"
#include "network/portmap.hpp"
#include "network/endpoint.hpp"
#include "network/machine_guard.hpp"

#include <vector>

#define MAX_BIT_RATE (1<<27)
#define BIT_INCREMENT (MAX_BIT_RATE / 0xFF)
#define MAX_PACKET_RATE 256000
#define PACK_INCREMENT (MAX_PACKET_RATE / 0xFF)

// Version 2: Makes sure to write empty string to name field in MACHINED_VERSION
//            messages
// Version 3: Listener notifications are now sent back to the registration
//            address instead of 127.0.0.1.
// Version 4: Added support for tags specified in /etc/bwmachined.conf
// Version 5: Using mgm.id to pass back replyID for QUERY_TAG_MESSAGE
// Version 6: mgm.pid == -1 in QUERY_TAG_MESSAGE means category is undefined
// Version 7: Now mgm.uid > mgm.pid means category is undefined
// Version 8: MACHINED_VERSION_MESSAGE replies marked as outgoing
// Version 9: Broadcast replies are now staggered (bigworld-1-7-patch only)
// Version 10:Machined's now have complete knowledge of the network
// Version 11:MGM's are variable length and there can be many on a single packet
// Version 12:Messages are sized prefixed and UnknownMessages are added
// Version 13:Bugfixes for dead watcher nubs and SMP cpu stat calculation
// Version 14:Critical fix for possible segfault on user info init
// Version 15:Dead listeners are now auto-deregistered
// Version 16:Memory leak and death notification fixes
// Version 17:Refactored monolithic machine_guard.cpp into classes
// Version 18:Broadcast-reply-based fault tolerance, handles segmentation
// Version 19:Added ResetMessage
// Version 20:Will not create processes under non-existent UIDs
// Version 21:Dropped packet stats are current values, not deltas
// Version 22:Added ErrorMessage
// Version 23:Bugfix for buddy selection post-bootstrap
// Version 24:Check filehandles for /proc files before reading
// Version 25:Fixed fd leak; more error handling for possible fopen() failures
// Version 26:Added support for ANY_UID ListenerMessages
// Version 27:Repeatedly-offset updates fixed, uses 64-bit timestamps
// Version 28:Coredump reporting is done in UserMessages
// Version 29:Fix for segfault in updateProcessStats()
// Version 30:Added UserMessage::PARAM_REFRESH_ENV
// Version 31:Fixed CPU load calculation on SMP systems
// Version 32:Won't refuse to start processes under UID's not in usermap
// Version 33:Reload usermap after a flush request
// Version 34:Broadcast route interface discovery
// Version 35:Preserve processes on restart; set BW_TIMING_METHOD for children
// Version 36:Take caching into account in system memory calculations
// Version 37:Handles changes to system time (fix in cstdmf/time_queue.*)
// Version 38:setgid() before fork() so users have permissions on their procs
// Version 39:No longer send oversized MGMPacket responses. Max 10 core files.
// Version 40:Added CreateWithArgsMessage
// Version 41:Added HighPrecisionMachineMessage

// NOTE: This should stay in sync with the value in pycommon/messages.py
#define BWMACHINED_VERSION 41

extern const char * machinedConfFile;

// every platform should implement these
void initProcessState( bool daemon );
void cleanupProcessState();

// bool updateProcessStats(ThingData & td);
void getProcessorSpeeds( std::vector<float> &speeds );

class BWMachined;

// Returns the PID of the child process
uint16 startProcess( const char * mfRoot,
	const char * bwResPath,
	const char * config,
	const char * type,
	uint16 uid,
	uint16 gid,
	const char * home,
	int argc,
	const char ** argv,
	BWMachined &machined );

// Checks whether a given pid is running
bool checkProcess( uint16 pid );

// Check for coredumps in a particular directory
void checkCoreDumps( const char *mfroot, UserMessage &um );

/**
 * Structs used for maintaining high-resolution statistics.
 */

template <class T>
class Stat
{
public:
	Stat() : v1_(), v2_(), v1curr_( true ) {}
	inline T& cur() { return v1curr_ ? v1_ : v2_; }
	inline T& old() { return v1curr_ ? v2_ : v1_; }
	inline T delta() const { return v1curr_ ? v1_ - v2_ : v2_ - v1_; }
	inline void update (const T &t) {
		if (v1curr_)
			v2_ = t;
		else
			v1_ = t;
		v1curr_ = !v1curr_;
	}
	inline T& next() {
		v1curr_ = !v1curr_;
		return this->cur();
	}
	inline bool state() const { return v1curr_; }

private:
	T v1_, v2_;
	bool v1curr_;
};

typedef Stat< uint64 > HighResStat;

struct MaxStat
{
	HighResStat val, max;
};

struct InterfaceInfo
{
	std::string name;
	HighResStat bitsTotIn, bitsTotOut, packTotIn, packTotOut;
};

struct SystemInfo
{
	uint nCpus, cpuSpeed;
	std::vector< MaxStat > cpu;	//!< Per cpu load information
	MaxStat iowait;		//!< Total time spent waiting for IO
	MaxStat mem;		//!< System wide memory usage

	HighResStat packTotIn, packDropIn, packTotOut, packDropOut;
	std::vector< struct InterfaceInfo > ifInfo;

	WholeMachineMessage m;
	HighPrecisionMachineMessage hpm;
};

struct ProcessInfo
{
	HighResStat cpu, mem;
	uint affinity;

	ProcessStatsMessage m;

	// Platform specific implementation
	void init( const ProcessMessage &pm );
};

bool updateSystemInfoP( SystemInfo &si );
bool updateProcessStats( ProcessInfo &pi );

#endif
