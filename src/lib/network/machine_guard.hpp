/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef MACHINE_GUARD_HPP
#define MACHINE_GUARD_HPP

#include "cstdmf/smartpointer.hpp"
#include "cstdmf/binary_stream.hpp"
#include "endpoint.hpp"
#include "misc.hpp"

#if !defined( PLAYSTATION3 )
#include <signal.h>
#endif

#if !defined( _WIN32 ) && !defined( PLAYSTATION3 )
# include <sys/types.h>
# include <pwd.h>
#else
#define SIGINT 2
#define SIGQUIT 3
#define SIGUSR1 10
#endif


class MachineGuardMessage;
class MemoryIStream;
class MemoryOStream;


/**
 *  This class represents a packet containing MachineGuardMessages.  The only
 *  packet level data is the packet flags and the buddy field.  All other data
 *  (including sequence numbers) is contained in the messages themselves.
 */
class MGMPacket
{
public:
	static const int MAX_SIZE = 32768;

	enum Flags {
		PACKET_STAGGER_REPLIES = 0x1
	};

	typedef std::vector< MachineGuardMessage* > MGMs;

	uint8	flags_;
	uint32	buddy_;
	MGMs	messages_;

protected:
	// Each message is tagged with a flag of whether or not to delete it when
	// this packet is destroyed
	std::vector< bool > delInfo_;

	// If this is true then nothing is cleaned up when this packet is destroyed
	bool	dontDeleteMessages_;

public:
	MGMPacket() : flags_( 0 ), buddy_( 0 ), dontDeleteMessages_( false ) {}
	MGMPacket( MemoryIStream &is ) :
		flags_( 0 ), buddy_( 0 ), dontDeleteMessages_( false )
	{
		this->read( is );
	}

	~MGMPacket();

	// TODO: implement fragmentation here
	void read( MemoryIStream &is );
	bool write( MemoryOStream &os ) const;

	void append( MachineGuardMessage &mgm, bool shouldDelete=false );
	inline void stealMessages() { dontDeleteMessages_ = true; }

	// Machined's use this to set a buddy field that will be written into every
	// reply message instead of the buddy that is actually on the message
	static void setBuddy( uint32 addr );

	// The buddy that is automatically inserted into replies
	static uint32 s_buddy_;
};

class HighPrecisionMachineMessage;
class WholeMachineMessage;
class ProcessMessage;
class ProcessStatsMessage;
class ListenerMessage;
class CreateMessage;
class SignalMessage;
class TagsMessage;
class UserMessage;
class PidMessage;
class ResetMessage;
class ErrorMessage;
class MachinedAnnounceMessage;
class QueryInterfaceMessage;
class CreateWithArgsMessage;

/**
 *  This class represents a message that is sent either to or from a bwmachined2
 *  process.
 */
class MachineGuardMessage
{
public:
	enum Message
	{
		// tool/server -> machined messages
		WHOLE_MACHINE_MESSAGE = 1,
		PROCESS_MESSAGE = 2,
		PROCESS_STATS_MESSAGE = 3,
		LISTENER_MESSAGE = 4,
		CREATE_MESSAGE = 5,
		SIGNAL_MESSAGE = 6,
		TAGS_MESSAGE = 7,
		USER_MESSAGE = 8,
		PID_MESSAGE = 9,
		RESET_MESSAGE = 10,
		ERROR_MESSAGE = 11,
		QUERY_INTERFACE_MESSAGE = 12,
		CREATE_WITH_ARGS_MESSAGE = 13,
		HIGH_PRECISION_MACHINE_MESSAGE = 14,

		// machined -> machined messages
		MACHINED_ANNOUNCE_MESSAGE = 64,
	};

	enum Flags
	{
		MESSAGE_DIRECTION_OUTGOING = 0x1,
		MESSAGE_NOT_UNDERSTOOD = 0x2
	};

	uint8	message_;	// REGISTER_MESSAGE, STATS_MESSAGE etc
	uint8 	flags_;		// MESSAGE_DIRECTION_OUTGOING etc

private:
	uint16 	seq_;		// this is generally set by the sender

public:
	MachineGuardMessage() {};
	MachineGuardMessage( uint8 message, uint8 flags = 0, uint16 seq = 0 );

	virtual ~MachineGuardMessage() {}

	// You should always check is.error() after calling read()
	virtual void read( BinaryIStream &is );
	virtual void write( BinaryOStream &os );
	virtual const char *c_str() const;

	// Create an MGM from the provided stream (this does a 'new' so you must
	// delete later).
	static MachineGuardMessage *create( BinaryIStream &is );

