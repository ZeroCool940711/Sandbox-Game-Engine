"""
Module for querying info about a BigWorld cluster.  Class layout is hierarchical
and most layers of the structure should not define methods which provide direct
access to internals of other layers, although the top level class (Cluster) does
this in some cases for substantial convenience.

Hierarchy
=========

Cluster
|
+- User-----+
|           |
+- Machine  |
   |        |
   +- Process
      |
	  +- WatcherData

"""

import sys
import os
import socket
import time
import re
import struct
import random
import operator
import copy
import threading
import signal
import itertools
import telnetlib
import types
from xml.dom import minidom
from StringIO import StringIO

import messages
import log
import util
import socketplus
import memory_stream
import uid as uidmodule
import watcher_data_type as WDT

import bwsetup; bwsetup.addPath( ".." )

# ------------------------------------------------------------------------------
# Section: Globals
# ------------------------------------------------------------------------------

# How long we'll wait for machined and watcher replies
TIMEOUT = 0.2

# How many bots we add at once
NBOTS_AT_ONCE = 10

# CPU threshold for machines running bots
MAX_BOTS_CPU = 0.65

# CPU threshold for machines running server components
MAX_SERVER_CPU = 0.75

# Time to sleep while waiting for CPU load to drop
CPU_WAIT_SLEEP = 2

# Time between checks to see if the server is/isn't running
POLL_SLEEP = 1

# Maximum number of times we'll wait for the server to stop
MAX_POLL_SLEEPS = 10

# Maximum seconds we'll wait for the server to start up
MAX_STARTUP_SLEEPS = 30

# If enabled, only spawn bots processes on machines that already have bots
# processes on them
BOTS_EXCLUSIVE = True

# Buffer size for each recv() call
RECV_BUF_SIZE = 65536

# Adjustment to cell entities for static entities
CELL_ENTITY_ADJ = 0

# Whether or not we want spew for debugging
VERBOSE = False

# This is the default name that server tools use when sending once-off messages
# to message_logger
MESSAGE_LOGGER_NAME = "Tools"

# ------------------------------------------------------------------------------
# Section: Listener Registration
# ------------------------------------------------------------------------------

def registerListener( name = "", kind = "birth",
					  sock = None, host = "localhost", **kwargs ):
	"""
	Registers a birth or death listener with machined.

	@param name     The name of the interface to monitor, e.g. LoginInterface
	@param kind     Either 'birth' or 'death'
	@param sock     The socket replies will go to, or None for a new socket
	@param host     The host of the machined we'll register with

	@kwarg category The category of component to listen for,
	                e.g. SERVER_COMPONENT

    @kwarg uid      The uid to listen for, defaults to self
	@kwarg before   String that will appear before the address in the reply
	@kwarg after    String that will appear after the address in the reply

	@return         The socket to expect notifications on
	"""

	# Category of component
	if kwargs.has_key( "category" ):
		category = kwargs[ "category" ]
	else:
		category = messages.ProcessMessage.SERVER_COMPONENT

	# UID to look for
	if kwargs.has_key( "uid" ):
		uid = kwargs[ "uid" ]
	else:
		uid = uidmodule.getuid()

	# Bytes before and after
	if kwargs.has_key( "before" ):
		before = kwargs[ "before" ]
	else:
		before = ""
	if kwargs.has_key( "after" ):
		after = kwargs[ "after" ]
	else:
		after = ""

	# Create a listening socket if none provided
	if sock == None:
		sock = socketplus.socket()
		sock.bind( ( "localhost", 0 ) )

	# Form MGM to register the listener
	lm = messages.ListenerMessage()

	if kind == "birth":
		lm.param = lm.PARAM_IS_MSGTYPE | lm.ADD_BIRTH_LISTENER
	elif kind == "death":
		lm.param = lm.PARAM_IS_MSGTYPE | lm.ADD_DEATH_LISTENER
	else:
		raise RuntimeError, "Unknown kind: %s" % kind

	# Reply port
	lm.port = socket.htons( sock.getsockname()[1] )

	lm.name = name
	lm.category = category
	lm.uid = uid
	lm.pid = os.getpid()

	# Register the listener
	lm.send( sock, host )

	# Get the registration reply
	while True:
		data, srcaddr = sock.recvfrom( 2048 )
		stream = memory_stream.MemoryStream( data )
		try:
			messages.MGMPacket().read( stream )
			break
		except memory_stream.MemoryStream.error, e:
			log.error( "Couldn't destream listener registration reply" )
			print e

	# Return the reply socket
	return sock

# ------------------------------------------------------------------------------
# Section: Exposed
# ------------------------------------------------------------------------------

class ExposedType( type ):
	"""
	This is a metaclass used by Exposed (below) to assist with the static
	tracking of member methods marked with @expose.
	"""

	def __init__( cls, *args, **kw ):

		type.__init__( cls, *args, **kw )

		# Track marked exposed methods
		cls.s_exposedMethods = []
		for superclass in cls.__mro__:
			for name, meth in superclass.__dict__.items():
				if hasattr( meth, "exposedLabel" ):
					cls.s_exposedMethods.append( name )


class Exposed( object ):
	"""
	This is the exposed capabilties interface for cluster objects.  Classes
	wishing to expose methods using this interface should decorate their methods
	with the @expose function defined below.

	Any derived class that wishes to provide random-esque access to its
	instances (i.e. the Web Console's root controller callExposed() method) must
	also implement getExposedID().  getExposedID() should return enough
	information to reacquire a reference to the object later on.
	"""

	__metaclass__ = ExposedType

	@staticmethod
	def expose( label = None, args = [], precond = lambda self: True ):
		"""
		Decorator used to expose a method of an Exposed derived class.

		A user-defined label can be supplied for the method, or a name based on
		the function name will be inferred instead.

		If the method takes any arguments, they should be passed as a list of
		(name, description) tuples.  This list is used in interactive apps to
		prompt the user for input values.  The method should expect these values
		to be passed as strings irrespective of their intrinsic type.

		A precondition for this method's execution can also be supplied, which
		MUST BE a method that accepts a single argument (the instance itself).
		"""

		# We tag the decorated method with attributes that are used by init() to
		# actually populate the static list with the exposed methods
		def inner( f, label = label, args = args, precond = precond ):
			f.exposedPrecond = precond
			f.exposedArgs = args

			if label is None:
				label = re.sub( "[A-Z]", lambda m: " " + m.group( 0 ),
								f.__name__ )
				label = re.sub( "^[a-z]", lambda m: m.group( 0 ).upper(),
								label )

			f.exposedLabel = label

			return f

		return inner


	def getExposedMethods( self ):
		methods = []
		for funcname in self.s_exposedMethods:
			func = getattr( self, funcname )
			if func.exposedPrecond( self ):
				methods.append( (func.__name__,
								 func.exposedLabel,
								 func.exposedArgs) )
		return methods


	def getExposedID( self ):
		log.critical( "You really need to implement getExposedID() for %s",
					  self.__class__.__name__ )


# ------------------------------------------------------------------------------
# Section: Process
# ------------------------------------------------------------------------------

class Process( Exposed ):
	"""Represents a process in a BigWorld cluster.  May have optional extra
	   fields attached (eg. bot processes have an 'nbots' field)."""

	SERVER_PROCS = ["cellappmgr", "baseappmgr", "loginapp", "dbmgr",
					"cellapp", "baseapp"]

	ALL_PROCS = SERVER_PROCS + ["bots", "reviver"]

	# Strictly speaking, loginapp is not a singleton process, but since we have
	# no numbering scheme for them yet and typically only run one, it just makes
	# life easier to think of them in this way.  TODO: Number loginapps and bots
	# processes, probably by using the dbmgr as the id broker.
	SINGLETON_PROCS = ["cellappmgr", "baseappmgr", "loginapp", "dbmgr"]

	INTERFACE_NAMES = { "ReviverInterface": "reviver",
						"LoginIntInterface": "loginapp",
						"LoginInterface": "loginapp",
						"BaseAppIntInterface": "baseapp",
						"BaseAppInterface": "baseapp",
						"CellAppInterface": "cellapp",
						"CellAppMgrInterface": "cellappmgr",
						"BaseAppMgrInterface": "baseappmgr",
						"DBInterface": "dbmgr",
						"BotsInterface" : "bots" }

	COMPONENT_NAMES = { "cellapp": "CellApp",
						"cellappmgr": "CellAppMgr",
						"baseapp": "BaseApp",
						"baseappmgr": "BaseAppMgr",
						"dbmgr": "DBMgr",
						"loginapp": "LoginApp",
						"bots": "Bots",
						"reviver" : "Reviver" }

	HAS_WATCHER_NUB = ALL_PROCS[:]

	def __init__( self, machine, mgm ):

		Exposed.__init__( self )
		self.machine = machine
		self.component = mgm.name
		self.uid = mgm.uid
		self.pid = mgm.pid
		self.id = mgm.id
		self.load = mgm.load / 255.0
		self.mem = mgm.mem / 255.0
		self.mute = False
		self.mercuryPort = socket.ntohs( mgm.port )

		if Process.INTERFACE_NAMES.has_key( self.component ):
			self.name = Process.INTERFACE_NAMES[ self.component ]
		elif mgm.category == mgm.WATCHER_NUB:
			self.name = re.sub( "\d+$", "", self.component )
		else:
			self.name = self.component

		# This the watcher port for this process.  This is private to force
		# everyone to go via port() and addr()
		if not hasattr( self, "_port" ):
			self._port = None

	def __str__( self ):
		return "%-15s on %-8s %2d%% cpu  %2d%% mem  pid:%d" % \
			   (self.label(), self.machine.name,
				self.load * 100, self.mem * 100, self.pid)

	def __cmp__( self, other ):
		"""
		Sorts world processes first, alphabetically otherwise.
		"""

		# Early breakout on type error
		if not isinstance( other, Process ):
			return -1

		# Macro to return position in list or len( list ) if not present
		def index( list, x ):
			try:
				return list.index( x )
			except ValueError:
				return len( list )

		return cmp( index( self.ALL_PROCS, self.name ),
					index( self.ALL_PROCS, other.name ) )


	def requiresOwnCPU( self ):
		"""This method can be used to decide whether a process should be run on
			its own CPU/core."""

		if (self.name in Process.SINGLETON_PROCS) and \
				self.name != "dbmgr":
			return False

		return True


	def getExposedID( self ):
		return dict( type = "process", machine = self.machine.name,
					 pid = self.pid )


	def label( self ):
		if self.id > 0:
			return "%s%02d" % (self.name, self.id)
		else:
			return self.name


	def status( self ):
		"""Returns a string representation of the status of the current process.
			This enables access to the DBMgr's new statusDetail watcher, and
			provides the ability for similar functionality to be added to other
			server process types."""

		if self.name == "dbmgr":
			return self.getWatcherValue( "status", "" )
		else:
			return ""


	def componentName( self ):
		return self.COMPONENT_NAMES[ self.name ]

	def addr( self ):
		return (self.machine.ip, self.port())

	def port( self ):
		"""
		Returns the port number of the watcher nub for this process, but if it
		is unknown as yet, discovers it first.

		Generally, this is OK to do for small query sets because it doesn't time
		out, but if you know you're going to need a lot of watcher ports ahead
		of time, query them beforehand with Cluster.getWatcherPorts() because it
		uses broadcast and is more network efficient.
		"""

		if self._port is None:
			psm = messages.ProcessStatsMessage()
			psm.param = psm.PARAM_USE_CATEGORY | psm.PARAM_USE_PID
			psm.category = psm.WATCHER_NUB
			psm.pid = self.pid

			replies = psm.query( self.machine )
			if replies:
				if replies[0].pid > 0:
					self._port = socket.ntohs( replies[0].port )
				else:
					del self.machine.procs[ self.pid ]
			else:
				log.error( "Could not get watcher port for %s on %s",
						   self.name, self.machine.name )

		return self._port

	def user( self ):
		return self.machine.cluster.getUser( self.uid, self.machine )

	def getWatcherData( self, path ):
		return WatcherData( self, path )

	def isServerProc( self ):
		return self.name in Process.SERVER_PROCS

	def getWatcherValue( self, path, default=None ):
		"""Slightly quicker way to query a single Watcher value."""

		# If this process has been marked as mute, then just return the default
		# immediately
		if self.mute:
			return default

		v = self.getWatcherData( path ).value
		if v != None:
			return v
		else:
			return default

	def setWatcherValue( self, path, value ):
		"""Set the watcher value at the given path to the given value.  This is
	       defined in Process rather than WatcherData because more often than
		   not a Watcher set command does not need the whole hierarchical
		   WatcherData thing going on."""

		(status, returnValue) = self.callWatcher( path, value )

		if status:
			isSame = True

			# Let's double check the value now
			if isinstance( value, WDT.WatcherDataType ):
				if returnValue != value.value:
					isSame = False

			elif type(returnValue) == type(value):
				if returnValue != value:
					isSame = False
			elif str(returnValue) != str(value):
				# If the response isn't a bool, or the lower case
				# conversion of both set value and response doesn't
				# match up, then we know for certain that we've
				# failed.
				if (type(returnValue) != bool) or \
						(str(returnValue).lower() != str(value).lower()):
					isSame = False

			if not isSame:
				log.info( "Value returned = '%s'. Value set = '%s'" %
							(returnValue, value) )

		return status


	def callWatcher( self, path, value ):
		"""Call or set a watcher at the given path to the given value. A tuple
		containing a success as a bool and the watcher's return value is
		returned."""

		Cluster().log( "Attempting to set watcher '%s'" % path,
			self.user() )

		# Force string conversion
		path = str( path ); #value = str( value )

		wdm = messages.WatcherDataMessage()
		wdm.message = wdm.WATCHER_MSG_SET2
		wdm.count = 0

		wdm.addSetRequest( path, value )

		sock = socketplus.socket()
		sock.sendto( wdm.get(), self.addr() )
		sock.settimeout( 2 )

		# Recv replies until the right one comes through
		while True:
			try:
				data, srcaddr = sock.recvfrom( RECV_BUF_SIZE )
			except socket.timeout:
				break

			if srcaddr != self.addr():
				log.warning( "Got reply from wrong address: %s:%d", *srcaddr )
				continue

			# Re-use the same WDM as was used to send the request so we
			# can use the sequence number as validation of response.
			wdm.set( data )

			if wdm.count == 0:
				log.error( "Expected single reply on set watcher reply packet, "
						   "got empty packet instead" )
				continue

			if wdm.count > 1:
				log.warning( "Expected single reply to " +
							 "setWatcherValue(), got:\n%s" % wdm )
				continue

			reply = wdm.getReply(0)
			replyPath = wdm.getWatcherValue( reply[0] )
			if replyPath != path:
				log.warning( "Incorrect reply to setWatcherValue(): %s" % \
							 replyPath )
				continue
			else:
				# Watcher protocol 2 specifies the status of the set operation
				# as part of the response, so let's use it.
				status = reply[4]
				returnValue = reply[3]

				return (status, returnValue)

		return (False, None)

	#--------------------------------------------------------------------------
	# Subsection: Static methods
	#--------------------------------------------------------------------------

	@staticmethod
	def cmpByLoad( p1, p2 ):
		return Machine.cmpByLoad( p2.machine, p1.machine )

	@staticmethod
	def cmpByName( n1, n2 ):
		ordering = Process.ALL_PROCS
		n1 = n1.lower()
		n2 = n2.lower()
		if n1 in ordering and n2 in ordering:
			return cmp( ordering.index( n1 ), ordering.index( n2 ) )
		elif n1 in ordering:
			return -1
		elif n2 in ordering:
			return 1
		else:
			return cmp( n1, n2 )

	@staticmethod
	def clean( name ):
		"""Strip digits from a process name to reveal its type."""
		return re.sub( "[0-9]", "", name )

	@staticmethod
	def getProcess( machine, mgm ):

		# I'm defining this in here instead of in the class or global scope to
		# avoid having to declare the specific class implementations before
		# this.
		name2proc = { "cellapp" : CellAppProcess,
					  "baseapp" : BaseAppProcess,
					  "cellappmgr" : CellAppMgrProcess,
					  "baseappmgr" : BaseAppMgrProcess,
					  "loginapp" : LoginAppProcess,
					  "dbmgr" : DBMgrProcess,
					  "bots" : BotProcess,
					  "reviver": ReviverProcess,
					  "client": ClientProcess,
					  "message_logger": MessageLoggerProcess }

		if Process.INTERFACE_NAMES.has_key( mgm.name ):
			name = Process.INTERFACE_NAMES[ mgm.name ]
		else:
			name = mgm.name

		if name2proc.has_key( name ):
			return name2proc[ name ]( machine, mgm )
		else:
			return Process( machine, mgm )

	@staticmethod
	def getPlural( name, count = 0 ):
		if count == 1:
			return name

		if name[-1] == "s":
			return name
		else:
			return "%ss" % name

