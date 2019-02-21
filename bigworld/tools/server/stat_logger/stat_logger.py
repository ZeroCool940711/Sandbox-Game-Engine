#!/usr/bin/env python

# Import pycommon modules (always do this first!)
import bwsetup
bwsetup.addPath("..")

from pycommon import util, uid, log as logmodule

# Import standard modules
import time
import sys
import os
import constants
import logging
import signal
from optparse import OptionParser

# Import local files
import gather
import prefxml
import utilities
import dbmanager
import comparepref

# Logging module
log = logging.getLogger("stat_logger")

#------------------------------------------------------------------------------
# Section: main function
#------------------------------------------------------------------------------
def main( args ):
	"""
	StatLogger main function.
	"""

	# Add log handler only after we might have rebound sys.stdout,
	# otherwise it'll be bound to the original value of sys.stdout
	logHandler = logging.StreamHandler( sys.stdout )
	logFormatter = logging.Formatter(
		"%(asctime)s: %(levelname)s  %(message)s" )
	logHandler.setFormatter( logFormatter )
	log.addHandler( logHandler )

	# Set log output made from cluster.py to match our output
	# Log module is the imported python module at pycommon/log.py
	# This module has a global variable, console, which we
	# change the formatter for.
	logmodule.console.setFormatter( logFormatter )

	# Check pref filename to use
	if args.prefFilename:
		prefFilename = args.prefFilename
	else:
		prefFilename = constants.prefFilename

	# Read preferences
	try:
		options, prefTree = prefxml.loadPrefsFromXMLFile( prefFilename )
		# args uid overrides
		if args.uid:
			options.uid = int(args.uid)
	except prefxml.StatLoggerPrefError, e:
		print "Error in preference file '%s':\n%s" % (prefFilename, e)
		sys.exit( 1 )

	# Adjust xml output format
	utilities.xmlModifyWriter()

	# Create db manager
	manager = dbmanager.DbManager( options.dbHost,
		options.dbPort, options.dbUser, options.dbPass, options.dbPrefix )

	if args.listDb:
		printLogList( manager, options )
		sys.exit( 0 )

	# Connect to log db, grab db pref tree
	try:
		prefTree = connectToLogDb( manager, args, prefTree, options )
	except AbortConnect:
		manager.close()
		sys.exit( 1 )
	log.info( "Connected to database \"%s\"", manager.logDbName )
	manager.addSessionStart()


	# Start the network side of things...
	usingLogOutput = (args.stdoutFile != None)
	gatherer = gather.Gather( options, usingLogOutput, prefTree, manager )
	gatherer.setDaemon(True)
	gatherer.start()

	# Setup the SIGUSR1 signal handler (to make it start debugger)
	def sigHandler( signum, frame ):
		gatherer.pushDebug()
	signal.signal( signal.SIGUSR1, sigHandler )

	# Enter thread monitor loop
	log.info( "Collecting stats, press Ctrl-C to stop..." )
	try:
		while gatherer.isAlive():
			time.sleep( 1.0 )
		log.error( "Gather thread exited unexpectedly, aborting." )
		manager.close()
		sys.exit( 1 )
	except KeyboardInterrupt:
		pass

	# And close the database
	#print "Telling Gather thread to finish up"
	gatherer.pushFinish()

	# Wait for the Gather thread to finish up writing log
	log.info( "Waiting for Gather thread to complete logging" )

	# If there's an error which freezes, time out after a minute
	gatherer.join(60.0)

	manager.endSession()
	manager.close()


def connectToLogDb( manager, args, prefTree, options ):
	"""
	Connect to a log database.
	"""
	logDbPrefTree = None
	try:
		manager.connectToLogDb( args.dbName )
		logDbPrefTree = manager.getLogDbPrefTree()
		if not args.useDbPrefs:
			comparepref.comparePrefTreesWithError( prefTree, logDbPrefTree )

		# Different tick time
		logDbInterval = manager.getLogDbInterval()
		if logDbInterval != options.sampleTickInterval:
			log.info( "Mismatch between preference file and database:" )
			log.info( "  - Interval is different (file %f, db %f)",
				options.sampleTickInterval, logDbInterval )
			logDbPrefTree = createLogDb( manager, prefTree, options, args )

		# Different database version
		logDbVersion = manager.getLogDbVersion()
		if logDbVersion != constants.dbVersion:
			log.info( "The database structure is incompatible with " \
				"the current structure. (Old version: %s, New version: %s)",
				logDbVersion, constants.dbVersion )
			logDbPrefTree = createLogDb( manager, prefTree, options, args )


	# Errors which occurred trying to connect to db
	except dbmanager.LogDbError, e:
		if e.code == dbmanager.LogDbError.ERR_NOLOGDB:
			log.warning( "WARNING: %s.", e.msg )
			logDbPrefTree = createLogDb( manager, prefTree, options, args,
				alreadyExists = False)

		elif e.code == dbmanager.LogDbError.ERR_NOACCESS:
			log.error( "ERROR: %s.", e.msg )
			log.error( " - Either the database permissions have not been set, "\
				"or the username was misspelled in \"preferences.xml\"." )
			raise AbortConnect()

		elif e.code == dbmanager.LogDbError.ERR_BADPASSWORD:
			log.error( "ERROR: %s.", e.msg )
			log.error( " - Please correct the \"dbPass\" setting in "\
				"\"preferences.xml\"." )
			raise AbortConnect()

	# Db table structure validation error
	except dbmanager.LogDbValidateError, e:
		log.error( "ERROR: Database did not validate:" )
		e.printErrors("  -")
		logDbPrefTree = createLogDb( manager, prefTree, options, args )

	# Preference matching error
	except comparepref.PrefMatchError, e:
		print "ERROR: Mismatch between preference file and database:"
		e.printErrors("  -")
		logDbPrefTree = createLogDb( manager, prefTree, options, args )

	return logDbPrefTree


