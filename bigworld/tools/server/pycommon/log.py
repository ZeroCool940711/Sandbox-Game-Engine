"""
This provides similar functionality to debug.hpp by using the python logging
library.  Typical usage of this module would be:

import log

log.warning( 'Help me!' )
"""

import sys
import logging
import traceback
from StringIO import StringIO

import util

# Log severity levels from debug.hpp
MESSAGE_PRIORITY_TRACE, \
MESSAGE_PRIORITY_DEBUG, \
MESSAGE_PRIORITY_INFO, \
MESSAGE_PRIORITY_NOTICE, \
MESSAGE_PRIORITY_WARNING, \
MESSAGE_PRIORITY_ERROR, \
MESSAGE_PRIORITY_CRITICAL, \
MESSAGE_PRIORITY_HACK, \
MESSAGE_PRIORITY_SCRIPT, \
NUM_MESSAGE_PRIORITY = range( 10 )

# And in reverse
SEVERITY_LEVELS = dict( TRACE = 0,
						DEBUG = 1,
						INFO = 2,
						NOTICE = 3,
						WARNING = 4,
						ERROR = 5,
						CRITICAL = 6,
						HACK = 7,
						SCRIPT = 8 )

# Console output the way I like it
class CleanFormatter( logging.Formatter ):

	def __init__( self, stripInfoPrefix = False ):
		logging.Formatter.__init__( self )
		self.stripInfoPrefix = stripInfoPrefix

	def format( self, record ):
		if self.stripInfoPrefix and record.levelno in (logging.INFO, VERBOSE):
			return record.getMessage()
		else:
			return "%-10s%s" % (logging.getLevelName( record.levelno ) + ":",
								record.getMessage())

# StreamHandler that terminates process on CRITICAL
class CriticalStreamHandler( logging.StreamHandler ):

	def __init__( self, strm = sys.stdout ):
		logging.StreamHandler.__init__( self, strm )

	def emit( self, record ):
		if record.levelno == logging.CRITICAL:

			logging.StreamHandler.emit( self, record )

			# Extract the stack trace up until the last critical() call
			buf = StringIO()
			traceback.print_stack( None, None, buf )
			lines = buf.getvalue().split( "\n" )
			for i in xrange( len( lines ) - 1, -1, -1):
				if ".critical(" not in lines[i]:
					lines.pop()
				else:
					break
			for l in lines:
				self.stream.write( l + "\n" )

			sys.exit( 1 )
		else:
			return logging.StreamHandler.emit( self, record )

# Handler to buffer one stream of info messages and one of error messages
class TwinBufferHandler( logging.Handler ):

	def __init__( self, bufsize = 256 ):
		logging.Handler.__init__( self )
		self.bufsize = bufsize
		self.messages = util.RingBuffer( bufsize )
		self.errors = util.RingBuffer( bufsize )

	def emit( self, record ):
		if record.levelno >= logging.WARNING:
			self.errors.push( self.format( record ) )
		else:
			self.messages.push( self.format( record ) )

	def clear( self ):
		self.messages = util.RingBuffer( self.bufsize )
		self.errors = util.RingBuffer( self.bufsize )

# Global stream handler
console = CriticalStreamHandler( sys.stdout )
console.setFormatter( CleanFormatter( True ) )

# Make a BigWorld style logging object
def getLogger( name ):
	global console

	# Setup logger for BigWorld stuff
	logger = logging.getLogger( name )

	# Setup basic console handler
	logger.addHandler( console )

	# Add extra log level hooks
	logger.verbose = lambda msg, *args, **kw: \
					 logger.log( VERBOSE, msg, *args, **kw )
	logger.notice = lambda msg, *args, **kw: \
					logger.log( NOTICE, msg, *args, **kw )
	return logger

bwlogger = getLogger( "bigworld" )

# Hook in funcs for ease of use
def debug( msg, *args, **kw ): return bwlogger.debug( msg, *args, **kw )
def info( msg, *args, **kw ): return bwlogger.info( msg, *args, **kw )
def warning( msg, *args, **kw ): return bwlogger.warning( msg, *args, **kw )
def error( msg, *args, **kw ): return bwlogger.error( msg, *args, **kw )
def critical( msg, *args, **kw ): return bwlogger.critical( msg, *args, **kw )

# Translation for messages coming from C apps
def cmsg( level, msg ):
	if level == MESSAGE_PRIORITY_DEBUG:	return debug( msg )
	if level == MESSAGE_PRIORITY_INFO: return info( msg )
	if level == MESSAGE_PRIORITY_WARNING: return warning( msg )
	if level == MESSAGE_PRIORITY_ERROR: return error( msg )
	if level == MESSAGE_PRIORITY_CRITICAL: return critical( msg )
	return info( msg )

# Add a VERBOSE message handler func
VERBOSE = 5
logging.addLevelName( VERBOSE, "VERBOSE" )
def verbose( msg, *args, **kw ):
	return bwlogger.log( VERBOSE, msg, *args, **kw )

# Add a NOTICE message handler func
NOTICE = logging.WARNING + 1
logging.addLevelName( NOTICE, "NOTICE" )
def notice( msg, *args, **kw ):
	return bwlogger.log( NOTICE, msg, *args, **kw )

# Add a HACK message handler func
HACK = logging.WARNING + 1
logging.addLevelName( HACK, "HACK" )
def hack( msg, *args, **kw ):
	return bwlogger.log( HACK, msg, *args, **kw )

logging.getLogger().setLevel( VERBOSE+1 )
