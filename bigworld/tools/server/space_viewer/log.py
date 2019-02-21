"""
This module defines all the stuff that is used for reading and writing logs.
"""

import sys
import os
import re
import cPickle as pickle
import time
import struct
import anydbm
import threading
import traceback

import bwsetup; bwsetup.addPath( ".." )
from pycommon import util
from pycommon.util import synchronized

# Some useful sizes (to avoid using magic numbers so this will all work when we
# port it to 64-bit architectures :P)
DBL_SIZE = struct.calcsize( "d" )
INT_SIZE = struct.calcsize( "i" )
FLOAT_SIZE = struct.calcsize( "f" )

class Log:
	"""
	Coalesces an entire log together.
	"""

	# Defaults
	DEFAULT_INTERVAL = 1
	DEFAULT_CHUNKSIZE = 100 * (2 ** 20)
	DEFAULT_PREFIX = "log"

	# Flags used to indicate the first and last samples in a log
	FIRST = 1
	LAST = 2

	def __init__( self, dir, prefix = None, mode = "r", **kwargs ):

		# The directory where the logs reside
		self.dir = dir

		# The prefix for all the files in this log
		if not prefix:
			prefix = self.DEFAULT_PREFIX
		self.prefix = prefix

		# The maximum size of a LogChunk
		self.chunkSize = None

		# A list of the LogChunks that comprise this log.  Except for really big
		# logs there will probably only ever be one element in this list
		self.logChunks = []

		# The mode this log is in
		self.mode = mode

		# The number of samples in this log
		self.nSamples = 0

		# The start and end times for this log
		self.starttime = self.endtime = None

		if self.mode == "r":
			self._initForReading( **kwargs )
		else:
			self._initForWriting( **kwargs )


	@synchronized
	def addSample( self, obj, timestamp = None ):
		"""
		Add a sample to the end of this log.  If you want getSample() to be able
		to extract samples from the log in constant time, you must add samples
		to the log every interval as specified during construction or with the
		most recent call to setInterval().

		@param obj        The picklable object to be added to the log.
		@param timestamp  (optional) The timestamp to tag the object with.  If
	                      no timestamp is given, the current system time is
		                  used.
		"""

		# If this log is open for reading, then this is an error
		if self.mode == "r":
			raise RuntimeError, "Can't addSample() to a read-only log!"

		# This is the time that will be written to the log with the pickle
		timestamp = time.time()

		if self.endtime and timestamp <= self.endtime:
			raise RuntimeError, "Added a sample to this log out of sequence"

		# Set markers as necessary
		if not self.starttime:
			self.starttime = timestamp
		self.endtime = timestamp

		# If the last chunk in the list is full, make another one now
		if self.logChunks[-1].getSize() >= self.chunkSize:

			# Terminate the current chunk
			curr = self.logChunks[-1]
			curr.terminate()

			# Reload it in read-only mode
			self.logChunks[-1] = LogChunkR( curr.filename, self )

			# Make new chunk with same interval as the last one
			self._addChunk( curr.getInterval() )

		# Write the sample to the end of the current LogChunk
		self.logChunks[-1].addSample( obj, timestamp )
		self.nSamples += 1

	@synchronized
	def getSample( self, tgttime ):
		"""
		Get the sample at tgttime out of this log, or if there isn't one at that
		exact time, the next one after that time.

		@param tgttime   The time to fetch a sample from
		@return          (flags, timestamp, sample)
		"""

		ret = None
		prevchunk = None
		for chunk in self.logChunks:

			# If tgttime is between the start and end times, or
			# tgttime is before the starttime and this is the first chunk, or
			# tgttime is after the endtime and this is the last chunk
			# - it's gotta be in this chunk!
			if chunk.starttime <= tgttime and tgttime <= chunk.endtime or \
				   tgttime < chunk.starttime and chunk is self.logChunks[0] or \
				   tgttime > chunk.endtime and chunk is self.logChunks[-1]:
				ret = chunk.getSample( tgttime )
				break

			# If the tgttime is before this chunk's starttime then it must lie
			# between the endtime of the previous chunk and the start of this
			# one.  Use whichever one is closer.
			elif tgttime < chunk.starttime:

				if abs( chunk.starttime - tgttime) <= \
				   abs( prevchunk.endtime - tgttime):
					ret = chunk.getSample( tgttime )
					break
				else:
					ret = prevchunk.getSample( tgttime )
					break

			prevchunk = chunk

		# This should only happen if the log is empty
		if not ret:
			return None

		t, sample = ret
		if t == self.starttime:
			flags = Log.FIRST
		elif t == self.endtime:
			flags = Log.LAST
		else:
			flags = 0

		return (flags, t, sample)

	@synchronized
	def getFirstSample( self ):
		"""
		Returns the first sample from this log.
		"""
		if not self.logChunks or self.logChunks[0].getSampleCount() == 0:
			return None
		else:
			t, sample = self.logChunks[0].getFirstSample()
			return (Log.FIRST, t, sample)

	@synchronized
	def getLastSample( self ):
		"""
		Returns the last sample from this log.
		"""
		if not self.logChunks:
			return None
		else:
			# Go backwards from last chunk trying to find a chunk with a sample
			for chunk in self.logChunks[::-1]:
				if chunk.getSampleCount() > 0:
					t, sample = chunk.getLastSample()
					return (Log.LAST, t, sample)
			return None

	@synchronized
	def dbSet( self, name, obj ):
		"""
		Write an object to this log's database under the given name.
		"""
		db = anydbm.open( self._getPath( "db" ), "w" )
		db[ name ] = pickle.dumps( obj, -1 )

	@synchronized
	def dbGet( self, name ):
		"""
		Load an object from this log's database.
		"""
		db = anydbm.open( self._getPath( "db" ) )
		return pickle.loads( db[ name ] )

	@synchronized
	def getCurrentInterval( self ):
		"""
		Returns the interval this log is currently expecting samples to be
		written at.
		"""
		return self.logChunks[-1].getInterval()

	@synchronized
	def setCurrentInterval( self, interval ):
		"""
		Set a new interval for this log to expect samples at.
		"""

		# Terminate current chunk and reload it read-only if it had any samples
		curr = self.logChunks[-1]
		if curr.terminate():
			self.logChunks[-1] = LogChunkR( curr.filename, self )
		else:
			del self.logChunks[-1]

		# Add new chunk with new interval
		self._addChunk( interval )

	@synchronized
	def terminate( self ):

		# If we're in writing mode, terminate the final chunk before cleanup
		if self.mode == "rw":
			self.logChunks[-1].terminate()

	# --------------------------------------------------------------------------
	# Private methods
	# --------------------------------------------------------------------------

	def _initForWriting( self, **kwargs ):
		"""
		Create a new log in self.dir, which must not exist prior to calling this
		method.

		@param interval  The time (in seconds) between samples (optional kwarg)
		@param chunkSize The size (in bytes) of the log chunks (optional kwarg)
		"""

		# If path exists but is not a dir, abort
		if os.path.exists( self.dir ) and not os.path.isdir( self.dir ):
			raise RuntimeError, "Path %s already exists as a non-directory" % \
				  self.dir

		# If the path doesn't exist, make the dir
		if not os.path.exists( self.dir ):
			os.mkdir( self.dir )

		# If there's already a log in our directory with our prefix, then alter
		# our prefix
		if os.path.exists( self._getPath( "db" ) ):
			prefix, kind = self.prefix.split( "." )
			i = 0
			self.prefix = "%s-%03d.%s" % (prefix, i, kind)
			while os.path.exists( self._getPath( "db" ) ):
				i += 1
				self.prefix = "%s-%03d.%s" % (prefix, i, kind)

			# If there are heaps of logs, probably time to warn about it
			if i > 999:
				print "WARNING: There are lots of logs in %s" % self.dir

		# Handle any kwargs
		if not kwargs.has_key( "chunkSize" ) or not kwargs[ "chunkSize" ]:
			self.chunkSize = self.DEFAULT_CHUNKSIZE
		else:
			self.chunkSize = kwargs[ "chunkSize" ]

		if not kwargs.has_key( "interval" ) or not kwargs[ "interval" ]:
			interval = self.DEFAULT_INTERVAL
		else:
			interval = kwargs[ "interval" ]

		# Make first logChunk
		self._addChunk( interval )

		# Initialise database then store chunkSize to it
		anydbm.open( self._getPath( "db" ), "n" )
		self.dbSet( "chunkSize", self.chunkSize)

	def _initForReading( self, **kwargs ):
		"""
		Read an existing log in self.dir
		"""

		# Verify the directory exists
		if not os.path.isdir( self.dir ):
			raise RuntimeError, "Supplied path is not a dir: %s" % self.dir

		# Read chunkSize out of the database
		self.chunkSize = self.dbGet( "chunkSize" )

		# Get a list of the chunk files in this directory with this prefix (in
		# order) and make readable chunks from them
		self.logChunks = []
		for fname in self._getChunkFilenames():
			self.logChunks.append( LogChunkR( fname, self ) )

		# Set up start and end times
		_, self.starttime, _ = self.getFirstSample()
		_, self.endtime, _ = self.getLastSample()

	def _getChunkFilenames( self ):
		"""
		Returns a list of the filenames of the chunk files for this log.
		"""

		files = map( lambda x: "%s/%s" % (self.dir, x),
					 filter( lambda x: re.match( "%s\.\d{3}$" % self.prefix, x ),
							 os.listdir( self.dir ) ) )
		files.sort()
		return files

	def _addChunk( self, interval ):

		if self.mode != "rw":
			raise RuntimeError, "Can't add samples to read-only logs!"

		# Find out what chunk number we're up to
		chunkFiles = self._getChunkFilenames()
		if not chunkFiles:
			chunkNum = 0
		else:
			chunkNum = int(
				re.search( "\d+$", chunkFiles[-1] ).group( 0 ) ) + 1

		# Chunk filenames are log.<chunkNum>
		chunkFile = self._getPath( "%03d" % chunkNum )

		# Create and append the LogChunk object
		self.logChunks.append( LogChunkRW( chunkFile, self, interval ) )

	def _getPath( self, filename ):
		return os.path.join( self.dir, "%s.%s" % (self.prefix, filename) )