	// Same as above, but creates a MemoryIStream for you before passing to ^^^
	static MachineGuardMessage *create( void *buf, int length );

	// Convert message types to strings
	const char * typeStr() const;

	// Wrap this message in an MGMPacket and send it (i.e. does all the
	// MGMPacket and BinaryStream management for you)
	bool sendto( Endpoint &ep, uint16 port, uint32 addr = BROADCAST,
		uint8 packFlags = 0 );

	/**
	 * @internal
	 * Calling contexts that are interested in the content of MGM replies need
	 * to subclass this and implement whichever of the handlers they are
	 * interested in.  Each handler should return true to keep processing the
	 * packet or false to terminate processing early.
	 */
	class ReplyHandler
	{
	public:
		virtual ~ReplyHandler() {};

		bool handle( MachineGuardMessage &mgm, uint32 addr );

		virtual bool onUnhandledMsg(
			MachineGuardMessage &mgm, uint32 addr );

		virtual bool onHighPrecisionMachineMessage(
			HighPrecisionMachineMessage &hpmm, uint32 addr );
		virtual bool onWholeMachineMessage(
			WholeMachineMessage &wmm, uint32 addr );
		virtual bool onProcessMessage(
			ProcessMessage &pm, uint32 addr );
		virtual bool onProcessStatsMessage(
			ProcessStatsMessage &psm, uint32 addr );
		virtual bool onListenerMessage(
			ListenerMessage &lm, uint32 addr );
		virtual bool onCreateMessage(
			CreateMessage &cm, uint32 addr );
		virtual bool onSignalMessage(
			SignalMessage &sm, uint32 addr );
		virtual bool onTagsMessage(
			TagsMessage &tm, uint32 addr );
		virtual bool onUserMessage(
			UserMessage &um, uint32 addr );
		virtual bool onPidMessage(
			PidMessage &pm, uint32 addr );
		virtual bool onResetMessage(
			ResetMessage &rm, uint32 addr );
		virtual bool onErrorMessage(
			ErrorMessage &em, uint32 addr );
		virtual bool onMachinedAnnounceMessage(
			MachinedAnnounceMessage &mam, uint32 addr );
		virtual bool onQueryInterfaceMessage(
			QueryInterfaceMessage &wmm, uint32 addr );
		virtual bool onCreateWithArgsMessage(
			CreateWithArgsMessage &cwam, uint32 addr );
	};

	Mercury::Reason sendAndRecv( Endpoint &ep, uint32 destaddr,
		ReplyHandler *pHandler = NULL );

	Mercury::Reason sendAndRecv( uint32 srcip, uint32 destaddr,
		ReplyHandler *pHandler = NULL );

	Mercury::Reason sendAndRecvFromEndpointAddr( Endpoint & ep, uint32 destaddr,
		ReplyHandler * pHandler = NULL );

	// Copy sequence number from another message (usually used for tagging
	// pre-generated replies with the seq of an incoming message)
	inline void copySeq( const MachineGuardMessage &mgm )
	{
		seq_ = mgm.seq_;
		seqSent_ = false;
	}

	inline uint16 seq() const { return seq_; }

	// Get a fresh sequence number
	void refreshSeq();

	// Methods for getting/setting flags
	inline void setFlag( uint8 flag, bool status )
	{
		if (status)
			flags_ |= flag;
		else
			flags_ &= ~flag;
	}

	inline bool getFlag( uint8 flag ) const
	{
		return (flags_ & flag) ? true : false;
	}

	inline void outgoing( bool b ) {
		this->setFlag( MESSAGE_DIRECTION_OUTGOING, b ); }
	inline bool outgoing() const {
		return this->getFlag( MESSAGE_DIRECTION_OUTGOING ); }

protected:
	// Scratchpad for derived class implementations of c_str()
	static char s_buf_[ 1024 ];

	// The sequence number ticker
	static uint16 s_seqTicker_;

	// Flag indicating whether the packet has already been sent with its current
	// sequence number.  We use this to automatically generate new sequence
	// numbers for messages that are stored and sent multiple times.
	bool seqSent_;
};


/**
 * Low precision network statistics for a single interface.
 *
 * These low details statistics are used for backwards compatability for
 * anything talking to bwmachined.
 */
struct LowPrecisionInterfaceStats
{
public:
	LowPrecisionInterfaceStats() :
		bitsIn_( 0 ),
		bitsOut_( 0 ),
		packIn_( 0 ),
		packOut_( 0 ) {}

