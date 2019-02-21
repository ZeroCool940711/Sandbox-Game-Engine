#!/usr/bin/env python
"""
This utility takes a snapshot of the primary and secondary databases.
These databases are optionally consolidated. Messages are sent
to message_logger instead of stdout/stderr.
"""

import sys
import os
import shutil
import time
import datetime
import socket
import tempfile
import MySQLdb
import optparse

# Import pycommon modules
import bwsetup
bwsetup.addPath( ".." )

from pycommon import bwconfig
from pycommon import cluster
from pycommon import uid as uidmodule
from pycommon import log

# Snapshots are achived in directories named in this time format
ARCHIVE_STR_F_TIME = "%Y-%m-%d_%H:%M:%S"

LOCK_FILE = ".lock"

SECONDARY_DB_FILE_LIST = "secondarydb.list"


# ------------------------------------------------------------------------------
# Section: Main
# ------------------------------------------------------------------------------

USAGE = "usage: %prog [options] SNAPSHOT_DIR"

OPTIONS = [	( ["-b", "--bwlimit-kbps"], {"type": "int",
			  	"help": "file transfer bandwidth limit in kbps, default is unlimited"} ),
			( ["-n", "--no-consolidate"], {"action": "store_true",
			  	"help": "skip consolidation, default is false"} ) ]

def main():
	# Parse command line
	parser = optparse.OptionParser( USAGE )
	for switches, kwargs in OPTIONS:
		parser.add_option( *switches, **kwargs )
	(options, args) = parser.parse_args()

	# Check snapshot directory arg
	if not args:
		parser.error( "missing argument" )

	snapshotDir = os.path.abspath( args[0] )
	if not os.access( snapshotDir, os.F_OK ):
		parser.error( "directory %s is non-existent" % snapshotDir )

	# Set options
	bwLimit = "0"
	if options.bwlimit_kbps:
		bwLimit = str( options.bwlimit_kbps )

	shouldConsolidate = True
	if options.no_consolidate:
		shouldConsolidate = not options.no_consolidate

	c = cluster.Cluster()
	me = c.getUser( uidmodule.getuid() )

	# Cannot take simultaneous snapshots
	error = lock( snapshotDir + os.sep + LOCK_FILE )
	if error == None:
		try:
			error =	snapshot( c, me, snapshotDir, bwLimit, shouldConsolidate )
		finally:
			unlock( snapshotDir + os.sep + LOCK_FILE )

	if error:
		me.log( "Snapshot: %s" % error, "ERROR" )
		sys.exit( -1 )
	else:
		sys.exit( 0 )


# ------------------------------------------------------------------------------
# Section: Snapshot
# ------------------------------------------------------------------------------

def snapshot( cluster, me, snapshotDir, bwLimit, shouldConsolidate ):
	startTime = time.time()

	if not me.serverIsRunning():
		return "Server is not running"

	# Not using /tmp as it may not be on the same drive
	tempDir = snapshotDir + os.sep + "temp"
	if not os.path.exists( tempDir ):
		os.mkdir( tempDir )

	(error, secDBs, secDBFileList) = requestSecondaryDBs( cluster, me, tempDir,
															bwLimit )
	if error:
		return error

	(error, priDB) = requestPrimaryDB( cluster, me, tempDir, bwLimit )
	if error:
		return error

	# Incomming databases
	dbs = [priDB]
	dbs.extend( secDBs )

	waitForDBs( dbs )

	filesToArchive = []
	filesToArchive.append( priDB )

	isConsolidated = shouldConsolidate

	if shouldConsolidate:
		error = consolidate( dbs, secDBFileList )
		if error:
			me.log( "Snapshot: %s" % error, "ERROR" )
			isConsolidated = False

	if not isConsolidated:
		filesToArchive.extend( secDBs )

	os.remove( secDBFileList )

	archiveDir = archive( filesToArchive, snapshotDir )

	endTime = time.time() - startTime

	me.log( "Snapshot: Created %s in %s seconds (consolidated=%s)" % \
			(archiveDir, int( endTime ), isConsolidated) )


# ------------------------------------------------------------------------------
# Section: Helpers
# ------------------------------------------------------------------------------

def lock( lockFile ):
	if os.access( lockFile, os.F_OK ):
		return "A snapshot is still in progress: %s" % lockFile 
	else:
		file = os.open( lockFile, 755 )
		os.close( file )
		return None


def unlock( lockFile ):
	os.remove( lockFile )


def findDBMgr( me ):
	p = me.getProc( "dbmgr" )
	if p:
		return p.machine.ip
	else:
	 	return None


def requestSecondaryDBs( cluster, me, snapshotDir, bwLimit ):
	dbHost = bwconfig.get( "dbMgr/host" )
	dbUser = bwconfig.get( "dbMgr/username" )
	dbPass = bwconfig.get( "dbMgr/password" )
	dbName = bwconfig.get( "dbMgr/databaseName" )

	# It is only localhost for dbMgr
	if dbHost == "localhost":
		dbHost = findDBMgr( me )
		if dbHost == None:
			return ( "Could not find MySQL server localhost to DBMgr", [], None )

	# Read secondary databases info
	try:
		conn = MySQLdb.connect( host = dbHost,
								user = dbUser,
								passwd = dbPass,
								db = dbName )

		cursor = conn.cursor()
		cursor.execute( "SELECT INET_NTOA(ip), location FROM "	\
			   			"bigworldSecondaryDatabases" )
		results = cursor.fetchall()
		cursor.close()
		conn.close()
	except:
		return ( "Could not read secondary database info from primary "	\
			   "database: %s" % dbHost, [], None )

	secDBs = []

	args = [ "snapshotsecondary",
			 "filenamePlaceHolder",
			 socket.gethostname(),
			 snapshotDir,
			 bwLimit ]

	# Get SQLite files via transfer_db
	for ip, remotePath in results:
		m = cluster.getMachine( ip )
		if not m:
			return ( "Could not find %s to request %s" % (ip, remotePath), [], None )

		args[1] = remotePath

		if m.startProcWithArgs( "commands/transfer_db", args, me.uid ) == 0:
			return ( "Could not start transfer_db on %s" % ip, [], None )

		localPath = snapshotDir + os.sep + remotePath.split( os.sep )[-1]
		secDBs.append( localPath )

	# Used as a consolidate_dbs args
	secDBFileList = snapshotDir + os.sep + SECONDARY_DB_FILE_LIST

	file = open( secDBFileList, "w" )
	for db in secDBs:
		file.write( db + "\n" )
	file.close()

	return ( None, secDBs, secDBFileList )


