#!/usr/bin/env python

"""
Display a listing of the segments in a user's message log.
"""

import sys

import util
import message_log

import bwsetup; bwsetup.addPath( ".." )
import pycommon.util


USAGE = "%prog [options] [logdir]\n" + __doc__.rstrip()


def main():
	parser = util.getBasicOptionParser( USAGE )
	parser.add_option( "-a", "--all", action = "store_true",
					   help = "List segments for all users" )

	mlog, kwargs, options, args = util.basicInit( parser )

	if options.all:
		users = sorted( mlog.getUsers().items(), key = lambda (x,y): y )
	else:
		users = [(mlog.getUserLog( options.uid ).uid, options.uid)]

	for username, uid in users:

		if len( users ) > 1:
			print "* %s (%d) *" % (username, uid)

		ls( mlog, uid )

		if len( users ) > 1:
			print

	return 0


def ls( mlog, uid, number = False ):

	segments = mlog.getUserLog( uid ).getSegments()
	totalSize = totalEntries = 0
	i = 0
	if segments:
		if number:
			print "  # ",
		print message_log.Segment.FMT % ("Time", "Duration", "Entries", "Size")
		for seg in segments:
			if number:
				print ("%3d " % i),
			print seg
			totalSize += seg.entriesSize + seg.argsSize
			totalEntries += seg.nEntries
			i += 1

		if not number:
			print "%d entries, %s total" % (totalEntries,
											pycommon.util.fmtBytes( totalSize ))

	return segments


if __name__ == "__main__":
	sys.exit( main() )