	std::string name_;
	uint8 bitsIn_;
	uint8 bitsOut_;
	uint8 packIn_;
	uint8 packOut_;
};

/**
 * Network statistics for a single interface.
 */
struct HighPrecisionInterfaceStats
{
public:
	HighPrecisionInterfaceStats() :
		bitsIn_( 0 ),
		bitsOut_( 0 ),
		packIn_( 0 ),
		packOut_( 0 ) {}

	std::string name_;
	uint32 bitsIn_;
	uint32 bitsOut_;
	uint32 packIn_;
	uint32 packOut_;
};


/**
 * @internal
 * Query Interface Message.
 *
 * Used to query bwmachined for the address of a specific
 * interface.
 */
class QueryInterfaceMessage: public MachineGuardMessage
{
public:
	QueryInterfaceMessage();
	virtual ~QueryInterfaceMessage();

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	enum {
		INTERNAL = 0x0
	};

	uint32	address_;	//!< Address of the requesting interface

};


/**
 * @internal
 *
 * A HighPrecisionMachineMessage provides a more detailed version of the
 * machine statistics that are provided by a WholeMachineMessage.
 */
class HighPrecisionMachineMessage : public MachineGuardMessage
{
public:
	HighPrecisionMachineMessage();
	virtual ~HighPrecisionMachineMessage();

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );

	virtual const char *c_str() const;

	inline void setNCpus( int n )
	{
		nCpus_ = n;
		cpuLoads_.clear();
		cpuLoads_.resize( n );
	}

	inline void setNInterfaces( int n )
	{
		nInterfaces_ = n;
		ifStats_.clear();
		ifStats_.resize( n );
	}


	std::string	hostname_;			//!< Hostname
	uint16		cpuSpeed_;			//!< CPU speed in MHz
	uint8		nCpus_;				//!< CPU count
	std::vector< uint8 > cpuLoads_;	//!< Load on each CPU

	uint8		ioWaitTime_;		//!< Percent of time spent waiting for IO

	uint8		mem_;				//!< Memory usage
	uint8		nInterfaces_;		//!< Network interface count

	std::vector< struct HighPrecisionInterfaceStats >
				ifStats_;			//!< Network statistics for each interface

	uint8		inDiscards_, outDiscards_; //!< System-wide packet loss stats
	uint8		version_;			//!< Machined version
};




/**
 *  @internal
 *  A WholeMachineMessage provides info about a machine, such as CPU load,
 *  memory load, network throughput, and other system level stuff.
 */
class WholeMachineMessage : public MachineGuardMessage
{
public:
	WholeMachineMessage();
	virtual ~WholeMachineMessage();

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	inline void setNCpus( int n )
	{
		nCpus_ = n;
		cpuLoads_.clear();
		cpuLoads_.resize( n );
	}

	inline void setNInterfaces( int n )
	{
		nInterfaces_ = n;
		ifStats_.clear();
		ifStats_.resize( n );
	}

public:
	std::string	hostname_;			//!< Hostname
	uint16		cpuSpeed_;			//!< CPU speed in MHz
	uint8		nCpus_;				//!< CPU count
	std::vector<uint8> cpuLoads_;	//!< Load on each CPU

	uint8		mem_;				//!< Memory usage
	uint8		nInterfaces_;		//!< Network interface count

	std::vector< struct LowPrecisionInterfaceStats >
				ifStats_;			//!< Network statistics for each interface

	uint8		inDiscards_, outDiscards_; //!< System-wide packet loss stats
	uint8		version_;			//!< Machined version
};

class SignalMessage;

/**
 *  @internal
 *  A ProcessMessage provides info about a running process that is registered
 *  with bwmachined, such as the process name, pid, port, and so on.
 */
class ProcessMessage : public MachineGuardMessage
{
public:
	// Set param to one of the following constants or'ed with PARAM_IS_MSGTYPE
	// if sending one of these messages
	enum Type {
		FIND = 0, // not implemented yet, use ProcessStatsMessage for now
		REGISTER = 1,
		DEREGISTER = 2,
		NOTIFY_BIRTH = 3,
		NOTIFY_DEATH = 4
	};

	// Valid flags for param
	enum Param
	{
		PARAM_USE_CATEGORY = 0x1,
		PARAM_USE_UID = 0x2,
		PARAM_USE_PID = 0x4,
		PARAM_USE_PORT = 0x8,
		PARAM_USE_ID = 0x10,
		PARAM_USE_NAME = 0x20,
		PARAM_IS_MSGTYPE = 0x80
	};

