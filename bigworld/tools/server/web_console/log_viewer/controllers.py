import logging
import cherrypy
import turbogears
import sqlobject

from turbogears import controllers, expose, redirect
from turbogears import validate, validators, identity
from turbogears import widgets
from turbojson import jsonify

import re
import time

# BigWorld includes
import bwsetup; bwsetup.addPath( "../.." )
from pycommon import cluster
from pycommon import log
from pycommon import async_task
import pycommon.util
from message_logger import bwlog
from message_logger import message_log
from web_console.common import util
from web_console.common import module
from web_console.common import ajax
import model
import inspect


class LogViewer( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		self.addPage( "Search", "search" )
		self.addPage( "Live Output", "live" )
		self.addPage( "Usage Summary", "summaries" )
		self.addPage( "Help", "help", popup=True )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: LogViewer Help" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="log_viewer" )

	@identity.require( identity.not_anonymous() )
	@expose()
	def index( self, **kw ):
		raise redirect( "search", **kw )

	# --------------------------------------------------------------------------
	# Section: Pages
	# --------------------------------------------------------------------------

	@expose( template="web_console.common.templates.error" )
	def error( self, msg = None ):
		return dict( msg = msg, debug = [] )


	# Determine if message_logger is running or not, and whether or not the
	# current user session needs to be notified about the status.
	def queryMLStatus( self, mlog = None ):

		# Session variable initialization (or recall if exists)
		cherrypy.session['mlnotified'] = cherrypy.session.get('mlnotified', False)
		mlnotify = False

		if not mlog:
			mlog = message_log.MessageLog()

		mlstatus = mlog.isRunning()

		if not cherrypy.session['mlnotified']:
			cherrypy.session['mlnotified'] = True
			mlnotify = not mlstatus

		return (mlstatus, mlnotify)


	@identity.require( identity.not_anonymous() )
	@expose( template = "log_viewer.templates.show" )
	def search( self, **kw ):

		try:
			mlog = message_log.MessageLog()
		except IOError:
			log.error( "Failed to initialise interface to Message Logger" )
			return self.error( "Message Logger has not been configured." )

		(mlstatus, mlnotify) = self.queryMLStatus( mlog )

		# Generate hostnames, component names and severities in order
		hostnames = sorted( mlog.getHostnames().values() )
		components = sorted( mlog.getComponentNames(),
							 cluster.Process.cmpByName )
		severities = [k for k, v in sorted( bwlog.SEVERITY_LEVELS.items(),
											key = lambda (k,v): v )]

		# Generate basic options for the "Manage Settings..." menu
		menuOptions = util.ActionMenuOptions()

		menuOptions.addScript(
			"Send Message...",
			script = "annotate()",
			help = "Send a message to the log" )

		menuOptions.addScript(
			"Save As...", group = "Save Settings:",
			script = "saveQuery( prompt( 'Enter name to save settings as:' ) )",
			help = "Save the current filter settings" )

		menuOptions.addScript(
			"Save As Default", group = "Save Settings:",
			script = "saveQuery( 'default' )",
			help = "Save the current filter settings as the default" )

		menuOptions.addGroup( "Load Settings:", "loadQueries" )
		menuOptions.addGroup( "Delete Settings:", "deleteQueries" )

		# Extract this user's last annotation (if any)
		try:
			results = model.lvAnnotation.select(
				model.lvAnnotation.q.userID == util.getSessionUID() )
			annotation = results[0].message

		except IndexError:
			annotation = ""

		# We need to return an instance of the CalendarDateTimePicker so the
		# 'sitepackage' template will include all the appropriate CSS / js
		# files.
		return dict( now = time.time(), users = sorted( mlog.getUsers() ),
					 hostnames = hostnames,
					 components = components, severities = severities,
					 datetimepicker = widgets.CalendarDateTimePicker("time"),
					 menuOptions = menuOptions,
					 mlstatus = mlstatus, mlnotify = mlnotify,
					 annotation = annotation )


	@identity.require( identity.not_anonymous() )
	@expose( template = "log_viewer.templates.live" )
	def live( self, **kw ):

		(mlstatus, mlnotify) = self.queryMLStatus()

		return dict( now = time.time(), serveruser = util.getServerUsername(),
					 mlstatus = mlstatus, mlnotify = mlnotify )


	@identity.require( identity.not_anonymous() )
	@expose( template = "log_viewer.templates.summaries" )
	def summaries( self, **kw ):
		"""
		Displays a table of summaries of log usage for all users.
		"""

		mlog = message_log.MessageLog()
		info = []

		for userLog in [mlog.getUserLog( uid ) for uid in mlog.getUIDs()]:

			segments = userLog.getSegments()

			# This should only happen if a user registers a process with
			# message_logger but never sends a single log message
			if not segments:
				continue

			size = pycommon.util.fmtBytes(
				sum( [s.entriesSize + s.argsSize for s in segments] ), True )

			entries = sum( [s.nEntries for s in segments] )
			start = time.ctime( segments[0].start )
			end = time.ctime( segments[-1].end )

			t = int( segments[-1].end - segments[0].start )
			duration = pycommon.util.fmtSecondsLong( t )

			info.append( (userLog.username, size, entries, len( segments ),
						  start, end, duration ) )

		info.sort()
		return dict( info = info )


	@identity.require( identity.not_anonymous() )
	@expose( template = "log_viewer.templates.summary" )
	def summary( self, user ):

		mlog = message_log.MessageLog()
		userLog = mlog.getUserLog( mlog.getUsers()[ user ] )
		segments = userLog.getSegments()

		return dict( user = user, segments = segments )

	# --------------------------------------------------------------------------
	# Section: Log querying interface
	# --------------------------------------------------------------------------

	@identity.require( identity.not_anonymous() )
	@validate( validators = dict( pid = validators.Int(),
								  appid = validators.Int(),
								  caseSens = validators.StringBool(),
								  regex = validators.StringBool(),
								  interpolate = validators.StringBool(),
								  showMask = validators.Int(),
								  max = validators.Int(),
								  context = validators.Int(),
								  live = validators.StringBool() ) )
	@ajax.expose
	def fetch( self,
			   time,
			   period,
			   host = "",
			   serveruser = None,
			   pid = 0,
			   appid = 0,
			   procs = "",
			   severity = "",
			   message = "",
			   exclude = "",
			   caseSens = False,
			   regex = False,
			   interpolate = False,
			   showMask = bwlog.SHOW_ALL,
			   max = 0,
			   live = False,
			   startAddr = None,
			   endAddr = None,
			   context = 0,
			   **kw ):

		mlog = message_log.MessageLog()
		usermap = mlog.getUsers()

		if not serveruser or serveruser not in usermap:
			raise ajax.Error, "Unknown server user: %s" % serveruser

		else:
			uid = usermap[ serveruser ]
			userlog = mlog.getUserLog( uid )

		# Dictionary we'll populate with optional args for the C++ fetch() call
		fetchkw = {}

		# If start and end addresses are provided, use them
		if startAddr:
			fetchkw[ "startaddr" ] = self.parseAddr( startAddr )
			
			if endAddr:
				fetchkw[ "endaddr" ] = self.parseAddr( endAddr )

		# If an event has been requested for the start time, calculate it
		else:
			if time == "server startup":
				startup = userlog.getLastServerStartup()
				if startup is not None:
					fetchkw[ "startaddr" ] = startup[ "addr" ]
				else:
					fetchkw[ "start" ] = bwlog.LOG_BEGIN

			# A time beginning with a # character means find the last instance
			# of that message and search forwards from there
			elif time.startswith( "#" ):
				offset = userlog.getLastMatchingMessage( time[1:] )
				if offset:
					fetchkw[ "startaddr" ] = offset
				else:
					raise ajax.Error( "No message matching '%s' in the logs" %
									  time[1:] )

			else:
				fetchkw[ "start" ] = float( time )

			fetchkw[ "period" ] = period

		cnames = mlog.getComponentNames()

		if procs:
			mask = 0
			for p in procs.split( "," ):
				if p in cnames:
					mask |= 1 << cnames.index( p )
			procs = mask
		else:
			procs = -1

		if severity:
			severities = sum( [1 << bwlog.SEVERITY_LEVELS[ s ] for s in
							   severity.split( "," )] )
		else:
			severities = -1

		if interpolate:
			interpolate = bwlog.PRE_INTERPOLATE
		else:
			interpolate = bwlog.POST_INTERPOLATE

		# validators.Int() doesn't convert "" -> 0
		if pid is None: pid = 0
		if appid is None: appid = 0
		if context is None: context = 0

		# Escape metachars in pattern if not using regexes
		if not regex:
			message = re.escape( message )
			exclude = re.escape( exclude )

		try:
			args = dict( uid = uid,
						 host = host,
						 pid = pid,
						 appid = appid,
						 procs = procs,
						 severities = severities,
						 message = message,
						 exclude = exclude,
						 interpolate = interpolate,
						 casesens = caseSens,
						 context = context,
						 **fetchkw )

			# In live mode we fetch some context lines for the query which are
			# at the start of the output.  We must do this now before we create
			# the forwards query as you can't have two queries existing
			# simultaneously (limitation of current bwlog implementation)
			if live:
				context, query = mlog.fetchContext( args, max = 40 )
				context = [result.format( showMask ) for result in context]
			else:
				context = []
				query = mlog.fetch( **args )

			# In non-live mode, need slots for 'progress' and 'results'
			# TODO: Investigate the performance implications of having live
			# console queries in blocking mode (which will make response times
			# much faster but will increase load on the server I'm guessing).
			if live:
				queuesize = 0
			else:
				queuesize = 2

			# Start fetch and make sure it's running
			task = async_task.AsyncTask( queuesize, self.runQuery, query,
										 showMask, live, context, max )

		except SyntaxError:

			try:
				re.compile( message )
			except Exception, e:
				raise ajax.Error, ("Regex error in message pattern", str( e ))

			try:
				re.compile( exclude )
			except Exception, e:
				raise ajax.Error, ("Regex error in exclude pattern", str( e ))

		return dict( id = task.id )


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	@validate( validators = dict( id = validators.Int(),
								  blocking = validators.StringBool() ) )
	def pollQuery( self, id, blocking = True ):
		return util.poll( id )


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	@validate( validators = dict( id = validators.Int() ) )
	def stopQuery( self, id ):
		return util.terminate( id )


	def parseAddr( self, s ):
		"""
		Parse an EntryAddress sent from the browser as either the startAddr or
		endAddr parameter to 'fetch()'.
		"""
		s = s.split( "," )
		return (s[0], int( s[1] ), int( s[2] ))


	def runQuery( self, query, showMask, live, context, maxLines, _async_ ):
		"""
		This is the workhorse method that runs searches and returns results to
		the client.  It must be run as an AsyncTask.
		"""

		# Release updates to the client at least this often
		MAX_UPDATE_WAIT = 1.0

		# Release updates to the client when this many results are accumulated
		MAX_UPDATE_SIZE = 5000

		# Need this ugly piece of work because local variables don't resolve
		# properly to the outer scope in inner function definitions
		class Status:

			def __init__( self ):
				self.lines = []
				self.count = 0
				self.sent = False
				self.updateTime = time.time()

			# It's very important that query is passed in, instead of directly
			# referenced in the outer scope, or you get a cyclic reference bug
			def flush( self, query ):
				if not self.sent:
					self.sent = True

				_async_.update( "results",
								dict( lines = self.lines,
									  reverse = query.inReverse() ) )

				# We need to include the amount of context in the total
				# otherwise the client-side progress display is wrong for small
				# result sets.  The magic -1 here is to adjust down for the
				# first position that is skipped over in fetchContext().
				conlen = max( len( context ) - 1, 0 )
				progress = [x + conlen for x in query.getProgress()]
				_async_.update( "progress",
								[query.tell(), self.count] + progress )

				self.lines = []
				self.updateTime = time.time()

		status = Status()

		# Send through context lines immediately in live mode
		if live:
			status.lines.extend( context )
			status.count = len( status.lines )
			status.flush( query )

		# Make sure the client receives updates even when filters aren't
		# matching anything
		query.setTimeout( MAX_UPDATE_WAIT, status.flush )

		while True:

			# Assemble query results and send them to the client if need be
			for result in query:

				status.lines.append( result.format( showMask ) )
				status.count += 1

				# If we terminate early due to maxlines restrictions, pass back
				# the start and end offsets for the next query so we don't have
				# to work them out again
				if maxLines and status.count >= maxLines:
					status.flush( query )
					_async_.update( "truncated", query.tell( True ) )
					break

				# Flush results to client if we've accumulated enough or it's
				# been too long since we last sent something through
				if len( status.lines ) >= MAX_UPDATE_SIZE or \
					   time.time() - status.updateTime > MAX_UPDATE_WAIT:
					status.flush( query )

			if status.lines or not status.sent:
				status.flush( query )

			if live:

				# We can't use query.waitForResults() because it blocks and
				# doesn't allow us to call _async_.update() to check if we've
				# been terminated by the user
				while not query.hasMoreResults():
					_async_.update()
					time.sleep( 0.5 )
					query.resume()

			# Non-live queries never execute this loop twice
			else:
				break


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def annotate( self, user, message ):

		# Get the message logger process running on this machine
		m = cluster.Machine.localMachine()

		try:
			p = m.getProcs( "message_logger" )[0]

		except IndexError:
			raise ajax.Error( "message_logger isn't running on this machine!" )

		p.sendMessage( message, user )

		# Forget old annotation (if any)
		try:
			old = model.lvAnnotation.select(
				model.lvAnnotation.q.userID == util.getSessionUID() )
			old[0].destroySelf()
		except IndexError:
			pass

		# Remember this annotation
		model.lvAnnotation( user = util.getSessionUser(), message = message )

		return "Message logged at %s" % time.ctime()

	# --------------------------------------------------------------------------
	# Section: Saved settings management
	# --------------------------------------------------------------------------

	@identity.require( identity.not_anonymous() )
	@validate( validators = dict( caseSens = validators.StringBool(),
								  regex = validators.StringBool(),
								  interpolate = validators.StringBool(),
								  showMask = validators.Int(),
								  live = validators.StringBool(),
								  autoHide = validators.StringBool() ) )
	@ajax.expose
	def saveQuery( self, **kw ):

		if not kw.has_key( "name" ) or not kw[ "name" ]:
			raise ajax.Error, "You must specify a name"

		try:
			old = model.lvSavedQuery.select( sqlobject.AND(
				model.lvSavedQuery.q.userID == util.getSessionUID(),
				model.lvSavedQuery.q.name == kw[ "name" ] ) )
			old[0].destroySelf()

		except IndexError:
			pass

		try:
			rec = model.lvSavedQuery( user = util.getSessionUser(), **kw )
			return "'%s' saved successfully" % kw[ "name" ]

		except Exception, e:
			raise ajax.Error, ("Save query failed", str( e ))


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def deleteQuery( self, name ):

		errhdr = "Delete query failed"

		if name == "default":
			raise ajax.Error, (errhdr, "Can't delete the default query")

		try:
			old = model.lvSavedQuery.select( sqlobject.AND(
				model.lvSavedQuery.q.userID == util.getSessionUID(),
				model.lvSavedQuery.q.name == name ) )[0]
			old.destroySelf()

		except IndexError:
			raise ajax.Error, (errhdr, "Query %s not found" % name)

		return dict()


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def fetchQueries( self ):

		queries = list( model.lvSavedQuery.select(
			model.lvSavedQuery.q.userID == util.getSessionUID(),
			orderBy = "id" ) )

		if not queries:
			queries = [model.lvSavedQuery( user = util.getSessionUser(),
										   serveruser = util.getServerUsername(),
										   name = "default" )]

		# Keying operator for queries.
		def order( rec ):
			if rec.name == "default": return -2
			if rec.name == "most recent": return -1
			return rec.id

		queries = map( dict, sorted( queries, key = order ) )

		for query in queries:
			query[ "procs" ] = query[ "procs" ].split( "," )
			query[ "severity" ] = query[ "severity" ].split( "," )

		return dict( queries = queries )
