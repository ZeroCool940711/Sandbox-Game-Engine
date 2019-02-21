#!/usr/bin/env python

import os
import sys
import optparse

winFiles = []
unixFiles = []
otherFiles = []

def checkFileLineEnding( filename ):
	checkFile = open( filename )
	line = checkFile.readline()
	checkFile.close()

	# Empty files don't matter
	if not len( line ):
		return

	if line.endswith( "\r\n" ):
		winFiles.append( filename )
	elif line.endswith( "\n" ):
		unixFiles.append( filename )
	else:
		otherFiles.append( filename )

	return


def checkDir( dirname ):
	dirList = os.listdir( dirname )

	for filename in dirList:
		fullPathToFile = dirname + "/" + filename
		if os.path.isdir( fullPathToFile ):
			if ( filename != ".svn" ) and ( filename != "CVS" ):

				checkDir( fullPathToFile )

		elif filename.endswith( ".sh" ) or \
			 filename.endswith( ".py" ) or \
			 filename.endswith( ".conf" ) or \
			 filename.endswith( ".logrotate" ) or \
			 filename.endswith( ".xml" ):

			checkFileLineEnding( fullPathToFile )

	return


def checkArgs():
	opt = optparse.OptionParser( "Usage: %prog [-cv]" )
	opt.add_option( "-c", "--convert",
					dest = "convert",
					action = "store_true",
					default = False,
					help = "Convert any suspect files using 'dos2unix'" \
							"/'mac2unix'." )
	opt.add_option( "-v", "--verbose",
					dest = "verbose",
					action = "store_true",
					default = False,
					help = "Output any suspect files found." )
	return opt.parse_args()


def convertInvalidFiles( startDir ):

	if os.system( "which dos2unix >/dev/null 2>&1" ):
		print "ERROR: Unable to locate 'dos2unix' for conversion."
		sys.exit( 1 )

	if os.system( "which mac2unix >/dev/null 2>&1" ):
		print "ERROR: Unable to locate 'mac2unix' for conversion."
		sys.exit( 1 )

	checkDir( startDir )

	if len( winFiles ) > 0:
		fileCount = 0
		for filename in winFiles:
			if os.system( "dos2unix '%s' >/dev/null 2>&1" % filename ):
				print "Failed to convert:", filename
			else:
				fileCount += 1

		print "Successfully converted %d dos line ending file(s)." % fileCount

	if len( otherFiles ) > 0:
		fileCount = 0
		for filename in otherFiles:
			if os.system( "mac2unix '%s' >/dev/null 2>&1" % filename ):
				print "Failed to convert:", filename
			else:
				fileCount += 1

		print "Successfully converted %d mac line ending file(s)." % fileCount



############################################
# Check everything in bigworld/tools/server

(options, args) = checkArgs()

startDir = ".."

if options.convert:
	convertInvalidFiles( startDir )
else:
	checkDir( startDir )

	suspectCount = len( winFiles ) + len( otherFiles )

	if suspectCount > 0:
		print "Found %d file(s) with suspicious line endings." % suspectCount

		if options.verbose:
			for filename in winFiles:
				print filename
			for filename in otherFiles:
				print filename

		sys.exit( 1 )
	else:
		print "All files appear to have correct line endings."
		sys.exit( 0 )

