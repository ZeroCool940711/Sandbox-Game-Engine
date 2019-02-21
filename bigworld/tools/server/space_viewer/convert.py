#!/usr/bin/env python

import sys
import struct
import cPickle as pickle
import optparse

from svreplay import *
from svlogger import *

def main():

	opt = optparse.OptionParser()
	opt.add_option(
		"--static", dest = "static", action = "store_true",
		help = "Convert an instance-pickled log to a statically pickled one" )
	opt.add_option(
		"--header", dest = "header", action = "store_true",
		help = "Add a header to a log" )
	opt.add_option(
		"--index", dest = "index", action = "store_true",
		help = "Add a header and an index to a log" )
	(options, args) = opt.parse_args()
	
	try:
		(inname, outname) = args
	except:
		opt.print_help()
		return 1
		
	inf = open( inname, "r" )
	outf = open( outname, "w" )

	# For converting logs made with a pickler instance into logs that can be
	# unpickled with the static pickle.load()
	if options.static:
		
		unpick = pickle.Unpickler( inf )
		i = 0
	
		while True:
			try: obj = unpick.load()
			except EOFError: break
			pickle.dump( obj, outf, pickle.HIGHEST_PROTOCOL )
			i += 1
			
		print "Converted %d objects" % i
		return 0

	# For adding a file header which has information about the format of the log
	if options.header:

		outf.write( struct.pack( "I", 0 ) )
		bytes = 4
		
		while True:
			try: s = inf.read( 8192 )
			except EOFError: break
			if not s: break
			outf.write( s )
			bytes += len( s )

		print "Wrote %d bytes" % bytes
		return 0

	# For adding a header and index table to the tail of a file
	if options.index:

		# Macro to get a pickle and it's address
		def readPickle():
			addr = inf.tell()
			return (pickle.load( inf ), addr)
		def writePickle( o ):
			pickle.dump( o, outf, pickle.HIGHEST_PROTOCOL )

		# Macros to read and write ints
		def readInt(): return struct.unpack( "I", inf.read( 4 ) )[0]
		def writeInt( i ): outf.write( struct.pack( "I", i ) )

		# Write new header
		writeInt( HAS_INDEX )

		# Read + write the space bounds
		writePickle( readPickle()[0] )

		# Read all the CMTs and remember their addresses
		indexTable = []
		while True:
			try:
				(cmt, addr) = readPickle()
				writePickle( cmt )
				indexTable.append( addr )
			except EOFError:
				break
		print "Copied %d CMTs" % len( indexTable )

		# Print out the index table (magic +4 is to account for header)
		for i in indexTable:
			writeInt( i+4 )

		# Print out length of index table
		writeInt( len( indexTable ) )

if __name__ == "__main__":
	sys.exit( main() )
