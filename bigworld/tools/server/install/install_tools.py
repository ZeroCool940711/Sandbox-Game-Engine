#!/usr/bin/env python

import bwsetup;

import re
import os
import sys
import pwd
import grp
import time
import crypt
import optparse
import shutil
import ConfigParser
import selinux

bwsetup.addPath( ".." )
from pycommon import util
from pycommon.util import prompt, promptYesNo
from pycommon import log
from stat_logger import prefxml

HOMEDIR_ROOT = "/usr/local/share"
VARDIR_ROOT = "/var/local"
PID_DIR = "/var/run/bigworld"
LOG_DIR = "/var/log/bigworld/message_logger"
ML_ARCHIVES = "%s/message_logs.tar.gz" % LOG_DIR
ML_LOGROTATE_SAMPLE = "../message_logger/message_logger.logrotate"
ML_LOGROTATE = "/etc/logrotate.d/bw_message_logger"
ST_LOGROTATE_ORIG = "./bw_server_tools.logrotate"
ST_LOGROTATE = "/etc/logrotate.d/bw_server_tools"
CONF_FILE = "/etc/bigworld.conf"
ML_CONF_FILE = "../message_logger/message_logger.conf"
WC_CONF_FILE = "../web_console/prod.cfg"
SL_CONF_FILE = "../stat_logger/preferences.xml"
BWLOG_SO = "../message_logger/bwlog.so"
TOOLS_SECTION = "tools"
ML_SECTION = "message_logger"
MF_ROOT = ""
# LOG_OUTPUT_DIR is hardcoded in the init.d scripts as the location where
# daemon logs to stdout/stderr should be redirected to.
LOG_OUTPUT_DIR = "/var/log/bigworld"

SERVICES = [ ( "bw_stat_logger", "./stat_logger.sh" ),
			 ( "bw_message_logger", "./message_logger.sh" ),
			 ( "bw_web_console", "./web_console.sh" ) ]

FUNCTIONS = ( "/etc/init.d/bw_functions", "./bw_functions.sh" )

def updateConf( section, key, value = None, file = CONF_FILE ):

	config = ConfigParser.SafeConfigParser()
	config.read( file )
	if not config.has_section( section ):
		config.add_section( section )
	if value:
		config.set( section, key, value )
	else:
		config.remove_option( section, key )
	return config.write( open( file, "w" ) )

def readConf( section = TOOLS_SECTION, file = CONF_FILE ):

	config = ConfigParser.SafeConfigParser()
	config.read( file )
	return dict( config.items( section ) )


def uninstallService( package ):

	if os.path.isfile( "/etc/debian_version" ):
		chkcmd = "ls /etc/init.d/%s > /dev/null 2>&1" % package
		rmcmd  = "rm /etc/init.d/%s; update-rc.d %s remove" % (package, package)
	elif os.path.isfile( "/etc/redhat-release" ):
		chkcmd = "/sbin/chkconfig --list %s > /dev/null 2>&1" % package
		rmcmd  = "/sbin/chkconfig --del %s; rm /etc/init.d/%s" % (package, package)
	else:
		log.error( "Unsupported linux distribution" )
		sys.exit( 1 )

	if not os.system( chkcmd ):
		print "- Uninstalling existing system service '%s'" % package
		os.system( "/etc/init.d/%s stop > /dev/null 2>&1" % package )
		os.system( rmcmd )

	# Wait for a moment to ensure the process has shutdown and cleaned up
	time.sleep( 2 )

	return


def installService( package, src ):

	# If an old service exists (and could be running), remove it
	uninstallService( package )

	print "+ Installing package: %s" % package

	dest = "/etc/init.d/%s" % package

	# Ensure the src filename is absolute for the symlink
	src = os.path.abspath( src )

	# Copy the script into /etc/init.d now
	try:
		shutil.copy( src, dest )
	except:
		print "ERROR: Unable to copy '%s' to '%s'." % ( src, dest )
		(type, value, trace) = sys.exc_info()
		print value

	# TODO: check status here
	if os.path.isfile( "/usr/sbin/update-rc.d" ): # Debian systems

		runLevels = None
		runSeqStart = None
		runSeqStop = None
		# Extract the start / stop levels for chkconfig
		file = open(src)

		for line in file:
			m = re.match( "#\s+chkconfig:\s+(-|\d+)\s+(\d+)\s+(\d+)", line )
			if m:
				runLevels = m.group( 1 )
				if runLevels == "-":
					runLevels = ""
				runSeqStart = m.group( 2 )
				runSeqStop  = m.group( 3 )
				break

		file.close()

		if not runSeqStart or not runSeqStop:
			log.error( "Failed to extract start / stop sequence numbers "\
					"from %s",
				src )
			log.error( "Not installing service" )
		else:
			if runLevels:
				cmd = "/usr/sbin/update-rc.d %s start %s %s . stop %s %s ." \
					% (package,
						runSeqStart, " ".join( runLevels ),
						runSeqStop, " ".join( stopRunLevel
							for stopRunLevel in "0123456"
							if stopRunLevel not in runLevels ))

				os.system( cmd )
				os.system( "/usr/sbin/invoke-rc.d %s start" % package );

	elif os.path.isfile( "/sbin/chkconfig" ): # RedHat systems
		os.system( "/sbin/chkconfig --add %s" % package );
		os.system( "/etc/init.d/%s start" % package );
	else:
		log.error( "Unsupported linux distribution" )
		exit( 1 )

	return


