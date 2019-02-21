/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "bwmachined.hpp"
#include "common_machine_guard.hpp"
#include "cstdmf/memory_stream.hpp"
#include "network/portmap.hpp"
#include "network/file_stream.hpp"
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// This is the file that state gets written to in between bwmachined restarts.
const char* BWMachined::STATE_FILE = "/var/run/bwmachined.state";

/// BWMachined Singleton.
BW_SINGLETON_STORAGE( BWMachined )


/**
 *  If we are shutdown with `service bwmachined2 stop`, drop the process table
 *  to a file so we can restart bwmachined without losing track of all our
 *  processes.
 */
static void sigterm( int sig )
{
	if (BWMachined::pInstance())
		BWMachined::pInstance()->save();

	exit( 0 );
}


/**
 *	Constructor.
 */
BWMachined::BWMachined() :
	packetTimeoutHandler_(),
	broadcastAddr_( 0 ),
	ep_(),
	epBroadcast_(),
	epLocal_(),
	cluster_( *this ),
	tags_(),
	timingMethod_( "rdtsc" ),
	systemInfo_(),
	procs_(),
	birthListeners_( *this ),
	deathListeners_( *this ),
	users_(),
	callbacks_()
{

	// Bind the 3 endpoints we listen on
	ep_.socket( SOCK_DGRAM );
	epLocal_.socket( SOCK_DGRAM );
	epBroadcast_.socket( SOCK_DGRAM );

	if (!this->readConfigFile())
	{
		syslog( LOG_CRIT, "Invalid configuration file" );
		exit( 1 );
	}

	// Determine which network interface will be sending broadcast messages
	if (broadcastAddr_ == 0 && !this->findBroadcastInterface())
	{
		syslog( LOG_CRIT, "Failed to determine default broadcast interface" );
		exit( 1 );
	}

	if (!ep_.good() ||
		 ep_.bind( htons( PORT_MACHINED ), broadcastAddr_ ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to '%s'. %s.",
							inet_ntoa((struct in_addr &)broadcastAddr_),
							strerror(errno) );
		exit( 1 );
	}
	ep_.setbroadcast( true );

	if (!epLocal_.good() ||
		 epLocal_.bind( htons( PORT_MACHINED ), LOCALHOST ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to (lo). %s.",
							strerror(errno) );
		exit( 1 );
	}

	if (!epBroadcast_.good() ||
		 epBroadcast_.bind( htons( PORT_MACHINED ), BROADCAST ) == -1)
	{
		syslog( LOG_CRIT, "Failed to bind socket to '%s'. %s.",
							inet_ntoa((struct in_addr &)BROADCAST),
							strerror(errno) );
		exit( 1 );
	}

	cluster_.ownAddr_ = broadcastAddr_;


	// Do all the once-off writing to the SystemInfo struct, things that don't
	// change, like hostname, number of CPUs etc.
	std::vector<float> speeds;
	getProcessorSpeeds( speeds );
	if (speeds.size() == 0)
	{
		syslog( LOG_CRIT, "Unable to obtain any valid processor speed info "
						"from /proc/cpuinfo" );
		exit( 1 );
	}

	char bigname[256];
	gethostname( bigname, sizeof( bigname ) );
	char *walk = strchr( bigname, '.' );
	if (walk != NULL) *walk = '\0';

	SystemInfo &si = systemInfo_;

	si.nCpus = speeds.size();
	si.cpuSpeed = (int)speeds[0];
	for (uint j=0; j < si.nCpus; j++)
		si.cpu.push_back( MaxStat() );

	si.m.cpuSpeed_ = si.cpuSpeed;
	si.m.setNCpus( si.nCpus );
	si.m.hostname_ = bigname;
	si.m.version_ = BWMACHINED_VERSION;
	si.m.outgoing( true );

	si.hpm.cpuSpeed_ = si.cpuSpeed;
	si.hpm.setNCpus( si.nCpus );
	si.hpm.hostname_ = bigname;
	si.hpm.version_ = BWMACHINED_VERSION;
	si.hpm.outgoing( true );

	// Force the first update so we know all the stats are in a readable state
	this->updateSystemInfo();

	// Try to re-read any existing bwmachined state
	this->load();

	// Register SIGTERM handler
	signal( SIGTERM, sigterm );
}

BWMachined::~BWMachined()
{}

