"""
An interface to various network protocols implemented by BigWorld server
components and daemons.
"""

import struct
import os
import socket
import random
import select

import socketplus
import util
import memory_stream
import log
import watcher_data_type as WDT
import watcher
from cStringIO import StringIO

# TODO We aren't really using this at the moment
useMulticast = os.environ.has_key( "BW_MULTICAST" )

# ------------------------------------------------------------------------------
# Section: MGMPacket
# ------------------------------------------------------------------------------

class MGMPacket( object ):

	MAX_SIZE = 32768
	PACKET_STAGGER_REPLIES = 0x1

	def __init__( self, stream = None ):
		self.flags = 0
		self.buddy = 0
		self.messages = []
		if stream:
			self.read( stream )

	def __str__( self ):
		sbuddy = socket.inet_ntoa( struct.pack( "I", self.buddy ) )
		return "Flags: 0x%x\n" % self.flags + \
			   "Buddy: %s\n" % sbuddy + \
			   "\n".join( [str( mgm ) for mgm in self.messages] )

	def read( self, stream ):
		self.messages = []
		self.flags, self.buddy = stream.unpack( "BI" )
		while stream.remainingLength() > 0:
			msglen, = stream.unpack( "H" )
			msgstream = memory_stream.MemoryStream( stream.read( msglen ) )
			if msgstream.remainingLength() != msglen:
				log.error( "Only %d bytes on stream, needed %d",
						   msgstream.remainingLength(), msglen )
				raise stream.error

			mgm = MachineGuardMessage.create( msgstream )
			if mgm:
				self.append( mgm )
			else:
				mgm = UnknownMessage()
				msgstream.seek( 0 )
				mgm.read( msgstream )
				self.append( mgm )
				log.error( "Unknown message received:\n %s" % mgm )

	def write( self, stream ):
		stream.pack( ("BI", self.flags, self.buddy) )
		for mgm in self.messages:
			pSize = stream.tell()
			stream.seek( 2, 1 )
			sizeBefore = stream.tell()
			mgm.write( stream )
			sizeAfter = stream.tell()
			stream.seek( pSize )
			stream.pack( ("H", sizeAfter - sizeBefore) )
			stream.seek( sizeAfter )

	def append( self, mgm ):
		self.messages.append( mgm )

	def get( self ):
		stream = memory_stream.MemoryStream()
		self.write( stream )
		return stream.data()

	def set( self, s ):
		stream = memory_stream.MemoryStream( s )
		self.read( stream )

# ------------------------------------------------------------------------------
# Section: MachineGuardMessage
# ------------------------------------------------------------------------------