def verifyUser( username ):

	pwent = None
	try:
		pwent = pwd.getpwnam( username )

	except KeyError:
		print "\nUser '%s' doesn't exist." % username
		print "\nIn order to proceed with the installation the user '%s' needs" % username
		print "to exist. Please create the user and re-run this installation script."
		sys.exit( 0 )

	return pwent

def verifyGroup( groupname ):

	grpent = None
	try:
		grpent = grp.getgrnam( groupname )
	except KeyError:
		print "\nGroup '%s' doesn't exist." % groupname
		print "\nIn order to proceed with the installation the group '%s' needs" % groupname
		print "to exist. Please create the group and re-run this installation script."
		sys.exit( 0 )

	return grpent

def verifyToolsDir( tooldir ):
	if not os.path.isdir( tooldir ):
		print "%s is not a directory.\n" % tooldir
		sys.exit( 1 )

	# Perform a simple check of web_console existance to determine whether
	# the directory provided is actually a server tools directory
	if not os.path.isfile( "%s/web_console/start-web_console.py" % tooldir ):
		print "'%s' doesn't appear to contain the server tools.\n" % tooldir
		sys.exit( 1 )

	return True


def confirmUserDetails( username, groupname ):
	print "Server tools will be run as:"
	print "User  : %s" % username
	print "Group : %s" % groupname
	if not promptYesNo( "\nAre these details correct?\n" ):
		print "\nThe owner of all files within the server tools directory"
		print "must be the same as the user / group the tools will run as."
		print "\nPlease refer to the server installation guide."
		sys.exit( 1 )
	print


def confirmDetails( tooldir, logdir, mlarchive, piddir, username ):

	print "\nServer tools directories:"
	print "Server Tools  : %s" % tooldir
	print "PID directory : %s" % piddir
	print "Message Logger Log directory : %s" % logdir
	print "Message Logger Archive File  : %s" % mlarchive

	if not promptYesNo( "\nAre these details correct?\n" ):
		return False

	return True


# TODO: need a better method of notification of username / group not being
#        valid, as it doesn't match up with the /etc/bigworld.conf path
def queryUser( default_tooldir, username ):

	print "\n\n* Server Tools: Installation Paths"

	validDetails = False
	while not validDetails:
		print
		print "This installation is being run from under '%s'." % default_tooldir
		print "Configuration files and system startup scripts will be modified"
		print "to refer to this installation directory.\n"

		if not promptYesNo( "Is this directory correct?\n" ):
			print "If you wish to run the server tools into a different path,"
			print "please move the tools to that location before running this"
			print "installation script. Refer to the server installation guide"
			print "for assistance."
			sys.exit( 1 )

		verifyToolsDir( default_tooldir )

		logdir = prompt( "\nWhere should message_logger store its logs?\n", LOG_DIR )
		mlarchive = prompt( "\nWhere should message_logger logs be archived?\n", ML_ARCHIVES )

		piddir = prompt( "\nWhere should .pid files be stored?\n", PID_DIR )

		validDetails = confirmDetails( default_tooldir, logdir, mlarchive, piddir, username )

	return ( logdir, mlarchive, piddir )


