import operator
import collections
import itertools
import threading
import time
import sys
import Queue
import logging
import traceback
import math

# Import pycommon modules
import bwsetup
bwsetup.addPath( ".." )
from pycommon import messages, cluster

# Logging module
log = logging.getLogger( "stat_logger" )

import utilities
import constants
import MySQLdb

class Gather( threading.Thread ):
	""" Thread for gathering statistics.  """
	FINISH = 0
	START_DEBUG = 1

	def __init__( self, options, usingFileOutput, prefTree,
			dbManager ):
		threading.Thread.__init__( self, name="Gather" )

		# Set to true if profiling Gather thread
		self.profiling = False

		# Update interval, pulled from options file
		self.sampleTickInterval = options.sampleTickInterval

		# Don't print the tick rotater if we're logging to a file
		self.usingFileOutput = usingFileOutput

		# Cluster object (from pycommon) which does the heavyweight work
		if options.uid:
			self.cluster = cluster.Cluster( uid = options.uid )
		else:
			self.cluster = cluster.Cluster()
		# Flush cluster information in periodically
		self.reflushClusterPeriodSecs = 3600
		# The last time flush cluster information
		self.lastReflushClusterTime = time.time()

		#  Message queue for handling messages from the main thread
		self.messageQueue = Queue.Queue()

		# Machine statistics: [cluster.Machine object] -> [Statistics]
		self.machineStats = {}
		self.missingMachines = set()

		# Machine statistics: [cluster.Process object] -> [Statistics]
		self.processStats = {}
		self.missingProcesses = set()

		# Process database ids (Machines use ip integer as their id)
		self.dbIds = {}

		# Set of processes that aren't responding to watcher queries
		self.deadProcs = {}

		# Set of cluster.User objects
		self.userStats = set()

		# Some statPrefs require lambdas to be created
		self.compiledStatFuncs = {}
		self.dbManager = dbManager

		# Preference tree containing all preferences (important!)
		self.prefTree = prefTree

		# Map of ProcPref.matchtext -> {ProcPref -> watcherValue}
		self.watcherStatPrefs = self.getWatcherStatPrefs( prefTree )

		# Map of cluster.Process.name -> ProcPref
		self.nameToProcPref = \
			dict( (pref.matchtext, pref) for pref in prefTree.iterProcPrefs() )

		# Daemon log mark output period. Mark every hour.
		self.markPeriodSecs = 3600
		# The last time we printed a mark
		self.lastMarkOutputTime = time.time()

		# Warning thresholds (go above these and we start printing warnings)
		self.warnDumpQueueSize = 500
		self.warnSQLTime = 3.0

		# Guard against having the computer clock set backwards
		self.latestKnownTime = None

		# Set when we need to terminate quickly, and ignore queued SQL 
		# statements
		self.quickTerminate = False

		# This is the time at which we want the current tick to have
		# completed by.
		# NB: The initialisation is done elsewhere, added here for reference
		#self.desiredTickTime = time.time()

		# Rotating characters
		self.rotateChars = "|/-\\"
		self.rotatePos = 0

		# Sleep intervals
		self.sleepTime = 0.1

	class FinishException(Exception):
		pass

	# ------------------------------------------------------
	# Section: exposed communication methods
	# ------------------------------------------------------
	def pushFinish( self, quick=False ):
		"""
		Send a finish signal to this gather thread.

		@param quick: True if we should stop immediately upon receiving
			the signal (does not write pending insert statements to the
			database)
		"""
		self.messageQueue.put( (self.FINISH, quick) )

	def pushDebug( self ):
		"""
		Send a signal to the gather thread to start the debugger
		(requires winpdb).
		"""
		self.messageQueue.put( (self.START_DEBUG,None) )

	# ------------------------------------------------------
	# Section: Internal general methods
	# ------------------------------------------------------
	def getWatcherStatPrefs( self, prefTree ):
		""" Determine which statistic preferences are watcher preferences """
		watcherStatPrefs = {}
		for procPref in prefTree.iterProcPrefs():
			# Index by matchtext, which is supposed to match the value returned
			# by cluster.Process.name.
			# This means if we have a cluster.Process object, we can
			# back reference from it to the watcherStatPref.
			watcherStatPrefs[procPref.matchtext] = dict( (s, s.valueAt[1:]) \
				for s in procPref.iterAllStatPrefs() \
					if self.isWatcherPath(s.valueAt) )

		return watcherStatPrefs


	def run( self ):
		""" Thread entry point """
		if self.profiling:
			import cProfile
			prof = cProfile.Profile()
			prof.runcall( self.runGather )
			prof.dump_stats( "stat_logger.prof" )
			log.debug( "Dumped profile statistics to stat_logger.prof" )
		else:
			self.runGather()
		log.info( "Gather thread terminating normally" )


	def runGather( self ):
		""" Handle the main gather loop """
		self.statDumper = StatDumper( self.dbManager.cloneWithNewConnection() )
		self.statDumper.start()
		self.latestKnownTime = self.retrieveLatestTime()

		# Retrieve the next tick id and the time at which it should start
		self.tick, self.desiredTickTime = self.calculateStartTick()

		# If this isn't the start of the log, wait a bit so that our ticks
		# are in sync (in order to maintain regular intervals between ticks)
		if self.tick > 1:
			time.sleep( self.desiredTickTime - time.time() )

		# Delete old data. Old data may span multiple ticks, and thus the 
		# amount data to delete will vary.  
		self.deleteOldData()

		log.info( "StatLogger ready to collect data." )

		try:
			# Start the gather loop!
			while True:
				self.tickTime = time.time()
				if self.checkSystemClock() == False:
					# Our system clock has gone backwards?
					log.info( "Going to stop collecting data until clock " \
						"has caught up to previous maximum time. This is " \
						"about %.3fs.", time.time() - self.latestKnownTime )
					self.waitUntilTime( self.latestKnownTime )
					log.info( "Catchup complete, resuming data collection." )
					self.calculateNextTick()
					self.waitUntilTime( self.desiredTickTime )
					continue

				# Check whether need to flush cluster information
				if self.tickTime - self.lastReflushClusterTime >= self.reflushClusterPeriodSecs:
					self.cluster.refresh()
					self.lastReflushClusterTime = self.tickTime

				self.latestKnownTime = self.tickTime
				self.dbManager.addTickToDb( self.tick, self.tickTime )
				# Most important step: Collect and log data to the database
				self.collectData()
				self.consolidateStats()
				self.handlePeriodicOutput()

				self.checkStatDumper()
				while not self.messageQueue.empty():
					self.processMessage( self.messageQueue.get(0) )

				self.calculateNextTick()
				self.waitUntilTime( self.desiredTickTime )
		except self.FinishException:
			pass

		self.statDumper.pushFinish( self.quickTerminate )
		log.debug("Waiting for StatDumper to terminate...")
		self.statDumper.join(60.0)


	def retrieveLatestTime( self ):
		""" Grab the last tick out of the database """
		cursor = self.dbManager.getLogDbCursor()
		cursor.execute( "SELECT id, time FROM %s "\
			"ORDER BY time DESC LIMIT 1" % constants.tblStdTickTimes )
		if cursor.rowcount > 0:
			lastTick, lastTime = cursor.fetchone()
			lastTime = float( lastTime ) / 1000
		else:
			lastTime = None
		cursor.close()
		return lastTime


	def calculateStartTick( self ):
		"""
		Calculate which tick we start gathering on and what timestamp
		that tick corresponds to
		"""
		results = self.dbManager.getFirstTick()
		if results == None:
			nextTick = (1, time.time())
		else:
			firstTick, firstTickTime = results
			timeElapsed = time.time() - firstTickTime
			numTicks = timeElapsed/self.sampleTickInterval
			nextTickId = firstTick + math.ceil( numTicks )
			nextTickTime = firstTickTime + \
				(float(nextTickId) * self.sampleTickInterval)
			nextTick = (nextTickId, nextTickTime)
		return nextTick

	
	def deleteOldData( self ):
		""" Delete old data that may span multiple ticks.  The amount of data to
			delete varies.  This is performed only on StatLogger startup. """
		log.info( "Performing maintenance task...This may take a while." )	
		# Uses deletion code in consolidateStats method.  No consolidation is
		# performed.
		self.consolidateStats( False )
		log.info( "Finished performing maintenance task." )

	def checkSystemClock( self ):
		""" Check that the system clock hasn't been set backwards """
		if self.tickTime < self.latestKnownTime:
			log.warning( "Current time %s is lower than maximum recorded " \
				"time of %s (difference of %.3fs)",
				time.ctime( self.tickTime ),
				time.ctime( self.latestKnownTime ),
				self.latestKnownTime - self.tickTime )
			log.warning( "Has the computer's clock been set backwards?" )
			return False
		return True


	def waitUntilTime( self, targetTime):
		""" Process messages until the target time is reached """
		if time.time() > targetTime:
			return
		checkUntil = targetTime - 2 * self.sleepTime
		while time.time() < checkUntil:
			try:
				self.processMessage( self.messageQueue.get(0) )
			except Queue.Empty:
				pass
			time.sleep( self.sleepTime )
		timeRemaining = targetTime - time.time()
		if timeRemaining > 0:
			time.sleep( timeRemaining )
		else:
			log.warning( "We slept overtime by %.3fs :(", timeRemaining )


	def calculateNextTick( self ):
		""" Calculates when the next tick should start """
		self.tick += 1
		self.desiredTickTime += self.sampleTickInterval

		# If we've gone overtime...then "skip" ticks
		currTime = time.time()
		timeRemaining = self.desiredTickTime - currTime
		multiTimeCorrection = False

		while timeRemaining < 0:

			# Skip the number of ticks that we've missed
			ticksMissed = math.ceil(
				(-timeRemaining) / self.sampleTickInterval )
			self.tick += ticksMissed

			# Recalculate the new desired tick finish time based on the
			# number of ticks we're skipping
			self.desiredTickTime += ticksMissed * self.sampleTickInterval

			log.warning( "Last tick went overtime by %fs. " \
				"Waiting %fs for tick %d (skipping %d)",
				-timeRemaining, self.desiredTickTime - currTime, self.tick,
				ticksMissed )

			# Make sure the generated state to progress to is valid
			timeRemaining = self.desiredTickTime - currTime
			if (timeRemaining < 0):
				log.error( "Failed to recover from exceeding previous tick " \
					"time allocation." )

				if multiTimeCorrection == True:
					log.error( "Failed to correct time sync. Terminating." )
					raise self.FinishException()

				multiTimeCorrection = True



	def processMessage( self, messageTuple ):
		""" Handles messages passed to us from the main thread """
		msg, params = messageTuple
		if msg == self.START_DEBUG:
			import rpdb2
			pauseTime = 15
			log.info( "Waiting %ds for debugger to connect", pauseTime )
			rpdb2.start_embedded_debugger(
				"abcd",
				fAllowUnencrypted = True,
				fAllowRemote = True,
				timeout = pauseTime,
				fDebug = False )
		elif msg == self.FINISH:
			log.info( "Stopping data collection (Current tick is %i)." \
				% self.tick )
			self.quickTerminate = params
			raise self.FinishException()


	def handlePeriodicOutput( self ):
		""" Handles output that we're expected to make at regular intervals """
		if self.usingFileOutput:
			if time.time() - self.lastMarkOutputTime > self.markPeriodSecs:
				log.info( "-- MARK --" )
				self.lastMarkOutputTime = time.time()
		else:
			# Assume stdout output
			sys.stdout.write( "%c\r" % (self.rotateChars[self.rotatePos]) )
			sys.stdout.flush()
			self.rotatePos = (self.rotatePos + 1) % len( self.rotateChars )


	def checkStatDumper( self ):
		""" Check StatDumper's status (if it's dead) """
		if not self.statDumper.isAlive():
			log.error( "Error: StatDumper thread died unexpectedly, "\
				"stopping Gather thread." )
			raise self.FinishException()

		# Note: Following two lines are unthreadsafe, I'm relying on Python's
		# global interpreter lock here
		queueSize = len(self.statDumper.generalQueue)
		delQueueSize = len(self.statDumper.deleteQueue)
		if queueSize > self.warnDumpQueueSize:
			log.warning( "Queue size is larger than %d (now %d)",
				self.warnDumpQueueSize, queueSize )
		queryInfo = self.statDumper.getCurrentQueryInfo()
		if queryInfo and queryInfo[1] > self.warnSQLTime:
			log.warning( "Currently stuck on query: %s (taken %.3fs so far)",
				queryInfo[0], queryInfo[1] )
		return True


	def isWatcherPath( self, value ):
		return value[0] == "/"


	# ------------------------------------------------------
	# Section: Statistic collection methods
	# ------------------------------------------------------
	def collectData( self ):
		"""
		Collects data from machines and process on the network,
		and logs them to the database.
		"""
		# Refresh cluster, this is the most important operation for 
		# collecting statistics
		try:
			self.cluster.refresh( 1 )
		except:
			log.error( traceback.format_exc() )

		# Extracts statistics from cluster
		self.extractMachineStats()
		self.extractProcessStats()
		self.extractUserStats()
		self.checkDeadProcessesAndMachines()
		self.logProcessStats()
		self.logMachineStats()


	def extractUserStats(self):
		""" Manage users detected on the network """
		users = self.cluster.getUsers()
		for u in users:
			if u not in self.userStats:
				self.logNewUser(u)


	def extractMachineStats( self ):
		""" Manage stats from machines detected on the network """
		machines = self.cluster.getMachines()
		lastKnownMachines = set(self.machineStats)
		for m in machines:
			if m not in lastKnownMachines:
				# Log a machine if it wasn't there before
				self.logNewMachine(m)
				self.machineStats[m] = {}
			else:
				lastKnownMachines.remove(m)
			assert (not self.machineStats[m])
			self.machineStats[m] = dict( (pref, self.applyStatFunc(m, pref) )
				for pref in self.prefTree.iterMachineStatPrefs() )

		# This is the set of machines that have disappeared from the network
		self.missingMachines = lastKnownMachines


	def extractProcessStats( self ):
		"""
		Retrieve stats for processes detected on the network.
		First get the stats which access attributes of the cluster.Process
		object, then after all that is done get the watcher stats.
		"""
		lastKnownProcs = set(self.processStats)
		availableProcs = (p for p in self.cluster.getProcs() if \
			p.name in self.nameToProcPref)
		for p in availableProcs:
			procPref = self.nameToProcPref[p.name]
			if p not in lastKnownProcs:
				self.logNewProcess(p)
				self.processStats[p] = {}
			else:
				lastKnownProcs.remove(p)
			assert (not self.processStats[p])

			# Retrieve non-watcher stats for the process
			nonWatcherPrefs = (pref for pref in procPref.iterAllStatPrefs() \
				if not self.isWatcherPath( pref.valueAt ))

			for pref in nonWatcherPrefs:
				self.processStats[p][pref] = self.applyStatFunc( p, pref )

		# Now, get watcher values for all processes
		self.retrieveWatcherValues()

		# This is the set of processes that have disappeared from the network
		self.missingProcesses = lastKnownProcs


	def applyStatFunc( self, obj, statPref ):
		"""
		Applies the function specified in the valueAt attribute of each
		statistic preference to the actual cluster.Process or cluster.Machine
		object.
		"""
		try:
			func = self.compiledStatFuncs[statPref.valueAt]
		except KeyError:
			func = eval("lambda m: m." + statPref.valueAt)
			self.compiledStatFuncs[statPref.valueAt] = func

		try:
			result = func( obj )
		except IndexError:
			return 0.0
		except KeyError:
			return 0.0
		return result


	def retrieveWatcherValues( self ):
		"""
		Retrieves watcher values for all processes that we've detected
		on the network. This is a blocking operation, and can take some time.
		"""
		# Create dictionary of {processType -> [processes])
		getName = operator.attrgetter('name')
		processes = sorted( (p for p in self.cluster.getProcs() \
			if p.name in self.nameToProcPref), key=getName )
		groupedProcesses = itertools.groupby( processes, getName )

		# The number of times to attempt to query a process when
		# it's not replying to watcher queries.
		DEAD_ATTEMPTS = 3

		# Iterate through all process types
		for procType, processList in groupedProcesses:
			watcherPaths = self.watcherStatPrefs[procType].values()

			if len(watcherPaths) == 0:
				continue

			# Check every process we are told about to see whether we
			# have already queried them before and determined they are
			# 'dead' or are talking an older protocol.
			newProcList = []
			for queryProc in processList:

				putInList = True
				for deadProc in self.deadProcs.keys():
					if (self.deadProcs[ deadProc ])[0] >= DEAD_ATTEMPTS:
						if (queryProc.id == deadProc.id) and \
							(queryProc.pid == deadProc.pid) and \
							(queryProc.uid == deadProc.uid) and \
							(queryProc.machine == deadProc.machine):

							# Every 5 minutes give the process another chance
							if (self.tickTime - (self.deadProcs[ deadProc ])[1]) > 300:
								# Remove from the dead list
								log.info( "5 minute timeout for (%s on %s " \
									"pid:%s). Removing from dead list.",
									deadProc.name, deadProc.machine.name,
									deadProc.pid )
								self.deadProcs.pop( deadProc )

							putInList = False
							break

				# Put it in the list to query unless told otherwise
				if putInList:
					newProcList.append( queryProc ) 


			try:
				watcherResponse = messages.WatcherDataMessage.batchQuery(
					watcherPaths, newProcList, 0.5 )
			except Exception, e:
				log.error( "Watcher query failed. Dropping all current " \
					"statistic requests. Exception follows:\n" )
				log.error( str(e) )
				for msg in traceback.format_tb(sys.exc_info()[2]):
					log.error("%s", msg)

				continue


			# Process watcher response for processes in a process type
			for p, watcherReplyDict in watcherResponse.iteritems():
				if not len(watcherReplyDict):

					entry = None
					if p in self.deadProcs: 
						entry = self.deadProcs[ p ]
					else:
						entry = ( 0, self.tickTime )

					if entry[0] < DEAD_ATTEMPTS:
						self.deadProcs[ p ] = ( entry[0] + 1, entry[1] )

					# no need to do anything else with this process
					continue

				elif self.deadProcs.has_key( p ):
					# Remove from the dead list
					self.deadProcs.pop( p )
					log.info( "Removing %s on %s pid:%s from dead list, " \
						"responding again.", deadProc.name,
						deadProc.machine.name, deadProc.pid )


				for statPref, watcherPath in \
						self.watcherStatPrefs[p.name].iteritems():
					replies = watcherReplyDict.get(watcherPath,None)
					if replies:
						if len(replies) > 1:
							log.warning( "Watcher request %s to %s " \
								"resulted in multiple replies. Possible "\
								"delayed watcher response to earlier query. ",
								watcherPath, p)
						self.processStats[p][statPref] = replies[-1][1]
					else:
						self.processStats[p][statPref] = None


	def checkDeadProcessesAndMachines( self ):
		"""
		Handles processes and machines which have been lost
		(either shutdown, crashed, or lost network access)
		"""
		for machine in self.missingMachines:
			log.info( "Lost machine %s (%s)", machine.name, machine.ip )
			del self.machineStats[machine]

		for process in self.missingProcesses:
			try:
				user = self.cluster.getUser( process.uid ).name
			except cluster.User.error, e:
				log.warning(e)
				user = process.uid
			log.info( "Lost process %s(user:%s, pid:%d, host:%s)",
				process.label(), user, process.pid,
				process.machine.name )
			del self.processStats[process]
			del self.dbIds[process]

		self.missingProcesses.clear()
		self.missingMachines.clear()


	# ------------------------------------------------------
	# Section: Logging methods
	# ------------------------------------------------------

	def logNewMachine( self, machine ):
		""" Adds machine info to the database if it doesn't already exist """
		ipInt = utilities.ipToInt( machine.ip )
		c = self.dbManager.getLogDbCursor()
		# Seach the database first, and grab the name stored in the database
		c.execute( "SELECT ip, hostname FROM %s WHERE ip=%%s" %
				(constants.tblSeenMachines), ipInt )
		results = c.fetchone()
		if results:
			# We found a corresponding entry, compare the name
			name = results[1]
			if machine.name and name != machine.name:
				# Name is different, update name change in the database
				log.info( "Machine name %s is different to stored name %s! "\
					"Updating...", machine.name, name )
				c.execute( "UPDATE %s SET hostname=%%s WHERE ip=%%s" % \
					(constants.tblSeenMachines), (machine.name, ipInt) )
			else:
				log.info( "Retrieved machine %s (%s) from database",
					name, machine.ip )
		else:
			log.info( "Adding machine %s (%s)", machine.name, machine.ip )
			c.execute( "INSERT INTO %s (ip, hostname) VALUES (%%s, %%s)" % \
					(constants.tblSeenMachines), (ipInt, machine.name) )
		c.close()


	def logNewProcess( self, process ):
		""" Adds process info to the database if it doesn't already exist """
		if process.name not in self.nameToProcPref:
			return
		ipInt = utilities.ipToInt( process.machine.ip )
		procPref = self.nameToProcPref[process.name]
		c = self.dbManager.getLogDbCursor()
		c.execute("SELECT id FROM %s "
					"WHERE machine = %%s AND pid = %%s AND name = %%s" \
				% (constants.tblSeenProcesses),
			(ipInt, process.pid, process.label()) )
		results = c.fetchone()

		try:
			userName = self.cluster.getUser( process.uid ).name
		except cluster.User.error:
			userName = "<%d>" % process.uid

		if results:
			# Great! We already found our process then
			dbId = results[0]
			log.info( "Retrieved process %s (user:%s, pid:%d, host:%s) from " \
				"database (dbid %d)", process.label(), userName,
				process.pid, process.machine.name, dbId )
		else:
			c.execute( "INSERT INTO %s "\
				"(machine, userid, name, processPref, pid) "\
				"VALUES (%%s, %%s, %%s, %%s, %%s)" \
				% (constants.tblSeenProcesses),
					(ipInt, process.uid, process.label(), procPref.dbId,
					process.pid) )
			dbId = c.connection.insert_id()
			log.info( "Added process %s (user:%s, pid:%d, host:%s)"\
					" to database. (dbid %d)",
				process.label(), userName, process.pid,
				process.machine.name, dbId )
		self.dbIds[process] = dbId
		c.close()


	def logNewUser(self, user):
		""" Adds user info to the database if it doesn't already exist """
		if not hasattr( user, "uid" ):
			log.warning("User object %%s has no attribute uid" % (user))
			return

		c = self.dbManager.getLogDbCursor()
		c.execute( "SELECT name FROM %s WHERE uid = %%s" \
			% (constants.tblSeenUsers), user.uid )

		# If the user hasn't been logged to the database, do it now
		if c.rowcount == 0:
			c.execute( "INSERT INTO %s (uid, name)" "VALUES (%%s, %%s)" % \
					(constants.tblSeenUsers), (user.uid, user.name) )
		c.close()


	def logMachineStats( self ):
		""" Logs stats for all machines """
		baseWindowId = self.prefTree.windowPrefs[0].dbId
		table = "%s_lvl_%03d" % (constants.tblStatMachines, baseWindowId)
		values = []
		columns = ["tick", "machine"]
		columns.extend( pref.columnName for pref in \
				self.prefTree.iterMachineStatPrefs() )
		for m, statDict in self.machineStats.iteritems():
			ipInt = utilities.ipToInt( m.ip )
			row = [self.tick, ipInt]
			row.extend( (statDict[pref] for pref \
				in self.prefTree.iterMachineStatPrefs()) )
			values.append( row )

			# Empty the stats, we don't need it anymore
			statDict.clear()

		self.statDumper.pushInsert( table, columns, values )


	def logProcessStats( self ):
		""" Logs stats for all processes """
		getName = operator.attrgetter( 'name' )
		baseWindowId = self.prefTree.windowPrefs[0].dbId
		processes = itertools.groupby(
			sorted(self.processStats.iterkeys(), key=getName), getName )

		# Go through each process type and construct an insert
		# command for each one
		for procName, procList in processes:
			procPref = self.nameToProcPref[ procName ]
			values = []
			columns = ["tick", "process"]
			columns.extend( pref.columnName for pref in \
				procPref.iterAllStatPrefs() )
			table = "%s_lvl_%03d" % (procPref.tableName, baseWindowId)

			procList = list(procList)
			for p in procList:
				statDict = self.processStats[p]
				row = [self.tick, self.dbIds[p]]
				row.extend( statDict.get(pref, None) for pref \
					in procPref.iterAllStatPrefs() )
				values.append( row )

				# Empty the stats, we don't need it anymore
				statDict.clear()

			#log.debug( "Adding stats for %s, %d rows", table, len(values) )
			self.statDumper.pushInsert( table, columns, values )


	def consolidateStats( self, shouldLimitDeletion=True ):
		""" 
		Consolidate statistics for all machines and processes 
		
		@param shouldLimitDeletion True if deletion should be limited to 
			StatDumper.deleteSize and consolidation should be performed;
			else false.
		
		"""
		# Iterate through all windows
		for windowPref in self.prefTree.windowPrefs:
			windowId = windowPref.dbId
			samples = windowPref.samples
			samplePeriodTicks = windowPref.samplePeriodTicks
			tickFrom = self.tick - samplePeriodTicks
			tickTo = self.tick
			keepStatThreshold = int(self.tick - (samples * samplePeriodTicks))

			# Consolidate from the window below into the current one
			if self.tick % samplePeriodTicks != 0:
				continue

			# To avoid allowing the DB size to grow out of control, we
			# remove older statistics.
			if keepStatThreshold > 0:
				# Remove old rows from process statistics
				for procPref in self.prefTree.iterProcPrefs():
					table = "%s_lvl_%03d" % (procPref.tableName, windowId)
					self.statDumper.pushDeleteBefore( table,
							"tick", keepStatThreshold, shouldLimitDeletion )

				# Remove old rows from machine statistics
				table = "%s_lvl_%03d" % (constants.tblStatMachines, windowId)
				self.statDumper.pushDeleteBefore( table, "tick",
					keepStatThreshold, shouldLimitDeletion )

				# Remove unused ticks no longer referenced by any window
				if windowPref == self.prefTree.windowPrefs[-1]:
					self.statDumper.pushDeleteBefore( constants.tblStdTickTimes,
						"id", keepStatThreshold, shoudlLimitDeletion )

			if ((windowId == 1) or (not shouldLimitDeletion)):
				continue

			# Consolidate machine statistics
			tableTo = "%s_lvl_%03d" % (constants.tblStatMachines, windowId)
			tableFrom = "%s_lvl_%03d" % (constants.tblStatMachines, windowId-1)
			groupColumn = "machine"
			columns= [pref.columnName for pref in \
				self.prefTree.iterMachineStatPrefs()]
			aggregators = [pref.consolidateColumn() for pref in \
				self.prefTree.iterMachineStatPrefs()]
			self.statDumper.pushConsolidate( tableFrom, tableTo, groupColumn,
				columns, aggregators, tickFrom, tickTo )

			# Consolidate process statistics
			for procPref in self.prefTree.iterProcPrefs():
				tableTo = "%s_lvl_%03d" % (procPref.tableName, windowId)
				tableFrom = "%s_lvl_%03d" % (procPref.tableName, windowId - 1)
				groupColumn = "process"
				columns = [pref.columnName for pref in \
							procPref.iterAllStatPrefs()]
				aggregators = [pref.consolidateColumn() for pref in \
							procPref.iterAllStatPrefs()]
				self.statDumper.pushConsolidate( tableFrom, tableTo,
					groupColumn, columns, aggregators, tickFrom, tickTo )




