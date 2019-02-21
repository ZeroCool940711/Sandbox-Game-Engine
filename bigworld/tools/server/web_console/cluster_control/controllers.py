import logging
import cherrypy
import turbogears
import sqlobject

from turbogears import controllers, expose, redirect
from turbogears import validate, validators, identity
from turbogears import widgets
from turbojson import jsonify

# Standard python modules
from StringIO import StringIO
import os
import re
import random
import threading
import traceback
import signal as sigmodule

# BigWorld modules
import bwsetup; bwsetup.addPath( "../.." )
from pycommon import cluster
from pycommon import uid as uidmodule
from pycommon import log
from pycommon import async_task
from pycommon import watcher_data_type as WDT
import pycommon.util
from web_console.common import util
from web_console.common import module
from web_console.common import ajax
from web_console.common import model as common_model
import model

class ClusterControl( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		self.addPage( "Manage Servers", "procs" )
		self.addPage( "All Users", "users" )
		self.addPage( "All Machines", "machines" )
		self.addPage( "Saved Layouts", "layouts" )
		self.addPage( "Custom Watchers", "customWatcherList" )
		self.addPage( "Help", "help", popup=True )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: ClusterControl Help" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="cluster_control" )

	def appendToLayout( self, layout, tag ):
		for i in xrange( len( layout ) ):
			layout[i] = layout[i] + (tag,)

	def getCustomWatcherList( self ):
		# Obtain the current users list of custom watcher pages
		try:
			customList = list( model.ccCustomWatchers.select(
					model.ccCustomWatchers.q.userID == util.getSessionUID() ))
		except:
			customList = []

		return customList



	@identity.require(identity.not_anonymous())
	@validate( validators = dict( id=validators.Int() ) )
	@ajax.expose
	def poll( self, id, blocking ):
		return util.poll( id, blocking )

	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.procs" )
	def procs( self, user=None ):
		c = cluster.Cluster()
		user = util.getUser( c, user )

		areWorldProcessesRunning = True
		layoutErrors = None
		if user:
			procs = sorted( user.getProcs() )
			procsName = [p.name for p in procs]
			if "dbmgr" not in procsName or \
			   "loginapp" not in procsName or \
			   "cellappmgr" not in procsName or \
			   "baseappmgr" not in procsName:
				areWorldProcessesRunning = False

			layoutErrors = user.getLayoutErrors()
		else:
			raise redirect( "/error", msg = "Unable to resolve active user" )
			areWorldProcessesRunning = False

		return dict( procs = procs, user = user, layoutErrors = layoutErrors,
					 page_specific_js = ["/log/static/js/query.js"],
					 areWorldProcessesRunning = areWorldProcessesRunning )


	@identity.require(identity.not_anonymous())
	@expose()
	def index( self ):
		raise redirect( turbogears.url( "procs" ) )

	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.machine" )
	def machine( self, machine ):
		c = cluster.Cluster()
		machine = c.getMachine( machine )
		procs = sorted( machine.getProcs() )
		procs.sort( key = lambda p: p.uid )
		return dict( m = machine, ps = procs )

	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.machines" )
	def machines( self, group = None ):
		c = cluster.Cluster()
		machines = c.getMachines()
		machines.sort()

		if group:
			c.queryTags( "Groups" )
			machines = [m for m in machines if m.tags.has_key( "Groups" )
						and group in m.tags[ "Groups" ]]

		return dict( ms = machines, group = group )

	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.users" )
	def users( self, user=None ):

		c = cluster.Cluster()
		activeUsers = sorted( c.getUsers() )
		inactiveUsers = {}
		for user in uidmodule.getall():
			inactiveUsers[ user.name ] = user
		for user in activeUsers:
			if inactiveUsers.has_key( user.name ):
				del inactiveUsers[ user.name ]
		inactiveUsers = sorted( inactiveUsers.values() )

		return dict( activeUsers = activeUsers,
					 inactiveUsers = inactiveUsers )


	# Perform the same functionality as the control_cluster 'flush' command.
	# This should only be called from the "All Users" page, and then return
	# the user to that page with an updated user list.
	@identity.require( identity.not_anonymous() )
	@expose()
	def usersFlush( self ):

		c = cluster.Cluster()
		ms = c.getMachines()

		for m in ms:
			m.flushMappings()

		raise redirect( "users" )


	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.watcher" )
	@util.unicodeToStr
	def watcher( self, machine, pid, path="", newval=None, dataType=None ):

		c = cluster.Cluster()
		m = c.getMachine( machine )
		p = m.getProc( int( pid ) )
		if not p:
			raise redirect( "/error", msg = "Process %s no longer exists" % pid )
		wd = p.getWatcherData( path )

# TODO: this needs to be adjusted!
		if newval:
			status = False
			if dataType != None:
				wdtClass = WDT.WDTRegistry.getClass( int(dataType) )
				wdtObj = wdtClass( newval )
				status = wd.set( wdtObj )
			else:
				status = wd.set( newval )

			wd = p.getWatcherData(os.path.dirname( wd.path ))

		else:
			status = `True`

		children = wd.getChildren()
		subdirs = []
		watchersList = []
		if children:
			subdirs = [c for c in children if c.isDir()]
			watchersList = [c for c in children if not c.isDir()]
		customPages = list( self.getCustomWatcherList() )

		watchers = []
		for w in watchersList:
			# Unfortunately this menu is created here because
			# the list of watchers is generated above
			# (this should be moved into controllers.py)
			menu = util.ActionMenuOptions()
			menu.addGroup( "Action..." )
			menu.addRedirect( "Edit", util.alterParams( path=w.path ),
							  help="Edit this watcher value" )
			menu.addGroup( "Add to Custom Watcher..." )
			for i in customPages:
				menu.addScript( "%s" % i.pageName,
					args = ( i.pageName, p.name, w.path ),
					group = "Add to Custom Watcher...",
					script = "saveWatcherToLayout" )

			watchers.append( (w, menu) )

		return dict( p = p, m = m, status = status, wd = wd,
					 subdirs=subdirs, watchers=watchers )


	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.start" )
	def start( self, user ):
		c = cluster.Cluster()
		try:
			user = c.getUser( user, refreshEnv = True, checkCoreDumps = True )
		except Exception, e:
			raise redirect( "/error", msg = str(e) )

		try:
			prefs = model.ccStartPrefs.select(
				model.ccStartPrefs.q.userID == util.getSessionUID() )[0]
		except:
			prefs = model.ccStartPrefs( user = util.getSessionUser(),
										mode = "single",
										arg = "",
										useTags = True )

		savedLayouts = model.ccSavedLayouts.select(
			model.ccSavedLayouts.q.userID == util.getSessionUID() )

		return dict( user = user, c = c, prefs = prefs,
					 savedLayouts = [x.name for x in savedLayouts] )


	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.startproc" )
	@validate( validators = dict( count=validators.Int() ) )
	def startproc( self, user=None, pname=None, machine=None, count=None ):

		c = cluster.Cluster()
		user = util.getUser( c, user )

		# We're actually starting the processes
		if pname and machine and count:

			# Start the processes
			pname = pname.encode( "ascii" )
			user.startProc( c.getMachine( machine ), pname, count )

			# Set this is as the default for next time
			for rec in model.ccStartProcPrefs.select(
				model.ccStartProcPrefs.q.userID == util.getSessionUID() ):
				rec.destroySelf()

			model.ccStartProcPrefs(
				user = util.getSessionUser(),
				proc = pname,
				machine = machine,
				count = count )

			raise redirect( "procs", user = user.name )

		# We're displaying the page to select what to start
		else:

			machines = sorted( c.getMachines(), key = lambda x: x.name )

			try:
				prefs = model.ccStartProcPrefs.select(
					model.ccStartProcPrefs.q.userID == util.getSessionUID() )[0]

			except IndexError:
				prefs = model.ccStartProcPrefs( user = util.getSessionUser() )

			return dict( user = user, machines = machines, prefs = prefs )


	@identity.require(identity.not_anonymous())
	@expose()
	@validate( validators = dict( pid = validators.Int() ) )
	def stopproc( self, machine, pid ):

		c = cluster.Cluster()
		m = c.getMachine( machine )
		p = m.getProc( pid )
		u = p.user()

		if not identity.in_group( "admin" ) and \
		   util.getServerUsername() != u.name:
			return self.error( "You can't stop other people's processes" )

		p.stopNicely()

		raise redirect( "procs", user=u.name )



	@identity.require(identity.not_anonymous())
	@expose()
	@validate( validators = dict( pid = validators.Int(),
								  signal = validators.Int(),
								  restart = validators.StringBool() ) )
	def killproc( self, machine, pid, signal, restart = False ):

		c = cluster.Cluster()
		m = c.getMachine( machine )
		p = m.getProc( pid )
		u = p.user()

		if not identity.in_group( "admin" ) and \
		   util.getServerUsername() != u.name:
			return self.error( "You can't kill other people's processes" )

		if restart:
			u.startProc( m, p.name )

		m.killProc( p, signal )

		raise redirect( "procs", user=u.name )


	@identity.require(identity.not_anonymous())
	@expose()
	@util.unicodeToStr
	def doStart( self, user, mode, group=None, machine=None, layout=None,
				 restrict=False ):

		c = cluster.Cluster()
		user = c.getUser( user )
		kw = {}
		util.clearDebugOutput()

		# Saved layout mode
		if mode == "layout":
			rec = self.getLayout( layout )
			if not rec:
				return self.error( "Couldn't find saved layout '%s' "
								   "in the database" % layout )
			task = async_task.AsyncTask( 0, user.startFromXML,
										 StringIO( rec.xmldata ) )

		# Single and group modes
		else:
			if mode == "single":
				machines = [c.getMachine( machine )]
			else:
				machines = None

			if mode == "group":
				kw[ "group" ] = group
				if restrict:
					kw[ "tags" ] = True

			task = async_task.AsyncTask( 0, user.start, machines, **kw )

		# Delete old pref
		for rec in model.ccStartPrefs.select(
			model.ccStartPrefs.q.userID == util.getSessionUID() ):
			rec.destroySelf()

		# Insert new pref
		if mode == "single":
			model.ccStartPrefs( user = util.getSessionUser(),
								mode = "single",
								arg = machine,
								useTags = bool( restrict ) )
		elif mode == "group":
			model.ccStartPrefs( user = util.getSessionUser(),
								mode = "group",
								arg = group,
								useTags = bool( restrict ) )

		elif mode == "layout":
			model.ccStartPrefs( user = util.getSessionUser(),
								mode = "layout",
								arg = layout,
								useTags = False )

		# Block until we get the layout out of the async task
		try:
			layout = task.waitForState( "layout" )[1][:]
		except task.TerminateException:
			return self.error( "Couldn't get layout" )

		# Tag each process as "running"
		for i in xrange( len( layout ) ):
			layout[i] = layout[i] + ("running",)

 		return self.toggle( "start", task.id, layout, user )

	@identity.require(identity.not_anonymous())
	@expose()
	def restart( self, user ):
		c = cluster.Cluster()
		user = c.getUser( user )
		layout = user.getLayout()
		self.appendToLayout( layout, "registered" )
		task = async_task.AsyncTask( 0, user.restart )
		return self.toggle( "restart", task.id, layout, user )

	@identity.require(identity.not_anonymous())
	@expose()
	def stop( self, user ):
		c = cluster.Cluster()
		user = c.getUser( user )
		layout = user.getLayout()
		self.appendToLayout( layout, "registered" )
		task = async_task.AsyncTask( 0, user.smartStop )
		return self.toggle( "stop", task.id, layout, user )

	@identity.require(identity.not_anonymous())
	@expose()
	def kill( self, user ):
		"""
		Kill the server.  Data loss may occur if BaseApps are in the 
		process of writing entities to the database.
		"""
		c = cluster.Cluster()
		user = c.getUser( user )
		layout = user.getLayout()
		self.appendToLayout( layout, "registered" )
		task = async_task.AsyncTask( 0, user.smartStop, forceKill=True )
		return self.toggle( "stop", task.id, layout, user )

	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.toggle" )
	def toggle( self, action, id, layout, user ):

		# Figure out pnames set
		pnames = set()
		for _, pname, _, _ in layout:
			pnames.add( pname )
		pnames = list( pnames )
		pnames.sort( cluster.Process.cmpByName )

		return dict( action = action, layout = layout,
					 pnames = pnames, user = user, id = id )

	@identity.require(identity.not_anonymous())
	@ajax.expose
	def verifyEnv( self, user, type, value ):

		c = cluster.Cluster()
		util.clearDebugOutput()
		ms = []

		if type == "machine":
			ms.append( c.getMachine( value ) )

		elif type == "group":
			groups = c.getGroups()
			if value == "(use all machines)":
				ms = c.getMachines()
			elif groups.has_key( value ):
				ms = groups[ value ]
			else:
				raise ajax.Error( "Unknown group: %s" % value,
								  util.getDebugErrors() )

		elif type == "layout":
			layout = self.getLayout( value )
			mnames = [m for (m,p) in cluster.User.parseXMLLayout( layout.xmldata )]

			ms = []; missing = False
			for mname in mnames:
				ms.append( c.getMachine( mname ) )
				if not ms[-1]:
					log.error( "Layout refers to unknown machine %s", mname )
					missing = True

			if missing:
				raise ajax.Error( "Layout refers to unknown machines",
								  util.getDebugErrors() )

		user = c.getUser( user, random.choice( ms ), refreshEnv = True )

		if user.verifyEnvSync( ms ):
			return dict( mfroot = user.mfroot, bwrespath = user.bwrespath )

		else:
			raise ajax.Error(
				"Inconsistent environment settings across target machines",
				util.getDebugErrors() )


	@identity.require(identity.not_anonymous())
	@expose( template="cluster_control.templates.coredumps" )
	def coredumps( self, user = None ):

		if not user:
			user = util.getServerUsername()

		c = cluster.Cluster.get( user = user )
		user = c.getUser( user, checkCoreDumps = True )
		coredumps = sorted( user.coredumps, key = lambda x: x[2] )
		return dict( user = user, coredumps = coredumps )


	# --------------------------------------------------------------------------
	# Section: Saved XML layouts
	# --------------------------------------------------------------------------

	@util.unicodeToStr
	def getLayout( self, name ):
		recs = list( model.ccSavedLayouts.select( sqlobject.AND(
			model.ccSavedLayouts.q.userID == util.getSessionUID(),
			model.ccSavedLayouts.q.name == name ) ) )
		if len( recs ) == 1:
			return recs[0]
		elif len( recs ) == 0:
			return None
		else:
			log.critical( "Multiple saved layouts called '%s' exist for %s",
						  name, util.getSessionUsername() )

	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def saveLayout( self, user, name ):

		c = cluster.Cluster()
		user = c.getUser( user )
		stream = StringIO()
		user.saveToXML( stream )

		# Delete any existing query with the same name
		old = self.getLayout( name )
		if old: old.destroySelf()

		model.ccSavedLayouts( user = util.getSessionUser(),
							  name = name,
							  serveruser = user.name,
							  xmldata = stream.getvalue() )

		return "Server layout saved successfully"

	@identity.require( identity.not_anonymous() )
	@expose()
	def deleteLayout( self, name ):
		rec = self.getLayout( name )
		if not rec:
			return self.error( "Can't delete non-existant layout '%s'" % name )
		else:
			rec.destroySelf()
			raise redirect( "layouts" )

	@identity.require(identity.not_anonymous())
	@expose( template="web_console.cluster_control.templates.layouts" )
	def layouts( self ):
		recs = model.ccSavedLayouts.select(
			model.ccSavedLayouts.q.userID == util.getSessionUID() )

		# Convert each XML layout into a mapping of process counts
		layouts = []
		pnames = set()
		for rec in recs:
			counts = {}
			layout = cluster.User.parseXMLLayout( rec.xmldata )
			for mname, pname in layout:
				pnames.add( pname )
				if counts.has_key( pname ):
					counts[ pname ] += 1
				else:
					counts[ pname ] = 1
			layouts.append( counts )

		# Not all layouts have the same types of server processes.
		# If a particular process type (e.g. bots) is missing, set
		# count for that process type to 0.
		for counts in layouts:
			for pname in pnames.difference( counts.keys() ):
				counts[ pname ] = 0

		# Sort pnames by pre-arranged ordering here
		pnames = list( pnames )
		pnames.sort( cluster.Process.cmpByName )

		return dict( recs = recs, layouts = layouts, pnames = pnames )



	# Return the requested customWatcherPage for the current
	# sessions user or if provided an alternate user.
	@identity.require(identity.not_anonymous())
	def getCustomWatcherPage( self, customWatcherPage, userid=None ):
		if userid == None:
			userid = util.getSessionUID()

		watcherObj = list(model.ccCustomWatchers.select( sqlobject.AND(
				model.ccCustomWatchers.q.userID == userid,
				model.ccCustomWatchers.q.pageName == customWatcherPage ) ))
		return watcherObj[0]


	# Display the contents of a custom watcher page
	@identity.require(identity.not_anonymous())
	@expose(template="web_console.cluster_control.templates.viewcustomwatcher")
	@util.unicodeToStr
	def viewCustomWatcher( self, name=None ):

		if not name:
			return self.error( "No custom watcher page name provided." )

		try:
			page = self.getCustomWatcherPage( name )
		except:
			return self.error( "No custom watcher page '%s' known." % name )

		try:
			recs = list( model.ccCustomWatcherEntries.select(
						 model.ccCustomWatcherEntries.q.customWatcherPageID ==
							page.id ))
		except Exception, e:
			recs = []
			msg = "No watchers attached to this custom layout"

		# If there are no watcher path entries for this page (which could occur
		# through reloading the page after deleting the final watcher path),
		# redirect back to the list of custom watcher pages.
		if not len(recs):
			raise redirect( turbogears.url( "customWatcherList" ) )

		# Store the watchers / components
		queryDict = {}
		for rec in recs:
			if not queryDict.has_key( rec.componentName ):
				queryDict[ rec.componentName ]= []

			queryDict[ rec.componentName ].append( rec.watcherPath )


		# The cluster / process information we need to
		# query for our watcher layouts
		c = cluster.Cluster()
		user = util.getUser( c )
		procs = user.getServerProcs()

		# Each component has a list inserted into a dict containing:
		# ( [watchers, ... ],
		#   { 'component': [ val, ... ], ... }
		# )
		#
		# eg:
		# ( [ 'load', 'gameTime', ... ],
		#   { 'cellapp01': [ '0.0140622', '0.24834', '0.8734552' ],
		#     'cellapp02': [ '0.0140622', '0.24834', '0.8734552' ] }
		# )

		# NB: Potential change for this structure to provide a
		#     more flexible user based option display in the template.
		#
		#
		# ( [ 'load', 'gameTime', ... ],
		#   [ 'cellapp01', 'cellapp02', ... ],
		#   [
		#       ['0.0140622', '0.24834', ... ],
		#       ['0.24834', '0.8734552', ... ],
		#       ...
		#   ]
		# )

		finalResults = {}

		for i in procs:

			if not queryDict.has_key( i.name ):
				continue

			if not finalResults.has_key( i.name ):
				finalResults[ i.name ] = (queryDict[ i.name ], {})


			# Remember the process label so we can display it
			pname = i.label()

			# Insert the current process into the list of all processes
			# of the same component type
			if not finalResults[ i.name ][1].has_key( pname ):
				finalResults[ i.name ][1][ pname ] = []

			vlist = queryDict[ i.name ]

			for strPath in vlist:

				# TODO: when types are implemented for the watcher
				#       add the type as a 3rd field
				tmpres = i.getWatcherValue( strPath )

				# If the value returned by the Watcher happens to be None
				# the process will be set to mute, so we need to force
				# it back for any subsequent requests to work.
				i.mute = False

				tmp = ( strPath, tmpres )

				finalResults[ i.name ][1][ pname ].append( tmpres )

		return dict( results=finalResults, name=name )


	# Called by static/js/watchers.js: deleteCustomWatcher()
	@identity.require(identity.not_anonymous())
	@ajax.expose
	def deleteCustomWatcher( self, customWatcher ):

		try:
			watcherObj = self.getCustomWatcherPage( customWatcher )
			# Due to the table definitions, destroying this object will
			# cascade row deletions into the cc_custom_watcher_entries table
			watcherObj.destroySelf()
		except Exception, e:
			log.error( "Exception while searching for custom watcher entry" )
			log.error( str(e) )

			return "Unable to find the requested custom watcher page to delete"

		return "Custom watcher %s deleted" % customWatcher


	# Display a list of custom watchers for the current user
	@identity.require(identity.not_anonymous())
	@expose( template="web_console.cluster_control.templates.customwatchers" )
	def customWatcherList( self ):

		c = cluster.Cluster()
		user = util.getUser( c )

		customList = self.getCustomWatcherList()
		customWatchers = []

		# Generate the list of watchers and associated menu items
		for custom in customList:

			try:
				entryObj = model.ccCustomWatcherEntries.select( sqlobject.AND(
					model.ccCustomWatcherEntries.q.customWatcherPageID ==
					custom.id ) )
				count = len(list(entryObj))
			except Exception, e:
				log.error( "Trying to determine number of Custom Watcher entries: %s",
							str(e) )
				count = 0

			menu = util.ActionMenuOptions()
			menu.addScript( "Delete",
				# NB: The comma after 'custom.name' is required so
				#     that the variable is passed through to the
				#     javascript function correctly.
				args = ( custom.pageName, ),
				script = "deleteCustomWatcher" )

			customWatchers.append( (custom, menu, count) )


		return dict( customWatchers=customWatchers, user=user )


	# Called by static/js/watchers.js: saveWatcherToLayout()
	#
	# Used to save a specific component/watcher pair into a
	# custom watcher page.
	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def saveValueToCustomWatcher( self, customWatcher, component, watcher ):
		try:
			watcherObj = self.getCustomWatcherPage( customWatcher )
		except Exception, e:
			return "Error encountered while adding watcher to custom watcher.\n%s" % str( e )

		try:
			model.ccCustomWatcherEntries( customWatcherPage = watcherObj,
									componentName = component,
									watcherPath = watcher )
		except Exception, ie:
			return "'%s' already saved to custom watcher '%s'" % ( watcher, customWatcher )

		return "'%s' added to %s." % ( watcher, customWatcher )


	# Called by static/js/watchers.js: createWatcherPage()
	# to create a new watcher page layout for a user
	@identity.require( identity.not_anonymous() )
	@ajax.expose
	def createCustomWatcher( self, user, name ):

		c = cluster.Cluster()
		user = c.getUser( user )

		model.ccCustomWatchers( user = util.getSessionUID(),
							  pageName = name)

		return "Custom watcher page successfully created"


	# Called by static/js/watchers.js: deleteCustomWatcherEntry()
	# Delete a single watcher path associated to a component/page
	@identity.require( identity.not_anonymous() )
	@ajax.expose
	@util.unicodeToStr
	def deleteCustomWatcherEntry( self, customWatcherPage,
								  component, watcherPath ):

		# Find the user
		# find the customWatcherPage
		try:
			page = self.getCustomWatcherPage( customWatcherPage )
		except:
			return "Failed to find custom watcher page '%s'" % customWatcherPage

		try:
			entries = list(model.ccCustomWatcherEntries.select(
			sqlobject.AND(
				model.ccCustomWatcherEntries.q.customWatcherPageID == page.id,
				model.ccCustomWatcherEntries.q.componentName == component,
				model.ccCustomWatcherEntries.q.watcherPath == watcherPath )))

			for i in entries:
				i.destroySelf()
		except:
			return "Failed to locate and destroy entry for '%s'" % watcherPath

		return "Watcher path removed"


	@identity.require(identity.not_anonymous())
	@expose( template="web_console.common.templates.error" )
	def error( self, msg ):
		debugmsgs = util.getDebugErrors()
		return dict( msg = msg, debug = debugmsgs )