class LogChunk:
	"""
	Interface to chunks of logs.
	"""

	def __init__( self, filename, log ):

		# The timestamp of the first/last samples in this chunk
		self.starttime = self.endtime = None

		# How often we'll be writing samples to this chunk
		self.interval = None

		# The file this chunk lives in
		self.filename = filename

		# The total size (in bytes) of this chunk
		self.size = 0

		# A filehandle to the logfile
		self.file = None

		# The log that this chunk is part of
		self.log = log

	def addSample( self, obj, t ):
		raise "You shouldn't have instantiated a LogChunk directly"

	def getSample( self, tgttime ):
		"""
		Returns the sample closest to the tgttime from this chunk.
	    """

		# If we have no samples, abort now
		if not self.starttime:
			return None

		# Work out roughly where the sample should live on disk
		index = util.clamp( int( (tgttime - self.starttime) /
								 self.interval ),
							0, self.getSampleCount() - 1 )

		# Get the timestamp at this offset
		t = self._getTimestampAtOffset( self._getOffset( index ) )

		# Set up direction and limit for search
		if t >= tgttime:
			direction = -1
			stopIndex = 0
		else:
			direction = +1
			stopIndex = self.getSampleCount() - 1

		# Simple linear search, since we should already be very close to the
		# target sample
		while index != stopIndex:
			tnext = self._getTimestampAtOffset(
				self._getOffset( index + direction ) )
			if abs( tnext - tgttime) >= abs( t - tgttime ):
				break
			t = tnext
			index += direction

		# Return the correct sample or None if we couldn't get one
		return (t, self._getSampleAtOffset( self._getOffset( index ) ))

	def getSize( self ):
		return self.size

	def getSampleCount( self ):
		raise "You shouldn't have instantiated a LogChunk directly"

	def getFirstSample( self ):
		"""
		Returns the first sample from this chunk.
		"""

		return (self._getTimestampAtOffset( 0 ), self._getSampleAtOffset( 0 ) )

	def getLastSample( self ):
		"""
		Returns the last sample from this chunk.
		"""

		offset = self._getOffset( self.getSampleCount() - 1 )
		return (self._getTimestampAtOffset( offset ),
				self._getSampleAtOffset( offset ))

	def getSamples( self ):
		"""
		Returns all samples in this chunk.  Should only be used for debugging.
		"""

		samples = []

		for i in xrange( self.getSampleCount() ):
			offset = self._getOffset( i )
			samples.append( (self._getTimestampAtOffset( offset ),
							 self._getSampleAtOffset( offset ) ) )

		return samples

	def getInterval( self ):
		return self.interval

	# --------------------------------------------------------------------------
	# Private methods
	# --------------------------------------------------------------------------

	def _getOffset( self, index ):
		"""
		Returns the offset into the chunk file that the sample at the given
		index can be found at.

		@param index   The index we're trying to find the offset for
		"""
		raise "You shouldn't have instantiated a LogChunk directly"

	def _getTimestampAtOffset( self, offset ):
		"""
		Fetch the timestamp from the given offset in the underlying chunk file.
		"""

		self.file.seek( offset )
		return struct.unpack( "d", self.file.read( DBL_SIZE ) )[0]

	def _getSampleAtOffset( self, offset ):
		"""
		Fetch the sample at the given offset from the underlying chunk file.
		Note that the sample will actually live 8 bytes past the given offset,
		as the first 8 bytes of any logged sample is its timestamp.

		NOTE: This does not reset the file pointer to its original position when
		it's done so you need to do this in the calling context.
		"""

		self.file.seek( offset + DBL_SIZE )
		obj = pickle.load( self.file )
		return obj

