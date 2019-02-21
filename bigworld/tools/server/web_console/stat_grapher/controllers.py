import cherrypy
import turbogears

from turbogears import controllers, expose, redirect
from turbogears import identity

from pycommon import uid as uid_module
from web_console.common import model, module, util

# Local modules
import utils
import graph
import managedb
import socket

class StatGrapher( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		managedb.setup()
		utils.setup()

		self.graph = graph.StatGrapherBackend()

		self.addPage( "My Processes", "latestUserProcesses" )
		self.addPage( "Machines", "latestMachines" )
		self.addPage( "All Users", "allUsers" )
		self.addPage( "List Of Logs", "list" )
		self.addPage( "Help", "help", popup=True )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: StatGrapher Help" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="stat_grapher" )

	@identity.require(identity.not_anonymous())
	@expose()
	def index(self):
		raise turbogears.redirect( "latestUserProcesses" )

	@identity.require(identity.not_anonymous())
	@expose()
	def logout(self):
		identity.current.logout()
		raise redirect("/")

	@identity.require( identity.not_anonymous())
	@expose()
	def latestUserProcesses( self, **kwargs ):
		userServerName = util.getServerUsername()
		uid = uid_module.getuid( userServerName )

		maxLogDb = self.getLatestUpdatedLog()
		return self.graph.processes( maxLogDb, uid, **kwargs )

	@identity.require( identity.not_anonymous() )
	@expose()
	def latestMachines( self, **kwargs ):
		maxLogDb = self.getLatestUpdatedLog()
		return self.graph.machines( maxLogDb, **kwargs )

	@identity.require( identity.not_anonymous() )
	@expose(html="stat_grapher.templates.listusers")
	def allUsers( self ):
		maxLogDb = self.getLatestUpdatedLog()
		created, used, active, users = \
			managedb.getRawDbManager()._getInfoAboutLogDb( maxLogDb )

		action = turbogears.url("statg/graph/processes")

		# List of users
		users.sort( lambda x, y: cmp( x[1], y[1] ) )

		return dict( outputList = users,
			action = action,
			log = maxLogDb,
			pageHeader="List of users in log database %s" % (maxLogDb) )

	def getLatestUpdatedLog( self ):
		# find the log database that has been updated the latest
		list = managedb.getRawDbManager().getLogDbList()
		if not list:
			raise redirect( "list" )
		maxLogDb = None
		maxUsed = None
		for name, created, used, active, user in list:
			if maxUsed == None or maxUsed < used:
				maxUsed = used
				maxLogDb = name
		return maxLogDb

	@identity.require(identity.not_anonymous())
	@expose(html="stat_grapher.templates.listlogs")
	def list(self):
		list = managedb.getRawDbManager().getLogDbList()
		# List of databases
		outputList = []
		for name, created, used, active, user in list:
			# List of actions
			actions = util.ActionMenuOptions()
			actions.addRedirect( "View machine graphs",
				"../statg/graph/machines/%s" % name,
				help="View a graph of machines recorded " \
					"in \"%s\"" % (name) )
			actions.addRedirect( "View process graphs",
				"../statg/listUsers/%s" % name,
				help="View a graph of a user's processes recorded " \
					"in \"%s\"" % (name) )

			actions.addRedirect( "View preferences",
				"../statg/prefs/%s" % name,
				help="View a list of a preferences used " \
					"in \"%s\"" % (name) )

			outputList.append( (name, created, used, active, actions) )

		return dict( outputList = outputList )

	@identity.require(identity.not_anonymous())
	@expose(html="stat_grapher.templates.listusers")
	def listUsers(self, log):
		created, used, active, users = \
			managedb.getRawDbManager()._getInfoAboutLogDb( log )

		# List of users
		action = turbogears.url("statg/graph/processes")

		# sort by name
		users.sort( lambda x, y: cmp( x[1], y[1] ) )
		return dict( outputList = users,
			action = action,
			log = log,
			pageHeader="List of users in log database %s" % (log) )

	@identity.require(identity.not_anonymous())
	@expose(html="stat_grapher.templates.prefs")
	def prefs(self, log):
		_, prefTree = managedb.ptManager.requestDbManager( log )
		displayPrefs = self.graph.requestDisplayPrefs( log )

		return dict(
			prefTree = prefTree,
			displayPrefs = displayPrefs,
			log = log,
			pageHeader="Preferences used in log database %s" % (log) )