class MachineGuardMessage( object ):

	MACHINED_PORT = 20018
	MACHINED_VERSION = 41

	# Message types
	WHOLE_MACHINE_MESSAGE = 1
	PROCESS_MESSAGE = 2
	PROCESS_STATS_MESSAGE = 3
	LISTENER_MESSAGE = 4
	CREATE_MESSAGE = 5
	SIGNAL_MESSAGE = 6
	TAGS_MESSAGE = 7
	USER_MESSAGE = 8
	PID_MESSAGE = 9
	RESET_MESSAGE = 10
	ERROR_MESSAGE = 11
	QUERY_INTERFACE_MESSAGE = 12
	CREATE_WITH_ARGS_MESSAGE = 13
	HIGH_PRECISION_MACHINE_MESSAGE = 14

	# Flags
	MESSAGE_DIRECTION_OUTGOING = 0x1
	MESSAGE_NOT_UNDERSTOOD = 0x2

	messageMap = {}
	seqTicker = random.randint( 0, 65536 )

	def __init__( self, message, flags = 0, seq = 0 ):
		self.message = message
		self.flags = flags
		self.seq = seq


	def __str__( self ):
		return "Message: %s\n" % self.messageMap[ self.message ] + \
			   "Flags: %x\n" % self.flags + \
			   "Seq: %d" % self.seq


	@util.synchronized
	def refreshSeq( self ):
		"""
		Generates a new sequence number for this message.
		"""

		MachineGuardMessage.seqTicker = (MachineGuardMessage.seqTicker + 1) % 0xffff
		self.seq = MachineGuardMessage.seqTicker


	def write( self, stream ):
		"""
		As in machine_guard.cpp, calling this method will generate a new
		sequence number for this message, to prevent ever sending the same
		sequence number twice.  This of course assumes that calling this method
		precedes sending this message (a fairly safe assumption I think).
		"""

		self.refreshSeq()
		stream.pack( ("BBH", self.message, self.flags, self.seq) )


	def read( self, stream ):
		try:
			self.message, self.flags, self.seq = stream.unpack( "BBH" )
			return True
		except stream.error:
			return False


	def send( self, sock, addr = "<broadcast>" ):
		try:
			packet = MGMPacket()
			packet.append( self )
			if addr == "<broadcast>":
				packet.flags = packet.PACKET_STAGGER_REPLIES
			stream = memory_stream.MemoryStream()
			packet.write( stream )
			sock.sendto( stream.getvalue(), (addr, self.MACHINED_PORT) )
			return True
		except stream.error:
			return False


	@classmethod
	def create( self, stream ):

		# Set up message id -> subclass mapping if it isn't done yet
		if not self.messageMap:
			self.messageMap[ self.WHOLE_MACHINE_MESSAGE ] = WholeMachineMessage
			self.messageMap[ self.PROCESS_MESSAGE ] = ProcessMessage
			self.messageMap[ self.PROCESS_STATS_MESSAGE ] = ProcessStatsMessage
			self.messageMap[ self.LISTENER_MESSAGE ] = ListenerMessage
			self.messageMap[ self.CREATE_MESSAGE ] = CreateMessage
			self.messageMap[ self.SIGNAL_MESSAGE ] = SignalMessage
			self.messageMap[ self.TAGS_MESSAGE ] = TagsMessage
			self.messageMap[ self.USER_MESSAGE ] = UserMessage
			self.messageMap[ self.PID_MESSAGE ] = PidMessage
			self.messageMap[ self.RESET_MESSAGE ] = ResetMessage
			self.messageMap[ self.ERROR_MESSAGE ] = ErrorMessage
			self.messageMap[ self.CREATE_WITH_ARGS_MESSAGE ] = \
												CreateWithArgsMessage
			self.messageMap[ self.HIGH_PRECISION_MACHINE_MESSAGE ] =  \
												HighPrecisionMachineMessage

		# Just peek the message type and then defer to derived read() impls
		try:
			message = stream.peek()
		except struct.error:
			log.error( "Empty MGM stream" )
			return None

		try:
			subclass = self.messageMap[ message ]
		except:
			log.error( "Unknown message id %d" % message )
			return None

		# Try to destream
		mgm = subclass()
		if mgm.read( stream ):
			return mgm
		else:
			log.error( "Couldn't read %s from stream" %
							 mgm.__class__.__name__ )
			return None


	@classmethod
	def batchQuery( self, mgms, timeout = 1.0, machines = [], attempts = 3 ):
		"""
		Send each mgm query to each machine and await the response.  If no
		machines are specified, mgms are sent broadcast.  Machines can either be
		Machine objects or string IP addresses.

		If machines are specified, this method will return as soon as each
		machine has replied.  If messages were sent broadcast, this method will
		return when we know each machined has replied (from the buddy system).
		In either case, this method will also return if the timeout expires.

		A mapping from input mgm -> [ (replymgm, srcaddr) ... ]	is returned.

		This method is efficient whether you are expecting one or many replies.
		"""

		sock = socketplus.socket( "bm" )

		# Convert all machines to IP addresses
		ips = []
		for m in machines:
			if type( m ) is not str:
				ips.append( m.ip )
			else:
				ips.append( m )
		machines = ips

		# Make packet with all the requests on it
		packet = MGMPacket()
		for mgm in mgms:
			packet.append( mgm )
		if not machines:
			packet.flags = packet.PACKET_STAGGER_REPLIES
		sendStream = memory_stream.MemoryStream()
		packet.write( sendStream )

		for i in xrange( attempts ):

			# Pool of addresses we have received replies from
			replied = set()

			# Pool of addresses we are expecting replies from
			if machines:
				waiting = set( machines )
			else:
				waiting = set()

			if machines:
				for ip in machines:
					sock.sendto( sendStream.data(), (ip, self.MACHINED_PORT) )
			else:
				try:
					#socket.error: (101, 'Network is unreachable')
					sock.sendto(
						sendStream.data(), ("<broadcast>", self.MACHINED_PORT) )
				except socket.error, e:

					# 101 = 'Network is unreachable'
					if e[0] == 101:
						log.error( "Unable to find default broadcast network "
								"interface.\n" )
					raise

			# Return map
			ret = {}
			for mgm in mgms:
				ret[ mgm ] = []

			# Receive replies until we're done or until timeout
			while True:
				if not select.select( [sock], [], [], timeout )[0]:
					break

				data, srcaddr = sock.recvfrom( MGMPacket.MAX_SIZE )
				stream = memory_stream.MemoryStream( data )

				try:
					packet = MGMPacket( stream )
					replies = packet.messages
				except stream.error:
					log.error( "Garbage packet on stream from %s:%d", *srcaddr )
					util.hexdump( stream.getvalue() )
					continue

				# Map each reply to its corresponding request
				for reply in replies:

					# Drop message if the server didn't understand the request
					if reply.flags & reply.MESSAGE_NOT_UNDERSTOOD:
						log.error(
							"Dropping reply to %s because %s didn't understand it",
							reply.__class__.__name__, srcaddr[0] )
						continue

					try:
						query = [mgm for mgm in mgms if reply.seq == mgm.seq][0]
					except:
						log.warning( "Bad seq %d from %s (wanted one of %s)",
									 reply.seq, srcaddr[0],
									 [m.seq for m in mgms] )
						continue

					if isinstance( reply, ErrorMessage ):
						log.cmsg( reply.severity, reply.msg )

					ret[ query ].append( (reply, srcaddr) )

				replyip = srcaddr[0]
				buddyip = socket.inet_ntoa( struct.pack( "I", packet.buddy ) )
				replied.add( replyip )
				if replyip in waiting:
					waiting.remove( replyip )
				if not machines and (buddyip not in replied and packet.buddy != 0):
					waiting.add( buddyip )

				# Break early if we know we're done
				if replied and not waiting:
					break

			if waiting:
				if i < attempts-1:
					log.warning( "Still waiting for %s after %d tries",
								 ",".join( waiting ), i+1 )
				else:
					log.error( "No MGM replies from %s after %d tries",
							   ",".join( waiting), attempts )

			if waiting and i < attempts-1:
				continue

			return ret


	def query( self, machine, timeout = 1.0, attempts = 3 ):
		"""
		Convenience method for querying a single machine with a single MGM.

		Returns a list of replies or [] if timed out.
		"""
		replies = self.batchQuery( [self], timeout, [machine], attempts )
		if replies[ self ]:
			return [mgm for mgm, addr in replies[ self ]]
		else:
			return []