def requestPrimaryDB( cluster, me, snapshotDir, bwLimit ):
	dbHost = bwconfig.get( "dbMgr/host" )
	dbUser = bwconfig.get( "dbMgr/username" )
	dbPass = bwconfig.get( "dbMgr/password" )
	dbName = bwconfig.get( "dbMgr/databaseName" )

	# It's only localhost for dbMgr
	if dbHost == "localhost":
		dbHost = findDBMgr( me )
		if dbHost == None:
			return ( "Could not resolve localhost to DBMgr machine", None )

	m = cluster.getMachine( dbHost )

	args = ["snapshotprimary",
			socket.gethostname(),
			snapshotDir,
			bwLimit]

	if m.startProcWithArgs( "commands/transfer_db", args, me.uid ) == 0:
		return ( "Could not start transfer_db on %s" % dbHost, None )

	localPath = snapshotDir + os.sep + "mysql"

	return ( None, localPath )


def waitForDBs( dbs ):
	# transfer_db must send this file last
	indicators = ["%s.complete" % db for db in dbs]

	while indicators:
		time.sleep( 2 )

		for i in indicators:
			if os.access( i, os.F_OK ):
				os.remove( i )
				indicators.remove( i )



def consolidate( dbs, secDBFileList ):
	dbUser = bwconfig.get( "dbMgr/username" )
	dbPass = bwconfig.get( "dbMgr/password" )
	dbName = bwconfig.get( "dbMgr/databaseName" )
	dbSock = "%s/mysql.sock" % dbs[0]
	dbDataDir = dbs[0]

	# Pick a free port
	s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
	s.bind( ("127.0.0.1", 0) )
	dbPort = s.getsockname()[1]
	s.close()

	# Create temp MySQL config file
	mycnf = tempfile.mkstemp()[1]
	file = open( mycnf, "w" )
	file.writelines( ["[mysqld1]\n",
					"datadir=%s\n" % dbDataDir,
					"socket=%s\n" % dbSock,
					"port=%s\n" % dbPort] )
	file.close()

	# Start a MySQL server
	cmd = "mysqld_multi --defaults-file=%s start 1 &> /dev/null" % mycnf
	if os.system( cmd ) != 0:
		return "Could not start MySQL server for consolidation "	\
				"(host = localhost, port = %s, dataDir = %s)" %		\
				( dbPort, dbDataDir )

	# Wait for MySQL server to ramp up
	# TODO: Currently tests forever
	while (True):
		try:
			conn = MySQLdb.connect( user = dbUser,
									passwd = dbPass,
									db = dbName,
									unix_socket = dbSock )
			break
		except:
			time.sleep( 1 )

	# Consolidate
	consolidateDBsCmd = os.path.dirname( os.path.abspath( __file__ ) ) +	\
					"/../../../bin/Hybrid/commands/consolidate_dbs"
	primaryDBArg = "%s;%s;%s;%s;%s" %							\
		( socket.gethostname(), dbPort, dbUser, dbPass, dbName )
	cmdConsolidate = "%s '%s' '%s' &> /dev/null" %	\
		( consolidateDBsCmd, primaryDBArg, secDBFileList )

	isConsolidateOK = os.system( cmdConsolidate ) == 0

	# Deregister secondary databases
	if isConsolidateOK:
		cursor = conn.cursor()
		cursor.execute( "DELETE FROM bigworldSecondaryDatabases" )
		cursor.close()
		conn.commit()

	conn.close()

	# Stop server
	cmdStop = "mysqld_multi --defaults-file=%s stop 1 &> /dev/null" % mycnf
	isStopOK = os.system( cmdStop ) == 0

	# Wait for MySQL server to ramp down
	# TODO: Currently tests forever
	while (True):
		try:
			conn = MySQLdb.connect( user = dbUser,
									passwd = dbPass,
									db = dbName,
									unix_socket = dbSock )
			conn.close()
			time.sleep( 1 )
		except:
			break

	if not isConsolidateOK:
		return "Could not consolidate [%s]" % cmdConsolidate

	# Remove secondary databsses
	for db in dbs[1:]:
		os.remove( db )
		dbs.remove( db )

	if not isStopOK:
		return "Could not stop MySQL server for consolidation "	\
				"(host=localhost, port=%s, dataDir=%s)" %		\
				( dbPort, dbDataDir )

	return None


def archive( dbs, snapshotDir ):
	dt = datetime.datetime.now()
	archiveDir = snapshotDir + os.sep + dt.strftime( ARCHIVE_STR_F_TIME )
	os.mkdir( archiveDir )

	for db in dbs:
		dst = archiveDir + os.sep + db.split( os.sep )[-1]

		if os.path.isfile( db ):
			shutil.copy( db, dst )
		else:
			shutil.copytree( db, dst )

	return archiveDir


if __name__ == "__main__":
	sys.exit( main() )