# Obtain details about the MySQL server and setup the server
# tools configuration files to reflect the database setup.
def databaseInstall():

	# Read the defaults from the current stat_logger configuration
	# file.
	if not os.path.isfile( SL_CONF_FILE ):
		print "Stat Logger options file doesn't exist."
		print "Unable to continue"
		sys.exit( 1 )

	(opt, pref) = prefxml.loadPrefsFromXMLFile( SL_CONF_FILE )

	# Obtain database information and test the connection
	while True:
		print "\nPlease input MySQL database information:"
		dbHost = prompt( "Hostname", opt.dbHost )
		dbPort = prompt( "Port", opt.dbPort )
		dbUser = prompt( "Username", opt.dbUser )
		dbPass = prompt( "Password", opt.dbPass )
		dbPrefix = prompt( "StatLogger DB name prefix", opt.dbPrefix )

		print "\nMySQL database configuration:"
		print "Hostname : %s" % dbHost
		print "Port     : %s" % dbPort
		print "Username : %s" % dbUser
		print "Password : %s" % dbPass
		print "Prefix   : %s" % dbPrefix

		if promptYesNo( "\nAre these details correct?\n" ):
			break

	print

	# Test the connection can actually be established
	# Retry a couple of times to ensure a failure isn't network related.
	connfail = 0
	import MySQLdb
	while connfail < 2:
		try:
			conn = MySQLdb.connect( host=dbHost, port=int(dbPort),
								user=dbUser, passwd=dbPass )
			break
		except:
			connfail += 1
			time.sleep( 1 )

	if connfail:
		print "Unable to establish connection to the database using supplied details."
		sys.exit( 1 )

	cursor = conn.cursor()

	# Attempt to create the web_console database
	try:
		print "+ Creating web_console database in MySQL"
		cursor.execute( "create database bw_web_console" )
	except MySQLdb.ProgrammingError, e:

		# 1007 == ER_DB_CREATE_EXISTS
		# (Can't create database '%s'; database exists)
		if e[0] != 1007:
			print "Failed to create database 'bw_web_console'."
			print e[1]
			print "\nPlease correct this issue and re-run the installation."
			conn.close()
			sys.exit( 1 )

	conn.close()

	# Now attempt to re-connect to the specific database
	try:
		conn = MySQLdb.connect( host=dbHost, port=int(dbPort),
								user=dbUser, passwd=dbPass,
								db="bw_web_console" )
	except:
		print "Unable to establish connection to the 'bw_web_console' database"
		print "using supplied details."
		sys.exit( 1 )



	### WEB_CONSOLE

	# search + replace from sample-prod.cfg -> prod.cfg
	tgsqluri = "sqlobject.dburi=\"notrans_mysql://%s:%s@%s:%s/bw_web_console\"\n" \
				% ( dbUser, dbPass, dbHost, dbPort )


	# Take our sample web_console production configuration file and
	# write out the final copy, placing the sql uri in as we go
	webConf = open( "./web_console.cfg" )
	if os.path.isfile( WC_CONF_FILE ):
		if not promptYesNo( "About to overwrite '%s'. " \
			"Is it safe to continue?\n" % WC_CONF_FILE ):
			sys.exit( 1 )
		print

	print "+ Writing '%s'." % WC_CONF_FILE
	prodConf = open( WC_CONF_FILE, "w" )

	for line in webConf:
		prodConf.write( re.sub( "^###BW_SQLURI###", tgsqluri, line ) )


	webConf.close()
	prodConf.close()


	### STAT_LOGGER

	## Now write the stat_logger options file
	print "+ Writing '%s'." % SL_CONF_FILE

	opt.dbHost = dbHost
	opt.dbPort = dbPort
	opt.dbUser = dbUser
	opt.dbPass = dbPass
	opt.dbPrefix = dbPrefix

	prefxml.writePrefsToXMLFile( SL_CONF_FILE, opt, pref )

	return


def createChownDir( dir, username, groupname ):

	# Retrieve the pwent for mkdir
	pwent = verifyUser( username )

	if not os.path.isdir( dir ):
		print "\n+ Creating %s" % dir
		if not util.softMkDir( dir, pwent, parents=True ):
			return False
	else:
		print

	print "+ Chown'ing '%s' to %s:%s" % (dir, username, groupname)
	ret = os.system( "chown -R %s:%s %s" % ( username, groupname, dir ) )
	if ret != 0:
		log.warning("unable to recursively chown '%s' to user: %s, group: %s\n",
					dir, username, groupname )
		return False

	return True