	enum Category
	{
		SERVER_COMPONENT = 0,
		WATCHER_NUB = 1
	};

	uint8			param_;	//!< Bitmap for filtering
	uint8			category_;
	uint16			uid_;
	uint16			pid_;
	uint16			port_;
	uint16			id_;	//!< BigWorld app ID (as in cellapp01)
	std::string		name_;	//!< The nub's interface, e.g. CellAppInterface

	ProcessMessage() :
		MachineGuardMessage( MachineGuardMessage::PROCESS_MESSAGE ) {}

	virtual ~ProcessMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	bool matches( const ProcessMessage &query ) const;

	static const char *categoryToString( uint8 category );

	ProcessMessage & operator<<( const ProcessMessage &pm )
	{
		param_ = pm.param_;
		category_ = pm.category_;
		uid_ = pm.uid_;
		pid_ = pm.pid_;
		port_ = pm.port_;
		id_ = pm.id_;
		name_ = pm.name_;
		return *this;
	}
};

/**
 *  @internal
 *  A ProcessStatsMessage is a more detailed version of ProcessMessage that
 *  provides CPU and memory usage info about the process.
 */
class ProcessStatsMessage : public ProcessMessage
{
public:
	uint8			cpu_;	//!< Process cpu as proportion of max
	uint8			mem_;	//!< Mem usage as proportion of max

	ProcessStatsMessage() : ProcessMessage()
	{
		message_ = MachineGuardMessage::PROCESS_STATS_MESSAGE;
	}

	virtual ~ProcessStatsMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};

/**
 *  @internal
 *  A ListenerMessage is used to register a listener with bwmachined.  Listeners
 *  are informed of various process births and deaths on the network.
 */
class ListenerMessage : public ProcessMessage
{
public:
	enum Type
	{
		ADD_BIRTH_LISTENER = 0,
		ADD_DEATH_LISTENER = 1
	};

	// Pass this in the UID field if you want notifications for births/deaths
	// for any user (i.e. you are a logging daemon).
	static const uint16 ANY_UID = 0xffff;

	std::string		preAddr_;
	std::string		postAddr_;

	ListenerMessage()
	{
		message_ = MachineGuardMessage::LISTENER_MESSAGE;
	}

	virtual ~ListenerMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};

/**
 *  @internal
 *  A CreateMessage is sent to bwmachined to command it to spawn a new process.
 */
class CreateMessage : public MachineGuardMessage
{
public:
	std::string		name_;		//!< Name of executable to start
	std::string		config_;	//!< Hybrid, Debug etc
	uint16			uid_;		//!< UserID to start the process as
	uint8			recover_;	//!< Set to true to start with -recover
	uint32			fwdIp_;		//!< IP to forward output to
	uint16			fwdPort_;	//!< Port to forward output to

	CreateMessage( MachineGuardMessage::Message messageType =
			MachineGuardMessage::CREATE_MESSAGE )
	{
		message_ = messageType;
	}

	virtual ~CreateMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	const char *c_str_name( const char * className ) const;
};

/**
 *  @internal
 *  A CreateMessageArgsMessage is sent to bwmachined to command it to spawn a
 * 	new process. It differs from CreateMessage, in that it allows command-line
 * 	arguments be passed and does not include any automatic BigWorld-specific
 * 	arguments like -recover, -machined and -forward.
 *
 * 	NOTE: The --res argument is still passed though. This may be changed in the
 * 	future.
 */
class CreateWithArgsMessage : public CreateMessage
{
public:
	typedef std::vector< std::string >	Args;

	Args	args_;	//!< Arguments to pass to the command.

	CreateWithArgsMessage() : CreateMessage( CREATE_WITH_ARGS_MESSAGE ) {}
	virtual ~CreateWithArgsMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};

/**
 *  @internal
 *  A SignalMessage is sent to bwmachined to command it to send an interrupt to
 *  a particular process, e.g. SIGINT, SIGQUIT etc.
 */
class SignalMessage : public ProcessMessage
{
public:
	uint8		signal_;

	SignalMessage()
	{
		message_ = MachineGuardMessage::SIGNAL_MESSAGE;
	}

	virtual ~SignalMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	void setControlledShutdown() { signal_ = SIGUSR1; }
	void setKill() { signal_ = SIGINT; }
	void setHardKill() { signal_ = SIGQUIT; }
};

/**
 *  @internal
 *  This is the type of a bwmachined tags list.
 */
typedef std::vector< std::string > Tags;


/**
 *  @internal
 *  A TagsMessage is used to query bwmachined for the various tags that are
 *  defined on that machine in /etc/bwmachined.conf.
 */