class LogChunkR( LogChunk ):
	"""
	A LogChunk that is open for reading only.
	"""

	def __init__( self, filename, log ):
		LogChunk.__init__( self, filename, log )

		# Open up the logfile
		self.file = open( self.filename, "r+b" )

		# Read out the starttime from the first sample
		self.starttime, = struct.unpack( "d", self.file.read( DBL_SIZE ) )

		# Read sample count and interval from end of file
		self.file.seek( -(DBL_SIZE + INT_SIZE), 2 )
		self.sampleCount, self.interval = \
						  struct.unpack( "=Id",
										 self.file.read( DBL_SIZE + INT_SIZE ) )
		self.size = self.file.tell()

		# Remember where the index table starts
		self.indexOffset = self.size - DBL_SIZE - INT_SIZE - \
						   INT_SIZE * self.sampleCount

		# Deduce the endtime
		self.endtime = self._getTimestampAtOffset(
			self._getOffset( self.sampleCount - 1 ) )

	def getSampleCount( self ):
		return self.sampleCount

	def _getOffset( self, index ):

		# Verify index
		if index < 0 or index >= self.sampleCount:
			raise RuntimeError, "Index %d is outside [0,%d)" % \
				  (index, self.sampleCount)

		# Seek to the right point in the index table and read it off
		self.file.seek( self.indexOffset + INT_SIZE * index, 0 )
		return struct.unpack( "I", self.file.read( INT_SIZE ) )[0]