def setSecurityContextType( file, type ):

	se = selinux

	# SELinux enforcing modes.
	SELINUX_ENFORCE_MODE_ENFORCING = 1
	#SELINUX_ENFORCE_MODE_PERMISSIVE = 0
	#SELINUX_ENFORCE_MODE_DISABLED = -1

	if se.is_selinux_enabled():
	
		if se.security_getenforce() == SELINUX_ENFORCE_MODE_ENFORCING:

			# Security context is in the format user:role:type
			sep = ":"
			context = se.getfilecon( file )[1]
			elements = context.split( sep )
			
			if elements[2] == "nfs_t":
				log.error(
						"Unable to set security context '%s' to file '%s'. "
						"Security context unable to be set on network "
						"drive.", type, file )
				return False
			else:
				elements[2] = type

			context = sep.join( elements )

			print "\n+ Chcon'ing '%s' type to %s" % (file, context)
			res = se.setfilecon( file, context )
			if res != 0:
				log.warning( "Unable to set security context '%s' to file: "
							 "%s.\n", type, file )
				return False

	return True


def install():

	username = ""
	groupname = ""

	if os.getuid() != 0:
		log.error( "You must run this script as root" )
		sys.exit( 1 )

	print "\nBigWorld Server Tools Installation"
	print "\nNOTE: This installation program will not copy binaries into your"
	print "      system paths, the system startup scripts and configuration"
	print "      files are used to reference back into the current tools"
	print "      directory. If you intend on moving the tools you will need"
	print "      to re-run this script or manually update your configuration"
	print "      files.\n"


	print "* Database Configuration"
	databaseInstall()


	print "\n"
	print "* Server Tools Installation\n"
	shouldQuery = True

	# If an existing configuration file exists, give the user the option
	# of using the settings already stored in that file.
	if os.path.isfile( CONF_FILE ):
		shouldQuery = not promptYesNo(
					"Would you like to use existing %s ?\n" % CONF_FILE )
		print

	# The tools directory where this script is running from
	tooldir = re.sub( "\/install\/?$", "",
					os.path.abspath( os.path.dirname(sys.argv[0]) ) )


	# What user / group is the installation script running as
	st = os.stat( os.path.abspath( sys.argv[0] ) )
	st.st_uid
	st.st_gid

	username = pwd.getpwuid( st.st_uid ).pw_name
	groupname = grp.getgrgid( st.st_gid ).gr_name


	if not shouldQuery:
		# If the user chose to use the installed bigworld.conf file
		# confirm that all the information required exists so the
		# init.d scripts will run.

		toolsdict = readConf( TOOLS_SECTION )

		# Check the user
		if toolsdict.has_key( "username" ):
			if username != toolsdict[ 'username' ]:
				print "Username ('%s') in '%s' doesn't match the owner of the " \
					  "install script ('%s').\n" % ( toolsdict[ 'username' ], \
					  CONF_FILE, username )

				if not promptYesNo( "Should the username '%s' be used?\n" % username ):
					print "\nPlease change the owner of the tools directory to reflect the"
					print "desired user:group ownership. Refer to the server installation"
					print "guide for assistance."
					sys.exit( 1 )

		else:
			print "\nNo 'username' entry exists in '%s'." % CONF_FILE
			print "+ Setting username to default: '%s'\n" % username


		# Check the group
		if toolsdict.has_key( "groupname" ):
			if groupname != toolsdict[ 'groupname' ]:
				print "Groupname ('%s') in '%s' doesn't match the group owner of the " \
					  "install script ('%s').\n" % ( toolsdict[ 'groupname' ], \
					  CONF_FILE, groupname )

				if not promptYesNo( "Should the groupname '%s' be used?\n" % groupname ):
					print "\nPlease change the group owner of the tools directory to reflect"
					print "the desired user:group ownership. Refer to the server installation"
					print "guide for assistance."
					sys.exit( 1 )

		else:
			print "\nNo 'groupname' entry exists in '%s'." % CONF_FILE
			print "+ Setting groupname to default: '%s'\n" % groupname


		# Check the script is running from within the tooldir specified
		# in the /etc/bigworld.conf
		if tooldir != toolsdict[ 'location' ]:
			print "Tools directory in '%s' differs from location of this " % CONF_FILE
			print "installation script. Please correct and re-run the installation"
			sys.exit( 1 )

		verifyToolsDir( tooldir )

		## Now the message_logger log directory
		mldict = readConf( ML_SECTION, ML_CONF_FILE )
		if mldict.has_key( "logdir" ):
			logdir = mldict[ 'logdir' ]
		else:
			print "\nNo 'logdir' entry exists in '%s'." % ML_CONF_FILE
			print "+ Setting log directory to default: '%s'\n" % LOG_DIR
			logdir = LOG_DIR

		## message_logger archive file location
		if mldict.has_key( "default_archive" ):
			mlarchive = mldict[ 'default_archive' ]
		else:
			print "\nNo 'default_archive' entry exists in '%s'." % ML_CONF_FILE
			print "+ Setting archive location to default: '%s'\n" % ML_ARCHIVES
			mlarchive = ML_ARCHIVES


		# Check the PID Directory
		if toolsdict.has_key( "piddir" ):
			piddir = toolsdict[ 'piddir' ]
		else:
			print "\nNo 'pid' entry exists in '%s'." % CONF_FILE
			print "+ Setting pid directory to default: '%s'\n" % PID_DIR
			piddir = PID_DIR


		# If the details from the configuration file haven't been
		# confirmed, allow the user to re-input the information
		if not confirmDetails( tooldir, logdir, mlarchive, piddir, username ):
			shouldQuery = True




	# If we still need to query the user, ask all the questions we need
	# to, and then write out the results to /etc/bigworld.conf
	if shouldQuery:

		confirmUserDetails( username, groupname )

		(logdir, mlarchive, piddir) = queryUser( tooldir, username )

	#################
	updateConf( TOOLS_SECTION, "username", username )
	updateConf( TOOLS_SECTION, "groupname", groupname )
	updateConf( TOOLS_SECTION, "location", tooldir )
	updateConf( TOOLS_SECTION, "piddir", piddir )

	updateConf( ML_SECTION, "logdir", logdir, ML_CONF_FILE )
	updateConf( ML_SECTION, "default_archive", mlarchive, ML_CONF_FILE )

	# We now have all the appropriate data needed to perform the installation

	# Create and gain ownership of any directories we need
	if not createChownDir( LOG_OUTPUT_DIR, username, groupname ) or \
	   not createChownDir( piddir, username, groupname ) or \
	   not createChownDir( logdir, username, groupname ):
		sys.exit( 1 )

	# Set type in shared lib's security context
	if not setSecurityContextType( BWLOG_SO, "textrel_shlib_t" ):
		sys.exit( 1 )

	# Write out the new message logger logrotate script
	print "\n+ Writing message_logger logrotate script to %s" % ML_LOGROTATE
	if os.path.isfile( ML_LOGROTATE ):
		print "\n%s already exists, this step will overwrite this file." \
			% ML_LOGROTATE
		if not promptYesNo( "Continue?" ):
			sys.exit( 1 )

	logSamplefh = open( ML_LOGROTATE_SAMPLE, "r")
	logRotatefh = open( ML_LOGROTATE, "w")

	cmdLogRotateInit = None
	cmdAppend = False
	for i in logSamplefh:
		i = re.sub("<default_archive>", mlarchive, i)
		i = re.sub("<message_logger_tools_dir>", "%s/message_logger" % tooldir, i)
		i = re.sub("<logdir>", logdir, i)

		# If we see 'lastaction' start storing all subsequent lines for
		# execution up until we see the 'endscript' statement.
		if re.search("endscript$", i):
			cmdAppend = False
		elif cmdAppend:
			if cmdLogRotateInit:
				cmdLogRotateInit += "; %s" % i.strip()
			else:
				cmdLogRotateInit = i.strip()
		elif re.search("lastaction$", i):
			cmdAppend = True

		logRotatefh.write(i)

	logRotatefh.close()
	logSamplefh.close()

	# Write out the server tools logrotate script
	print "\n+ Writing server tools logrotate script to %s" % ST_LOGROTATE
	if os.path.isfile( ST_LOGROTATE ):
		print "\n%s already exists, this step will overwrite this file." \
			% ST_LOGROTATE
		if not promptYesNo( "Continue?" ):
			sys.exit( 1 )

	# Copy it now
	try:
		shutil.copy( ST_LOGROTATE_ORIG, ST_LOGROTATE )
	except:
		print "ERROR: Unable to copy '%s' to '%s'." % \
			( ST_LOGROTATE_ORIG, ST_LOGROTATE )
		(type, value, trace) = sys.exc_info()
		print value

	print "\n\n* Installing system services"

	try:
		os.remove( FUNCTIONS[0] )
	except:
		pass

	# Install the common bw functions
	src = os.path.abspath( FUNCTIONS[1] )
	dest = FUNCTIONS[0]
	# Copy the bw_functions file into /etc/init.d now
	try:
		shutil.copy( src, dest )
	except:
		print "ERROR: Unable to copy '%s' to '%s'." % ( src, dest )
		(type, value, trace) = sys.exc_info()
		print value

	# Install the init.d scripts and start the services
	for item in SERVICES:
		installService( item[0], item[1] )

	# Finally initialise the message logger archives so that log
	# rotate doesn't fall over with its first run
	if cmdLogRotateInit and not os.path.isfile( mlarchive ):
		print "\n+ Initialising message_logger archives"
		os.system( cmdLogRotateInit )

	return 0