# ------------------------------------------------------------------------------
# Section: BasicMachineMessage
# ------------------------------------------------------------------------------

class BasicMachineMessage( MachineGuardMessage ):

	def __init__( self, mgmType ):
		MachineGuardMessage.__init__( self, mgmType )

		self.hostname = ""
		self.cpuSpeed = 0
		self.nCpus = 0
		self.nInterfaces = 0
		self.cpuLoads = []
		self.mem = 0
		self.ifStats = {}
		self.inDiscards = self.outDiscards = 0
		self.version = 0

		return


	def __str__( self ):
		return super( BasicMachineMessage, self ).__str__() + "\n" + \
			"Hostname: %s\n" % self.hostname + \
			"CPUs: %dMHz x %d\n" % (self.cpuSpeed, self.nCpus) + \
			"Mem usage: %d%%\n" % (self.mem/2.55) + \
			"CPU loads: %s\n" % \
			" ".join( ["%d%%" % (x/2.55) for x in self.cpuLoads] ) + \
			"\n".join( [str( x ) for x in self.ifStats.itervalues()] ) + "\n" \
			+ \
			"Discards: %d/%d\n" % (self.inDiscards, self.outDiscards) + \
			"Version: %d" % self.version


	def read( self, stream ):
		if not super( BasicMachineMessage, self ).read( stream ):
			return False
		try:

			self.hostname, self.cpuSpeed, self.nCpus, self.nInterfaces, \
				self.mem, self.version, self.inDiscards, self.outDiscards \
				= stream.unpack( "s", "HBBBBBB" )

			self.cpuLoads = [stream.unpack( "B" )[0]
							 for i in xrange( self.nCpus )]

			return True

		except stream.error:
			return False


	def write( self, stream ):
		super( BasicMachineMessage, self ).write( stream )
		stream.pack( self.hostname,
					 ("HBBBBBB", self.cpuSpeed, self.nCpus, self.nInterfaces,
					  self.mem, self.version, self.inDiscards,
					  self.outDiscards) )
		for i in xrange( self.nCpus ):
			stream.pack( ("B", self.cpuLoads[i]) )

		return



# ------------------------------------------------------------------------------
# Section: WholeMachineMessage
# ------------------------------------------------------------------------------

class WholeMachineMessage( BasicMachineMessage ):

	class LowPrecisionInterfaceStats( object ):

		# Taken from common_machine_guard.hpp
		MAX_BIT_RATE = 1 << 27
		BIT_INCREMENT = MAX_BIT_RATE / 0xFF
		MAX_PACKET_RATE = 256000
		PACK_INCREMENT = MAX_PACKET_RATE / 0xFF

		def __init__( self, name="", bitsIn=0, bitsOut=0, packIn=0, packOut=0 ):

			self.name = name

			# We translate these values now to make things easier for
			# stat_logger/stat_grapher.  Since we never write non-zero values
			# for these, it's not too much of a problem, even if it is
			# out-of-sync wrt/ reading and writing sides.
			self.bitsIn = bitsIn * self.BIT_INCREMENT
			self.bitsOut = bitsOut * self.BIT_INCREMENT
			self.packIn = packIn * self.PACK_INCREMENT
			self.packOut = packOut * self.PACK_INCREMENT

		def __str__( self ):
			return "%s: %d/%d bit/s %d/%d pack/s" % \
				   (self.name, self.bitsIn, self.bitsOut,
					self.packIn, self.packOut)

		def __nonzero__( self ):
			return self.bitsIn or self.bitsOut or self.packIn or self.packOut

		def write( self, stream ):
			stream.pack( self.name,
						 ("BBBB", self.bitsIn, self.bitsOut,
						  self.packIn, self.packOut) )


	# WholeMachineMessage
	def __init__( self ):
		BasicMachineMessage.__init__(
			self, MachineGuardMessage.WHOLE_MACHINE_MESSAGE )

		return


	def read( self, stream ):
		if not super( WholeMachineMessage, self ).read( stream ):
			return False
		try:

			# Unpack each interface's statistics
			ifStatList = []
			for i in xrange( self.nInterfaces ):
				ifStatList.append(
					self.LowPrecisionInterfaceStats(
						*stream.unpack( "s", "BBBB" ) )
				)

			# Now put the interface statistics back into the main dictionary
			self.ifStats = {}
			for stat in ifStatList:
				self.ifStats[ stat.name ] = stat

			return True

		except stream.error:
			return False


	def write( self, stream ):
		super( WholeMachineMessage, self ).write( stream )

		assert( len(self.ifStats) == self.nInterfaces )
		for ifStat in self.ifStats.itervalues():
			ifStat.write( stream )