class TagsMessage : public MachineGuardMessage
{
public:
	Tags 	tags_;
	uint8	exists_;	//!< Flag to indicate if a category exists at all

	TagsMessage() :
		MachineGuardMessage( MachineGuardMessage::TAGS_MESSAGE ) {}

	virtual ~TagsMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};

/**
 *  @internal
 *  A UserMessage is used to query bwmachined for information about a user.
 */
class UserMessage : public MachineGuardMessage
{
public:
	enum Param {
		PARAM_USE_UID = 0x1,
		PARAM_USE_NAME = 0x2,
		PARAM_CHECK_COREDUMPS = 0x4,
		PARAM_REFRESH_ENV = 0x8,
		PARAM_SET = 0x80
	};

	// Unsuccessful queries will have this in the 'uid' field
	static const int UID_NOT_FOUND = 0xffff;

	/**
	 * @internal
	 * For reporting coredumps
	 */
	struct CoreDump
	{
		std::string filename_;
		std::string assert_;
		uint32      time_;
	};

	typedef std::vector< CoreDump > CoreDumps;

	uint8			param_;
	uint16			uid_;
	uint16			gid_;
	std::string		username_;
	std::string		fullname_;
	std::string		home_;
	std::string		mfroot_;
	std::string		bwrespath_;
	CoreDumps		coredumps_;

	UserMessage() :
		MachineGuardMessage( MachineGuardMessage::USER_MESSAGE ) {}

	virtual ~UserMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

#ifndef _WIN32
	void init( const struct passwd &pwent );
#endif

	bool matches( const UserMessage &query ) const;
	const char *getConfFilename() const;
};

// Streaming operators for the CoreDump structs
/** @internal
 *  @{
 */
BinaryIStream& operator>>( BinaryIStream &is, UserMessage::CoreDump &cd );
BinaryOStream& operator<<( BinaryOStream &os, const UserMessage::CoreDump &cd );
/** @} */


/**
 *  @internal
 *  A ResetMessage causes bwmachined to flush its tags and user mappings and
 *  re-create them.
 */
class ResetMessage : public MachineGuardMessage
{
public:
	ResetMessage() :
		MachineGuardMessage( MachineGuardMessage::RESET_MESSAGE ) {}
};


/**
 *  @internal
 *  An ErrorMessage is the means by which bwmachined communicates errors to
 *  other processes.
 */
class ErrorMessage : public MachineGuardMessage
{
public:
	ErrorMessage() :
		MachineGuardMessage( MachineGuardMessage::ERROR_MESSAGE ),
		severity_( MESSAGE_PRIORITY_ERROR ) {}

	uint8			severity_;
	std::string		msg_;

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};


/**
 *  @internal
 *  A MachinedAnnounceMessage is sent between bwmachined instances when a new
 *  bwmachined appears on the network, and is used to help construct the network
 *  ring.
 */
class MachinedAnnounceMessage : public MachineGuardMessage
{
public:
	enum Type
	{
		ANNOUNCE_BIRTH = 0,
		ANNOUNCE_DEATH = 1,
		ANNOUNCE_EXISTS = 2
	};

	uint8 type_;
	union {
		uint32 count_; // Birth replies use this to tell size of network
		uint32 addr_;  // Death/exists messages use this to indicate machine
	};

	MachinedAnnounceMessage() :
		MachineGuardMessage( MachineGuardMessage::MACHINED_ANNOUNCE_MESSAGE )
	{}

	virtual ~MachinedAnnounceMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};


/**
 *  @internal
 *  A PidMessage is used to query bwmachined as to the existence of a particular
 *  PID on that system.
 */
class PidMessage : public MachineGuardMessage
{
public:
	uint16 	pid_;
	uint8	running_;

	PidMessage() :
		MachineGuardMessage( MachineGuardMessage::PID_MESSAGE )
	{}

	virtual ~PidMessage() {}

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;
};


/**
 *  This message type is unusual - it has no message type enumeration in
 *  MachineGuardMessage::Message, it is designed to echo back unrecognised
 *  messages unmodified except for setting the MESSAGE_NOT_UNDERSTOOD flag.
 */
class UnknownMessage : public MachineGuardMessage
{
public:
	UnknownMessage() : data_( NULL ), len_( 0 ) {}

	virtual ~UnknownMessage();

	virtual void write( BinaryOStream &os );
	virtual void read( BinaryIStream &is );
	virtual const char *c_str() const;

	char *data_;
	uint16 len_;
};

#endif // MACHINE_GUARD_HPP
