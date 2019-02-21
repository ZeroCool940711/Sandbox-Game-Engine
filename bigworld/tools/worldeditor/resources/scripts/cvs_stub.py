##
## This script implements WorldEditor's CVS wrapper for use with BWLockD.
##
## IMPORTANT NOTE: This script needs to be compiled into a windows executable
## so it can be used by WorldEditor. The executable for this file was generated
## using py2exe (www.py2exe.org).
##

from os import popen4
from os import execv
from os import path
from sys import argv
from sys import exit
from sys import stdout
from sys import stderr

CVS_PATH			=	"C:\\Program Files\\TortoiseCVS\\cvs.exe"
QUOTED_CVS_PATH		=	"\"" + CVS_PATH + "\""

FAILURE				=	3
SUCCESS				=	0

UNKNOWN				=	0
ADDED				=	1
REMOVED				=	2
UPTODATE			=	3
LOCALMODIFIED		=	4
NEEDCHECKOUT		=	5
MAXLINELIMIT		= 2040

COMMANDS = ( "check", "addfolder", "addfile", "addbinaryfile", "removefile", "commitfile", "updatefile", "updatefolder", "refreshfolder", "managed" )

def printUsageAndAbort():
	print "USAGE: cvs_stub <" + '|'.join( COMMANDS ) + "> [file list]"
	exit( FAILURE )

def batchCommit( argv ):
	msg = argv[0]
	if msg[0:1] == '\"' and msg[-1:] == '\"':
		msg = msg[1:-1]
	filename = argv[1]
	if filename[0:1] == '\"' and filename[-1:] == '\"':
		filename = filename[1:-1]
	batName = filename.replace( ".txt", ".bat" )
	fileDest = open( batName, "w" )
	if fileDest:
		fileDest.write( "@echo off\n" )
		fileSource = open( filename )
		if fileSource:
			cmdLn = "\n%s commit -m \"%s\" -R " % ( QUOTED_CVS_PATH, msg )
			lnLen = 0
			lines = fileSource.readlines()
			for line in lines:
				while line[-1:] == '\n' or line[-1:] == 'r':
					line = line[:-1]
				if len( line ):
					if line[0:1] == '\"' and line[-1:] == '\"':
						line = line[1:-1]
					if lnLen + len( line ) + 3 > MAXLINELIMIT:
						lnLen = 0
					if lnLen == 0:
						fileDest.write( cmdLn )
						lnLen += len( cmdLn )
					fileDest.write( "\"%s\" " % line )
					lnLen += len( line ) + 3
			fileSource.close()
		fileDest.write( "\ndel /q \"%s\"\n" % batName )
		fileDest.close()
	return batName

if len( argv ) < 2 or not argv[1] in COMMANDS:
	printUsageAndAbort()

del argv[0]
command = argv[0]
del argv[0]

if command == "check":
	if path.isfile( CVS_PATH ):
		print 'CVS\n0'
		exit(SUCCESS)
	print 'Cannot find ', CVS_PATH
	exit(FAILURE)
elif command == "addfolder":
	del argv[0] # the message
	argv.insert( 0, "add" )
elif command == "addfile":
	argv.insert( 0, "add" )
elif command == "addbinaryfile":
	argv.insert( 0, "-kb" )
	argv.insert( 0, "add" )
elif command == "removefile":
	argv.insert( 0, "remove" )
elif command == "commitfile":
	exit( execv( batchCommit( argv ), [] ) )
elif command == "updatefile":
	argv.insert( 0, "-d" )
	argv.insert( 0, "-C" )
	argv.insert( 0, "update" )
elif command == "updatefolder":
	argv.insert( 0, "-d" )
	argv.insert( 0, "update" )
elif command == "refreshfolder":
	argv.insert( 0, "-R" )
	argv.insert( 0, "status" )
elif command == "managed":
	if path.isdir( argv[ 0 ] ):
		if argv[ 0 ][ -1 : 0] != '/':
			argv[ 0 ] = argv[ 0 ] + '/'
		if path.isfile( argv[ 0 ] + "CVS/Entries" ):
			exit( 0 )
		exit( 1 )
	if path.isfile( argv[ 0 ] ):
		argv.insert( 0, "status" )
		argv.insert( 0, QUOTED_CVS_PATH )
		fds = popen4( ' '.join( argv ) )
		if fds[ 1 ]:
			output = ''.join( fds[ 1 ].readlines() )
			stderr.write( output )
			if output.find( "Unknown" ) == -1:
				exit( 0 )
	exit( 1 )
else:
	printUsageAndAbort()

argv.insert( 0, QUOTED_CVS_PATH )
exit( execv( CVS_PATH, argv ) )