# ------------------------------------------------------------------------------
# Section: HighPrecisionMachineMessage
# ------------------------------------------------------------------------------
class HighPrecisionMachineMessage( BasicMachineMessage ):


	class HighPrecisionInterfaceStats( object ):

		def __init__( self, name="", bitsIn=0, bitsOut=0, packIn=0, packOut=0 ):

			self.name = name
			self.bitsIn = bitsIn
			self.bitsOut = bitsOut
			self.packIn = packIn
			self.packOut = packOut
			return

		def __str__( self ):
			return "%s: %d/%d bit/s %d/%d pack/s" % \
				   (self.name, self.bitsIn, self.bitsOut,
					self.packIn, self.packOut)

		def __nonzero__( self ):
			return self.bitsIn or self.bitsOut or self.packIn or self.packOut

		def write( self, stream ):
			stream.pack( self.name,
						 ("IIII", self.bitsIn, self.bitsOut,
						  self.packIn, self.packOut) )


	# HighPrecisionMachineMessage
	def __init__( self ):
		BasicMachineMessage.__init__( self,
			MachineGuardMessage.HIGH_PRECISION_MACHINE_MESSAGE )

		self.ioWaitTime = 0

		return


	def read( self, stream ):
		if not super( HighPrecisionMachineMessage, self ).read( stream ):
			return False
		try:

			self.ioWaitTime = stream.unpack( "B" )

			# Unpack each interface's statistics
			ifStatList = []
			for i in xrange( self.nInterfaces ):
				ifStatList.append(
					self.HighPrecisionInterfaceStats(
						*stream.unpack( "s", "IIII" ) )
				)

			# Now put the interface statistics back into the main dictionary
			self.ifStats = {}
			for stat in ifStatList:
				self.ifStats[ stat.name ] = stat

			return True

		except stream.error:
			return False


	def write( self, stream ):
		super( HighPrecisionMachineMessage, self ).write( stream )

		stream.pack( ("B", self.ioWaitTime) )

		assert( len(self.ifStats) == self.nInterfaces )
		for ifStat in self.ifStats.itervalues():
			ifStat.write( stream )



