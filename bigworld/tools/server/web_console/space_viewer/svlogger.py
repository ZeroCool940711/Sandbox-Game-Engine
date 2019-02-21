#!/usr/bin/env python

import sys
import os
import time
import copy
import optparse
import cPickle as pickle
import threading
import atexit
import struct
import select
import zlib
import signal
import socket
import re
import tempfile

from cell_app_mgr_talker import CellAppMgrTalker
from cell_talker import CellTalker
from log import Log

import bwsetup; bwsetup.addPath( ".." )
from pycommon import uid as uidmodule
from pycommon import cluster
from pycommon import util

# Environ stuff
FULLPATH = os.path.abspath( __file__ )

DEFAULT_LOGDIR = tempfile.gettempdir() + "/svlog-bwtools"
DEFAULT_PREFIX = "svlog"

VERBOSE = True

def vPrint( s ):
	if VERBOSE:
		print s

# A flag that any thread can set to cause the entire process to shut down
gShutdown = False

def main():

	global VERBOSE

	# Parse cmdline args
	opt = optparse.OptionParser()
	opt.add_option( "-u", dest = "uid", default = "bwtools",
					help = "Specify the UID you wish to log" )
	opt.add_option( "-s", dest = "space", type = "int", default=1,
					help = "Specify the space you wish to log" )
	opt.add_option( "-o", dest = "logdir", default = DEFAULT_LOGDIR,
					help = "Directory to write the log to" )
	opt.add_option( "-p", "--prefix", dest = "prefix", default = DEFAULT_PREFIX,
					help = "Prefix for files in logdir" )
	opt.add_option( "-r", "--replay", dest = "replay",
					help = "Path to CMTL db file to replay" )
	opt.add_option( "-l", dest = "listenaddr", default=None,
					help = "specify port or ip:port to listen on for viewers" )
	opt.add_option( "-w", "--writeaddr", dest = "writeAddr",
					help = "Write the listener address to a file" )
	opt.add_option( "-k", "--keepalive", dest = "keepAlive",
					action = "store_true",
					help = "Logger will terminate if all clients disconnect" )
	opt.add_option( "-q", "--quiet", dest = "quiet", action = "store_true",
					help = "Suppress non-error output" )
	opt.add_option( "--cell-int", dest = "cellInt", type = "float",
					help = "Specify cell sample interval" )
	opt.add_option( "--mgr-int", dest = "mgrInt", type = "float",
					help = "Specify cellappmgr sample interval" )
	(options, args) = opt.parse_args()

	# Convert supplied username to UID
	options.uid = uidmodule.getuid( options.uid )

	# Disable STDOUT if quiet mode is on
	if options.quiet:
		VERBOSE = False

	online = not bool( options.replay )
	if online:
		cmtl, ctl = initOnline( options )
	else:
		cmtl, ctl = initOffline( options )

	# If we didn't get proper talker objects back, exit
	if not cmtl or not ctl:
		if options.writeAddr:
			open( options.writeAddr, "w" ).write( "none:0" )
		return 1

	# The address we listen on for viewer connections
	if options.listenaddr:
		if ":" in options.listenaddr:
			(ip, port) = options.listenaddr.split( ":" )
		else:
			(ip, port) = (socket.gethostname(), options.listenaddr)
		port = int( port )
		listenaddr = (ip, port)
	else:
		listenaddr = None

	# Listen for and serve viewer connections
	try:
		listener = SVListener( cmtl, ctl, listenaddr,
							   options.keepAlive, online )

		# Write listener address to file if required
		if options.writeAddr:
			open( options.writeAddr, "w" ).write( "%s:%d" % listener.addr )

		listener.run()

	except KeyboardInterrupt:
		pass

	if online:

		# Shutdown TalkerLoggers and wait for threads to terminate
		vPrint( "Shutting down logger threads" )
		ctl.shutdown(); ctl.join()
		cmtl.shutdown(); cmtl.join()

		# Terminate logs
		ctl.log.terminate()
		cmtl.log.terminate()