#------------------------------------------------------------------------------
# Section: StoppableProcess
#------------------------------------------------------------------------------

class StoppableProcess( Process ):

	def __init__( self, machine, mgm ):
		Process.__init__( self, machine, mgm )

	#--------------------------------------------------------------------------
	# Subsection: Exposed stuff
	#--------------------------------------------------------------------------

	@Exposed.expose()
	def stop( self, signal = None ):

		if signal is None:
			signal = messages.SignalMessage.SIGINT

		mgm = messages.SignalMessage()
		mgm.signal = signal
		mgm.pid = self.pid
		mgm.uid = self.uid
		mgm.param = mgm.PARAM_USE_UID | mgm.PARAM_USE_PID
		mgm.send( socketplus.socket(), self.machine.ip )


	def stopNicely( self ):
		self.stop()


#------------------------------------------------------------------------------
# Section: ScriptProcess
#------------------------------------------------------------------------------

class ScriptProcess( Process ):

	def __init__( self, machine, mgm ):
		Process.__init__( self, machine, mgm )


	@Exposed.expose()
	def reloadScript( self ):
		return Cluster.runscript( [self], "BigWorld.reloadScript()", True )

#------------------------------------------------------------------------------
# Section: Specific Process implementations
#------------------------------------------------------------------------------

class CellAppMgrProcess( StoppableProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )


	def shouldOffload( self, enable ):
		stream = memory_stream.MemoryStream()
		stream.pack( ("BBB", 0, 9, enable) )
		sock = socketplus.socket()
		sock.sendto( stream.data(), (self.machine.ip, self.mercuryPort) )


class CellAppProcess( StoppableProcess, ScriptProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )

	def stopNicely( self ):
		self.callWatcher( "command/stopCellAppNicely", () )


class BaseAppProcess( StoppableProcess, ScriptProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )


class BaseAppMgrProcess( StoppableProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )


class LoginAppProcess( StoppableProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )

	def statusCheck( self, verbose = True ):
		(isWatcherOkay, returnValue) = \
			self.callWatcher( "command/statusCheck", () )

		try:
			(output, status) = returnValue
		except TypeError:
			return False

		if output.value and verbose:
			print output.value,

		return isWatcherOkay and status.value


class DBMgrProcess( StoppableProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )


class ReviverProcess( StoppableProcess ):

	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )



class BotProcess( StoppableProcess ):
	# technically, it is a script process, but it does not have a
	# BigWorld.reloadScript method
	def __init__( self, machine, mgm ):
		StoppableProcess.__init__( self, machine, mgm )
		self._nbots = None


	def __str__( self ):
		return StoppableProcess.__str__( self ) + " [%d bots]" % self.nbots()


	@Exposed.expose( args = [("num", "the number of bots to add", 1)] )
	def addBots( self, num ):
		"""
		Add bots to this bot process.
		"""
		self.setWatcherValue( "command/addBots", num )


	@Exposed.expose( args = [("num", "the number of bots to delete", 1)] )
	def delBots( self, num ):
		"""
		Delete bots from this bot process.
		"""
		self.setWatcherValue( "command/delBots", num )


	def nbots( self, n = None ):
		"""
		Setter/getter for the number of bots hosted on this bot process.
		"""

		# Get
		if n is None:
			if self._nbots is None:
				self._nbots = int( self.getWatcherValue( "numBots", 0 ) )
			return self._nbots

		# Set
		else:
			self._nbots = n


class ClientProcess( Process ):

	def __init__( self, machine, component, id, pid, uid, load, mem ):
		Process.__init__( self, machine, component, id, pid, uid, load, mem )
		self.name = "client"


class MessageLoggerProcess( Process ):

	def __init__( self, machine, mgm ):
		Process.__init__( self, machine, mgm )


	@Exposed.expose( label = "Roll Logs" )
	def breakSegments( self ):
		mgm = messages.SignalMessage()
		mgm.signal = signal.SIGHUP
		mgm.pid = self.pid
		mgm.uid = self.uid
		mgm.param = mgm.PARAM_USE_UID | mgm.PARAM_USE_PID
		mgm.send( socketplus.socket(), self.machine.ip )


	@Exposed.expose( args = [("user", "the username to log as"),
							 ("message", "the log message")] )
	def sendMessage( self, message, user, severity = "INFO" ):
		"""
		Send the message to this logger, as the given user.  'user' can be
		passed as a string, in which case the User object is looked up.
		"""

		# We have to do all this crap because Windows can't load bwlog.so
		MESSAGE_LOGGER_MSG = 107
		MESSAGE_LOGGER_REGISTER = MESSAGE_LOGGER_MSG + 1
		MESSAGE_LOGGER_PROCESS_DEATH = MESSAGE_LOGGER_REGISTER + 2

		try:
			from message_logger import bwlog
			assert bwlog.MESSAGE_LOGGER_MSG == MESSAGE_LOGGER_MSG
			assert bwlog.MESSAGE_LOGGER_REGISTER == MESSAGE_LOGGER_REGISTER
			assert bwlog.MESSAGE_LOGGER_PROCESS_DEATH == \
				   MESSAGE_LOGGER_PROCESS_DEATH
			assert log.SEVERITY_LEVELS == bwlog.SEVERITY_LEVELS

		except ImportError, e:
			log.warning( "Failed to import bwlog.so on this system: %s", e )
			log.warning( "Recompiling the extension in "
						 "bigworld/src/server/tools/message_logger "
						 "will probably fix this" )

		if type( user ) == str:
			user = self.machine.cluster.getUser( user )

		# Make sure socket is bound to a specific address (not INADDR_ANY) so
		# that it can deregister properly
		sock = socketplus.socket()
		uid = user.uid
		lcm = messages.LoggerComponentMessage( uid, MESSAGE_LOGGER_NAME )
		wdm = messages.WatcherDataMessage()

		# Register this process with the logger
		stream = wdm.getExtensionStream( MESSAGE_LOGGER_REGISTER )
		lcm.write( stream )
		sock.sendto( wdm.get(), self.addr() )

		# Send the message
		stream = wdm.getExtensionStream( MESSAGE_LOGGER_MSG )
		stream.pack( ("BB", 0, log.SEVERITY_LEVELS[ severity ]), message )
		sock.sendto( wdm.get(), self.addr() )

		# Deregister this process with the logger
		stream = wdm.getExtensionStream( MESSAGE_LOGGER_PROCESS_DEATH )
		addr, port = sock.getsockname()
		stream.pack( ("4sHxx",
					  socket.inet_aton( "0.0.0.0" ), socket.htons( port )) )
		sock.sendto( wdm.get(), self.addr() )


#------------------------------------------------------------------------------
# Section: WatcherData
#------------------------------------------------------------------------------