def uninstall():

	if os.getuid() != 0:
		log.error( "In order to uninstall, you must run this script as root" )
		sys.exit( 1 )

	revServices = SERVICES
	revServices.reverse()

	print "* Uninstalling system services\n"
	# Shutdown each service and remove all files from /etc/*
	for item in revServices:
		uninstallService( item[0] )

	if not os.path.isfile( CONF_FILE ):
		log.error( "No config file '%s' exists, unable to continue uninstallation", CONF_FILE )
		return False

	toolsdict = readConf( TOOLS_SECTION )

	print "* Removing directories"
	# Remove the PID directory if it exists
	if toolsdict.has_key( "piddir" ):
		piddir = toolsdict[ 'piddir' ]
		if os.path.isdir( piddir ):
			os.system( "rm -rf %s" % piddir )

	# Read the message logger configuration file
	# prompt user if they want to delete the logs
	## Now the message_logger log directory
	mldict = readConf( ML_SECTION, ML_CONF_FILE )
	if mldict.has_key( "logdir" ):
		logdir = mldict[ 'logdir' ]
		if os.path.isdir( logdir ):
			if promptYesNo( "Do you want to delete the log directory '%s'?\n" % logdir, False ):
				os.system( "rm -rf %s" % logdir )


	print "* Removing logrotation scripts"
	try:
		if os.path.isfile( ST_LOGROTATE ):
			print "- unlinking %s" % ST_LOGROTATE
			os.unlink( ST_LOGROTATE )
	except:
		log.error( "Unable to remove %s", ST_LOGROTATE )
	try:
		if os.path.isfile( ML_LOGROTATE ):
			print "- unlinking %s" % ML_LOGROTATE
			os.unlink( ML_LOGROTATE )
	except:
		log.error( "Unable to remove %s", ML_LOGROTATE )



	print "\n* Removing '%s'" % CONF_FILE
	try:
		os.unlink( CONF_FILE )
	except Exception, e:
		log.error( "Couldn't unlink global conf file %s: %s", CONF_FILE, e )
		return False

	return True


