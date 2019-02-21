"""
Functions used by the ml* family of shell utilities.
"""

import optparse
import os
import sys

import bwsetup; bwsetup.addPath( ".." )
from pycommon import log
import message_log


def getBasicOptionParser( usage = None ):
	"""
	Creates an option parser that supports all the basic arguments that tools in
	the ml*.py suite are expected to understand.
	"""

	opt = optparse.OptionParser( usage or "%prog [options] [logdir]" )
	opt.add_option( "-u", "--uid",
					help = "Specify the UID or username to work with" )
	return opt


def basicInit( parser ):
	"""
	Parse command line options using the given parser and return a log object,
	kwarg dictionary for fetches, and the parsed options.
	"""

	options, args = parser.parse_args()

	if args:
		mlog = message_log.MessageLog( args[0] )
	else:
		mlog = message_log.MessageLog()

	# We're happy to use os.getuid() instead of uidmodule.getuid() because this
	# won't work on win32 anyways as bwlog.so is required.
	if options.uid is None:
		options.uid = os.getuid()

	try:
		options.uid = int( options.uid )
	except:
		try:
			options.uid = mlog.getUsers()[ options.uid ]
		except KeyError:
			log.error( "log %s does not have entries for user %s",
					   mlog.root, options.uid )
			sys.exit( 1 )

	kwargs = dict( uid = options.uid )
	return (mlog, kwargs, options, args)