# ------------------------------------------------------------------------------
# Section: SQL statistic dumping class
# ------------------------------------------------------------------------------

class StatDumper( threading.Thread ):
	"""
	Statistics dumping thread.
	"""
	INSERT = 0
	DELETE = 1
	CONSOLIDATE = 2
	FINISH = 3

	def __init__( self, dbManager ):
		threading.Thread.__init__( self, name="StatDumper" )
		self.messageQueue = Queue.Queue()

		# Information on current database query
		self.currentQuery = None
		self.currentQueryStart = None

		# General command queues
		self.generalQueue = collections.deque()
		self.deleteQueue = collections.deque()

		# Rows to delete at a time
		self.deleteSize = 20
		self.sleepTime = 0.1
		self.warnSQLTime = 1.0

		# Database manager
		self.dbManager = dbManager
		self.cursor = self.dbManager.getLogDbCursor()

		# Set to true here if profiling StatDumper thread
		self.profiling = False

	class FinishException( Exception ):
		pass


	# -------------------------------------------------------------
	# Exposed functions
	# -------------------------------------------------------------
	def pushDeleteBefore( self, table, col, cutoff, shouldLimitDeletion=True):
		"""
		Adds a delete command to delete rows from a table.

		@param table: Name of table to delete from
		@param col: Name of column which will be used to determine which
			values to delete
		@param cutoff: Value for which a row will be deleted if its
			column has a value below this.
		@param shouldLimitDeletion: True if the number of rows deleted should be 
			limited; else False.
		"""
		self.messageQueue.put( 
			(self.DELETE, (table, col, cutoff, shouldLimitDeletion)) )


	def pushInsert( self, table, columns, values ):
		"""
		Adds an insert command to the queue.

		@param tables: Name of table to insert values into.
		@param columns: List of columns in the table corresponding to
			the values being inserted.
		@param values: List of value tuples being inserted.
		"""
		self.messageQueue.put( (self.INSERT, (table, columns, values)) )


	def pushConsolidate( self, tableFrom, tableTo, groupColumn, columns,
			aggregators, tickFrom, tickTo ):
		"""
		Adds a consolidate command to the queue

		@params tableFrom:   Table name from which consolidation starts.
		@params tableTo:     Table to which consolidated data is placed.
		@params groupColumn: The column on which the rows are grouped
		@params columns:     List of column stat names
		@params aggregators: Database functions to aggregate the data. Should
			correpond to the columns parameter.
		@params tickFrom:    The start tick
		@params tickTo:      The end tick
		"""
		self.messageQueue.put( (self.CONSOLIDATE, (tableFrom, tableTo,
			groupColumn, columns, aggregators, tickFrom, tickTo)) )


	def pushFinish( self, quick=False ):
		"""
		Adds a terminate command to the queue

		@param quick: True if we should stop immediately upon
			processing this command, False if we should wait until all
			SQL command queues are empty.
		"""
		self.messageQueue.put( (self.FINISH, quick) )


	def getCurrentQueryInfo( self ):
		""" Returns information on the current SQL query. """
		currentQueryStart = self.currentQueryStart
		if currentQueryStart != None:
			return (self.currentQuery, time.time() - currentQueryStart)
		else:
			return None


	# -------------------------------------------------------------
	# Internal functions
	# -------------------------------------------------------------
	def processMessage( self, msgTuple ):
		"""Handle messages passed to us from the Gather thread."""
		msg, params = msgTuple
		if msg == self.INSERT:
			self.generalQueue.appendleft( msgTuple )

		elif msg == self.CONSOLIDATE:
			self.generalQueue.appendleft( msgTuple )

		elif msg == self.DELETE:
			found = False
			table, column, cutoff, shouldLimitDeletion = params
			# Try to update the entry in deleteQueue if we already have 
			# a delete scheduled for the same table
			for i in range(len(self.deleteQueue)):
				if table == self.deleteQueue[i][0]:
					oldTable, oldColumn, oldCutoff, shouldLimitDeletion = \
						self.deleteQueue[i]
					self.deleteQueue[i] = oldTable, oldColumn, cutoff, shouldLimitDeletion
					log.debug( "Extended existing delete command on %s "\
						"from %s to %s", table, oldCutoff, cutoff )
					found = True
					break
			if not found:
				self.deleteQueue.appendleft( params )

		elif msg == self.FINISH:
			quick = params
			if not quick:
				self.processGeneralQueue( entireQueue=True )
			raise self.FinishException()

		else:
			raise Exception("StatDumper: Got unknown message %s" % (msg))


	def constructDeleteSQL( self, table, col, cutoff, shouldLimitDeletion ):
		"""
		Constructs the SQL statement for an insert command.
		Params correspond to those for pushDelete().
		"""
		sql = "".join( ("DELETE FROM ", table, " WHERE ",
			col, " < ", str( cutoff ) ) ) 
			
		if shouldLimitDeletion:
			sql = "".join( (sql, " LIMIT ", str( self.deleteSize)) )
		
		return sql


	def constructInsertSQL( self, table, columns, values ):
		"""
		Constructs the SQL statement for an insert command.
		Params correspond to those for pushInsert()
		We use str.join() a lot here because it's the fastest method
		of string concatenation. It's not as bad as it looks, really.
		"""
		if not values:
			return None
		conv = self.cursor._get_db().literal
		sqlValues = ",".join(
			"".join( ("(", ",".join( conv(v) for v in row ), ")") )
			for row in values )
		sqlColumns = ",".join(columns)
		sql = "".join( ("INSERT INTO ", table, "(", sqlColumns, ") VALUES ",
				sqlValues) )
		return sql


	def constructConsolidateSQL( self, tableFrom, tableTo, groupColumn, columns, aggregators, tickFrom, tickTo ):
		"""
		Construct consolidation SQL query.
		We don't use str.join as much here because it'd be far too messy.
		Params correspond to those for pushConsolidate()
		"""

		assert len(columns) == len(aggregators), \
			"Expected columns to be the same length as aggregators"

		conv = self.cursor._get_db().literal
		aggregatedColumns = ",".join( "".join( (
			a, "(", c, ")") ) for a,c in zip(aggregators, columns) )
		tickFrom = int(tickFrom)
		tickTo = int(tickTo)

		sql = """INSERT INTO %s(%s) SELECT
			%s AS currentTick,
			-- Current tick
			stat.%s,
			-- Aggregated columns
			%s
			FROM %s AS stat WHERE stat.tick >= %s AND stat.tick < %s
			GROUP BY stat.%s
			""" % ( tableTo, ",".join(["tick", groupColumn] + ( columns )),
				conv( tickFrom ),
				groupColumn,
				",".join(columns),
				tableFrom, conv( tickFrom ), conv( tickTo ),
				groupColumn)

		return sql


	def processGeneralQueue( self, entireQueue=False ):
		""" Handle our general database query queue """
		reportInterval = 1.0
		lastReport = time.time()
		if entireQueue:
			log.debug( "Beginning full SQL queue dump, %d batches left" % \
					(len(self.generalQueue)) )
		while self.generalQueue:
			msg, params = self.generalQueue.pop()
			if msg == self.INSERT:
				sql = self.constructInsertSQL( *params )
			elif msg == self.CONSOLIDATE:
				sql = self.constructConsolidateSQL( *params )
			self.timedSQLQuery( sql )
			if not entireQueue:
				break
			if time.time() - lastReport > reportInterval:
				log.info( "Processing entire queue, %d batches left" % \
					(len(self.generalQueue)) )


	def processDeleteQueue( self, entireQueue=False ):
		""" Handle our low priority delete queue """
		while self.deleteQueue:
			params = self.deleteQueue[-1]
			sql = self.constructDeleteSQL( *params )

			numDeleted = self.timedSQLQuery( sql )

			if params[-1]:
				# numDeleted is enforced to be at most self.deleteSize
				# by the SQL 'LIMIT' command. See constructDeleteSQL()
				if numDeleted < self.deleteSize:
					self.deleteQueue.pop()
			else:
				self.deleteQueue.pop()

			if not entireQueue:
				break


	def timedSQLQuery( self, sql, args=None ):
		"""
		Performs an SQL query. If time exceeds a certain
		threshold, print a warning message.
		"""
		if args == None:
			self.currentQuery = sql
		else:
			conv = self.cursor._get_db().literal
			self.currentQuery = sql % tuple( conv(a) for a in args )

		self.currentQueryStart = time.time()
		results = self.cursor.execute( self.currentQuery )
		timeTaken = time.time() - self.currentQueryStart
		showSQL = False
		if timeTaken > self.warnSQLTime:
			log.warning( "Query took (%.3fs, %d rows):",
				timeTaken, self.cursor.rowcount )
			showSQL = True
		numWarnings = self.cursor._warnings
		if numWarnings > 0:
			log.warning( "We got %d warnings", numWarnings )
			showSQL = True
		if showSQL:
			log.warning( self.currentQuery )

		self.currentQueryStart = None
		self.currentQuery = None
		return self.cursor.rowcount


	def run( self ):
		""" Thread entry point """
		if self.profiling:
			import cProfile
			prof = cProfile.Profile()
			prof.runcall( self.runDumper )
			prof.dump_stats( "stat_logger_dumper.prof" )
			log.debug( "Dumped profile statistics to stat_logger_dumper.prof" )
		else:
			self.runDumper()


	def runDumper(self):
		""" Start the main StatDumper loop """
		try:
			while True:
				if (not self.generalQueue) and (not self.deleteQueue):
					# If both queues are empty, sleep for a bit 
					# (should be a really small amount)
					time.sleep( self.sleepTime )
				else:
					# Process both queues. Deleting goes first, since 
					# inserting extra values might make the database 
					# seach longer to find the rows to delete (just 
					# being paranoid, it probably makes minimal difference)
					self.processDeleteQueue()
					self.processGeneralQueue()

				# Process message queue
				while not self.messageQueue.empty():
					self.processMessage( self.messageQueue.get(0) )

		except self.FinishException:
			pass

		log.info( "StatDumper thread terminating normally" )

# gather.py