def verify():

	# Pretty primitive for now
	try:
		items = readConf( TOOLS_SECTION )
	except ConfigParser.NoSectionError, e:
		log.error( "Config file doesn't exist or is corrupt: %s", e )
		return None

	try:
		pwent = pwd.getpwnam( items[ "username" ] )
	except KeyError:
		log.error( "BigWorld user '%s' doesn't exist on this system",
				   items[ "username" ] )
		return None

	if items.has_key( "piddir" ):
		piddir = items[ 'piddir' ]
		if not os.path.isdir( piddir ):
			log.error( "PID directory '%s' doesn't exist" % piddir )
	else:
		log.error( "No 'piddir' entry in '%s'" % CONF_FILE )


	## Now the message_logger log directory
	mldict = readConf( ML_SECTION, ML_CONF_FILE )
	if mldict.has_key( "logdir" ):
		logdir = mldict[ 'logdir' ]
		if not os.path.isdir( logdir ):
			log.error( "log directory '%s' doesn't exist" % logdir )
	else:
		log.error( "No 'logdir' entry in '%s'" % os.path.abspath( ML_CONF_FILE ))

	return items


def main():

	opt = optparse.OptionParser()
	opt.add_option( "-i", "--install", action = "store_true",
					help = "Set up the user environment for running " \
					"BigWorld daemons" )
	opt.add_option( "-u", "--uninstall", action = "store_true",
					help = "Remove the user environment for running " \
					"BigWorld daemons" )
	opt.add_option( "-c", "--config",
					help = "Set section,key,value into %s" % CONF_FILE )
	opt.add_option( "-v", "--verify", action = "store_true",
					help = "Verify the environment is set up correctly" )
	options, args = opt.parse_args()

	if options.verify:
		return not verify()

	elif options.uninstall:
		return not uninstall()

	elif options.config:
		section, key, value = options.config.split( "," )
		return not updateConf( section, key, value )

	else:
		# Handle the user Ctrl-C'ing out
		try:
			sys.exit( install() )
		except KeyboardInterrupt:
			print ""
			return -1

if __name__ == "__main__":
	sys.exit( main() )
