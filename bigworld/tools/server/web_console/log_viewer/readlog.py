"""
Module for reading data out of bwlogger logs.  Optimised for:
- reading per-user data
- reading recent data
"""

import time
import sys
import os
import re
from cStringIO import StringIO

def parseTime( s ):
	return time.mktime( time.strptime( s, LogEntry.TIME_FMT ) )

class LogEntry( object ):
	"""
	A single record in a bwlog.
	"""

	TIME_FMT = "%a %d %b %Y %H:%M:%S"
	TYPES = [ "TRACE",
			  "DEBUG",
			  "INFO",
			  "NOTICE",
			  "WARNING",
			  "ERROR",
			  "CRITICAL",
			  "HACK",
			  "SCRIPT" ]

	SHOW_TIME = 0x01
	SHOW_HOST = 0x02
	SHOW_USER = 0x04
	SHOW_PID = 0x08
	SHOW_PROCS = 0x10
	SHOW_SEVERITY = 0x20
	SHOW_MESSAGE = 0x40

	def __init__( self, line ):
		"""
		Create this object from a line of text.
		"""

		if line[-1] != '\n':
			raise ValueError

		fields = line.split( ",", 7 )

		# Time parsing
		t, ms = fields[0].split( "." )
		self.time = parseTime( t )
		self.ms = int( ms )

		self.host = fields[1].strip()
		self.user = fields[3].strip()
		self.pid = int( fields[4] )
		self.component = fields[5].strip().lower()
		self.type = fields[6].strip()
		self.msg = fields[7].strip()

	def __str__( self ):
		return "%-24s %-12s %-10s %-6d %-10s %-10s %s" % \
			   (time.ctime( self.time ), self.host, self.user,
				self.pid, self.component, self.type, self.msg)

	def filtStr( self, mask ):
		stream = StringIO()
		if mask & self.SHOW_TIME:
			stream.write( "%-25s " % time.ctime( self.time ) )
		if mask & self.SHOW_HOST:
			stream.write( "%-13s" % self.host )
		if mask & self.SHOW_USER:
			stream.write( "%-11s" % self.user )
		if mask & self.SHOW_PID:
			stream.write( "%-6d" % self.pid )
		if mask & self.SHOW_PROCS:
			stream.write( "%-11s" % self.component )
		if mask & self.SHOW_SEVERITY:
			stream.write( "%-11s" % self.type )
		if mask & self.SHOW_MESSAGE:
			stream.write( self.msg )
		stream.write( "\n" )
		return stream.getvalue()

	def dict( self ):
		return dict( time = self.time,
					 ms = self.ms,
					 host = self.host,
					 user = self.user,
					 pid = self.pid,
					 component = self.component,
					 type = self.type,
					 msg = self.msg );

class Log( object ):

	CHUNKSIZE = 8192
	DEFAULT_LOGFILE = "/var/log/bwlogger/bwlog"

	def __init__( self, filename = DEFAULT_LOGFILE ):
		self.file = open( filename )

	def getEntries( self, starttime, endtime=None, user=None,
					host=None, pid=None, procs=None, severities=None,
					message=None, caseSens=True ):

		entries = []

		# Find first record
		t, offset = self.locate( starttime )
		if t is None:
			return entries
		chunk = self.readChunk( offset )

		# Macro for doing msg pattern matching
		if caseSens:
			match = lambda e, s: s in e.msg
		else:
			match = lambda e, s: re.search( s, e.msg, re.IGNORECASE )

		# Read records until we hit end of log or endtime
		while True:

			linestart = chunk.tell()

			try:
				e = LogEntry( chunk.readline() )
			except:
				offset += linestart
				chunk = self.readChunk( offset )
				if not chunk.getvalue():
					break
				else:
					continue

			if endtime and e.time >= endtime:
				break

			if (user and e.user != user) or \
			   (host and e.host != host) or \
			   (pid and e.pid != pid) or \
			   (procs is not None and e.component not in procs) or \
			   (severities is not None and e.type not in severities) or \
			   (message is not None and not match( e, message )) :
				continue
			else:
				entries.append( e )

		return entries

	def getRecentEntries( self, seconds, *args ):

		# If the length of args is == 8, some idiot has specified endtime
		if len( args ) == 8:
			args[0] = None

		return self.getEntries( self.getLastTimeStamp() - seconds, *args )

	def locate( self, tgt ):
		"""
		Runs a binary search on the logfile to find the smallest time >= tgt
		"""

		self.file.seek( 0, 2 )
		left = 0; right = self.file.tell()

		while True:

			# Seek to middle and read a chunk
			offset = (left + right)/2
			chunk = self.readChunk( offset )

			# Get generator for times in that chunk
			times = self.readTimes( chunk, offset )
			try:
				t, o = times.next()

			# If there isn't even one time in the chunk, we've asked for too
			# great a time
			except StopIteration:
				return (None, None)

			# If this chunk is later than the tgt time and we can still go more
			# to the left, set new locators
			if t > tgt and offset > 0:
				right = offset
				continue

			# Otherwise scan the chunk for a time >= tgt
			while True:
				try:
					t, o = times.next()
					if t >= tgt:
						return t, o

				# If we hit the end of the chunk without finding a time greater
				# than the target we need to search inside the right half
				except StopIteration:
					break

			# Set locators to right half
			left = offset

	def readChunk( self, offset ):

		self.file.seek( offset )
		return StringIO( self.file.read( self.CHUNKSIZE ) )

	def readTimes( self, chunk, startoffset ):
		"""
		Generator to read times and offsets out of a chunk.
		"""

		# Find first complete line or EOF
		if startoffset > 0:
			while True:
				c = chunk.read( 1 )
				if c == '':	return
				if c == '\n': break

		# Yield successive line times
		while True:
			offset = chunk.tell() + startoffset
			line = chunk.readline()
			if line == "":
				return

			try:
				t = parseTime( line.split( ".", 2 )[0] )
			except ValueError:
				return

			yield (t, offset)

	def getLastTimeStamp( self ):

		self.file.seek( 0, 2 )
		offset = self.file.tell() - self.CHUNKSIZE
		chunk = self.readChunk( offset )
		for t, o in self.readTimes( chunk, offset ):
			pass
		return t

if __name__ == "__main__":
	l = Log( sys.argv[1] )
	starttime = parseTime( sys.argv[2] )
	#endtime = parseTime( sys.argv[3] )
	l.getEntries( starttime )