class WatcherData( Exposed ):

	def __init__( self, process, path, value=None, watcherType=None, watcherMode=None ):
		Exposed.__init__( self )
		self.process = process
		self.path = path
		self.name = os.path.basename( path )
		self.value = value
		self.type = watcherType
		self.mode = watcherMode

		# Special case for the root directory
		if path == "":
			self.value = "<DIR>"

		if self.value == None:
			self.refresh()


	def refresh( self ):
		"""Updates the value by asking the machine."""

		if self.value == "<DIR>":
			return

		newvalue = None
		queryType = None
		queryMode = None
		for path, value, queryType, queryMode in messages.WatcherDataMessage.query(
			os.path.dirname( self.path ), self.process, TIMEOUT ):

			if path == os.path.basename(self.path):
				newvalue = value
				newtype = queryType
				newmode = queryMode
				break

		if newvalue == None:
			self.process.mute = True
			self.value = None
		else:
			self.value = newvalue
			self.type = newtype
			self.mode = newmode

	def isDir( self ):
		return self.value == "<DIR>"


	def __str__( self ):
		return "%s = %s" % (self.name, self.value)


	def __cmp__( self, other ):
		"""Directories first, then sort by name"""

		if (other == None):
			return -1

		if (self.isDir() and other.isDir()) or \
		   (not self.isDir() and not other.isDir()):
			return cmp( self.name, other.name )
		elif self.isDir():
			return -1
		else:
			return 1


	def __iter__( self ):
		return iter( self.getChildren() )


	def getChildren( self ):
		"""Retrieve next level of Watcher tree."""

		if self.value != "<DIR>": return

		self.children = []
		for path, value, queryType, queryMode in messages.WatcherDataMessage.query(
			self.path, self.process, TIMEOUT ):

			# Reform the basename that has been returned into a complete
			# watcher path
			if self.path and len(self.path):
				path = "%s/%s" % (self.path, path)

			self.children.append( WatcherData( self.process, path, value,
				queryType, queryMode ) )

		self.children.sort()
		return self.children


	def getChild( self, name ):
		return WatcherData( self.process, os.path.join( self.path, name ) )


	@Exposed.expose( precond = lambda self: not self.isDir() )
	def set( self, value ):
		"""Set this watcher value."""
		return self.process.setWatcherValue( self.path, value )

# ------------------------------------------------------------------------------
# Section: LoadStats
# ------------------------------------------------------------------------------

class LoadStats( object ):
	def __init__( self, min, avg, max ):
		self.min = min
		self.avg = avg
		self.max = max

	def __str__( self ):
		return "%.2f %.2f %.2f" % (self.min, self.avg, self.max )

# ------------------------------------------------------------------------------
# Section: Machine
# ------------------------------------------------------------------------------

class Machine( Exposed ):

	def __init__( self, cluster, mgm, ip, clearProcs = True ):

		Exposed.__init__( self )

		self.cluster = cluster
		self.name = mgm.hostname
		self.ip = ip

		# TODO: assuming that mgm here is a WholeMachineMessage.. should verify
		#       and assert if not or a HPM too

		# List containing the loads for each CPU on the machine
		self.loads = [ l/255.0 for l in mgm.cpuLoads ]

		self.mhz = mgm.cpuSpeed
		self.ncpus = mgm.nCpus
		self.machinedVersion = mgm.version

		# Dictionary of { interface name -> messages.IfStats object }
		self.ifStats = mgm.ifStats

		# Number of discarded packets
		self.inDiscards = mgm.inDiscards
		self.outDiscards = mgm.outDiscards

		# Ratio of memory available on this machine (available/max)
		self.mem = mgm.mem/255.0

		# a map of machined tags for this machine, tag -> [ values ]
		self.tags = {}

		# a mapping of pid -> Process for each process running on this machine.
		# it is optional to clear this because when we are updating existing
		# machine objects we don't want to lose the process mapping
		if clearProcs:
			self.procs = {}
			self.unknownUsers = set()

	def __hash__( self ):
		return struct.unpack( "!I", socket.inet_aton( self.ip ) )[0] % 0x7fffffff

	def __cmp__( self, other ): return cmp( self.name, other.name )

	def __str__( self ): return self.str( 0 )

	def str( self, indent ):

		cpuloads = ",".join( ["%2d%%" % int( l*100 ) for l in self.loads] )
		cpuloads += " of %4dMHz" % self.mhz

		if VERBOSE:
			fmt = "%s%s (%s) %d process%s  %s (%d%% mem)"
		else:
			fmt = "%s%-10s %-15s %2d process%s  %s (%d%% mem)"

		nprocs = len( self.getProcs() )
		if nprocs == 1:
			suffix = "  "
		else:
			suffix = "es"

		# Start with the name, ip address, and cpu load
		s = fmt % ( "\t"*indent, self.name, self.ip,
					nprocs, suffix, cpuloads, self.mem * 100 )

		# Append machined version if different to the current known version
		if self.machinedVersion != \
			   messages.MachineGuardMessage.MACHINED_VERSION:
			s += " (v%d)" % self.machinedVersion

		if VERBOSE:

			# Sort server procs by uid
			cmpByUid = lambda p1, p2: cmp( p1.uid, p2.uid )
			serverProcs = self.getProcs()
			serverProcs.sort( cmpByUid )

			# Append a list of regular server components being run if there are
			# any
			for p in serverProcs:
				s += \
				  "\n%s%-16s running as %-10s using %2d%% cpu %2d%% mem" % \
				  ("\t"*(indent+1), p.name, p.user().name,
				   int( p.load * 100 ), int( p.mem * 100 ))

		return s

	def supportsHighPrecision( self ):
		"""Returns True if the machine we refer to supports communicating
			High Precision machine statistics."""

		return (self.machinedVersion >= 41)


	def totalmhz( self ): return self.ncpus * self.mhz

	def load( self ): return max( self.loads )

	def getServerProcs( self ):
		return filter( lambda p: p.name in Process.SERVER_PROCS,
					   self.getProcs() )

	def getProcs( self, name = None, uid = None ):
		"""Returns all processes with the given name, or all if no name
		   given, filtering by uid."""

		ps = self.procs.values()
		if name != None:
			ps = filter( lambda p: p.name == name, ps )
		if uid != None:
			ps = filter( lambda p: p.uid == uid, ps )
		return ps

	def getProcsIter( self, name = None, uid = None ):
		"""Returns an iterator to all procs in the cluster with the given
		   name, under the given UID"""

		def procFilter( p ):
			return (not name or p.name == name) and \
				(not uid or p.uid == uid)

		return itertools.ifilter( procFilter, self.procs.itervalues() )

	def getProc( self, pid ):
		if self.procs.has_key( pid ):
			return self.procs[ pid ]
		else:
			return None

	def isPidRunning( self, pid ):
		pm = messages.PidMessage()
		pm.pid = pid
		replies = pm.query( self )
		if not replies:
			return False
		else:
			return replies[0].running

	def getProcForPort( self, port ):
		ps = [ p for p in self.procs.values() if p.port() == port ]
		if ps:
			return ps[0]
		else:
			return None

	def setMachinedVersion( self, version ):
		self.machinedVersion = version

	def outOfDate( self ):
		return self.machinedVersion != \
			   messages.MachineGuardMessage.MACHINED_VERSION

	def startProc( self, name, uid, config = "Hybrid", _async_ = None ):
		"""
		Start a process on this machine.  Returns the PID of the started
		process.  If this is run as an AsyncTask, any errors reported by
		machined are sent into the update queue.
		"""

		mgm = messages.CreateMessage()
		mgm.uid = uid
		mgm.name = name
		mgm.config = config

		# We don't want extra attempts here because that results in multiple
		# processes being spawned most of the time, because the first one
		# actually worked but just took a while to reply to us
		replies = mgm.query( self, attempts = 1, timeout = 3.0 )
		pid = 0

		if not replies:
			log.error( "%s didn't reply to CreateMessage with PID for %s",
					   self.name, name )

		elif not replies[0].running:
			log.error( "newly created process %s (%d) on %s is not running!",
					   name, replies[0].pid, self.name )

		else:
			pid = replies[0].pid

		# Everything on the packet other than first message is an error message
		if _async_:
			for mgm in replies[1:]:
				if isinstance( mgm, messages.ErrorMessage ):
					_async_.updateFinal( "__warning__", mgm.msg )

		return pid


	def startProcWithArgs( self, name, args, uid, config = "Hybrid" ):
		"""
		Start a process on this machine.  Returns the PID of the started
		process.
		"""

		mgm = messages.CreateWithArgsMessage()
		mgm.uid = uid
		mgm.name = name
		mgm.config = config
		mgm.args = args

		# We don't want extra attempts here because that results in multiple
		# processes being spawned most of the time, because the first one
		# actually worked but just took a while to reply to us
		replies = mgm.query( self, attempts = 1, timeout = 3.0 )
		pid = 0

		if not replies:
			log.error( "%s didn't reply to CreateWithArgsMessage with PID " \
					"for %s", self.name, name )

		elif not replies[0].running:
			log.error( "newly created process %s (%d) on %s is not running!",
					name, replies[0].pid, self.name )

		else:
			pid = replies[0].pid

		return pid


	def killProc( self, proc, signal = None ):
		"""
		Kill a process on this machine.
		"""

		proc.stop( signal )
		pid = proc.pid
		uid = proc.uid
		name = proc.label()

		procIsDead = lambda: not self.getProc( pid )

		if not self.cluster.waitFor( procIsDead, POLL_SLEEP, MAX_POLL_SLEEPS ):
			proc.stop( messages.SignalMessage.SIGQUIT )
			if not self.cluster.waitFor(
				procIsDead, POLL_SLEEP, MAX_POLL_SLEEPS ):
				log.error( "Couldn't kill %s (pid:%d) on %s" %
						   (name, pid, self.name) )
				self.cluster.getUser( uid ).ls()
				return False

		return True

	def flushMappings( self ):
		rm = messages.ResetMessage()
		if not rm.query( self ):
			log.error( "%s refused to flush its mappings", self.name )
			return False
		else:
			return True

	@staticmethod
	def cmpByHostDigits( m1, m2 ):
		"""Sorts machines with the same alpha prefixed hostnames by the digits
		   at the end of their hostnames."""
		m1nums = re.sub( "\D+", "", m1.name )
		m2nums = re.sub( "\D+", "", m2.name )
		m1alpha = re.sub( "\d+", "", m1.name )
		m2alpha = re.sub( "\d+", "", m2.name )

		if len( m1nums ) == 0 and len( m2nums ) == 0 or m1alpha != m2alpha:
			return cmp( m1alpha, m2alpha )
		elif len( m1nums ) == 0 or len( m2nums ) == 0:
			return cmp( len( m1nums ), len( m2nums ) )
		else:
			return cmp( int( m1nums ), int( m2nums ) )

	@staticmethod
	def cmpByLoad( m1, m2 ):
		return cmp( m2.load(), m1.load() )

	def isBotCandidate( self, uid ):
		"""Returns true if this machine is a candidate for running bots
		   processes."""

		bpsForUser = filter( lambda p: p.uid == uid, self.getProcs( "bots" ) )
		return bpsForUser or not BOTS_EXCLUSIVE

	def canRun( self, pname ):
		"""
		Returns true if this Machine can run the specified process name.
		"""

		if not self.tags.has_key( "Components" ):
			return True

		for tag in self.tags[ "Components" ]:
			if tag.lower() == pname:
				return True

		return False


	@staticmethod
	def localMachine():
		"""
		Returns a reference to the local machine, or None if this machine isn't
		running bwmachined.
		"""

		mgm = messages.WholeMachineMessage()
		replies = mgm.query( "127.0.0.1" )

		if not replies:
			return None
		else:
			return Cluster.get().getMachine( replies[0].hostname )

# ------------------------------------------------------------------------------
# Section: User
# ------------------------------------------------------------------------------