def initOnline( options ):
	"""
	Performs init when we're writing a log from a live server.
	"""

	# Find cellappmgr viewer server port
	me = cluster.Cluster().getUser( options.uid )
	mgr = me.getProc( "cellappmgr" )
	if not mgr:
		print "Couldn't find cellappmgr, bailing out!"
		return (None, None)

	port = int( mgr.getWatcherValue( "viewer server port" ) )
	vieweraddr = (mgr.machine.ip, port)

	# Create cellappmgr talker object and get the cells so we can find out the
	# space bounds
	cmt = CellAppMgrTalker( options.space, vieweraddr )
	cmt.getCells()
	if not cmt.cells:
		print "No cells found!"
		return (None, None)

	# Create cell talker object to get spaceBounds
	ct = CellTalker( options.space, cmt.cells.values()[0] )
	ct.getGrid()

	# Create logger threads (but don't start em yet)
	cmtl = CellAppMgrTalkerLogger( cmt, options.logdir,
								   options.prefix, "rw", options.mgrInt )
	ctl = CellTalkerLogger( None, options.logdir,
							options.prefix, "rw", options.cellInt )

	# Write space ID and bounds to the log database
	cmtl.log.dbSet( "spaceID", options.space )
	cmtl.log.dbSet( "spaceBounds", ct.spaceBounds )

	# Write space geometry mappings for this space
	# TODO: space geometry mappings rarely changes, but it can change. So
	# need to poll and put into log entry somehow.
	spaceGeometryMappings = []
	if (cmt.getVersion() > 0):
		spaceGeometryMappings = cmt.getSpaceGeometryMappings();
	cmtl.log.dbSet( "spaceGeometryMappings", spaceGeometryMappings );

	# Start logger threads and wait for them to be ready
	cmtl.start(); cmtl.readySem.acquire();
	ctl.start(); ctl.readySem.acquire();

	vPrint( "Logging %s's space %d from %s:%d to %s/%s.*" % \
			(me.name, options.space, vieweraddr[0], vieweraddr[1],
			 cmtl.log.dir, cmtl.log.prefix.replace( ".cmt", "" )) )

	return (cmtl, ctl)

def initOffline( options ):
	"""
	Performs init when we're reading a log.
	"""

	# Work out logdir and prefixes for the CMTL and CTL objects
	logdir = os.path.dirname( options.replay )
	prefix = os.path.basename( options.replay )

	# Make LoggerTalkers (which are never run())
	cmtl = CellAppMgrTalkerLogger( None, logdir, prefix, "r" )
	ctl = CellTalkerLogger( None, logdir, prefix, "r" )

	return (cmtl, ctl)

class TalkerLogger( threading.Thread ):
	"""
	Class to provide basic CMT/CT logging.  Basically handles all the common log
	creation, looping, and locking behaviour.  Subclasses just need to implement
	the update() method to actually fetch the samples from the server and write
	them to the log.
	"""

	def __init__( self, talker, dir, prefix, mode, interval = None ):

		threading.Thread.__init__( self )
		self.talker = talker
		self.log = Log( dir, prefix, mode, interval=interval )
		self.lock = threading.Semaphore() # TODO: why doesn't RLock work?!
		self.lock.acquire()
		self._shutdown = False

		# Semaphore to indicate that at least one sample has been written
		self.readySem = threading.Semaphore( 0 )
		self._readySignalled = False

		# Verify prefix
		cleanprefix = re.sub( "\.(cmt|ct)", "", prefix )
		if "." in cleanprefix:
			raise RuntimeError, \
				  "ERROR: You cannot specify a prefix with a '.' in	it;", \
				  "this is a reserved character for Space Viewer logs"

	def run( self ):

		while not self._shutdown:

			# Mark the start of the loop
			iterTime = time.time()

			# Update as necessary
			self.update()

			# Signal that this log is ready to be read if not done already
			if not self._readySignalled:
				self.readySem.release()
				self._readySignalled = True

			# Sleep out the log interval and release the lock while sleeping
			self.lock.release()
			looptime = time.time() - iterTime
			sleeptime = max( self.log.getCurrentInterval() - looptime, 0 )

			if sleeptime == 0:
				newint = self.log.getCurrentInterval() * 1.1
				print "WARNING: %s is falling behind; bumping interval to %f" %\
					  (self.__class__.__name__, newint)
				self.log.setCurrentInterval( newint )

			time.sleep( sleeptime )
			self.lock.acquire()

	def update( self ):
		raise RuntimeError, "You shouldn't instantiate TalkerLogger"

	def shutdown( self ):
		self._shutdown = True

class CellAppMgrTalkerLogger( TalkerLogger ):

	def __init__( self, talker, dir, prefix, mode, interval = None ):
		TalkerLogger.__init__( self, talker, dir,
							   prefix + ".cmt", mode, interval )

	def update( self ):

		global gShutdown

		try:
			# Fetch new cmt info and write it to the log
			cmtStream = self.talker.getCells()
			self.log.addSample( cmtStream )

		except socket.error:
			vPrint( "CellAppMgr has died, terminating ..." )
			self.shutdown()
			gShutdown = True