def createLogDb( manager, prefTree, options, args, alreadyExists = True ):
	"""
	Create a new log database.
	"""
	log.info( "Attempting to create new database..." )
	if not args.allowCreate:
		log.info( "Cannot create new database: \"-p\" option specified." )
		raise AbortConnect()
	elif args.dbName and alreadyExists:
		log.info( "Database \"%s\" cannot be used, aborting.",
			manager.logDbName )
		raise AbortConnect()
	else:
		manager.createLogDb( args.dbName, prefTree, options.sampleTickInterval )
		logDbPrefTree = manager.getLogDbPrefTree()
	return logDbPrefTree


def printLogList( manager, options ):
	"""
	Prints a list of log databases.
	"""
	try:
		results = manager.getLogDbList( allowCreate=False )
	except dbmanager.LogDbError, e:
		if e.code == dbmanager.LogDbError.ERR_NOLOGDB:
			log.info( "No logs exist." )
		else:
			raise
		return

	if len( results ) == 0:
		log.info( "No logs exist." )
		return

	def boolToChar( val ):
		if val:
			return "Yes"
		else:
			return "No"

	for name, created, used, active, users in results:
		print "Name     : ", name
		print "Created  : ", utilities.formatTime( created )
		print "Last used: ", utilities.formatTime( used )
		print "Active   : ", boolToChar( active )
		print


def readArgs():
	"""
	Parse the command line arguments.
	"""
	#usage= "usage: %prog [options]"

	parser = OptionParser()
	parser.add_option( "-f", "--config-file",
		default = constants.prefFilename, dest = "prefFilename",
		metavar = "<preference_file>",
		help="Specifies a preference file to use instead of the default " \
			"(which is \"%default\")" )

	parser.add_option( "-n", "--database-name",
		default = None, type = "string", dest = "dbName",
		metavar = "db_name",
		help = "Name of the database to use. If the database does not exist, "\
				"it will automatically create it unless -p was specified. "\
				"Note: If using an existing database, it is recommended to "\
				"enable --use-db-prefs as well")
	
	parser.add_option( "-u", "--uid", default = None, dest = "uid", help = "uid of the processes to log" )

	parser.add_option( "-p", "--no-auto-create-db",
		default = True, dest = "allowCreate",
		action = "store_false",
		help = "Prevents creation of a new log database under any " \
				"circumstance." )

	parser.add_option("--pid", dest = "pidFile",
		metavar = "<pid_file>", default = "stat_logger.pid",
		help = "[daemon mode] Location to store PID file." )

	parser.add_option( "", "--use-db-prefs",
		default = False, dest = "useDbPrefs",
		action = "store_true",
		help = "Retrieve and use preferences from the log database being used "\
			"(ignores preference file)" )

	parser.add_option( "-l", "--list",
			default = False, action = "store_true",
			dest = "listDb",
			help = "Print a list of log databases" )

	parser.add_option( "-o", "--output",
			default = None, dest = "stdoutFile",
			metavar = "<output_file>",
			help = "Log file to dump standard output (default is stdout)" )

	parser.add_option( "-e", "--erroutput",
			default = None, dest = "stderrFile",
			metavar = "<output_file>",
			help = "Log file to dump error output (default is stderr)" )

	parser.add_option( "-d", "--daemon",
			default = True, action = "store_false", dest = "foreground",
			help = "Run stat_logger in daemon mode (default is to run in foreground)" )

	homeDefault = os.path.abspath( os.path.dirname(sys.argv[0]) )
	parser.add_option( '', '--home',
			type = 'string', dest = 'home',
			default = homeDefault, metavar = "<path_to_stat_logger>",
			help = "[daemon mode] Home directory of stat_logger (default is '%default)" )

	args, remain = parser.parse_args()

	return args


class AbortConnect( Exception ):
	""" Exception when we can't connect to the database. """
	pass

if __name__ == "__main__":
	# Read arguments
	args = readArgs()

	if args.foreground:
		main( args )
	else:

		from pycommon.daemon import Daemon
		d = Daemon( run = main,
			args = (args,),
			workingDir = args.home,
			outFile = args.stdoutFile,
			errFile = args.stderrFile,
			pidFile = args.pidFile
		)
		d.start()

# stat_logger.py