class User( Exposed ):
	"""
	Info about a user, and methods to query info about the server a user is
	running.

	The 'uid' param passed here can either be the username or the UID or an MGM
	that already has all the details in it.
	"""

	# The exception that gets thrown when a user can't be found
	class error( Exception ):
		def __init__( self, *args, **kw ):
			Exception.__init__( self, *args, **kw )

	def __init__( self, uid, cluster, machine = None,
				  checkCoreDumps = False, refreshEnv = False ):

		Exposed.__init__( self )
		self.cluster = cluster

		# Request the user info from the server if not already given
		if not isinstance( uid, messages.UserMessage ):

			query = mgm = messages.UserMessage()

			if type( uid ) == int:
				mgm.param = mgm.PARAM_USE_UID
				mgm.uid = uid
			elif isinstance( uid, types.StringTypes ):
				mgm.param = mgm.PARAM_USE_NAME
				mgm.username = uid.encode( "utf-8" )
			else:
				log.critical( "First param to User() must be an " +
							  "int, string or UserMessage, not %s" %
							  uid.__class__.__name__ )

			if checkCoreDumps:
				mgm.param |= mgm.PARAM_CHECK_COREDUMPS

			if refreshEnv:
				mgm.param |= mgm.PARAM_REFRESH_ENV

			# A list of machines that aren't reply to UserMessages
			blacklist = set()

			# Macro to select a machine to do the uid resolution
			def pickMachine():
				candidates = [m for m in cluster.getMachines() \
							  if uid not in m.unknownUsers and \
							  m not in blacklist]
				if candidates:
					return random.choice( candidates )
				else:
					raise self.error, "No machines on network able " \
						  "to resolve user %s" % uid

			# The machine we request the user info from
			if not machine:
				machine = pickMachine()

			while True:
				replies = mgm.query( machine )

				if not replies:
					log.error( "%s:%s didn't reply to UserMessage, blacklisting",
							   machine.name, machine.ip)
					blacklist.add( machine )

				elif replies[0].uid == mgm.UID_NOT_FOUND:
					log.verbose( "%s:%s couldn't resolve user %s, "
								 "will not ask again",
								 machine.name, machine.ip, uid )
					machine.unknownUsers.add( uid )

				else:
					mgm = replies[0]
					break

				machine = pickMachine()
		else:
			query = mgm = uid

		self.uid = mgm.uid
		self.name = mgm.username
		self.fullname = mgm.fullname
		self.home = mgm.home
		self.mfroot = mgm.mfroot
		self.bwrespath = mgm.bwrespath
		self.coredumps = mgm.coredumps

		# Can be a hash of the min/avg/max loads for various server components
		self.load = None

		# Some counters only used when doing bots stuff
		self.totalBots = self.numProxies = self.numEntities = None


	def __str__( self ):
		return "%s (%d)" % (self.name, self.uid)

	def __cmp__( self, other ):
		if not isinstance( other, User ):
			return -1
		return cmp( self.name, other.name )

	def __hash__( self ):
		return hash( self.uid ) ^ hash( self.name )

	def getLoad( self, name ):
		loads = self.getLoads()
		if loads.has_key( name ):
			return loads[ name ]
		else:
			return 0

	def getLoads( self ):

		if not self.load:
			self._getLoads()
		return self.load

	def getNumEntities( self ):

		if self.numEntities != None:
			return self.numEntities

		elif self.serverIsRunning():
			self.numEntities = int( self.getProc( "cellappmgr" ).
									getWatcherValue( "numEntities", 0 ) ) + \
									CELL_ENTITY_ADJ
			return self.numEntities
		else:
			return None

	def getNumProxies( self ):

		if self.numProxies != None:
			return self.numProxies

		elif self.serverIsRunning():
			self.numProxies = int( self.getProc( "baseappmgr" ).
								   getWatcherValue( "numProxies", 0 ) )
			return self.numProxies
		else:
			return None

	def getTotalBots( self ):

		if self.totalBots is None:
			self.totalBots = sum( map( lambda bp: bp.nbots(),
									   self.getProcs( "bots" ) ) )

		return self.totalBots

	def _getLoads( self ):

		# Argmin/max over reported machine CPU loads
		def botsLoad( f ):
			return f( [ p.load for p in self.getProcs( "bots" ) ] ) 

		# Macro for querying load watcher values
		def watcherload( cb, am ):
			return max(
				float( self.getProc( "%sappmgr" % cb ).\
					   getWatcherValue( "%sAppLoad/%s" % (cb, am), 0 ) ), 0.0 )

		# Mapping from name to LoadStats object for all those processes
		self.load = {}

		# Total up all known bots and calculate bot CPU usage
		if self.getProcs( "bots" ):
			def avg( x ):
				return sum( x ) / float( len( x ) )

			self.load[ "bots" ] = LoadStats( botsLoad( min ),
											 botsLoad( avg ),
											 botsLoad( max ) )

		# Some important stats
		if self.serverIsRunning():
			for name in [ "cell", "base" ]:
				self.load[ name + "app" ] = LoadStats(
					watcherload( name, "min" ),
					watcherload( name, "average" ),
					watcherload( name, "max" ) )

	# ------------------------------------------------------------------------
	# Subsection: Query methods
	# ------------------------------------------------------------------------

	# Convenience functions
	def getProc( self, name ):
		return self.cluster.getProc( name, self.uid )

	def getProcs( self, name=None ):
		return self.cluster.getProcs( name, self.uid )

	def getServerProcs( self ):
		return [p for p in self.getProcs() if p.name in Process.SERVER_PROCS]

	# Return a this user's process that matches the given name exactly
	def getProcExact( self, name ):

		for p in self.getProcs():
			if p.label() == name:
				return p
		return None

	def serverIsRunning( self ):
		"""Returns true if this user appears to be running a proper bigworld
		   server."""
		for sp in Process.SERVER_PROCS:
			if not self.getProcs( sp ):
				return False
		return True

	def serverIsOverloaded( self ):
		"""Returns true if any machine running server components for this user
		is overloaded."""

		return self.getLoad( "cellapp" ).max > MAX_SERVER_CPU or \
			   self.getLoad( "baseapp" ).max > MAX_SERVER_CPU

	def getLayout( self ):
		layout = []
		for p in self.getServerProcs():
			layout.append( (p.machine.name, p.name, p.pid) )
		return layout

	def getLayoutStatus( self, layout, _async_ = None ):
		"""
		Takes a list of (mname, pname, pid) and returns a list of
		(mname, pname, pid, status).
		"""

		status = []
		notregistered = []

		shouldExtendTime = False

		# Check for registered processes first
		for mname, pname, pid in layout:
			m = self.cluster.getMachine( mname )
			if not m:
				status.append( (mname, pname, pid, "nomachine", "") )
				continue
			p = m.getProc( pid )
			if p:
				procStatus = p.status()
				statusMsg = ""
				if p.name == "dbmgr":
					# Process status of 6 == consolidating
					if procStatus != None and procStatus == 6:
						statusMsg = "Consolidating"
						shouldExtendTime = True
				status.append( (mname, pname, pid, "registered", statusMsg) )
			else:
				notregistered.append( (mname, pname, pid) )

		# Check unregistered processes to see if they are actually running
		for mname, pname, pid in notregistered:

			m = self.cluster.getMachine( mname )
			pm = messages.PidMessage()
			pm.pid = pid
			replies = pm.query( m )

			if not replies:
				log.error( "No reply to PidMessage from %s", mname )
				status.append( (mname, pname, pid, "nomachine", "") )
			elif replies[0].running:
				status.append( (mname, pname, pid, "running", "") )
			else:
				status.append( (mname, pname, pid, "dead", "") )

		# Figure out return counts
		dead = running = registered = 0

		for mname, pname, pid, state, details in status:
			if state == "dead": dead += 1
			if state == "running": running += 1
			if state == "registered": registered += 1

		# Notify async handlers
		if _async_:
			# If the DBMgr is consolidating the secondary DB's then extend the async
			# process life
			if shouldExtendTime == True:
				_async_.extendTimeout( 1 )
			_async_.update( "status", dict( layout = status ) )

		return (dead, running, registered, status)


	def getLayoutErrors( self ):
		"""Return a list of strings containing error messages (if any) for the
		User's server layout.""" 

		# List of machines with bots running on them
		botMachines = {}
		errors = []
		machineInfo = {}

		dbMachine = None
		for proc in self.getProcs():

			if proc.requiresOwnCPU():
				uniqueProcs = machineInfo.get( proc.machine, 0 )
				uniqueProcs = uniqueProcs + 1
				machineInfo[ proc.machine ] = uniqueProcs

			if proc.name == "dbmgr":
				dbMachine = proc.machine
				# DBMgr should really be on a machine by itself
				if len( dbMachine.procs ) > 1:
					errors.append( "DBMgr is running on a machine with other "
						"server processes." )

			elif proc.name == "bots":
				# if bots - add to machine array
				botMachines[ proc.machine.name ] = True

		# Now check if there are too many processes for each core
		for machine, uniqueCount in machineInfo.iteritems():
			if machine.ncpus < uniqueCount:
				errors.append( "Machine %s appears to be running too many processes "
					"for the number of available CPUs/cores." % machine.name )


		# If there are any bots running, check that the processes aren't running
		# on server machines with other processes such as dbmgr/cellapp/baseapp 
		errorBotMachines = []
		for botMachineName in botMachines.keys():
			for uniqueMachine in machineInfo.keys():
				if uniqueMachine.name == botMachineName:
					errorBotMachines.append( botMachineName )

		if len( errorBotMachines ) > 0:
			errors.append( "bots running on the following machines should "
				"be moved to less critical server machines: %s" % \
				", ".join( errorBotMachines ) )

		return errors


	def layoutIsRunning( self, layout, status = [], _async_ = None ):

		dead, running, registered, status[:] = \
					   self.getLayoutStatus( layout, _async_ )

		if running == 0 and dead > 0:
			raise Cluster.TerminateEarlyException
		else:
			return registered == len( layout )


	def layoutIsStopped( self, layout, status = [], _async_ = None ):

		dead, running, registered, status[:] = \
					   self.getLayoutStatus( layout, _async_ )

		return dead == len( layout )


	# ------------------------------------------------------------------------
	# Subsection: Output methods
	# ------------------------------------------------------------------------

	def lsMiscProcesses( self ):
		"""
		Print out info for misc processes.  Returns a list of the processes
		displayed.
		"""

		miscps = [p for p in self.getProcs() if \
				  Process.clean( p.name ) in Process.SINGLETON_PROCS]

		if not miscps:
			log.info( "no misc processes found!" )
			return []

		log.info( "misc processes:" )
		for p in miscps:
			if p:
				log.info( "\t%s", p )
			else:
				log.info( "\tNo %s found!", pname )

		return miscps


	def ls( self ):
		"""
		Prints detailed server info for a particular user.
		"""

		# Assemble a list of already-displayed processes
		displayed = self.lsMiscProcesses()

		# Quick macro to get all machines for a given component
		def displayMachinesForProcess( name, warn = True ):
			cmpByMachineLoad = lambda p1, p2: \
							   Machine.cmpByLoad( p1.machine, p2.machine )
			procs = self.getProcs( name )
			procs.sort( cmpByMachineLoad )

			if procs:
				log.info( "%s (%d):" %
						  (Process.getPlural( name ), len( procs )) )
				for p in procs:
					log.info( "\t%s", p )

			elif warn:
				log.info( "no %s found!" % Process.getPlural( name ) )

			return procs

		# Display cellapps and baseapps
		displayed.extend( displayMachinesForProcess( "cellapp" ) )
		displayed.extend( displayMachinesForProcess( "baseapp" ) )

		# Display any processes not already done
		remaining = set( self.getProcs() ) - set( displayed )
		for pname in Process.ALL_PROCS:
			ps = [p for p in remaining if p.name == pname]
			if ps:
				displayMachinesForProcess( pname, False )

		# Display listing of used machines
		ms = util.uniq( [p.machine for p in self.getProcs()], cmp = cmp )
		ms.sort( Machine.cmpByLoad )
		if ms:
			log.info( "\nmachines (%d):", len( ms ) )
			for m in ms:
				log.info( "\t%s", m )

	def lsSummary( self ):
		"""Prints summarised info about this user's server."""

		# Can't go any further without a complete server
		if not self.serverIsRunning():
			log.error( "Server isn't running, can't display summary" )
			return

		# Find world server
		self.lsMiscProcesses()

		# Macro that displays a summary for a type of process
		def displaySummaryForProcess( name, warn = True ):
			procs = self.getProcs( name )
			if procs:
				log.info( "%d %s at (%s)" % \
						  (len( procs ),
						   Process.getPlural( name, len( procs ) ),
						   self.getLoad( name )) )
			elif warn:
				log.info( "no %s found!" % Process.getPlural( name ) )

		displaySummaryForProcess( "cellapp" )
		displaySummaryForProcess( "baseapp" )
		displaySummaryForProcess( "bots", False )

	# --------------------------------------------------------------------------
	# Subsection: Cluster control methods
	# --------------------------------------------------------------------------

	def verifyEnvSync( self, machines = [], writeBack = False ):
		"""
		Checks this users BW env settings on each machines (or entire network if
		machines == []) and warns if settings are out of sync.

		If writeBack is True, the largest set of machines with in-sync config
		settings will be written back to the passed-in 'machines' list.
		"""

		um = messages.UserMessage()
		um.param = um.PARAM_USE_UID
		um.uid = self.uid

		variants = {}

		for um, (ip, port) in messages.MachineGuardMessage.batchQuery(
			[um], 1.0, machines )[um]:
			if um.mfroot or um.bwrespath:
				glob = um.mfroot + ";" + um.bwrespath
			else:
				glob = "<undefined>"
			if not variants.has_key( glob ):
				variants[ glob ] = set( [ip] )
			else:
				variants[ glob ].add( ip )

		if len( variants ) <= 1:
			return True

		else:

			if not writeBack:
				log.warning( "~/.bwmachined.conf differs on network:" )
				for glob, ips in variants.items():
					log.warning( "" )
					log.warning( "%s:" % glob )
					for ip in sorted( ips, util.cmpAddr ):
						m = self.cluster.getMachine( ip )
						if m:
							log.warning( m.name )
						else:
							log.error( "Unable to resolve hostname for %s", ip )

			else:
				while machines:
					machines.pop()
				biggest = sorted( variants.values(), key = len )[-1]
				machines.extend( map( self.cluster.getMachine, biggest ) )

			return False


	def start( self, machines = None, _async_ = None,
			   bots = False, revivers = False, tags = False, group = None ):
		"""
		Start a server for this user.  If machines is not passed, all machines
		are considered as candidates.

		If 'bots' is True, bots processes will be allocated and started.

		If 'revivers' is True, a reviver will be started on each machine that a
		process of another type is.

		If 'tags' is True, machines will be restricted to starting process types
		listed in their [Components] tag list.

		If 'group' is defined, then only machines in that machined group will be
		considered.
		"""

		# Don't start up if we already have processes running
		if self.getServerProcs():
			log.error( "Server processes already running, aborting startup" )
			self.ls()
			return False

		# Get server candidates
		if not machines:
			machines = self.cluster.getMachines()
		machines.sort( lambda m1, m2: cmp( m1.totalmhz(), m2.totalmhz() ) )

		# Verify that env is in sync
		if not self.verifyEnvSync( machines, writeBack = True ):
			log.warning( "Candidate machines restricted due to out-of-sync env"	)
			log.warning( "Candidate set is now %s",
						 ",".join( [m.name for m in machines] ) )

		# If group is specified, restrict machine set to that group now
		if group:
			self.cluster.queryTags( "Groups" )
			machines = [m for m in machines \
						if "Groups" in m.tags and group in m.tags[ "Groups" ]]
			log.verbose( "Machines in group %s: %s",
						 group, " ".join( [m.name for m in machines] ) )

		log.verbose( "Candidate machines:" )
		for m in machines:
			log.verbose( m )

		# Bail now if no candidates found
		if not machines:
			log.error( "No machines found satisfying the filters" )
			return False

		# List of server processes we expect to be running
		layout = []

		# Make a list of 'cpus' sorted by speed
		cpus = reduce( operator.concat, [ [m]*m.ncpus for m in machines ] )
		cpus.sort( lambda m1, m2: cmp( m1.mhz, m2.mhz ) )

		# If we are going to need to know about machine Components tags, get
		# them now
		if tags:
			self.cluster.queryTags( "Components", machines )

		# A little class to represent a class of processes for spawning
		class Category:

			UID = self.uid

			# If multiplier is None, we must spawn exactly one process
			# If exclusive, we try to spawn only one of these processes per CPU
			def __init__( self, name, multiplier, exclusive ):
				self.name = name
				self.multiplier = multiplier
				self.exclusive = exclusive
				self.totalCPU = 0.0
				self.machines = []

			# Processes with multiplier == None are considered to be the lowest
			# so that they will be allocated first.  In the event of CPU
			# allocation ties, the higher multiplier is considered to be lower
			# since this will cause it to be allocated to a slower CPU.
			def __cmp__( self, other ):
				if self.multiplier is None or other.multiplier is None:
					return cmp( self.multiplier, other.multiplier )
				else:
					return cmp( self.totalCPU * self.multiplier,
								other.totalCPU * other.multiplier ) or \
								-cmp( self.multiplier, other.multiplier )

			def add( self, machine ):
				self.totalCPU += max( 1-machine.load(), 0 ) * machine.totalmhz()
				self.machines.append( machine )
				pid = machine.startProc( self.name, self.UID, _async_=_async_ )

				if pid == 0:
					log.error( "Couldn't even execute %s on %s",
							   self.name, machine.name )
					return False
				else:
					log.verbose( "Starting %s on %s", self.name, machine.name )
					layout.append( (machine.name, self.name, pid) )
					return True

		# Set up categories for process spawning
		allcats = [ Category( "cellappmgr", None, False ),
					Category( "baseappmgr", None, False ),
					Category( "dbmgr", None, False ),
					Category( "loginapp", None, False ),
					Category( "cellapp", 3.0, True ),
					Category( "baseapp", 5.0, True ) ]
		if bots:
			allcats.append( Category( "bots", 2.0, True ) )

		# Make a copy of the category list, cause we'll want to modify it in the
		# CPU allocation loop
		cats = allcats[:]

		# Make a log entry to say we're starting the server
		self.log( "Starting server" )

		# Allocate all CPUs to categories
		while cpus and cats:

			# Pick the most needy category
			cat = min( cats )

			# Find the slowest CPU that is capable of running this process type
			cpu = None
			for m in cpus:
				if not tags or m.canRun( cat.name ):
					cpu = m
					break

			# If no CPU found, stop trying to assign CPU to this category (we'll
			# fix it up later if no CPUs were allocated)
			if not cpu:
				cats.remove( cat )
				continue

			# Add this process to that category
			if not cat.add( cpu ):
				log.error( "Process execution failure; shutting down server" )
				self.cluster.refresh()
				self.smartStop( forceKill=True )
				return False

			# If the category is exclusive, remove that CPU from the list
			if cat.exclusive:
				cpus.remove( cpu )

			# If the category is singleton, remove it from the list
			if cat.multiplier is None:
				cats.remove( cat )

				# If the category is singleton and there are no more singleton
				# categories, remove the cpu (i.e. don't spawn other stuff on
				# the world server unless you have to)
				if not [c for c in cats if c.multiplier is None] and \
					   cpu in cpus:
					cpus.remove( cpu )

		# If any category didn't get allocated any CPU, allocate it to the
		# fastest machine capable of running it now
		allocateFailed = False
		for cat in [ c for c in allcats if not c.machines ]:

			for m in machines[::-1]:
				if not tags or m.canRun( cat.name ):
					cat.add( m )
					break

			if not cat.machines:
				log.error( "Couldn't find any machines capable of running %ss",
						   cat.name )
				allocateFailed = True

		if allocateFailed:
			log.error( "Some processes weren't started, aborting" )
			return False

		# Start revivers on all machines used if required
		if revivers:
			reviverCat = Category( "reviver", None, False )
			allcats.append( reviverCat )
			for m in machines:
				reviverCat.add( m )

		for c in allcats:
			log.verbose( "%s (%dMHz): %s" % \
						 (c.name, c.totalCPU,
						  " ".join( [m.name for m in c.machines] )) )

		if layout and self.verifyStartup( layout, _async_ = _async_ ):
			self.ls()
			return True
		else:
			return False

	@staticmethod
	def parseXMLLayout( s ):
		"""
		Reads a server layout from an XML string, and returns a list of
		processes in the format [(machine_name, process_name) ...].  Note that
		this is not the format that is expected by layoutIsRunning(), it doesn't
		include the PIDs.  It is up to startFromXML() to include this itself.
		"""

		# TODO: There is very little error checking for the expected format of
		# the XML here.  If the XML is not specified exactly as required, the
		# resulting errors will probably not make much sense.

		doc = minidom.parseString( s )
		layout = []

		for pname in Process.ALL_PROCS:

			# Find the section of the XML tree that deals with these procs.  The
			# plural case is for compatibility with old layouts.
			nodes = doc.getElementsByTagName( pname ) or \
					doc.getElementsByTagName( Process.getPlural( pname ) )

			if not nodes: continue
			root = nodes[0]

			for child in filter( lambda n: n.attributes, root.childNodes ):

				# For "machine" elements
				if child.tagName == "machine":
					mname = child.getAttribute( "name" )
					count = child.getAttribute( "count" ) or "1"
					count = int( count )
					for i in xrange( count ):
						layout.append( (mname, pname) )

				# For "range" elements
				elif child.tagName == "range":
					prefix = child.getAttribute( "prefix" )
					start = int( child.getAttribute( "start" ) )
					end = int( child.getAttribute( "end" ) )
					format = child.getAttribute( "format" ) or "%d"
					count = child.getAttribute( "count" ) or "1"
					count = int( count )

					for i in xrange( start, end+1 ):
						mname = prefix + (format % i)
						for j in xrange( count ):
							layout.append( (mname, pname) )

		# Special-case for old-style layouts with the 'world' section
		worldNodes = doc.getElementsByTagName( "world" )
		if worldNodes:
			mname = worldNodes[0].getAttribute( "name" )
			for pname in ("cellappmgr", "baseappmgr", "loginapp", "dbmgr"):
				layout.append( (mname, pname) )

		return layout

	def startFromXML( self, file, _async_ = None, enforceBasicLayout = False ):
		"""
		Start a server using the layout in the given XML.  The argument can
		either be a filename pointing to an XML file or a file-like object to
		read the XML data from.

		If enforceBasicLayout is True and the layout given in the XML is missing
		any basic server processes, then they will automatically be started too.
		"""

		# Don't do anything if a server's already running
		if self.serverIsRunning():
			log.error( "Can't load layout from XML while server is running!" )
			return False

		if type( file ) == str:
			try:
				xmldata = open( file ).read()
			except Exception, e:
				log.error( "Couldn't read XML data from %s: %s", file, e )
				return False
		else:
			xmldata = file.read()

		layout = self.parseXMLLayout( xmldata )

		# Verify that each machine actually exists
		missing = False
		mnames, pnames = set(), set()
		for mname, pname in layout:
			m = self.cluster.getMachine( mname )
			if m:
				mnames.add( mname )
				pnames.add( pname )
			else:
				log.error( "Layout refers to non-existent machine '%s'", mname )
				missing = True

		if missing:
			log.error( "Aborting startup due to missing machines" )
			return False

		self.log( "Starting server" )

		# Enforce basic layout if required
		if enforceBasicLayout:

			mnames = list( mnames )
			if not mnames:
				log.error( "Can't enforce basic layout with an "
						   "empty prior layout" )
				return False

			for pname in Process.SERVER_PROCS:
				if pname not in pnames:
					layout.append( (random.choice( mnames ), pname) )
					log.notice( "Added missing basic process %s to %s",
								layout[-1][1], layout[-1][0] )

		# Iterate through layout and add the pid to each entry for passing to
		# layoutIsRunning()
		for i in xrange( len( layout ) ):
			mname, pname = layout[i]
			machine = self.cluster.getMachine( mname )
			pid = machine.startProc( pname, self.uid, _async_ = _async_ )
			layout[i] = (mname, pname, pid)
			log.verbose( "Starting %s on %s (pid:%d)", pname, mname, pid )

		if layout and self.verifyStartup( layout, _async_ = _async_ ):
			self.ls()
			return True
		else:
			return False

	def verifyStartup( self, layout, _async_ = None ):

		# Signal async listeners with the layout
		if _async_:
			_async_.update( "layout", layout )

		status = []
		ok = self.cluster.waitFor(
			util.Functor( self.layoutIsRunning,
						  args = [layout, status],
						  kwargs = {"_async_": _async_} ),
			POLL_SLEEP, MAX_STARTUP_SLEEPS )

		if not ok:
			log.error( "The following processes failed to start:" )
			# NB: procStatus is used rather than unpacking directly
			#     in the for loop, as layoutIsRunning can return an
			#     extra 'details' element which will break unpacking
			for procStatus in status:
				mname = procStatus[ 0 ]
				pname = procStatus[ 1 ]
				pid   = procStatus[ 2 ]
				state = procStatus[ 3 ]
				if state != "registered":
					log.error( "%s on %s (pid: %d): %s",
							   pname, mname, pid, state )
		return ok

	def stop( self, signal = None, _async_ = None ):
		"""
		Kills all my processes by sending the given signal, or SIGINT if none
		given.
		"""

		if signal is None:
			signal = messages.SignalMessage.SIGINT

		# Kill revivers first if there are any
		for p in self.getProcs( "reviver" ):
			p.stop()

		# Wait for reviver death
		if not self.cluster.waitFor( lambda: not self.getProcs( "reviver" ),
									 POLL_SLEEP, MAX_POLL_SLEEPS ):
			log.error( "Some revivers haven't shut down!" )
			for p in self.getProcs( "reviver" ):
				log.error( p )
			return False

		# Kick off loginapp controlled shutdown on SIGUSR1
		if signal == messages.SignalMessage.SIGUSR1:
			loginapps = self.getProcs( "loginapp" )
			if loginapps:
				for p in loginapps:
					p.stop( messages.SignalMessage.SIGUSR1 )
			else:
				log.error( "Can't do controlled shutdown with no loginapp!" )
				return False

			# Loginapp's don't know about bots processes so kill them manually
			for bp in self.getProcs( "bots" ):
				bp.stop()

		# Otherwise send signals to all server components
		else:
			for p in self.getProcs():
				if isinstance( p, StoppableProcess ):
					p.stop( signal )

		# Wait for server to shutdown
		if not self.cluster.waitFor(
			util.Functor( self.layoutIsStopped,
						  args = [self.getLayout()],
						  kwargs = {"_async_": _async_} ),
			POLL_SLEEP, MAX_POLL_SLEEPS ):
			log.error( "Components still running after stop( %d ):" % signal )
			for p in self.getProcs():
				log.error( "%s (load: %.3f)" % (p, p.machine.load()) )
			return False
		else:
			return True

	def smartStop( self, forceKill=False, _async_ = None ):
		"""
		Stop the server by the most controlled means possible.
		"""

		errors = False

		# Because of the way the shutdown messages flow, you actually need all
		# the "world" processes running to do a controlled shutdown
		if self.getProc( "loginapp" ) and self.getProc( "dbmgr" ) and \
			   self.getProc( "cellappmgr" ) and self.getProc( "baseappmgr" ):

			self.log( "Starting controlled shutdown" )

			if self.stop( messages.SignalMessage.SIGUSR1, _async_ = _async_ ):
				return True
			else:
				if not forceKill:
					# Don't force processes to quit.
					# If we are doing controlled shutdown, we'll have to wait
					# until the components have all shutdown by themselves,
					# just report on what processes are still up every
					# MAX_POLL_SLEEPS.
					while not self.cluster.waitFor(
						util.Functor( self.layoutIsStopped,
							args = [self.getLayout()],
							kwargs = {"_async_": _async_} ),
						POLL_SLEEP, MAX_POLL_SLEEPS ):

						log.error( "Components still running after controlled "
							"shutdown initiated:" )
						for p in self.getProcs():
							log.error( "%s (load: %.3f)", p, p.machine.load() )

					# if we break out, then controlled shutdown is finished
					# (presumably succeeded)
					return True

				log.error( "Controlled shutdown failed" )
				errors = True


		if forceKill:
			self.log( "Starting SIGINT shutdown" )

			if self.stop( messages.SignalMessage.SIGINT, _async_ = _async_ ):
				if errors:
					log.info( "Shutdown via SIGINT successful" )
				return True
			else:
				log.error( "Forced shutdown with SIGINT failed" )
				errors = True

			self.log( "Starting SIGQUIT shutdown" )

			if self.stop( messages.SignalMessage.SIGQUIT, _async_ = _async_ ):
				if errors:
					log.info( "Shutdown via SIGQUIT successful" )
				return True
			else:
				log.error( "Forced shutdown with SIGQUIT failed" )
				errors = True

		else:	
			# Handles the case where only some of the server processes is running 
			# or no server process is running, and force kill is not allowed.
			# The server most likely is in the process of shutting down, although 
			# it may be the case that one or more of the "world" processes may 
			# have crashed.
			# If the user wants to kill the remaining running server processes, 
			# they should use ./control_cluster.py kill command.
			if self.getProcs():
				# List processes still running.
				self.ls()

				msg = \