class CellTalkerLogger( TalkerLogger ):

	def __init__( self, talker, dir, prefix, mode, interval = None ):
		TalkerLogger.__init__( self, talker, dir,
							   prefix + ".ct", mode, interval )

	def update( self ):

		# Get new cell info
		try:
			if self.talker: self.talker.update()
		except:
			self.talker = None

		if self.talker:
			# Get streams out of StringIOs
			# TODO: optimise this to only update streams if they've changed
			# from the previous sample
			ctStreams = {}
			for k, (stream, t) in self.talker.streamBufs.items():
				ctStreams[ k ] = (stream.getvalue(), t)

			# Write sample to log
			self.log.addSample( (self.talker.addr, ctStreams) )

		# Enter dummy value if nothing being sampled
		else:
			self.log.addSample( (None, None) )


# Class to allow listening to an svlogger remotely
class SVListener( object ):

	# Message IDs (these are intentionally kept out of the ASCII range so they
	# don't collide with the message IDs defined in CellAppMgrTalker)
	(GET_STATIC_INFO,
	 GET_SAMPLES,
	 SELECT_CELL,
	 SET_INTERVALS,
	 EXIT) = \
	 range( 5 )

	def __init__( self, cmtl, ctl, addr = None,
				  keepAlive = False, online = True ):
		"""
		Creates a listener connected to two TalkerLoggers that listens on the
		provided address.  If keepAlive is True, the listener will terminate if
		all the viewer sockets disconnect.  If online is False, any request from
		a viewer that is a 'write' operation will return an error on the stream.
		"""

		# The TalkerLogger objects
		self.cmtl = cmtl
		self.ctl = ctl

		# If no address is specified, just bind to any port
		if not addr:
			addr = (socket.gethostname(), 0)

		# Whether or not to terminate if all viewers disconnect
		self.keepAlive = keepAlive

		# Whether or not we're talking to a live server
		self.online = online

		# Bind to provided address
		try:
			try:
				self.sock = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
				self.sock.bind( addr )
			except:
				vPrint( "%s:%d taken, binding to any available port on %s" % \
						(addr[0], addr[1], addr[0]) )
				self.sock.bind( (addr[0], 0) )
		except:
			vPrint( "Could not bind to " + addr[0] + ". Binding to all interfaces." )
			self.sock.bind( ("", 0) )

		self.sock.listen( 5 )
		self.addr = self.sock.getsockname()
		vPrint( "Listening for viewer connections on %s:%d" % self.addr )

	def send( self, object, sock ):
		"""Pickles object and sends it to the provided address."""

		s = pickle.dumps( object, pickle.HIGHEST_PROTOCOL )

		# TODO: enable compression when logger and viewer are not on the same
		# host
		#print "Message would have been compressed to %f%%" % \
		#	  (float( len( zlib.compress( s ) ) ) / len( s ))

		sock.send( struct.pack( "I", len( s ) ) )
		sock.send( s )

	def run( self ):

		global gShutdown

		# A pool of sockets from clients.  The messages on these sockets will be
		# from the protocol outlined at the top of this class.
		clientsocks = []
		vPrint( "Hit CTRL+C to exit" )

		# Wait for requests from clients
		while not gShutdown:

			# Listen on the server socket and all client sockets.  We have a
			# timeout here because we want to be able to catch one of the logger
			# threads setting gShutdown to true
			(i, o, e) = select.select( [self.sock] + clientsocks, [], [], 1.0 )
			if not i: continue
			s = i[0]

			# If we get something on the server socket, it's a new connection
			if s == self.sock:
				(cs, srcaddr) = self.sock.accept()
				clientsocks.append( cs )
				continue

			# Client sockets
			try:
				data = s.recv( 1 )
			except socket.error:
				print "Client at %s:%d has disconnected unexpectedly" % \
					  s.getsockname()
				clientsocks.remove( s )
				if self.keepAlive and not clientsocks:
					vPrint( "All clients disconnected - logger shutting down" )
					gShutdown = True
				continue

			# If data length is 0, the client socket has been closed, so
			# remove it from the pool.
			if len( data ) == 0:
				clientsocks.remove( s )
				if self.keepAlive and not clientsocks:
					vPrint( "All clients disconnected - logger shutting down" )
					gShutdown = True
				continue

			msg = struct.unpack( "B", data )[0]

			# This is a viewer window telling us to shutdown
			if msg == self.EXIT:
				gShutdown = True
				continue

			# The static info is the online status, space ID, space bounds,
			# first timestamp, and current log interval
			elif msg == self.GET_STATIC_INFO:
				_, firstTime, _ = self.cmtl.log.getFirstSample()
				self.send( (self.online,
							self.cmtl.log.dbGet( "spaceID" ),
							self.cmtl.log.dbGet( "spaceBounds" ),
							self.cmtl.log.dbGet( "spaceGeometryMappings" ),
							firstTime,
							self.cmtl.log.getCurrentInterval(),
							self.ctl.log.getCurrentInterval()), s )

			# Requests for CMT and CT samples
			elif msg == self.GET_SAMPLES:

				# Read timestamp and ct timestamps flag from the stream
				reqTime, = struct.unpack( "=d", s.recv( 8 ) )

				# Read the CT stream timestamps
				ctTimes = [
					struct.unpack( "d", data )[0] for data in
					[ s.recv( 8 ) for i in
					  xrange( len( CellTalker.UPDATE_METHODS.keys() ) ) ] ]

				# Fetch log samples
				if reqTime == Log.FIRST:
					cmtSample = self.cmtl.log.getFirstSample()
					ctSample = self.ctl.log.getFirstSample()
				elif reqTime == Log.LAST:
					cmtSample = self.cmtl.log.getLastSample()
					ctSample = self.ctl.log.getLastSample()
				else:
					cmtSample = self.cmtl.log.getSample( reqTime )
					ctSample = self.ctl.log.getSample( reqTime )

				# Sort out flags, timestamps and data streams
				flags = cmtSample[0] | ctSample[0]
				t = min( cmtSample[1] , ctSample[1] )
				cmtStream = cmtSample[2]
				cellAddr, ctStreams = ctSample[2]

				# Make a hash of the updated CT streams
				updatedStreams = {}
				if ctStreams:
					keys = util.sort( ctStreams.keys() )
					for i in xrange( len ( keys ) ):
						if ctTimes[i] != ctStreams[ keys[i] ][1]:
							updatedStreams[ keys[i] ] = ctStreams[ keys[i] ]

				self.send( (flags, t, (cmtStream, cellAddr, updatedStreams)),
						   s )

			# Requests for a different cell (we need the lock for this)
			elif msg == self.SELECT_CELL:

				try:
					self.ctl.lock.acquire()

					# Read the address of the cellapp off the stream and find
					# out which cellapp it corresponds to
					ip, port = struct.unpack( "IH", s.recv( 6 ) )

					# If the address is the same as the cell we're already
					# logging, deselect it
					if self.ctl.talker and self.ctl.talker.addr == (ip, port):
						self.ctl.talker = None

					# If not, find out which cellapp this corresponds to and
					# re-assign the global cell talker object
					else:
						self.cmtl.lock.acquire()
						cell = self.cmtl.talker.cells[ (ip, port) ]
						self.cmtl.lock.release()
						self.ctl.talker = CellTalker(
							self.cmtl.log.dbGet( "spaceID" ), cell )

				finally:
					self.ctl.lock.release()

			# Set the sample intervals for the loggers
			elif msg == self.SET_INTERVALS:

				# Format is cmtInterval, ctInterval
				cmtInterval, ctInterval = struct.unpack( "ff", s.recv( 8 ) )

				# Alter intervals as necessary
				for i, tl in ( (cmtInterval, self.cmtl),
							  (ctInterval, self.ctl) ):
					tl.lock.acquire()
					if i != tl.log.getCurrentInterval():
						tl.log.setCurrentInterval( i )
					tl.lock.release()

				# Send the new intervals back to all viewers
				for sock in clientsocks:
					sock.send( struct.pack( "=Bff", self.SET_INTERVALS,
											cmtInterval, ctInterval ) )

			# These are the msg IDs from CellAppMgrTalker that we forward
			# through to the cellappmgr
			elif msg == ord( CellAppMgrTalker.REMOVE_CELL ):

				# Reconstruct message and forward it to the cellappmgr by
				# sneakily hijacking the CMTs socket
				fwdmsg = chr( msg ) + s.recv( 12 )
				self.cmtl.talker.s.send( fwdmsg )

			elif msg == ord( CellAppMgrTalker.STOP_CELL_APP ):

				# Same as above
				fwdmsg = chr( msg ) + s.recv( 8 )
				self.cmtl.talker.s.send( fwdmsg )

			else:
				print "ERROR: Unrecognised message id: %d" % msg

if __name__ == "__main__":
	main()