// Send a broadcast message out to the network and listen for it on all our
// interfaces to determine what our default broadcast interface is.
bool BWMachined::findBroadcastInterface()
{

	std::map< u_int32_t, std::string > interfaces;
	Endpoint epListen;
	struct timeval tv;
	fd_set fds;
	char streamBuf[ MGMPacket::MAX_SIZE ];

	// Initialise the endpoint
	epListen.socket( SOCK_DGRAM );
	if (!epListen.good() ||
		 epListen.bind( htons( PORT_BROADCAST_DISCOVERY ) ) == -1)
	{
		syslog( LOG_CRIT, "Couldn't bind broadcast listener socket to port %d",
				PORT_BROADCAST_DISCOVERY );
		return false;
	}
	epListen.setbroadcast( true );

	// Perform a discovery of all network interfaces on this host
	if (!epListen.getInterfaces( interfaces ))
	{
		syslog( LOG_CRIT, "Failed to discover network interfaces" );
		return false;
	}

	QueryInterfaceMessage qim;
	bool ok = qim.sendto( epListen, htons( PORT_BROADCAST_DISCOVERY ),
						  BROADCAST, MGMPacket::PACKET_STAGGER_REPLIES );
	if (!ok)
	{
		syslog( LOG_CRIT, "Failed to send broadcast discovery message." );
		return false;
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// Listen for the message we just sent to come back to ourselves.
	while (1)
	{
		FD_ZERO( &fds );
		FD_SET( (int)epListen, &fds );
		int selgot = select( epListen+1, &fds, NULL, NULL, &tv );
		if (selgot == 0)
		{
			syslog( LOG_CRIT,
				"Timed out before receiving any broadcast discovery responses");
			return false;
		}
		else if (selgot == -1)
		{
			syslog( LOG_CRIT, "Broadcast discovery select error. %s.",
					strerror(errno) );
			return false;
		}
		else
		{
			sockaddr_in	sin;

			// Read packet into buffer
			int len = epListen.recvfrom( &streamBuf, sizeof( streamBuf ), sin );
			if (len == -1)
			{
				syslog( LOG_ERR, "Broadcast discovery recvfrom error. %s.",
						strerror( errno ) );
				continue;
			}

			syslog( LOG_INFO, "Broadcast discovery receipt from %s.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr) );

			// check messages received against the list of our interfaces
			std::map< u_int32_t, std::string >::iterator iter;

			iter = interfaces.find( (u_int32_t &)sin.sin_addr.s_addr );
			if (iter != interfaces.end())
			{
				syslog( LOG_INFO,
					"Confirmed %s (%s) as default broadcast route interface.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr),
					iter->second.c_str() );
				broadcastAddr_ = sin.sin_addr.s_addr;
				break;
			}

			syslog( LOG_ERR,
					"Broadcast discovery %s not a valid interface.",
					inet_ntoa((struct in_addr&)sin.sin_addr.s_addr) );
		}
	}

	return true;
}


/**
 *  Writes this machine's process table to /var/run/bwmachined.state so it can
 *  be restarted under the same state.
 */
void BWMachined::save()
{
	FileStream out( STATE_FILE, "w" );

	if (out.error())
	{
		syslog( LOG_ERR, "Couldn't write process state to %s: %s",
			STATE_FILE, out.strerror() );
		unlink( STATE_FILE );
		return;
	}

	for (unsigned i=0; i < procs_.size(); i++)
	{
		ProcessInfo &pi = procs_[i];
		out << pi.cpu << pi.mem << pi.affinity;
		pi.m.write( out );
	}

	if (out.error())
	{
		syslog( LOG_ERR, "Failed to write process table to %s: %s",
			STATE_FILE, out.strerror() );
		unlink( STATE_FILE );
		return;
	}

	if (procs_.size() > 0)
	{
		syslog( LOG_INFO, "Wrote %zu entries to %s prior to shutdown",
			procs_.size(), STATE_FILE );
	}
}


/**
 *  Reads a process table from /var/run/bwmachined.state.
 */
void BWMachined::load()
{
	FileStream in( STATE_FILE, "r" );
	struct stat statinfo;

	// If the file doesn't exist, just bail silently
	if (in.error())
		return;

	// If the file is too old, blow it away.  10 minutes should be long enough to
	// restart machined for most people.
	if (in.stat( &statinfo ) == 0)
	{
		time_t age = time( NULL ) - statinfo.st_mtime;
		if (age > 10 * 60)
		{
			syslog( LOG_INFO, "Ignoring out-of-date %s (%d seconds old)",
				STATE_FILE, (int)age );
			unlink( STATE_FILE );
			return;
		}
	}
	else
	{
		syslog( LOG_ERR, "Couldn't stat %s: %s",
			STATE_FILE, in.strerror() );
		return;
	}

	int len = in.length();
	while (in.tell() < len)
	{
		ProcessInfo pi;
		in >> pi.cpu >> pi.mem >> pi.affinity;
		pi.m.read( in );

		if (in.error())
		{
			syslog( LOG_ERR, "Process table in %s corrupt",
				STATE_FILE );
			unlink( STATE_FILE );
			return;
		}

		procs_.push_back( pi );
		syslog( LOG_INFO, "Restored %s (pid:%d uid:%d)",
			pi.m.name_.c_str(), pi.m.pid_, pi.m.uid_ );
	}

	if (len > 0)
	{
		syslog( LOG_INFO, "Restored %zu entries from %s",
			procs_.size(), STATE_FILE );
	}

	unlink( STATE_FILE );
}


/**
 *  Close all of the open sockets owned by this BWMachined object.  This is
 *  called from inside startProcess() to ensure that child processes don't hold
 *  onto these sockets.
 */
void BWMachined::closeEndpoints()
{
	ep_.close();
	epBroadcast_.close();
	epLocal_.close();
}


// Read the tags from the configuration file.
bool BWMachined::readConfigFile()
{
	tags_.clear();

	FILE * file = fopen( machinedConfFile, "r" );
	if (file == NULL)
	{
		syslog( LOG_WARNING, "Global config file %s doesn't exist",
			machinedConfFile );
		return true;
	}

	char buf[ 512 ];
	std::string currTag;

	bool isOkay = true;

	while (isOkay && (fgets( buf, sizeof( buf ), file ) != NULL))
	{
		if (buf[0] == '#' || buf[0] == 0 || buf[0] == '\n')
			continue;

		int len = strlen( buf );

		if (buf[0] == '[')
		{
			if ((buf[ len - 1 ] == '\n') &&
				(buf[ len - 2 ] == ']'))
			{
				buf[ len - 2 ] = '\0';
				currTag = buf + 1;

				// We want to have it added even if it is empty.
				tags_[ currTag ];
			}
			else
			{
				isOkay = false;
				syslog( LOG_ERR, "Invalid tag '%s'\n", buf );
			}
		}
		else if (!currTag.empty())
		{
			if (buf[ len - 1 ] == '\n')
			{
				buf[ len - 1 ] = '\0';
				tags_[ currTag ].push_back( std::string( buf ) );
			}
			else
			{
				syslog( LOG_ERR, "Max tag length is %zu\n", sizeof( buf ) - 1 );
				isOkay = false;
			}
		}
	}

	fclose( file );

	if (!isOkay)
	{
		return false;
	}

	// Extract the [TimingMethod] tag if it was specified
	TagsMap::iterator it = tags_.find( "TimingMethod" );
	if (it != tags_.end() && it->second.size() > 0)
	{
		timingMethod_ = it->second[0];
		syslog( LOG_INFO, "Using %s timing", timingMethod_.c_str() );
	}


	it = tags_.find( "InternalInterface" );
	if (it != tags_.end() && it->second.size() > 0)
	{
		std::string internalIfaceStr = it->second[0];

		typedef std::map< u_int32_t, std::string > Interfaces;
		Interfaces interfaces;
		if (!ep_.getInterfaces( interfaces ) || interfaces.empty())
		{
			syslog( LOG_ERR, "Could not get interfaces" );
			return false;
		}

		// Use this as the broadcast interface address if it's a valid dotted
		// decimal address string.
		if (inet_aton( internalIfaceStr.c_str(),
					(in_addr *)(&broadcastAddr_) ))
		{
			Interfaces::iterator iIface = interfaces.find( broadcastAddr_ );
			if (iIface == interfaces.end())
			{
				syslog( LOG_ERR, "Could not find configured internal interface "
						"'%s' in interfaces for this machine, falling back to "
						"auto-discovery",
					internalIfaceStr.c_str() );
				broadcastAddr_ = 0;
			}
			else
			{
				syslog( LOG_INFO, "Using configured internal interface "
						"identified by address %s (%s)\n",
					inet_ntoa( (in_addr&)(broadcastAddr_)),
					iIface->second.c_str() );
			}
		}
		else
		{
			// Otherwise, assume this is the name of the interface, try and
			// find it in the map of interfaces.
			Interfaces::iterator iIface = interfaces.begin();
			bool found = false;

			while (iIface != interfaces.end())
			{
				if (iIface->second == internalIfaceStr)
				{
					found = true;
					broadcastAddr_ = iIface->first;
					syslog( LOG_INFO, "Using configured internal interface %s (%s)",
							inet_ntoa( (in_addr&)broadcastAddr_ ),
							iIface->second.c_str() );
					break;
				}
				++iIface;
			}

			if (!found)
			{
				syslog( LOG_ERR, "Could not find configured internal "
						"interface named '%s', falling back to auto-discovery",
					internalIfaceStr.c_str() );
			}
		}
	}
	return true;
}

int BWMachined::run()
{
	// Generate a timestamp to set the base timestamp offset, since the
	// subsequent calls to addTimer() will cause the offset to get stuck high
	this->timeStamp();

	cluster_.birthHandler_.addTimer();
	cluster_.floodTriggerHandler_.addTimer();

	// The UpdateHandler is the thing that updates machine and process
	// statistics, checks for dead listeners etc, basically all the housekeeping
	// we want every so often.
	UpdateHandler updateHandler( *this );
	callbacks_.add( this->timeStamp() + UPDATE_INTERVAL,
		UPDATE_INTERVAL, &updateHandler, NULL );

	// listen for requests
	syslog( LOG_INFO, "Listening for requests" );
	// Obtain the highest FD for the call to select()
	int maxfd = std::max( (int)ep_, std::max( (int)epBroadcast_, (int)epLocal_ ) );
	for(;;)
	{
		struct timeval tv;
		fd_set fds;

		// process packets until we time out
		for(;;)
		{
			// Process scheduled callbacks
			TimeQueue64::TimeStamp tickTime = this->timeStamp();
			callbacks_.process( tickTime );

			// We'll select() until the next timer is due to go off
			TimeQueue64::TimeStamp ttn = callbacks_.nextExp( tickTime );
			timeStampToTV( ttn, tv );

			// Note a Win32 improvement could be done here.  Issues an
			// overlapped RecvFrom and then do a WaitForMultipleObjects on the
			// Socket as well as the process handles

			FD_ZERO( &fds );
			FD_SET( (int)ep_, &fds );
			FD_SET( (int)epBroadcast_, &fds );
			FD_SET( (int)epLocal_, &fds );
			int selgot = select( maxfd+1, &fds, NULL, NULL, &tv );
			if (selgot == 0) break;
			if (selgot == -1)
			{
				if(errno != EINTR)
				{
					syslog( LOG_CRIT, "select got an error: %s\n",
						strerror( errno ) );
					return 1;
				}

				syslog( LOG_ERR, "select returned %d - %s\n",
					errno, strerror( errno ) );
				continue;
			}

			// Read from any of the endpoints that have data
			if (FD_ISSET( (int)ep_, &fds ))
			{
				this->readPacket( ep_, tickTime );
			}

			if (FD_ISSET( (int)epLocal_, &fds ))
			{
				this->readPacket( epLocal_, tickTime );
			}

			if (FD_ISSET( (int)epBroadcast_, &fds ))
			{
				this->readPacket( epBroadcast_, tickTime );
			}
		}
	}

	return 0;
}


void BWMachined::readPacket( Endpoint & ep, TimeQueue64::TimeStamp & tickTime )
{
	// ok, let's read the request then
	static char streamBuf[ MGMPacket::MAX_SIZE ];
	sockaddr_in	sin;

	// Read packet into buffer
	int len = ep.recvfrom( &streamBuf, sizeof( streamBuf ), sin );
	if (len == -1)
	{
		syslog( LOG_ERR, "recvfrom got an error: %s\n", strerror( errno ) );
		return;
	}

	// Destream the packet
	MemoryIStream is( streamBuf, len );
	MGMPacket *packet = new MGMPacket( is );
	if (is.error())
	{
		syslog( LOG_ERR, "Dropping packet with bogus message" );
		delete packet;
		return;
	}

	// Schedule broadcast packets for later
	if (packet->flags_ & packet->PACKET_STAGGER_REPLIES)
	{
		callbacks_.add( tickTime + rand() % STAGGER_REPLY_PERIOD,
						0, &packetTimeoutHandler_,
						new IncomingPacket( *this, packet, ep_, sin ) );
		return;
	}

	this->handlePacket( ep, sin, *packet );
	delete packet;
}


void BWMachined::updateSystemInfo()
{
	static bool firstTime = true;
	SystemInfo &si = systemInfo_;

	// Do the platform specific stuff
	if (!updateSystemInfoP( si ))
		return;

	// Do platform specific updates again if this is the first execution of this
	// function, since all the 'old' values won't have been set yet and we'll
	// get errors unless we update twice.
	if (firstTime)
	{
		firstTime = false;
		if (!updateSystemInfoP( si ))
			return;
	}

	// Write new CPU and mem stats into the MGM
	for (uint i=0; i < si.nCpus; i++)
	{
		uint64 cpuDiff = std::max( si.cpu[i].max.delta(), (uint64)1 );
		uint8 cpuLoad = (uint8)(si.cpu[i].val.delta()*0xff / cpuDiff);

		// val.delta() is effectively the % of CPU usage as the jiffy time on
		// x86 should result in a total diff of 100.
		si.m.cpuLoads_[i] = cpuLoad;

		// High Precision statistics
		si.hpm.cpuLoads_[i] = cpuLoad;
	}

	// If extended stats are being read from /proc/stat we can use the
	// IO wait time as well.
	uint64 ioWaitDiff = std::max( si.iowait.max.delta(), (uint64)1 );
	si.hpm.ioWaitTime_ = (uint8)((si.iowait.val.delta() * 0xff) / ioWaitDiff);

	si.m.mem_   = (uint8)(si.mem.val.cur()*0xff / si.mem.max.cur());
	si.hpm.mem_ = (uint8)(si.mem.val.cur()*0xff / si.mem.max.cur());

	// Write network stats into the MGM
	if (si.m.ifStats_.empty())
	{
		si.m.setNInterfaces( si.ifInfo.size() );
	}

	if (si.hpm.ifStats_.empty())
	{
		si.hpm.setNInterfaces( si.ifInfo.size() );
	}


	// Calculate the interface stats
	for (int i=0; i < si.m.nInterfaces_; i++)
	{

		// Low Precision interface stats
		si.m.ifStats_[i].name_ = si.ifInfo[i].name;

		si.m.ifStats_[i].bitsIn_ = std::min(
			si.ifInfo[i].bitsTotIn.delta() / BIT_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].bitsOut_ = std::min(
			si.ifInfo[i].bitsTotOut.delta() / BIT_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].packIn_ = std::min(
			si.ifInfo[i].packTotIn.delta() / PACK_INCREMENT, (uint64)0xff );

		si.m.ifStats_[i].packOut_ = std::min(
			si.ifInfo[i].packTotOut.delta() / PACK_INCREMENT, (uint64)0xff );

		// High Precision interface stats
		si.hpm.ifStats_[ i ].name_    = si.ifInfo[ i ].name;
		si.hpm.ifStats_[ i ].bitsIn_  = si.ifInfo[ i ].bitsTotIn.delta();
		si.hpm.ifStats_[ i ].bitsOut_ = si.ifInfo[ i ].bitsTotOut.delta();
		si.hpm.ifStats_[ i ].packIn_  = si.ifInfo[ i ].packTotIn.delta();
		si.hpm.ifStats_[ i ].packOut_ = si.ifInfo[ i ].packTotOut.delta();
	}

	// System-wide IP level packet loss stats
	si.m.inDiscards_ = (uint8)std::min( si.packDropIn.cur(), (uint64)0xff );
	si.m.outDiscards_ = (uint8)std::min( si.packDropOut.cur(), (uint64)0xff );

	si.hpm.inDiscards_ = (uint8)std::min( si.packDropIn.cur(), (uint64)0xff );
	si.hpm.outDiscards_ = (uint8)std::min( si.packDropOut.cur(), (uint64)0xff );
}


void BWMachined::update()
{
	// Update the machine's stats
	this->updateSystemInfo();

	// Check on all our processes
	for (unsigned int i=0; i < procs_.size(); i++)
	{
		ProcessInfo &pi = procs_[i];
		if (!updateProcessStats( pi ) && errno == ENOENT)
		{
			syslog( LOG_ERR, "%s died without deregistering!\n",
				pi.m.c_str() );
			removeRegisteredProc( i-- );
		}
	}

	// Check on all our listeners too, since they may have gone away
	birthListeners_.checkListeners();
	deathListeners_.checkListeners();

	// Cleanup zombie children that didn't send us a SIGCHLD for some reason
	waitpid( -1, NULL, WNOHANG );
}


/**
 * Inform other machined's on the network about the birth or death of a process
 * on this machine so that they can in turn inform their registered listeners
 * about it.
 */
bool BWMachined::broadcastToListeners( ProcessMessage &pm, int type )
{
	uint8 oldparam = pm.param_;
	pm.param_ = type | pm.PARAM_IS_MSGTYPE;

	bool ok = pm.sendto( ep_, htons( PORT_MACHINED ), BROADCAST,
		MGMPacket::PACKET_STAGGER_REPLIES );

	pm.param_ = oldparam;
	return ok;
}

void BWMachined::removeRegisteredProc( unsigned index )
{
	if (index >= procs_.size())
	{
		syslog( LOG_ERR, "Can't remove reg proc at index %d/%zu",
			index, procs_.size() );
		return;
	}

	ProcessInfo &pinfo = procs_[ index ];
	ProcessMessage pm;
	pm << pinfo.m;
	this->broadcastToListeners( pm, pm.NOTIFY_DEATH );

	procs_.erase( procs_.begin() + index );
}

void BWMachined::sendSignal (const SignalMessage & sm)
{
	for (uint i=0; i < procs_.size(); i++)
	{
		ProcessInfo &pm = procs_[i];

		if (pm.m.matches( sm ))
		{
			kill( pm.m.pid_, sm.signal_ );

			syslog( LOG_INFO, "sendSignal: signal = %d pid = %d uid = %d",
				sm.signal_, pm.m.pid_, pm.m.uid_ );
		}
	}
}

// Basically just invokes the handler for each message on the packet
void BWMachined::handlePacket( Endpoint & ep, sockaddr_in &sin,
	MGMPacket &packet )
{
	MGMPacket replies;
	for (unsigned i=0; i < packet.messages_.size(); i++)
		if (!this->handleMessage( sin, *packet.messages_[i], replies ))
			return;

	if (replies.messages_.size() > 0)
	{
		MemoryOStream os;

		if (replies.write( os ))
		{
			ep.sendto( os.data(), os.size(), sin );
		}
		else
		{
			// The most likely cause of this will be due to an over sized
			// MGMPacket.
			syslog( LOG_ERR, "handlePacket: Unable to generate reponse to "
				"message from %s.", ep.c_str() );
		}

	}
}

bool BWMachined::handleMessage( sockaddr_in &sin, MachineGuardMessage &mgm,
	MGMPacket &replies )
{
	switch (mgm.message_)
	{

	case MachineGuardMessage::LISTENER_MESSAGE:
	{
		ListenerMessage &lm = static_cast< ListenerMessage& >( mgm );

		if (lm.param_ == (lm.ADD_BIRTH_LISTENER | lm.PARAM_IS_MSGTYPE))
			birthListeners_.add( lm, sin.sin_addr.s_addr );
		else if (lm.param_ == (lm.ADD_DEATH_LISTENER | lm.PARAM_IS_MSGTYPE))
			deathListeners_.add( lm, sin.sin_addr.s_addr );
		else
		{
			syslog( LOG_ERR, "Unrecognised param field for ListenerMessage: %x",
				lm.param_ );
			return false;
		}

		// Ack to sender
		lm.outgoing( true );
		replies.append( lm );

		return true;
	}

	// Note: this is identical (virtually) to the case below, any changes
	// should be mirrored there too.
	case MachineGuardMessage::WHOLE_MACHINE_MESSAGE:
	{
		// This should be machines replying to a broadcast keepalive poll
		if (mgm.outgoing())
		{
			uint32 inaddr = sin.sin_addr.s_addr;

			if (cluster_.pFloodReplyHandler_ &&
				mgm.seq() == cluster_.pFloodReplyHandler_->seq())
			{
				cluster_.pFloodReplyHandler_->markReceived( inaddr );
			}
			else
			{
				syslog( LOG_ERR, "Unsolicited WMM from %s:%d",
					inet_ntoa( (in_addr&)inaddr ), ntohs( sin.sin_port ) );
			}
		}

		// These are regular requests that could be coming from tools or other
		// machineds
		else
		{
			systemInfo_.m.copySeq( mgm );
			replies.append( systemInfo_.m );
		}
		return true;
	}

	// Note: this is identical (virtually) to the case above, any changes
	// should be mirrored there too.
	case MachineGuardMessage::HIGH_PRECISION_MACHINE_MESSAGE:
	{
		// This should be machines replying to a broadcast keepalive poll
		if (mgm.outgoing())
		{
			uint32 inaddr = sin.sin_addr.s_addr;

			if (cluster_.pFloodReplyHandler_ &&
				mgm.seq() == cluster_.pFloodReplyHandler_->seq())
			{
				cluster_.pFloodReplyHandler_->markReceived( inaddr );
			}
			else
			{
				syslog( LOG_ERR, "Unsolicited HPMM from %s:%d",
					inet_ntoa( (in_addr&)inaddr ), ntohs( sin.sin_port ) );
			}
		}

		// These are regular requests that could be coming from tools or other
		// machineds
		else
		{
			systemInfo_.hpm.copySeq( mgm );
			replies.append( systemInfo_.hpm );
		}
		return true;
	}

	case MachineGuardMessage::PROCESS_MESSAGE:
	{
		ProcessMessage &pm = static_cast< ProcessMessage& >( mgm );

		// Explicit instances of this class shouldn't be using param filters
		if (!(pm.param_ & pm.PARAM_IS_MSGTYPE))
		{
			syslog( LOG_ERR, "PROCESS_MESSAGE tried to use param filters! (%x)",
				pm.param_ );
			return false;
		}

		int msgtype = pm.param_ & ~pm.PARAM_IS_MSGTYPE;

		// Don't allow other machines to register/deregister their processes here
		if ((msgtype == pm.REGISTER || msgtype == pm.DEREGISTER) &&
			(uint32&)sin.sin_addr != cluster_.ownAddr_ &&
			(uint32&)sin.sin_addr != LOCALHOST)
		{
			syslog( LOG_ERR, "%s tried to register a process here!",
				inet_ntoa( sin.sin_addr ) );
			return false;
		}

		switch (msgtype)
		{
		case ProcessMessage::REGISTER:
		{
			// Make sure this new proc doesn't clash with an existing one
			unsigned int i = 0;
			while (i < procs_.size())
			{
				ProcessMessage &psm = procs_[i].m;

				if (pm.pid_ == psm.pid_ && pm.category_ == psm.category_ &&
					pm.name_ == psm.name_)
					break;

				// can't have two things on one port
				if (pm.port_ == psm.port_)
				{
					syslog( LOG_ERR, "%d registered on port (%d) "
						"that belonged to %d",
						pm.pid_, pm.port_, psm.pid_ );
					removeRegisteredProc( i );
				}
				else
					++i;
			}

			// This indicates re-registration.  This is bad.
			if (i < procs_.size())
				syslog( LOG_ERR, "Received re-registration for %s\n",
					pm.c_str() );

			// Make new entry for the new process
			else
				procs_.push_back( ProcessInfo() );

			// Write the registration info into the ProcessStatsMessage and mark it
			// as outgoing for future sends
			ProcessInfo &pi = procs_[i];
			pi.m << pm;
			pi.m.outgoing( true );

			// Update it twice to ensure fields are in a readable state
			for (int j=0; j < 2; j++) updateProcessStats( pi );

			// platform-specific initialisation
			pi.init( pm );

			// Tell listeners about it
	 		broadcastToListeners( pm, pm.NOTIFY_BIRTH );

			// and confirm the registration to the sender
			pm.outgoing( true );
			replies.append( pm );

			syslog( LOG_INFO, "Added %s at %d\n", pm.c_str(), i );
			return true;
		}

		case ProcessMessage::DEREGISTER:
		{
			unsigned int i;
			for (i=0; i < procs_.size(); i++)
				if (pm.pid_ == procs_[i].m.pid_)
					break;

			if (i >= procs_.size())
				syslog( LOG_ERR, "Couldn't find pid %d to deregister it\n",
					pm.pid_ );
			else
				removeRegisteredProc( i );

			// confirm the deregistration to the sender
			pm.outgoing( true );
			replies.append( pm );

			syslog( LOG_INFO, "Deregistered %s\n", pm.c_str() );
			return true;
		}

		case ProcessMessage::NOTIFY_BIRTH:
		{
			birthListeners_.handleNotify( pm, sin.sin_addr );
			return true;
		}

		case ProcessMessage::NOTIFY_DEATH:
		{
			deathListeners_.handleNotify( pm, sin.sin_addr );
			return true;
		}

		default:
			syslog( LOG_ERR, "Unrecognised ProcessMessage type: %d", msgtype );
			return false;
		}
	}

	case MachineGuardMessage::PROCESS_STATS_MESSAGE:
	{
		ProcessStatsMessage &query = static_cast< ProcessStatsMessage& >( mgm );

		// Find the processes this matches
		bool found = false;
		for (std::vector< ProcessInfo >::iterator it = procs_.begin();
			 it != procs_.end(); ++it)
		{
			ProcessInfo &pi = *it;
			if (pi.m.matches( query ))
			{
				// Update load and mem stats on the MGM for this process
				SystemInfo &si = systemInfo_;
				uint64 cpuDiff = std::max(
					si.cpu[ pi.affinity % si.nCpus ].max.delta(),
					(uint64)1 );

				if (pi.affinity >= si.nCpus)
					syslog( LOG_ERR, "ProcessInfo (%s) has invalid affinity %d",
						pi.m.c_str(), pi.affinity );

				ProcessStatsMessage &reply = pi.m;
				reply.cpu_ = (uint8)(pi.cpu.delta()*0xff / cpuDiff);
				reply.mem_ = (uint8)(pi.mem.cur()*0xff / si.mem.max.cur());

				// Add reply to the stream
				reply.copySeq( query );
				replies.append( reply );
				found = true;
			}
		}

		// If nothing found, send back a message with pid == 0 so that recv
		// loops can terminate on client side
		if (!found)
		{
			query.pid_ = 0;
			query.outgoing( true );
			replies.append( query );
		}

		return true;
	}

	case MachineGuardMessage::CREATE_MESSAGE:
	case MachineGuardMessage::CREATE_WITH_ARGS_MESSAGE:
	{
		CreateMessage &cm = static_cast< CreateMessage& >( mgm );
		syslog( LOG_INFO, "Got message: %s", cm.c_str() );

		// We tell the client what the pid of the started process is
		PidMessage *pPm = new PidMessage();
		pPm->copySeq( cm );
		pPm->outgoing( true );
		replies.append( *pPm, true );

		// Extract the user message for this guy and refresh his config info
		UserMessage *pUm = users_.fetch( cm.uid_ );

		// If the user is not mapped yet, add the mapping now
		if (pUm == NULL)
		{
			struct passwd *ent = getpwuid( cm.uid_ );
			if (ent == NULL)
			{
				syslog( LOG_ERR, "UID %d doesn't exist on this system, "
					"not starting %s", cm.uid_, cm.name_.c_str() );
				pPm->running_ = 0;
				return true;
			}

			pUm = users_.add( ent );
		}

		if (!users_.getEnv( *pUm ))
		{
			syslog( LOG_ERR, "Couldn't get env for user %s, not starting %s",
				pUm->username_.c_str(), cm.c_str() );
			pPm->running_ = 0;
			return true;
		}

		// Make sure there are no ..'s in the exe path so that we are definitely
		// executing something from within bigworld/bin/Hybrid
		if (cm.name_.find( ".." ) != std::string::npos ||
			cm.config_.find( ".." ) != std::string::npos)
		{
			syslog( LOG_ERR,
				"Illegal '..' in process name or config, not starting %s/%s",
				cm.name_.c_str(), cm.config_.c_str() );

			pPm->running_ = 0;
			return true;
		}

		// magic +2 args is for --res <blah> in startProcess()
		static const unsigned int NUM_SPARE_ARGS_FOR_STARTPROCESS = 2;

		if (mgm.message_ == MachineGuardMessage::CREATE_MESSAGE)
		{
			// figure out what arguments we want to pass it
			unsigned int argc = 0;
			static const unsigned int MAX_ARGC = 10;

			const char *argv[ MAX_ARGC + NUM_SPARE_ARGS_FOR_STARTPROCESS ];

			// platform-specific code should fill in argv[0] once path is
			// decided
			argv[ argc++ ] = NULL;

			// first parameter as -machined signifies that we're starting things up
			argv[ argc++ ] = "-machined";

			// see if we are recovering
			if (cm.recover_)
				argv[ argc++ ] = "-recover";

			char forwardArg[32];
			if (cm.fwdIp_ != 0)
			{
				bw_snprintf( forwardArg, sizeof( forwardArg ), "%s:%d",
					inet_ntoa( (in_addr&)cm.fwdIp_ ),
					ntohs( cm.fwdPort_ ) );
				argv[ argc++ ] = "-forward";
				argv[ argc++ ] = forwardArg;
			}

			// make sure the code above doesn't get too unruly
			if (argc >= MAX_ARGC)
			{
				syslog( LOG_ERR, "Not starting process %s for uid %d: "
					"too many args (%d)", cm.name_.c_str(), cm.uid_, argc );
				pPm->running_ = 0;
			}
			else
			{
				pPm->running_ = 1;
				pPm->pid_ = startProcess( pUm->mfroot_.c_str(),
					pUm->bwrespath_.c_str(), cm.config_.c_str(),
					cm.name_.c_str(), pUm->uid_, pUm->gid_, pUm->home_.c_str(),
					argc, argv, *this );
			}
		}
		else	// mgm.message_ == MachineGuardMessage::CREATE_WITH_ARGS_MESSAGE
		{
			CreateWithArgsMessage &cwam =
				static_cast< CreateWithArgsMessage& >( cm );

			unsigned int argc = 0;
			// +2 args for process name and -recover
			const char **argv = new const char*[cwam.args_.size() +
			                        NUM_SPARE_ARGS_FOR_STARTPROCESS + 2];

			// platform-specific code should fill in argv[0] once path is
			// decided
			argv[ argc++ ] = NULL;

			// Append args from message
			for ( CreateWithArgsMessage::Args::const_iterator i =
					cwam.args_.begin(); i != cwam.args_.end(); ++i )
			{
				argv[ argc++ ] = i->c_str();
			}

			// see if we are recovering
			if (cm.recover_)
				argv[ argc++ ] = "-recover";


			pPm->pid_ = startProcess( pUm->mfroot_.c_str(),
				pUm->bwrespath_.c_str(), cm.config_.c_str(), cm.name_.c_str(),
				pUm->uid_, pUm->gid_, pUm->home_.c_str(), argc, argv, *this );

			pPm->running_ = 1;

			delete [] argv;
		}

		return true;
	}

	case MachineGuardMessage::SIGNAL_MESSAGE:
	{
		SignalMessage &sm = static_cast< SignalMessage& >( mgm );
		sendSignal( sm );
		return true;
	}

	case MachineGuardMessage::TAGS_MESSAGE:
	{
		TagsMessage &tm = static_cast< TagsMessage& >( mgm );

		// Queries send through a vector with a single tag in it
		if (tm.tags_.size() != 1)
		{
			syslog( LOG_ERR, "Tags queries must pass only one tag (%zu passed)",
				tm.tags_.size() );
			return false;
		}
		std::string query = tm.tags_[0];
		tm.tags_.clear();

		// An empty query indicates we're asking for the list of tag categories
		if (query == "")
		{
			for (TagsMap::iterator it = tags_.begin(); it != tags_.end(); ++it)
				tm.tags_.push_back( it->first );
			tm.exists_ = true;
		}

		// Otherwise we want a listing of a particular category
		else
		{
			TagsMap::iterator it = tags_.find( query );

			if (it != tags_.end())
			{
				Tags &tags = it->second;
				tm.tags_.resize( tags.size() );
				std::copy( tags.begin(), tags.end(), tm.tags_.begin() );
				tm.exists_ = true;
			}
			else
				tm.exists_ = false;
		}

		// Send the results back
		tm.outgoing( true );
		replies.append( tm );
		return true;
	}

	case MachineGuardMessage::USER_MESSAGE:
	{
		UserMessage &um = static_cast< UserMessage& >( mgm );
		std::vector< UserMessage* > matches;

#		define FAIL 							\
		{ 										\
			users_.notfound_.copySeq( um );		\
			replies.append( users_.notfound_ );	\
			return true;						\
		}

		// If we're using PARAM_USE_UID, do a hash lookup
		if (um.param_ & um.PARAM_USE_UID)
		{
			UserMessage *pMatch = users_.fetch( um.uid_ );
			if (pMatch != NULL)
				matches.push_back( pMatch );
		}

		// Otherwise do linear search through user mapping
		else
		{
			for (UserMap::Map::iterator it = users_.map_.begin();
				 it != users_.map_.end(); ++it)
			{
				if (it->second.matches( um ))
					matches.push_back( &it->second );
			}
		}

		// If no matches
		if (matches.empty())
		{
			// If it was a specific search, forcefully make a new entry for this
			// guy
			if (um.param_ & (um.PARAM_USE_UID | um.PARAM_USE_NAME))
			{
				struct passwd *ent = um.param_ & um.PARAM_USE_UID ?
					getpwuid( um.uid_ ) : getpwnam( um.username_.c_str() );
				if (ent == NULL)
					FAIL;

				matches.push_back( users_.add( ent ) );
			}
			else
				FAIL;
		}

		if (um.param_ & um.PARAM_SET)
		{
			// Must only be one match when setting config
			if (matches.size() > 1)
			{
				syslog( LOG_ERR, "Can't set user config for multiple users: %s",
					um.c_str() );
				FAIL;
			}

			UserMessage &match = *matches[0];
			match.mfroot_ = um.mfroot_;
			match.bwrespath_ = um.bwrespath_;
			match.copySeq( um );
			if (users_.setEnv( match ))
				replies.append( match );
			else
				FAIL;
		}

		else
		{
			for (unsigned i=0; i < matches.size(); i++)
			{
				matches[i]->copySeq( um );

				// Refresh environment if requested
				if (um.param_ & um.PARAM_REFRESH_ENV)
				{
					users_.getEnv( *matches[i] );
				}

				// We need to copy the param from the incoming message into the
				// outgoing one so it knows whether it needs to bother writing
				// coredump info or not.
				matches[i]->param_ = um.param_;
				if (um.param_ & um.PARAM_CHECK_COREDUMPS)
				{
					checkCoreDumps( matches[i]->mfroot_.c_str(), *matches[i] );
				}

				replies.append( *matches[i] );
			}
		}

		return true;
#		undef FAIL
	}

	case MachineGuardMessage::PID_MESSAGE:
	{
		PidMessage &pm = static_cast< PidMessage& >( mgm );
		pm.running_ = checkProcess( pm.pid_ );
		pm.outgoing( true );
		replies.append( pm );
		return true;
	}

	case MachineGuardMessage::RESET_MESSAGE:
	{
		ResetMessage &rm = static_cast< ResetMessage& >( mgm );
		this->readConfigFile();
		users_.flush();
		syslog( LOG_INFO, "Flushing tags and user mapping at %s's request",
			inet_ntoa( sin.sin_addr ) );
		rm.outgoing( true );
		replies.append( rm );
		return true;
	}

	case MachineGuardMessage::MACHINED_ANNOUNCE_MESSAGE:
	{
		MachinedAnnounceMessage &mam =
			static_cast< MachinedAnnounceMessage& >( mgm );

		if (mam.type_ == mam.ANNOUNCE_BIRTH)
		{
			// If you're getting these you're newborn.
			if (mam.outgoing())
			{
				cluster_.birthHandler_.markReceived( sin.sin_addr.s_addr,
					mam.count_ );
				return true;
			}

			// When machines initially announce their own birth, we tell them
			// how many machines are on the cluster
			else
			{
				uint32 inaddr = sin.sin_addr.s_addr;
				if (cluster_.machines_.count( inaddr ) == 0)
				{
					cluster_.machines_.insert( inaddr );
					cluster_.chooseBuddy();
				}

				mam.outgoing( true );
				mam.count_ = cluster_.machines_.size();
				replies.append( mam );
				return true;
			}
		}

		// This is a machined telling us that a machine has gone offline
		else if (mam.type_ == mam.ANNOUNCE_DEATH)
		{
			uint32 deadaddr = (uint32)mam.addr_;

			if (deadaddr != cluster_.ownAddr_)
			{
				char addrstr[32];
				strcpy( addrstr, inet_ntoa( sin.sin_addr ) );
				syslog( LOG_INFO, "%s says %s is gone",
					addrstr, inet_ntoa( (in_addr&)deadaddr ) );

				cluster_.machines_.erase( deadaddr );
				cluster_.chooseBuddy();
			}
			else
			{
				cluster_.birthHandler_.addTimer();
				syslog( LOG_INFO,
					"Reports of my death have been greatly exaggerated!" );
			}
			return true;
		}

		// This is confirming that a particular host exists
		else if (mam.type_ == mam.ANNOUNCE_EXISTS)
		{
			if (cluster_.machines_.count( mam.addr_ ) == 0)
			{
				syslog( LOG_INFO, "Apparently %s is running machined",
					inet_ntoa( (in_addr&)mam.addr_ ) );
				cluster_.machines_.insert( mam.addr_ );
				cluster_.chooseBuddy();
			}
			return true;
		}

		else
		{
			syslog( LOG_ERR, "Received unknown MAM type %d", mam.type_ );
			return false;
		}
	}

	// should limit these messages to only anything coming from the local interface
	case MachineGuardMessage::QUERY_INTERFACE_MESSAGE:
	{
		QueryInterfaceMessage &qim = static_cast< QueryInterfaceMessage& >(mgm);

		if (qim.address_ == QueryInterfaceMessage::INTERNAL)
		{
			qim.address_ = broadcastAddr_;
		}
		else
		{
			syslog( LOG_ERR, "Received QIF request for unknown type %d\n",
						qim.address_ );
		}

		// Now the query has been filled in, respond
		qim.outgoing( true );
		replies.append( qim );

		return true;
	}

	default:
		if (mgm.flags_ & mgm.MESSAGE_NOT_UNDERSTOOD)
		{
			syslog( LOG_ERR, "Received unknown message: %s", mgm.c_str() );
			mgm.outgoing( true );
			replies.append( mgm );
			return true;
		}
		else
		{
			syslog( LOG_ERR, "Unknown message (%d) not marked as "
				"MESSAGE_NOT_UNDERSTOOD!!!", mgm.message_ );
			return false;
		}

	}

	return false;
}

// -----------------------------------------------------------------------------
// Section: Broadcast handling stuff
// -----------------------------------------------------------------------------

BWMachined::IncomingPacket::IncomingPacket( BWMachined &machined,
	MGMPacket *pPacket, Endpoint &ep, sockaddr_in &sin ) :
	machined_( machined ),
	pPacket_( pPacket ),
	pEp_( &ep ),
	sin_( sin )
{
	if (!(pPacket_->flags_ & pPacket_->PACKET_STAGGER_REPLIES))
		syslog( LOG_ERR, "Broadcast packet didn't have flag: %x",
			pPacket_->flags_ );
}

void BWMachined::IncomingPacket::handle()
{
	MemoryOStream os;
	machined_.handlePacket( *pEp_, sin_, *pPacket_ );
}

void BWMachined::PacketTimeoutHandler::handleTimeout(
	TimeQueueId id, void *pUser )
{
	IncomingPacket *pIP = (IncomingPacket*)pUser;
	pIP->handle();
}

void BWMachined::PacketTimeoutHandler::onRelease( TimeQueueId id, void *pUser )
{
	IncomingPacket *pIP = (IncomingPacket*)pUser;
	delete pIP;
}

// -----------------------------------------------------------------------------
// Section: Callback handling stuff
// -----------------------------------------------------------------------------

// Gets a timestamp that can be used as a 'now' value for calls to
// the TimeQueue's methods.
TimeQueue64::TimeStamp BWMachined::timeStamp()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	return ((TimeQueue64::TimeStamp)tv.tv_sec)*1000 + tv.tv_usec/1000;
}

void BWMachined::UpdateHandler::handleTimeout( TimeQueueId id, void *pUser )
{
	machined_.update();
}