"\n\n" \
"The server cannot be shut down cleanly. It may already be in the process of\n" \
"shutting down.\n\n" \
"To force the server processes listed above to stop now rather than waiting for\n" \
"them to stop, use the following command:\n" \
"$ ./control_cluster.py kill\n\n" \
"WARNING: Using './control_cluster.py kill' command may cause data loss as\n" \
"BaseApps may be in the process of writing entities to the database."

				log.info( msg )

			else:
				log.info( "No server process is running." )	
				return True

		return False

	def restart( self, _async_ = None ):
		"""
		Shuts down the server then restarts it with the same layout.
		"""

		stream = StringIO()
		self.saveToXML( stream )
		stream.seek( 0 )
		if not self.smartStop( _async_ = _async_ ):
			return False
		return self.startFromXML( stream, _async_ = _async_,
								  enforceBasicLayout = True )

	def startProc( self, machine, pname, count = 1 ):

		# Start new processes and collect their PIDs
		pids = [machine.startProc( pname, self.uid ) for i in xrange( count )]

		# If there are any 0's in there something went wrong
		if 0 in pids:
			log.error( "%d processes didn't start!",
					   len( [pid for pid in pids if pid == 0] ) )
			return False

		# Function to test whether the new processes have started
		allDone = lambda: None not in [machine.getProc( pid ) for pid in pids]

		# Wait till they've started up
		if not self.cluster.waitFor( allDone, 1, 10 ):
			log.error( "Some processes didn't start!" )
			return False
		else:
			return True


	def verifyLayoutIsRunning( self, file ):
		layout = self.parseXMLLayout( open( file ).read() )
		runningProcs = self.getProcs()

		for machine,proc in layout:

			foundProc = False
			# If we are still expecting more items in the layout
			# but no more items are running, fail.
			if len(runningProcs) == 0:
				return False

			for activeProc in runningProcs:
				if machine == activeProc.machine.name and \
				proc == activeProc.name:
					foundProc = True
					runningProcs.remove( activeProc )
					break

			if not foundProc:
				return False

		if len(runningProcs) == 0:
			return True

		return False


	@Exposed.expose( args = [("file", "the filename to save the layout to")] )
	def saveToXML( self, file ):
		"""Writes this user's cluster layout to XML."""

		# Create XML document and root node
		doc = minidom.Document()
		root = doc.createElement( "cluster" )
		doc.appendChild( root )

		# Now do the non-world processes
		for pname in Process.ALL_PROCS:

			# Count up the number of procs on each machine
			pcounts = {}
			for p in self.getProcs( pname ):
				if pcounts.has_key( p.machine.name ):
					pcounts[ p.machine.name ] += 1
				else:
					pcounts[ p.machine.name ] = 1

			# Bail now if there aren't any
			if not pcounts: continue

			# Make a sorted list of the machines
			machines = filter( lambda m: pcounts.has_key( m.name ),
							   self.cluster.getMachines() )
			machines.sort( Machine.cmpByHostDigits )

			# The root node for these entries
			listNode = doc.createElement( pname )
			root.appendChild( listNode )

			# Make entries for each machine
			for m in machines:
				node = doc.createElement( "machine" )
				node.setAttribute( "name", m.name )
				if pcounts[ m.name ] > 1:
					node.setAttribute( "count", str( pcounts[ m.name ] ) )
				listNode.appendChild( node )

		# Write it to the file
		if type( file ) == str:
			file = open( file, "w" )
		file.write( doc.toprettyxml() )
		if not isinstance( file, StringIO ):
			file.close()
		return True

	#--------------------------------------------------------------------------
	# Subsection: Bot Operations
	#--------------------------------------------------------------------------

	def getBotMachines( self ):
		"""Returns a list of all the machines in the cluster that are candidates
		   for running bots."""

		return filter( lambda m: m.isBotCandidate( self.uid ),
					   self.cluster.getMachines() )

	def getBestAddCandidate( self ):
		"""
		Returns the lowest loaded bot process below the CPU thresh, or None
		if all bot processes are overloaded.
		"""

		bps = sorted( self.getProcs( "bots" ), key = lambda p: p.load )
		if not bps:
			return None

		if bps[0].load < MAX_BOTS_CPU:
			return bps[0]
		else:
			return None

	def getBestDelCandidate( self ):

		# Comparator for deletion.  Puts processes with the lowest number of
		# bots first, then orders by highest CPU load.
		def delcmp( bp1, bp2 ):
			botcmp = cmp( bp1.nbots(), bp2.nbots() )
			loadcmp = cmp( bp2.machine.load(), bp1.machine.load() )
			return botcmp or loadcmp

		# Throw away processes with no active bots
		bps = filter( lambda bp: bp.nbots() > 0, self.getProcs( "bots" ) )
		bps.sort( delcmp )

		if bps:
			return bps[0]
		else:
			return None

	def lsBots( self ):
		"""Prints out information about currently running bots processes, and
		   returns an array of BotProcess objects describing the same info."""

		# Get info about the cluster
		bms = sorted( self.getBotMachines(), cmp = util.cmpAddr,
					  key = lambda m: m.ip )

		usedms = util.uniq( map( lambda p: p.machine, self.getProcs( "bots" ) ),
							cmp )
		if usedms:
			log.info( "machines running bots processes:" )
			for bm in usedms:
				bps = bm.getProcs( "bots" )
				nbots = sum( [bp.nbots() for bp in bps] )
				log.info( "%-11s %d bots on %d bots processes",
						  bm.name, nbots, len( bps ) )
			log.info( "" )

		freems = self.getBotMachines()
		for m in usedms:
			freems.remove( m )
		if freems:
			log.info( "free candidate machines:" )
			for bm in freems:
				log.info( bm )
			log.info( "" )

		log.info( "%d clients\n%d proxies\n%d cell entities",
				  self.getTotalBots(), self.getNumProxies(),
				  self.getNumEntities() )

		for pname, load in self.getLoads().items():
			log.info( "min/avg/max %s load: %s",
					  re.sub( "app", "", pname ), load )


	def addBots( self, numToAdd ):
		"""Add the given number of bots to existing processes or create new
		   processes to handle them as necessary."""

		log.info( "Adding %d bot%s...", numToAdd, ("s","")[numToAdd==1] )
		totalToAdd = numToAdd
		starttime = time.time()

		while numToAdd > 0:

			try:
				# Refresh all cluster information
				self.cluster.refresh()

				# Find a bots process that isn't overloaded
				bp = self.getBestAddCandidate()

				if not bp:
					log.info( "\tall bots processes are overloaded; "
							  "waiting for more CPU..." )
					time.sleep( CPU_WAIT_SLEEP )
					continue

				# If server components are overloaded, wait for a little bit
				if self.serverIsOverloaded():
					log.info( "\tserver overloaded; waiting for more CPU ..." )
					while self.serverIsOverloaded():
						time.sleep( CPU_WAIT_SLEEP )
						self.cluster.refresh()

				# Add bots
				numToAddNow = min( numToAdd, NBOTS_AT_ONCE )
				bp.addBots( numToAddNow )
				numToAdd -= numToAddNow
				log.info( "\tadded %d to %s:%d (%d done in %.1fs)",
						  numToAddNow, bp.machine.name, bp.port(),
						  totalToAdd - numToAdd,
						  time.time() - starttime )

				# Wait a little bit.  We didn't need to do this before
				# because each refresh() took so long, but now we need to do
				# this or it's easy to overload a running system.
				time.sleep( 1.0 )

			except KeyboardInterrupt:
				break

		log.info( "Added %d bots in %.1fs",
				  totalToAdd - numToAdd, time.time() - starttime )


	def delBots( self, numtoop ):
		"""Deletes the given number of bots from running bots processes, least
		   loaded processes first.  Does not kill empty bots processes."""

		# If no bot processes, bail out
		if not self.getProcs( "bots" ):
			log.error( "no known bot processes to delete bots from" )
			return False

		# Bail if no known bots
		if self.getTotalBots() == 0:
			log.error( "can't delete - no known bots" )
			return False

		# Cap deletion amount if more than known
		numtoop = min( numtoop, self.getTotalBots() )

		log.info( "Deleting %d of %d known bots ..." %
				  (numtoop, self.getTotalBots() ) )
		deleted = 0

		while numtoop > 0:

			# Refresh cluster info
			self.cluster.refresh()

			# The process we're deleting from
			bp = self.getBestDelCandidate()

			# If we couldn't find a del candidate, chances are a bot process
			# timed out when asked for its bot count, so go round again.
			if bp is None:
				continue

			# The number we're actually going to delete at once
			chunksize = min( numtoop, NBOTS_AT_ONCE, bp.nbots() )

			# If the bot process has no bots, something's gone wrong
			assert bp.nbots() > 0

			# Do it
			bp.delBots( chunksize )
			bp.nbots( bp.nbots() - chunksize )
			numtoop -= chunksize
			deleted += chunksize
			log.info( "\tdeleted %d from %s:%d (%d done)",
					  chunksize, bp.machine.name, bp.port(), deleted )

		log.info( "Deleted %d bots OK." % deleted )
		return True

	def setWatchersOnAllBots( self, *values ):
		"""
		This method takes any number of tuples. Each tuple contains the path to
		the value to set and the value to set it to, eg:

		setWatchersOnAllBots( ('defaultControllerType', 'Patrol'),
		                      ('defaultControllerData', 'test.bwp') )

		It sets these watcher values on all bot processes.
		"""

		# Handle the case where you've just got two string arguments
		if values and (type(values[0]) == str):
			values=(values,)

		watcherMessage = messages.WatcherDataMessage()
		watcherMessage.message = watcherMessage.WATCHER_MSG_SET2
		watcherMessage.count = 0

		for value in values:
			log.info( "Setting '%s' to '%s'" % value )
			watcherMessage.addSetRequest( value[0], value[1] )

		sock = socketplus.socket()
		msg = watcherMessage.get()
		for bp in self.getProcs( "bots" ):
			sock.sendto( msg, bp.addr() )
			sock.recvfrom( RECV_BUF_SIZE )

	def setBotMovement( self, controllerType, controllerData, botTag ):
		"""Sets the controllerType and controllerData for all bots matching the
		   given tag, e.g. ('Patrol','server/bots/test.bwp','')."""

		self.setWatchersOnAllBots( ( "defaultControllerType", controllerType ),
								   ( "defaultControllerData", controllerData ),
								   ( "command/updateMovement", botTag ) )

	def runScriptOnBots( self, script = "" ):
		"""Runs Python script on all bot apps. With no args, command is read
		   from stdin, otherwise first arg is the command."""

		if script == "":
			log.info( "Input Python script to run (Ctrl+D to finish):" )
			self.setWatchersOnAllBots( "command/runPython", sys.stdin.read() )
		else:
			self.setWatchersOnAllBots( "command/runPython", script )


	#--------------------------------------------------------------------------
	# Subsection: Exposed stuff
	#--------------------------------------------------------------------------

	def log( self, text, severity = "INFO" ):
		"""
		Send a log message to all loggers on the network.
		"""

		for logger in self.cluster.getProcs( "message_logger" ):
			logger.sendMessage( text, self, severity )

