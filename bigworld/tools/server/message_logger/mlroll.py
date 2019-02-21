#!/usr/bin/env python

"""
Utility to roll logs.
"""

import os
import sys
import signal
import time

import bwsetup; bwsetup.addPath( ".." )
from pycommon import log
import util

USAGE = "%prog [options] [logdir]\n" + __doc__.rstrip()


def main():
	parser = util.getBasicOptionParser( USAGE )
	mlog, kwargs, options, args = util.basicInit( parser )

	# Make sure the log is currently being written to
	if not os.path.exists( "%s/pid" % mlog.root ):
		log.warning( "Not sending SIGHUP - no logger currently writing to %s",
					 mlog.root )
		return 0

	# Send HUP to the logger process
	os.kill( int( open( "%s/pid" % mlog.root ).read() ), signal.SIGHUP )

	# Wait a little bit to allow the select() loop in message_logger to cycle
	time.sleep( 1 )

	return 0


if __name__ == "__main__":
	sys.exit( main() )