class LogChunkRW( LogChunk ):
	"""
	A LogChunk that is open for reading and writing.
	"""

	def __init__( self, filename, log, interval ):
		LogChunk.__init__( self, filename, log )

		# Check that the file doesn't already exist
		if os.path.exists( self.filename ):
			raise RuntimeError, "Can't create log chunk in existing path %s" % \
				  self.filename

		# Open a filehandle
		self.file = open( self.filename, "w+b" )

		# The table of sample offsets in the written log
		self.offsets = []

		# Set the interval
		self.interval = interval

	def addSample( self, obj, t ):
		"""
		Add the given sample to the log.

		@param obj   The object to pickle into the log
		"""

		# If this is the first sample in this chunk, remember the timestamp
		if self.starttime == None:
			self.starttime = t

		# This is the new endtime
		self.endtime = t

		# Make sure file pointer is at the end of the file
		self.file.seek( 0, 2 )

		# Remember the sample's offset
		self.offsets.append( self.file.tell() )

		# Dump the timestamp to the file, then the object itself
		self.file.write( struct.pack( "d", t ) )
		pickle.dump( obj, self.file, -1 )

		# Update the size of this log
		self.size = self.file.tell()

	def terminate( self ):
		"""
		Finish off this chunk.  If there are no samples in this chunk, this
		chunk will be deleted, since having chunks with no samples in them is
		just a headache in other places in this module.  Returns True if this
		chunk had any samples in it.
		"""

		# Kill off zero length chunks
		if self.size == 0:
			self.file.close()
			os.unlink( self.filename )
			return False

		# Make sure we're at the end of the file
		self.file.seek( 0, 2 )

		# Dump offsets to the end of the file
		for offset in self.offsets:
			self.file.write( struct.pack( "I", offset ) )

		# Dump number of samples to the end of the file
		self.file.write( struct.pack( "I", len( self.offsets ) ) )

		# Dump interval to the end of the file
		self.file.write( struct.pack( "d", self.interval ) )
		self.file.close()
		return True

	def getSampleCount( self ):
		return len( self.offsets )

	def _getOffset( self, index ):

		# Since we hold the index table in memory, we just read straight off it
		# without going to disk
		return self.offsets[ index ]

if __name__ == "__main__":

	times = []
	n = 5
	l = Log( "test", "cmts", "rw", interval=0.05, chunkSize=60 )
	for i in xrange( n ):
	 	l.addSample( str( i ) )
	 	times.append( time.time() )
		print "Added %d at time %f" % (i, times[-1])
	 	time.sleep( l.getCurrentInterval() )

	l.setCurrentInterval( 0.2 )
	l.setCurrentInterval( 0.1 )

	for i in xrange( n ):
	 	l.addSample( str( i ) )
	 	times.append( time.time() )
		print "Added %d at time %f" % (i, times[-1])
	 	time.sleep( l.getCurrentInterval() )

	print "Times:"
	for t in times:
		print t
	print

	for t in times:
		print l.getSample( t )