# ------------------------------------------------------------------------------
# Section: Cluster
# ------------------------------------------------------------------------------

class Cluster( Exposed ):
	"""A way of organising information about the machines in a cluster."""

	# Set of machines known to have out-of-date machined's.  We keep track of
	# this to avoid multiple warnings.
	OLD_MACHINEDS = set()

	# A cache of previously created Cluster objects
	OBJECT_CACHE = []
	OBJECT_CACHE_SEM = threading.RLock()

	def __init__( self, **kw ):
		Exposed.__init__( self )

		if kw.has_key( "view" ):
			self.view = kw[ "view" ]
		else:
			self.view = "machine"

		if kw.has_key( "user" ):
			self.user = kw[ "user" ]
		else:
			self.user = None

		if kw.has_key( "uid" ):
			self.uid = kw[ "uid" ]
		else:
			self.uid = None

		# Mapping from ip addresses to Machine objects
		self.machines = {}

		# Mapping from uids to User objects
		self.users = {}

		# Stuff for recycling this object
		self.ctime = time.time()
		self.kw = kw
		self.thread = threading.currentThread()
		self.OBJECT_CACHE_SEM.acquire()
		self.expungeCache()
		self.OBJECT_CACHE.append( self )
		self.OBJECT_CACHE_SEM.release()

		self.refresh()

	def __str__( self ):

		ms = self.getMachines()
		ms.sort( Machine.cmpByHostDigits )

		lines = []
		for m in ms:
			lines.append( str( m ) )

		lines.append( "%d machines total" % len( ms ) )

		return "\n".join( lines )

	def lsUsers( self ):

		for u in sorted( self.getUsers(), key = lambda u: u.name ):
			numProcs = len( u.getProcs() )
			log.info( "%s running %d process%s" %
					(u, numProcs, ("", "es")[numProcs != 1]) )
			for p in u.getProcs():
				log.verbose( "\t%s", p )

	@classmethod
	def expungeCache( self ):
		"""
		Clears out-of-date cluster objects in the OBJECT_CACHE.
		"""

		for c in self.OBJECT_CACHE[:]:
			if time.time() - c.ctime > 1.0:
				self.OBJECT_CACHE.remove( c )


	@classmethod
	def get( self, **kw ):
		"""
		Factory method that accepts the same params as the constructor.  Use
		this if you want to recycle Cluster objects that are being created in
		quick succession.
		"""

		try:
			self.OBJECT_CACHE_SEM.acquire()
			self.expungeCache()

			# Try to find a cache object that is new enough
			for c in self.OBJECT_CACHE:
				if c.kw == kw and c.thread == threading.currentThread():
					return c

			return Cluster( **kw )

		finally:
			self.OBJECT_CACHE_SEM.release()

	def refresh( self, retry = 5, clearUsers = False ):
		for i in xrange( retry ):
			if self.refresh_( clearUsers ):
				return True
		return False

	def refresh_( self, clearUsers = False ):

		global TIMEOUT

		# Flat list of all processes (non-essential but useful)
		self.procs = set()

		# Only flush users if explictly required
		if clearUsers:
			self.users.clear()

		# We need to keep a record of the things that are reported by the
		# cluster that is separate to our own internal lists, so we can remove
		# any stale objects which refer to cluster components that no longer
		# exist
		newMachines = set(); newProcs = set()

		# Find all the machines in this cluster, we send out both a
		# HighPrecision and LowPrecision message to ensure we receive
		# replies from both quickly.
		wmm = messages.WholeMachineMessage()
		hpm = messages.HighPrecisionMachineMessage()

		# Find server processes and statistics for each
		psm = messages.ProcessStatsMessage()
		if self.uid:
			psm.uid = self.uid
			psm.param |= psm.PARAM_USE_UID
		elif self.user:
			psm.uid = uidmodule.getuid( self.user )
			psm.param |= psm.PARAM_USE_UID

		# Run the MGM query
		replies = messages.MachineGuardMessage.batchQuery( [hpm, wmm, psm] )

		def handleMachineMessage( mgm, srcIP, srcPort ):

			machArgs = (self, mgm, srcIP)

			# If we've never seen this machine before make a new object
			if not self.getMachine( srcIP ):
				self.machines[ srcIP ] = Machine( *machArgs )

			# Otherwise re-initialise the existing one, passing the extra
			# False argument to prevent the process map being cleared
			else:
				self.getMachine( srcIP ).__init__( *(machArgs + (False,)) )

			# Mark this machine as non-stale
			newMachines.add( self.getMachine( srcIP ) )

			return


		# Now set up Machine objects
		#  Handle the HighPrecision replies first, then  all machines that
		#  didn't reply with HighPrecision results.

		# dict of machines that have responded with a HighPrecision message
		seenHPM = {}
		for mgm, (srcip, srcport) in replies[ hpm ]:

			# Build a list of seen machines so we don't add them again while
			# processing the WholeMachineMessage replies

			handleMachineMessage( mgm, srcip, srcport )
			seenHPM[ srcip ] = True


		# All the old bwmachined processes that don't understand HPM
		for mgm, (srcip, srcport) in replies[ wmm ]:

			if seenHPM.has_key( srcip ):
				continue

			handleMachineMessage( mgm, srcip, srcport )


		# Sort SERVER_COMPONENTs before WATCHER_NUBs
		def cmpByCategory( m1, m2 ):
			if m1.category == m2.category == m1.WATCHER_NUB or \
			   m1.category == m2.category == m1.SERVER_COMPONENT:
				return 0
			elif m1.category == m1.SERVER_COMPONENT:
				return -1
			else:
				return 0

		replies[ psm ].sort( cmpByCategory, key = lambda (mgm, addr): mgm )
		serverComponents = set()

		# Now set up Process objects
		for mgm, (srcip, srcport) in replies[ psm ]:

			# If we don't know anything about srcip, the WHOLE_MACHINE mgm has
			# been lost, so restart this refresh
			if not self.machines.has_key( srcip ):
				log.error( "Got process %s on unknown host %s!",
						   mgm.name, srcip )
				return False

			# pid == 0 indicates no processes found
			if mgm.pid == 0:
				continue

			hostmachine = self.machines[ srcip ]
			pid = mgm.pid

			# If we have already received a SERVER_COMPONENT message for this
			# process, ignore the additional info
			if hostmachine.getProc( pid ) in serverComponents:
				continue

			# If we already have a Process for this pid, re-init now
			if hostmachine.getProc( pid ):
				p = hostmachine.getProc( pid )
				p.__init__( hostmachine, mgm )
			else:
				p = Process.getProcess( hostmachine, mgm )
				hostmachine.procs[ pid ] = p

			serverComponents.add( p )

			# Since some processes may register more than one interface
			# (i.e. LoginIntInterface and LoginInterface) we make sure we
			# don't add any process twice
			if p not in self.procs:

				# Since we always clear self.procs, we always have to re-append
				self.procs.add( p )

				# Mark this process as non-stale
				newProcs.add( p )

		# Build lists of stale objects
		machinesToRemove = []
		procsToRemove = []

		for m in self.getMachinesIter():
			if m not in newMachines:
				machinesToRemove.append( m )

			for p in m.getProcsIter():
				if p not in newProcs:
					procsToRemove.append( (m, p) )

		# Clear stale objects
		for m, p in procsToRemove:
			del m.procs[ p.pid ]

		for m in machinesToRemove:
			del self.machines[ m.ip ]

		# Re-initialise existing User objects
		for uid, user in self.users.items():
			try:
				user.__init__( uid, self )
			except User.error, errMsg:
				# Could not resolve user, remove it.
				del self.users[ uid ]
				log.error( errMsg )		

		# Warn about out-of-date machined's
		for m in self.getMachinesIter():
			if m.machinedVersion < \
				   messages.MachineGuardMessage.MACHINED_VERSION:
				log.verbose( "Out-of-date machined on %s (v%d)",
							 m.name, m.machinedVersion )

		# We made it!
		return True

	# Send a log message to all message logger instances in a cluster
	def log( self, text, user ):
		for logger in self.getProcs( "message_logger" ):
			try:
				logger.sendMessage( text, user )
			except:
				log.error( "Failed to log message to %s" % logger )


	def getProcs( self, name = None, uid = None ):
		"""Returns all procs in the cluster with the given name, under the given
		   UID"""

		ps = self.procs
		if name != None:
			ps = filter( lambda p: p.name == name, ps )
		if uid != None:
			ps = filter( lambda p: p.uid == uid, ps )

		return ps

	def getProcsIter( self, name = None, uid = None ):
		"""Returns an iterator to all procs in the cluster with the given
		   name, under the given UID"""

		def procFilter( p ):
			return (not name or p.name == name) and \
				(not uid or p.uid == uid)

		return itertools.ifilter( procFilter, self.procs )

	def getProc( self, name, uid = None ):
		"""Returns first process matching the given name, or None on failure."""

		for p in self.procs:
			if p.name == name and \
			   (uid is None or p.uid == uid):
				return p

		return None


	def getUser( self, uid = None, machine = None,
				 checkCoreDumps = False, refreshEnv = False ):
		"""
		Returns the User object for the given uid or username, or yourself if
		none given.
		"""

		if uid is None or uid == "":
			uid = uidmodule.getuid()

		# An actual call to machined is made if any flags are set or we don't
		# know anything about this user yet.
		if checkCoreDumps or refreshEnv or \
		   isinstance( uid, messages.UserMessage ) or \
		   (type( uid ) == int and uid not in self.users) or \
		   (isinstance( uid, types.StringTypes ) and \
			uid not in self.users.itervalues()):

			user = User( uid, self, machine, checkCoreDumps, refreshEnv )
			self.users[ user.uid ] = user

		# UID lookup
		if type( uid ) == int:
			return self.users[ uid ]

		# Name lookup
		for user in self.users.itervalues():
			if user.name == uid:
				return user

		return None


	def getUsers( self ):
		"""
		Returns a list of all users who are running at least one process, plus
		any user that has already been manually fetched with getUser().
		"""

		# Make objects for currently unknown users that are running processes
		for p in self.procs:
			if p.uid not in self.users:
				self.getUser( p.uid )

		return self.users.values()


	def getAllUsers( self, machines = [] ):
		"""
		Hits every machined on the network for its entire User mapping and
		returns the union of said mappings.  Note that this doesn't actually
		cause disk access of ~/.bwmachined.conf on every machined, they only
		return user objects that were already in memory.
		"""

		mgm = messages.UserMessage()
		replies = messages.MachineGuardMessage.batchQuery(
			[mgm], 1.0, machines )[ mgm ]

		for reply, _ in replies:
			self.getUser( reply )

		return self.users.values()


	def getMachine( self, ip ):
		"""Returns the Machine for the supplied ip or hostname."""

		if self.machines.has_key( ip ):
			return self.machines[ ip ]

		for m in self.machines.values():
			if m.name == ip:
				return m

		return None

	def getMachines( self ): return self.machines.values()

	def getMachinesIter( self ): return self.machines.itervalues()

	# You can raise this inside Cluster.waitFor() to cause it to fail early
	class TerminateEarlyException( Exception ):
		def __init__( self, *args, **kw ):
			Exception.__init__( self, *args, **kw )

	def waitFor( self, testmethod, sleep, maxretries=0, callback=None ):
		"""
		Waits for testmethod() to return true, in sleep intervals of 'sleep',
		with an optional maximum of 'maxretries' calls to sleep().  If
		'callback' is given, it will be executed on each iteration of the loop.
		"""

		try:
			retries = 0
			while not testmethod() and \
					  (retries < maxretries or not maxretries):
				if callback: callback()
				time.sleep( sleep )
				self.refresh()
				retries += 1

			return (not maxretries) or retries < maxretries

		except self.TerminateEarlyException:
			return False

	def queryTags( self, cat = None, machines = [] ):
		"""
		Query a tag category on each machine, or all machines in the cluster if
		'machines' is None.  Will write the tags to the Machines' tags
		dictionaries.

		If cat is None, then all tags will be fetched from each machine
		"""

		# Special little block for querying all tags on machine set
		if cat is None:

			# Get all category names for all machines
			self.queryTags( "" )

			# Find the set of unique categories
			categories = set()
			for m in self.getMachines():
				for c in m.tags.keys():
					categories.add( c )

			# Do a broadcast query on each category found
			for c in categories:
				self.queryTags( c )
			return

		# Do the request
		mgm = messages.TagsMessage()
		mgm.tags = [cat]
		for mgm, (srcip, srcport) in \
			messages.MachineGuardMessage.batchQuery( [mgm], 1, machines )[mgm]:

			if not self.machines.has_key( srcip ):
				log.error( "Reply from machine at unknown address %s" %
						   srcip )
				continue
			else:
				m = self.machines[ srcip ]

			if mgm.exists:
				if cat:
					m.tags[ cat ] = mgm.tags
				else:
					for t in mgm.tags:
						m.tags[ t ] = []

	def getGroups( self ):

		self.queryTags( "Groups" )
		groups = {}
		for m in self.getMachines():
			if m.tags.has_key( "Groups" ):
				for t in m.tags[ "Groups" ]:
					if not groups.has_key( t ):
						groups[ t ] = [ m ]
					else:
						groups[ t ].append( m )
		return groups


	@staticmethod
	def runscript( procs, script = None, lockCells = False, outputs = None,
				   prefix = True ):

		if not procs:
			return False

		# If we are talking to cellapps, lock cells
		if lockCells:
			log.info( "Locking cells ..." )
			procs[0].user().getProc( "cellappmgr" ).shouldOffload( False )
			time.sleep( 0.5 )

		# Make sure script is terminated with a newline
		if script and not script.endswith( "\n" ):
			script += "\n"

		try:

			# If we are interactively connecting to a single app, use the real telnet
			# program so that non line-based input (i.e. 'up arrow') works
			if len( procs ) == 1 and not script:
				p = procs[0]
				watcherPort = p.getWatcherValue( "pythonServerPort" )
				if watcherPort == None:
					log.error( "Unable to retrieve python port from watcher tree. This "
						"can indicate incompatible watcher protocols (ie: 1.9 tools "
						"talking to 1.8 servers)." )
					# inverting return value as the os.system does below
					return 0

				# Everything seems ok, so lets continue
				port = int( watcherPort )
				return not os.system( "telnet %s %s" % (p.machine.ip, port) )

			conns = {}
			for p in procs:
				watcherPort = p.getWatcherValue( "pythonServerPort" )
				if watcherPort == None:
					log.error( "Unable to retrieve python port from watcher tree. This "
						"can indicate incompatible watcher protocols (ie: 1.9 tools "
						"talking to 1.8 servers)." )
					# inverting return value as the os.system does above
					return 0

				# Everything seems ok, so lets continue
				port = int( watcherPort )
				conns[ telnetlib.Telnet( p.machine.ip, port ) ] = p

			if script: script = StringIO( script )

			if outputs:
				assert len(procs) == len(outputs), \
					"Must have one file-like object per process"
				outputDict = dict( zip( procs, outputs ) )
			else:
				outputDict = None

			# Add prefix to output if we're using command line input
			addPrefix = outputDict == None and len( procs ) > 1

			# Macro to read server output
			def slurp( conn, line = "", silent = False, output = sys.stdout ):

				index, match, s = conn.expect( ["\n>>> ", "\n\.\.\. "] )
				more = (index == 1)

				if silent:
					return more

				s = s.replace( line, "", 1 )
				s = s[:-4]

				if not s:
					return more

				if addPrefix and prefix:
					s = "%-12s%s" % (conns[ conn ].label() + ":", s)

				output.write( s )
				output.flush()
				return more

			# Flush initial prompts
			for conn in conns:
				slurp( conn, silent = len( procs ) > 1 or script != None )

			more = False
			connOrder = sorted( conns.items(), key = lambda (c,p): p )

			# Macro for sending code and reading the response
			def sendAndRecv( line ):

				if outputDict:
					output = outputDict[ p ]
				else:
					output = sys.stdout

				more = False

				for conn, p in connOrder:
					conn.write( line )
					more = slurp( conn, line, output = output ) or more

				return more

			# Interact
			while True:
				if script:
					line = script.readline()
				else:
					try:
						# logical expression, used like a ternary operator
						line = raw_input( (more and "... ") or ">>> " ) + '\r\n'
					except EOFError:
						line = ""
					except KeyboardInterrupt:
						line = ""

				line = re.sub( "\r?\n$", "\r\n", line )
				line = re.sub( "\t", "    ", line )

				# Line is empty only when a the ScriptIO object has reached
				# the end of the string, or when Ctrl-D is pressed.
				if line:
					more = sendAndRecv( line )
				else:
					break


			# Flush final part of output if the script left the interpreter wanting
			# a final newline (i.e. prompt is stuck on "... ")
			if more:
				sendAndRecv( "\r\n" )

		# If we are talking to cellapps, unlock cells
		finally:

			if lockCells:
				procs[0].user().getProc( "cellappmgr" ).shouldOffload( True )
				if script == sys.stdin:
					log.info( "" )
				log.info( "Unlocked cells" )

		return True

	@staticmethod
	def checkRing():
		"""
		Verifies the correctness of the buddy ring on the network.  This is a
		static method because we need to avoid going via refresh() (since that
		is essentially what we are checking).
		"""

		packet = messages.MGMPacket()
		packet.append( messages.WholeMachineMessage() )

		sock = socketplus.socket( "bm" )
		sock.sendto( packet.get(),
					 ("<broadcast>", messages.MachineGuardMessage.MACHINED_PORT) )

		dupeReplies = False

		# Mapping of {ip -> (Machine, buddy ip address)}
		machines = {}

		for data, (srcip, _) in socketplus.recvAll( sock, 1.0 ):
			packet.read( memory_stream.MemoryStream( data ) )

			if srcip not in machines:
				machines[ srcip ] = \
						  (Machine( None, packet.messages[0], srcip ),
						   socket.inet_ntoa( struct.pack( "I", packet.buddy ) ))
			else:
				log.error( "Duplicate reply from %s", srcip )
				dupeReplies = True


		# We deliberately
		ring = sorted( machines.keys(),
					   key = lambda ip: \
					   struct.unpack( "I", socket.inet_aton( ip ) )[0] )

		for ip in ring:

			m, buddyip = machines[ ip ]

			try:
				buddy = machines[ buddyip ][0]
			except KeyError:
				log.info( "%-15s %-12s -> %-15s (non-existent)",
						  m.ip, m.name, buddyip )
				continue

			correctbuddy = machines[
				ring[ (ring.index( ip ) + 1) % len( ring ) ] ][0]

			if buddy == correctbuddy:
				log.info( "%-15s %-12s -> %-15s %-12s",
						  m.ip, m.name, buddy.ip, buddy.name )
			else:
				log.info( "%-15s %-12s -> %-15s %-12s should be %s (%s)",
						   m.ip, m.name, buddy.ip, buddy.name,
						   correctbuddy.ip, correctbuddy.name )
				ok = False

		log.info( "" )

		if dupeReplies:
			log.error( "Some machines on your network are sending duplicate "
					   "replies.  This can indicate that you have dual-NIC "
					   "machines with both interfaces connected to "
					   "the same subnet.\n" )

		if machines:
			curr = machines.keys()[0]
			while machines:

				# TODO fix this
				if curr not in machines:
					log.error( "Ring broken by %s", curr )
					for addr in machines:
						log.error( "%s isn't anyone's buddy", addr )
					return False

				next = machines[ curr ][1]
				del machines[ curr ]
				curr = next

			if not machines:
				log.info( "* Ring is complete *" )

		return True
