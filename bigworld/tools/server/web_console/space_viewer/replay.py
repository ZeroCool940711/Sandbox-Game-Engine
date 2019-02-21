import os
import sys
import socket
import zlib
import pickle
import struct
import select
import thread

from StringIO import StringIO

from svlogger import SVListener
from cell_app_mgr_talker import CellAppMgrTalker
from cell_talker import CellTalker
from log import Log

import bwsetup; bwsetup.addPath( "../.." )
from pycommon import util

class Replayer( object ):
	"""
	Client-side of the log replaying protocol.
	"""

	REQUEST_TIMEOUT = 5
	RCV_BUF = 1024 * 1024

	def __init__( self, addr ):
		"""
		Create a replayer connected to the logger at the specified address.
		"""
		self.lock=thread.allocate_lock()

		# Flag saying whether or not we're in realtime
		self.realtime = False

		# The timestamp of the sample currently displayed
		self.dispTime = 0

		# The most recently _requested_ timestamp from the logger
		self.reqTime = None

		# The current intervals for the loggers
		self.cmtInterval = self.ctInterval = None

		# Connect to server
		self.addr = addr
		self.sock = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
		self.sock.setsockopt( socket.SOL_SOCKET, socket.SO_RCVBUF,
							  16*1024*1024 )
		self.sock.settimeout( self.REQUEST_TIMEOUT )
		self.sock.connect( self.addr )

		# Get static info
		try:
			(self.online, self.space, self.spaceBounds,
			 self.spaceGeometryMappings, self.minTime,
			 self.cmtInterval, self.ctInterval) = self._getStaticInfo()
		except:
			print "svlogger daemon didn't respond in time, bailing out ..."
			sys.exit( 1 )

		# Create CMT object and maybe CT object
		self.cmt = CellAppMgrTalker( self.space )
		self.ct = None

		# Get most recent sample
		self._getSample( Log.LAST )

		# The interval at which the client window wants to display
		self.displayInterval = min( self.cmtInterval, self.ctInterval )

	def seek( self, n ):
		"""
		Move to a point in the log at an offset of n seconds from the sample
		currently being viewed.
		"""

		# If we're going backwards, switch off realtime and don't allow
		# _getSample to update that flag.  The reason for all this
		# updateRealTime hackery is that it's possible that fetching the sample
		# at a particular offset from self.dispTime may actually return the
		# sample _at_ self.dispTime, and so if this seek interval is constant
		# (i.e. bound to the arrow keys in space_view_window) you may not be
		# able to seek away from a particular time.
		if n < 0:
			self.realtime = False
			updateRealTime = False
		else:
			updateRealTime = True

		# Fetch the sample from the logger
		self.reqTime = max( self.reqTime + n, self.minTime )
		self._getSample( self.reqTime, updateRealTime )

	def update( self ):
		"""
		Called from space_view_window - will request a new sample from the
		logger if we are in realtime mode (i.e. the last sample we viewed was
		the most recent one from the log.
		"""
		self.lock.acquire()
		try:
                        
                        if self.realtime and self.online:

                                # Check the socket for pending server messages
                                if select.select( [self.sock], [], [], 0.0 )[0]:

                                        # We might receive 0 bytes here indicating the socket has been
                                        # disconnected at the other end
                                        data = self.sock.recv( 1 )
                                        if not data:
                                                raise socket.error
                                        else:
                                                msg = struct.unpack( "B", data )

                                        # Handle interval update messages
                                        if msg == SVListener.SET_INTERVALS:
                                                self.cmtInterval, self.ctInterval = \
                                                                                  struct.unpack( "ff", self.sock.recv( 8 ) )
                                                self.displayInterval = max( self.displayInterval,
                                                                                                        min( self.cmtInterval,
                                                                                                                 self.ctInterval ) )
                                                print "New sample intervals are %f/%f" % (self.cmtInterval,
                                                                                                                                  self.ctInterval)

                                # Get new samples from logger
                                self._getSample( Log.LAST )
                                self.lock.release()
					
                except Exception:
                        self.lock.release()
                        

	def selectCell( self, addr ):
		"""
		Send a message to the logger to tell it to start logging the cell at the
		specified address.
		"""
		self.lock.acquire()
		try:                                

                        if not self.online:
                                wx.MessageBox( "Can't select cells when replaying a log!", "Error",
                                                          wx.ICON_ERROR )
                        elif self.realtime:
                                msg = struct.pack( "=BIH", SVListener.SELECT_CELL,
                                                                   addr[0], addr[1] )
                                self.sock.send( msg )
                                self.lock.release()
                                self.update()
                        else:
                                wx.MessageBox( "Can't select cells when viewing history!", "Error",
                                                          wx.ICON_ERROR )
                        #self.lock.release()
                        
                except Exception:
                        self.lock.release()

	def deleteCell( self, cell ):
		"""
		Delete a cell.  Light overload of the same method in CellAppMgrTalker.
		"""
		if not self.online:
			wx.MessageBox( "Can't retire cells when replaying a log!", "Error",
						  wx.ICON_ERROR )
		else:
			self.cmt.deleteCell( cell, self.sock )

	def deleteCellApp( self, cell ):
		"""
		Delete a cellapp.
		"""
		if not self.online:
			wx.MessageBox( "Can't retire cellapps when replaying a log!",
						  "Error", wx.ICON_ERROR )
		else:
			self.cmt.deleteCellApp( cell, self.sock )

	def stopLogger( self ):
		"""
		Tells the logger to shutdown.
		"""
		self.lock.acquire()
		try:
                        
                        msg = struct.pack( "B", SVListener.EXIT )
                        self.sock.send( msg )
                        self.realtime = False
                        self.lock.release()
                except Exception:
                        self.lock.release()

	def setIntervals( self ):

		msg = struct.pack( "=Bff", SVListener.SET_INTERVALS,
						   self.cmtInterval, self.ctInterval )
		self.sock.send( msg )

	# -------------------------------------------------------------------------
	# Section: Private Methods
	# -------------------------------------------------------------------------

	def _getSample( self, time, updateRealTime = True ):
		"""
		Request the sample for a specific time from the logger.  If
		updateRealTime is False, then this method will not modify the realtime
		flag for this replayer.
		"""

		# The message format is:
		# header
		# time we're requesting the samples from
		# single byte indicating whether or not timestamps follow
		# (maybe) list of timestamps of CT streambufs, sorted by key
		msg = struct.pack( "=Bd", SVListener.GET_SAMPLES, time )

		# If we have a CellTalker then send its timestamps
		if isinstance( self.ct, CellTalker ):
			for k in util.sort( self.ct.streamBufs.keys() ):
				msg += struct.pack( "d", self.ct.streamBufs[ k ][1] )

		# Otherwise send bogus values that will cause an update from the logger
		else:
			for i in xrange( len( CellTalker.UPDATE_METHODS.keys() ) ):
				msg += struct.pack( "d", 0 )

		# Request until it comes back successfully
		bundle = None
		while not bundle:
			bundle = self._request( msg )

		# Are we in realtime?
		oldrt = self.realtime
		if updateRealTime:
			self.realtime = bool( bundle[0] & Log.LAST )

		# Write to internal timestamp and extract streams
		self.dispTime, (cmtStream, cellAddr, updatedStreams) = bundle[1:]

		# If we're at the last sample in the log, don't allow the reqTime to
		# keep going forward
		if bundle[0] & Log.LAST and self.reqTime > self.dispTime:
			self.reqTime = self.dispTime

		# Keep reqTime and dispTime in sync if in realtime or no history
		# requests have been made
		if self.realtime or self.reqTime == None:
			self.reqTime = self.dispTime

		# Reconstruct CMT
		self.cmt.getCells( StringIO( cmtStream ) )

		# Destroy CT object if none sent from server, OR:
		# Possible race condition here - if a cell died between the time its CT
		# sample was taken and the time the CMT sample was taken, the cell will
		# not be in the cmt.cells map, and will cause KeyErrors in the drawing
		# code.
		if not cellAddr or not self.cmt.cells.has_key( cellAddr ):
			self.ct = None

		# Make new CT object if required
		elif not self.ct or self.ct.addr != cellAddr:
			self.ct = CellTalker( self.space, cellAddr )

		# Map in any updated streamBufs, rewind em, and call the relevant
		# methods to update internal state
		if self.ct:
			for key, (s, timestamp) in updatedStreams.items():
				self.ct.streamBufs[ key ] = (StringIO( s ), timestamp)
				CellTalker.UPDATE_METHODS[ key ]( self.ct )

	def _getStaticInfo( self ):
		"""
		Simple request for the static info tuple from the logger.
		"""

		return self._request( struct.pack( "=B", SVListener.GET_STATIC_INFO ) )

	def _request( self, msg ):
		"""
		Send a msg to the server and return the unpickled reply.
		"""

		# If we get a broken pipe here, the logger has probably shut down
		try:
			self.sock.send( msg )
		except socket.error:
			print "Logger has shutdown"
			return None

		try:
			# Read the length header
			nbytes = struct.unpack(
				"I", self.sock.recv( struct.calcsize( "I" ) ) )[0]

			# If 0 bytes on stream, then we tried to do an illegal op
			if nbytes == 0:
				return None

			# Read that many bytes out of the stream
			r = 0; data = "";
			while r < nbytes:
				s = self.sock.recv( min( nbytes - r, self.RCV_BUF ) )
				r += len( s )
				data += s

			# Decompress and unpickle
			#return pickle.loads( zlib.decompress( data ) )
			return pickle.loads( data )

		except socket.timeout:
			print "WARNING: Connection to %s:%d timed out!" % self.addr
			return None