class ProcessMessage( MachineGuardMessage ):

	SERVER_COMPONENT = 0
	WATCHER_NUB = 1

	PARAM_USE_CATEGORY = 0x1
	PARAM_USE_UID = 0x2
	PARAM_USE_PID = 0x4
	PARAM_USE_PORT= 0x8
	PARAM_USE_ID = 0x10
	PARAM_USE_NAME = 0x20
	PARAM_IS_MSGTYPE = 0x80

	REGISTER = 1
	DEREGISTER = 2
	NOTIFY_BIRTH = 3
	NOTIFY_DEATH = 4

	def __init__( self ):
		MachineGuardMessage.__init__(
			self, MachineGuardMessage.PROCESS_MESSAGE )
		self.param = 0
		self.category = 0
		self.uid = 0
		self.pid = 0
		self.port = 0
		self.id = 0
		self.name = ""

	def __str__( self ):
		return super( ProcessMessage, self ).__str__() + "\n" + \
			   "Param: %x\n" % self.param + \
			   "Category: %d\n" % self.category + \
			   "Name: %s\n" % self.name + \
			   "PID: %d\n" % self.pid + \
			   "UID: %d\n" % self.uid + \
			   "ID: %d\n" % self.id + \
			   "Port: %d" % self.port

	def read( self, stream ):
		if not super( ProcessMessage, self ).read( stream ):
			return False
		try:
			self.param, self.category, self.uid, self.pid, \
					   self.port, self.id, self.name = \
					   stream.unpack( "BBHHHH", "s" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( ProcessMessage, self ).write( stream )
		stream.pack( ("BBHHHH", self.param, self.category, self.uid,
					  self.pid, self.port, self.id), self.name )

class ProcessStatsMessage( ProcessMessage ):

	def __init__( self ):
		ProcessMessage.__init__( self )
		self.message = MachineGuardMessage.PROCESS_STATS_MESSAGE
		self.load = 0
		self.mem = 0

	def __str__( self ):
		return super( ProcessStatsMessage, self ).__str__() + "\n" + \
			   "Load: %d%%\n" % (self.load / 2.55) + \
			   "Mem: %d%%" % (self.mem / 2.55)

	def read( self, stream ):
		if not super( ProcessStatsMessage, self ).read( stream ):
			return False
		try:
			self.load, self.mem = stream.unpack( "BB" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( ProcessStatsMessage, self ).write( stream )
		stream.pack( ("BB", self.load, self.mem) )

class ListenerMessage( ProcessMessage ):

	ADD_BIRTH_LISTENER = 0
	ADD_DEATH_LISTENER = 1

	def __init__( self ):
		ProcessMessage.__init__( self )
		self.message = MachineGuardMessage.LISTENER_MESSAGE
		self.preAddr = ""
		self.postAddr = ""

	def read( self, stream ):
		if not super( ListenerMessage, self ).read( stream ):
			return False
		try:
			self.preAddr, self.postAddr = stream.unpack( "s", "s" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( ListenerMessage, self ).write( stream )
		stream.pack( self.preAddr, self.postAddr )

class CreateMessage( MachineGuardMessage ):

	def __init__( self, type = MachineGuardMessage.CREATE_MESSAGE ):
		MachineGuardMessage.__init__( self, type )
		self.name = None
		self.config = "Hybrid"
		self.uid = None
		self.recover = 0
		self.fwdIp = 0
		self.fwdPort = 0

	def __str__( self ):
		return super( CreateMessage, self ).__str__() + "\n" + \
			   "Name: %s\n" % self.name + \
			   "Config: %s\n" % self.config + \
			   "UID: %d\n" % self.uid + \
			   "Recover: %s\n" % bool( self.recover ) + \
			   "Forward Addr: %s:%d" % (socket.inet_ntoa( struct.pack(
			"I", self.fwdIp ) ), socket.ntohs( self.fwdPort ))

	def read( self, stream ):
		if not super( CreateMessage, self ).read( stream ):
			return False
		try:
			self.name, self.config, self.uid, self.recover, self.fwdIp, \
					   self.fwdPort = stream.unpack( "s", "s", "HBIH" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( CreateMessage, self ).write( stream )
		stream.pack( self.name, self.config, ("HBIH", self.uid, self.recover,
											  self.fwdIp, self.fwdPort) )

class CreateWithArgsMessage( CreateMessage ):
	
	def __init__( self ):
		CreateMessage.__init__( self,
				MachineGuardMessage.CREATE_WITH_ARGS_MESSAGE );
		self.args = []

	def __str__( self ):
		return super( CreateWithArgsMessage, self ).__str__() + "\n" + \
				"Args: %s\n" % str( self.args )

	def read( self, stream ):
		if not super( CreateWithArgsMessage, self ).read( stream ):
			return False
		try:
			numArgs, = stream.unpack( 'I' )
			self.args = []
			for i in xrange( numArgs ):
				args.append( stream.unpack( 's' ) )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( CreateWithArgsMessage, self ).write( stream )
		stream.pack( ( 'I', len( self.args) ) )
		for arg in self.args:
			stream.pack( arg )	# assumes arg is a string.
		
class SignalMessage( ProcessMessage ):

	SIGINT = 2
	SIGQUIT = 3
	SIGUSR1 = 10

	def __init__( self ):
		ProcessMessage.__init__( self )
		self.message = MachineGuardMessage.SIGNAL_MESSAGE
		self.signal = None

	def __str__( self ):
		return super( SignalMessage, self ).__str__() + "\n" + \
			   "Signal: %d" % self.signal

	def read( self, stream ):
		if not super( SignalMessage, self ).read( stream ):
			return False
		try:
			self.signal, = stream.unpack( "B" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( SignalMessage, self ).write( stream )
		stream.pack( ("B", self.signal) )

class TagsMessage( MachineGuardMessage ):

	def __init__( self ):
		MachineGuardMessage.__init__(
			self, MachineGuardMessage.TAGS_MESSAGE )
		self.tags = []
		self.exists = 0

	def __str__( self ):
		return super( TagsMessage, self ).__str__() + "\n" + \
			   "Tags: %s\n" % self.tags + \
			   "Exists: %s" % bool( self.exists )

	def read( self, stream ):
		if not super( TagsMessage, self ).read( stream ):
			return False
		try:
			size, self.exists = stream.unpack( "BB" )
			self.tags = [stream.unpack( "s" )[0] for i in xrange( size )]
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( TagsMessage, self ).write( stream )
		stream.pack( ("BB", len( self.tags ), self.exists) )
		for tag in self.tags:
			stream.pack( tag )

class UserMessage( MachineGuardMessage ):

	PARAM_USE_UID = 0x1
	PARAM_USE_NAME = 0x2
	PARAM_CHECK_COREDUMPS = 0x4
	PARAM_REFRESH_ENV = 0x8
	PARAM_SET = 0x80

	UID_NOT_FOUND = 0xffff

	def __init__( self ):
		MachineGuardMessage.__init__(
			self, MachineGuardMessage.USER_MESSAGE )
		self.param = 0
		self.uid = 0
		self.username = ""
		self.fullname = ""
		self.home = ""
		self.mfroot = ""
		self.bwrespath = ""
		self.coredumps = []

	def __str__( self ):
		return super( UserMessage, self ).__str__() + "\n" + \
			   "Param: %x\n" % self.param + \
			   "UID: %d\n" % self.uid + \
			   "Username: %s\n" % self.username + \
			   "Fullname: %s\n" % self.fullname + \
			   "Home: %s\n" % self.home + \
			   "MF_ROOT: %s\n" % self.mfroot + \
			   "BW_RES_PATH: %s\n" % self.bwrespath + \
			   "Coredumps: %s" % self.coredumps

	def read( self, stream ):
		if not super( UserMessage, self ).read( stream ):
			return False
		try:
			self.param, self.uid, self.username, self.fullname, \
			self.home, self.mfroot, self.bwrespath = \
			stream.unpack( "BH", "s", "s", "s", "s", "s" )

			# NOTE: This is basically an inline implementation of the
			# std::vector streaming operators from binary_stream.hpp.  The next
			# time an MGM needs this, we should really implement standardised
			# support for it in memory_stream.py
			nCores, = stream.unpack( "I" )
			self.coredumps = [stream.unpack( "s", "s", "I" )
							  for i in xrange( nCores )]
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( UserMessage, self ).write( stream )
		stream.pack( ("BH", self.param, self.uid), self.username,
					 self.fullname, self.home, self.mfroot, self.bwrespath )

		stream.pack( ("I", len( self.coredumps )) )
		for filename, assrt, time in self.coredumps:
			stream.pack( filename, assrt, ("I", time) )


class UnknownMessage( MachineGuardMessage ):

	def __init__( self ):
		self.data = ""

	def __str__( self ):
		return "Data: %s" % util.hexdump( self.data, False )

	def read( self, stream ):
		if not super( UnknownMessage, self ).read( stream ):
			return False
		try:
			self.data = stream.read()
			self.flags = self.flags | self.MESSAGE_NOT_UNDERSTOOD
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( UnknownMessage, self ).write( stream )
		stream.write( self.data )

class PidMessage( MachineGuardMessage ):

	def __init__( self ):
		MachineGuardMessage.__init__( self, MachineGuardMessage.PID_MESSAGE )
		self.pid = 0
		self.running = False

	def __str__( self ):
		return super( PidMessage, self ).__str__() + "\n" + \
			   "PID: %d\n" % self.pid + \
			   "Running: %s" % self.running

	def read( self, stream ):
		if not super( PidMessage, self ).read( stream ):
			return False
		try:
			self.pid, self.running = stream.unpack( "HB" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( PidMessage, self ).write( stream )
		stream.pack( ("HB", self.pid, self.running) )

class ResetMessage( MachineGuardMessage ):

	def __init__( self ):
		MachineGuardMessage.__init__( self, MachineGuardMessage.RESET_MESSAGE )

class ErrorMessage( MachineGuardMessage ):

	def __init__( self ):
		MachineGuardMessage.__init__( self, MachineGuardMessage.ERROR_MESSAGE )
		self.severity = 0
		self.msg = ""

	def __str__( self ):
		return super( ErrorMessage, self ).__str__() + "\n" + \
			   "Severity: %d\n" % self.severity + \
			   "Message: %s\n" % self.message

	def read( self, stream ):
		if not super( ErrorMessage, self ).read( stream ):
			return False
		try:
			self.severity, self.msg = stream.unpack( "B", "s" )
			return True
		except stream.error:
			return False

	def write( self, stream ):
		super( ErrorMessage, self ).write( stream )
		stream.pack( ("B", self.severity), self.message )

# ------------------------------------------------------------------------------
# Section: WatcherDataMessage 
#          watcher protocol v2 message
# ------------------------------------------------------------------------------

class WatcherDataMessage:
	WATCHER_MSG_GET2 = 26
	WATCHER_MSG_SET2 = 27
	WATCHER_MSG_TELL2 = 28
	WATCHER_MSG_SET2_TELL2 = 29


	WATCHER_MODE_INVALID = 0    # Indicates an error.
	WATCHER_MODE_READ_ONLY = 1  # Indicates the watched value cannot be changed.
	WATCHER_MODE_READ_WRITE = 2 # Indicates the watched value can be changed.
	WATCHER_MODE_DIR = 3        # Indicates that the watcher has children.
	WATCHER_MODE_CALLABLE = 4   # Indicates the watcher is a callable function
	# < - little endian
	# I - message type (ie: _GET2 / _SET2 )
	# I - num contained messages
	STRUCT_FORMAT = "<II"

	TELL2_FORMAT  = "<IBB"

	# The number of times we'll try to requery a Watcher value
	REQUERY_MAX = 2

	def __init__(self):
		self.message = 0
		self.count = 0
		self.replyData = []
		self.extensionStream = None

		# For a _GET2 request requestStrings contains the following tuple:
		# ( sequence #, watcher string )
		self.requestStrings = []

		# Sequence number is an int prefix to the watcher values
		# that are sent with a _GET2 request.
		self.seqNum = random.randint(0,(2**32)-1)


	def get( self ):
		"""
		Convert the fields of this message into a stream.
		"""

		if self.extensionStream:
			packet = struct.pack( "I", self.message ) + \
					 self.extensionStream.data()

		else:

			packet = struct.pack( self.STRUCT_FORMAT, self.message, self.count )

			for request in self.requestStrings:

				# request[0] = sequence #
				# request[1] = watcher path
				# request[2] = data type (_SET2_only)
				# request[3] = data      (_SET2 only)
				packet += struct.pack( "<I%ds" % (len(request[1])+1),
									request[0], request[1] + '\0' )

				# If it's a set request go ahead and append the data
				# to set the watcher path to.
				if self.message == self.WATCHER_MSG_SET2:
					# request[2] = WatcherDataMessage() (_SET2_only)
					packet += request[2].toStream()

		return packet


	def readDirEntries( self, stream ):

		DIR_PREFIX = "<BB"
		COUNT_STR  = "<I"

		# TODO: This could potentially be changed to a BW_*PACK3. See addWatcherCount
		#     in watcher_path_request.hpp.
		(count,) = struct.unpack( COUNT_STR,
						stream.read( struct.calcsize( COUNT_STR ) ) )


		size = 0
		tmpDirs = []
		for i in xrange( count ):
			# Read the sequence #, type and mode of the current entry
			(dataType, mode) = struct.unpack( DIR_PREFIX,
								stream.read( struct.calcsize( DIR_PREFIX ) ) )

			# Read off the size of the current entry
			dataSize = WDT.BW_UNPACK3( stream )
			data = stream.read( dataSize )


			# Dispatch unpack to WatcherDataType
			wdtClass = WDT.WDTRegistry.getClass( dataType )
			wdtObj = wdtClass.unpack( data )


			# The final part of all directory listings is the name of the
			# directory entry.
			entrySize = WDT.BW_UNPACK3( stream )
			data = stream.read( entrySize )

			# Dispatch unpack to WatcherDataType
			wdtClass = WDT.WDTRegistry.getClass( watcher.Constants.TYPE_STRING )
			entryObj = wdtClass.unpack( data )

			# Push into the cumulative list of directory entries.
			tmpDirs.append( (entryObj.value, wdtObj.value, dataType, mode) )

		return tmpDirs





	def BW_PACK3( self, str ):
		"""
		Returns a new string size prefixed with a BW_PACK3 format
		size followed by the argument string.
		"""


		return newStr




#	def printDebug( self, dt, mode ):
#		if dt == self.WATCHER_TYPE_BOOL:
#			print "Type is: Bool"
#		elif dt == self.WATCHER_TYPE_INT:
#			print "Type is: Int"
#		elif dt == self.WATCHER_TYPE_FLOAT:
#			print "Type is: Float"
#		elif dt == self.WATCHER_TYPE_STRING:
#			print "Type is: String"
#		elif dt == self.WATCHER_TYPE_TUPLE:
#			print "Type is: Tuple"
#		elif dt ==self. WATCHER_TYPE_FUNC_TUPLE:
#			print "Type is: Func Tuple"
#		else:
#			print "Type is: Unknown"
#
#		print "Mode is:", mode


	def set( self, str ):
		"""
		Unpack the contents of a stream into the fields of this message.
		"""

		# Wrap in a cStringIO - it should be fairly fast to do this
		stream = StringIO( str )

		(self.message, self.count) = struct.unpack( self.STRUCT_FORMAT,
				stream.read( struct.calcsize( self.STRUCT_FORMAT ) ) )

		for i in xrange( self.count ):

			# Read the sequence #, type and mode of the current entry
			(seqNum, type, mode) = struct.unpack( self.TELL2_FORMAT, 
						stream.read(struct.calcsize( self.TELL2_FORMAT )) )

			# Now read off the size of the data chunk
			dataSize = WDT.BW_UNPACK3( stream )
			data = stream.read( dataSize )

			# Dispatch unpack to WatcherDataType
			wdtClass = WDT.WDTRegistry.getClass( type )
			wdtData  = wdtClass.unpack( data )

			# Responses to a SET2 request have a success / fail byte appended
			# to the end of each operation.
			retStatus = None
			if self.message == self.WATCHER_MSG_SET2_TELL2:
				retStatus = bool( ord( stream.read( 1 ) ) )


			# If a directory listing was found, then handle the contained data
			if type == watcher.Constants.TYPE_STRING and \
			   mode == self.WATCHER_MODE_DIR:
				tmpDirs = self.readDirEntries( stream )
				self.replyData.append( (seqNum, type, mode, tmpDirs) )
				tmpDirs = None
			else:
				value = wdtData.value
				if isinstance(value, list):
					value = tuple( value )

				# Add the extracted data to the list
				if retStatus != None:
					self.replyData.append( (seqNum, type, mode, value, retStatus) )
				else:
					self.replyData.append( (seqNum, type, mode, value) )


		return self


	def __str__(self):
		s = "WatcherDataMessage (protocol v2):\n"
		s += " message = %d\n" % self.message
		s += " count = %d\n" % self.count
		ix = 0
		for i in xrange(0,self.count):
			s += "  string %d: %s\n" % (i, self.strings[ ix ])
			ix += 1
			if self.message != self.WATCHER_MSG_GET2:
				s += "  result %d: %s\n" % (i, self.strings[ ix ])
				ix += 1
		return s


	def addGetRequest( self, s ):
		"""
		Adds a watcher value along with a sequence number to the list of
		watchers to retrieve.
		"""

		self.requestStrings.append( (self.seqNum, s) )

		# Wrap the sequence number around at 32 bits
		self.seqNum += 1
		self.seqNum %= 0xffffffff

		self.count += 1
		return


	def addSetRequest( self, path, val ):

		if not isinstance(val, WDT.WatcherDataType):
			wdtValue = WDT.WDTRegistry.toWDT( val )
			self.requestStrings.append( (self.seqNum, path, wdtValue) )
		else:
			self.requestStrings.append( (self.seqNum, path, val) )

		# Wrap the sequence number around at 32 bits
		self.seqNum += 1
		self.seqNum %= 0xffffffff

		self.count += 1



	def getExtensionStream( self, message ):
		"""
		Marks this message as a watcher extension message, using the provided
		extension message ID.  Returns a MemoryStream that the extension data
		should be packed into.
		"""

		self.message = message
		self.extensionStream = memory_stream.MemoryStream()
		return self.extensionStream


	def getReply( self, index ):
		return (self.replyData[ index ])

	def getReplyCount( self ):
		return len( self.replyData )

	def getWatcherValue( self, seqNum ):
		for i in self.requestStrings:
			if i[0] == seqNum:
				return i[1]
		log.error( "Failed to lookup seqNum: %s", seqNum )
		raise KeyError( "Unknown watcher sequence number: %s" % seqNum )

	@staticmethod
	def query( path, proc, timeout ):
		"""
		Light overload for batchQuery(), returns list of replies
		"""

		replies = WatcherDataMessage.batchQuery( [path], [proc], timeout )

		if proc in replies:
			return replies[ proc ].get( path, [] )
		else:
			return []

	@staticmethod
	def batchQuery( paths, procs, timeout ):
		"""
		Batch retrieval of a watcher value from one or more processes.
		Return value is:

		{process: {request-path: [(path1,val1), (path2,val2), ...]}}
		"""

		sock = socketplus.socket( "m" )

		procs = set( procs )
		paths = set( paths )

		wdm = WatcherDataMessage()
		wdm.message = wdm.WATCHER_MSG_GET2

		for path in paths:
			wdm.addGetRequest( path )

		# Mapping from process -> number of times failed to reply in time
		notReplied = {}

		# Mapping from (ip, port) -> process to correlate replies
		addresses = {}

		# Mapping from process -> list of {watcher path -> watcher response}
		replies = {}

		# Send out all the requests and initialise dictionaries
		for p in procs:
			try:
				sock.sendto( wdm.get(), p.addr() )

			# These errors are caught on the sendto() call
			except socket.error:
				continue
			except TypeError:
				continue

			notReplied[ p ] = 0
			replies[ p ] = {}
			addresses[ p.addr() ] = p

		# Collect all the replies
		while notReplied:

			# This inner loop iterates until all replies received or timeout
			while notReplied and select.select( [sock], [], [], timeout )[0]:

				data, (srcip, srcport) = sock.recvfrom( 65536 )
				try:
					reply = WatcherDataMessage()
					reply.set( data )
				except:
					log.error( "Corrupt WDM v2 reply from %s" % srcip )
					log.error( util.hexdump( data ) )
					continue

				# If it's not a TELL and not an MGM either, start worrying
				if reply.message != reply.WATCHER_MSG_TELL2:
					log.error( "Received an unrecognised watcher message" )
					log.error( str( reply ) )
					continue

				# Lookup process and get its reply map
				try:
					p = addresses[ (srcip, srcport) ]
				except KeyError:
					log.warning( "Ignoring unexpected watcher response from " \
						"process at %s:%d" % (srcip, srcport) )
					continue

				pmap = replies[ p ]

				# Go through messages and sort them into the correct reply buckets
				for i in xrange( reply.getReplyCount() ):
					(replySeqNum, replyType, replyMode, replyValue) = reply.getReply( i )

					replyPath = wdm.getWatcherValue( replySeqNum )

					if isinstance( replyValue, list):
						pmap.setdefault( replyPath, replyValue )
						continue

					else:
						# First look for an exact match in request paths (ie for
						# value requests)
						if replyPath in paths:
							pmap.setdefault( replyPath, [] ).\
								append( (replyPath, replyValue, replyType, replyMode) )
							continue

						# Next look for dirname match (ie for directory contents)
						dirname = os.path.dirname( replyPath )
						if dirname in paths:
							pmap.setdefault( dirname, [] ).\
								append( (replyPath, replyValue, replyType, replyMode) )
							continue

					log.error( "WDM2: Incorrect reply (%s,%s) for %s" % \
							   (replyPath, replyValue, paths) )

				# Mark this process as having replied
				try:
					del notReplied[ p ]
				except KeyError:
					# In rare situations we get two responses from one process,
					# where one response is probably a delayed response
					# from an earlier query - so we can't delete from
					# this dictionary twice.
					log.warning( "Possibly delayed packet from process %s", p )

			# If we're not done yet, chances are a reply was lost, send out the
			# request again, hopefully we'll get lucky this time
			for p, n in notReplied.items():
				log.warning( "%s on %s:%d didn't reply to WDM (%d/%d)",
							 p.name, p.machine.name, p.port(), n + 1,
							 WatcherDataMessage.REQUERY_MAX )
				sock.sendto( wdm.get(), p.addr() )
				notReplied[ p ] += 1

				# If a process has failed too many times, just put in an empty
				# list for its replies
				if notReplied[ p ] >= WatcherDataMessage.REQUERY_MAX:
					del notReplied[ p ]
					replies[ p ] = {}

		return replies

# ------------------------------------------------------------------------------
# Section: LoggerComponentMessage
# ------------------------------------------------------------------------------

class LoggerComponentMessage( object ):

	MESSAGE_LOGGER_VERSION = 6
	FORMAT = "BBHI"

	def __init__( self, uid, componentName ):
		self.version = LoggerComponentMessage.MESSAGE_LOGGER_VERSION
		self.loggerID = 0
		self.uid = uid
		self.pid = os.getpid()
		self.componentName = componentName

	def write( self, stream ):
		stream.pack( (self.FORMAT, self.version, self.loggerID, self.uid,
					  self.pid), self.componentName )

	def read( self, stream ):
		(self.version, self.loggerID, self.uid, self.pid,
		 self.componentName) = stream.unpack( self.FORMAT, "s" )


# ------------------------------------------------------------------------------
# Section: QueryInterfaceMessage
# ------------------------------------------------------------------------------

# TODO: This message has not yet been implemented in python because its
#       functionality isn't currently required.
