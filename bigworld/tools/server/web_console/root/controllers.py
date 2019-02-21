# Standard Python includes
import traceback
import sys
import time
import inspect
from StringIO import StringIO

# Turbogears stuff
import cherrypy
import turbogears
from turbogears import controllers, expose, redirect, identity
from turbogears import validate, validators
from turbogears.toolbox.catwalk import CatWalk

# Other BigWorld python modules
import bwsetup; bwsetup.addPath( "../.." )
from web_console.common import model
from web_console.common import util
from web_console.common import ajax
from pycommon import async_task
from pycommon import cluster
from pycommon import log

# Allows disabling of Dev Viewer
ENABLE_DEV_VIEWER = False
ENABLE_SPACE_VIEWER = False

class Root( controllers.RootController ):

	def __init__( self, *args, **kw ):
		controllers.RootController.__init__( self, *args, **kw )

		# This doesn't actually prevent people from accessing these pages, it
		# just prevents links to these pages being shown in the nav menu
		isAdmin = lambda: "admin" in cherrypy.request.identity.groups

		import web_console.cluster_control.controllers
		self.cc = web_console.cluster_control.controllers.ClusterControl(
			self, "Cluster Control", "cc",
			"/static/images/cluster_control.png", lambda: not isAdmin() )

		import web_console.log_viewer.controllers
		self.log = web_console.log_viewer.controllers.LogViewer(
			self, "Log Viewer", "log",
			"/static/images/log_viewer.png", lambda: not isAdmin() )

		import web_console.stat_grapher.controllers
		self.statg = web_console.stat_grapher.controllers.StatGrapher(
			self, "Stat Grapher", "statg",
			"/static/images/stat_grapher.png", lambda: not isAdmin() )

		import web_console.console.controllers
		self.console = web_console.console.controllers.Console(
			self, "Python Console", "console",
			"/static/images/console.png", lambda: not isAdmin() )

		import web_console.commands.controllers
		self.commands = web_console.commands.controllers.Commands(
			self, "Commands", "commands",
			"/static/images/console.png", lambda: not isAdmin() )

		import web_console.admin.controllers
		self.admin = web_console.admin.controllers.Admin(
			self, "Admin", "admin",
			"/static/images/admin.png", isAdmin )

		if ENABLE_SPACE_VIEWER:
			import web_console.space_viewer.controllers
			self.sv = web_console.space_viewer.controllers.SpaceViewer(
				self, "Space Viewer", "sv",
				"/static/images/sv.png", lambda: not isAdmin() )

		if ENABLE_DEV_VIEWER:
			import web_console.service_status.controllers
			self.ss = web_console.service_status.controllers.ServiceStatus(
				self, "Service Status", "ss",
				"/static/images/admin.png", lambda: not isAdmin() )

	def cp_on_http_error(self, status, message):

		if status == 500:
			template = "web_console.common.templates.exception"

			trace = StringIO()
			(ty, val, tr) = sys.exc_info()
			traceback.print_exc( file=trace )

			output = dict(
				exception = str( val[0] ),
				traceback = trace.getvalue(),
				time = time.ctime() )

			log.error( "Exception raised:\n%s", output[ "traceback" ] )

		else:
			template = "web_console.common.templates.generic_error"

			error = {
				400: u'400 - Bad Request',
				401: u'401 - Unauthorized',
				403: u'403 - Forbidden',
				404: u'404 - Not Found',
				500: u'500 - Internal Server Error',
				501: u'501 - Not Implemented',
				502: u'502 - Bad Gateway',
			}.get(status, message or u'General Error')

			output = dict(
				message = message or '-',
				error = error )

			log.error( "Generic error: %s\n%s", message, error )

		format = 'html'
		content_type = 'text/html'
		mapping = None

		# Return customized page
		body = controllers._process_output(output, template,
			format, content_type, mapping)
		cherrypy.response.headers['Content-Length'] = len(body)
		cherrypy.response.body = body

	# Attach the error handler
	_cp_on_http_error = cp_on_http_error


	@identity.require( identity.not_anonymous() )
	@expose( template = "common.templates.exception" )
	def exception( self, enum ):
		e = "Exception number '%s' is invalid." % enum
		t = ""
		if ajax.exceptions.has_key( int(enum) ):
			(e, t, etime) = ajax.exceptions[ int(enum) ]
		return dict( exception=e, traceback=t, time=time.ctime( etime ) )


	@expose()
	def index( self ):
		if "admin" in cherrypy.request.identity.groups:
			return redirect( "admin" )
		else:
			return redirect( "cc" )

	@expose()
	def login( self, *args, **kw ):
		if not cherrypy.request.identity.anonymous \
			   and identity.was_login_attempted() \
			   and not identity.get_identity_errors():
			raise redirect( "/" )
		else:
			return self.error( "Login incorrect" )

	@expose()
	def logout( self ):
		if ENABLE_SPACE_VIEWER:
			self.sv.onLogOut()
		cherrypy.request.identity.logout()
		raise redirect( "/" )

	@expose( template="web_console.common.templates.error" )
	def error( self, msg = None ):
		return dict( msg = msg, debug = [] )

	@expose()
	def accessdenied( self, **kw ):
		return self.error( "You do not have access to that page" )

	# The default implementation of this is so damn verbose!
	def _cp_log_access(self):
		tmpl = '%(h)s %(u)s "%(r)s" %(p)s'
		try:
			username = cherrypy.request.user_name
			if not username:
				username = "-"
		except AttributeError:
			username = "-"

		s = tmpl % {'h': cherrypy.request.remoteHost,
					'u': username,
					'r': cherrypy.request.path,
					'p': cherrypy.request.params }

		self.accesslog.info( s )

	# ------------------------------------------------------------------------------
	# Section: AsyncTask stuff
	# ------------------------------------------------------------------------------

	# For polling async tasks and returning JSON-able data structures
	@validate( validators = dict( id = validators.Int(),
								  blocking = validators.StringBool() ) )
	@ajax.expose
	def poll( self, id, blocking = False ):

		task = async_task.AsyncTask.get( id )
		updates = task.poll( blocking )

		if updates:
			for state, data in updates:
				if state == "finished":
					task.terminate()
					break
			return dict( status = "updated", updates = updates )

		else:
			return dict( status = "nochange" )


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	@validate( validators = dict( id = validators.Int() ) )
	def terminate( self, id ):

		try:
			async_task.AsyncTask.get( id ).terminate()

		# We may get terminate() calls from the client-side when the server-side
		# function call has already cleaned itself up.  That's OK.
		except KeyError:
			pass


	@identity.require( identity.not_anonymous() )
	@ajax.expose
	@validate( validators = dict( id = validators.Int() ) )
	def callExposed( self, type, method, onSuccess = None, **kw ):
		"""
		Calls a method on an instance of cluster.Exposed.  The 'type' parameter
		controls how the object is looked up, and each lookup method has its own
		arguments which are passed via **kw.  At the moment, only 'process' is
		understood.

		Args to be passed to the callback are also in **kw, but should have all
		been prefixed with __ to distinguish them from the arguments that
		qualify 'type'.
		"""

		c = cluster.Cluster.get()

		# We only know how to look up processes at the moment
		if type == "process":
			machine = c.getMachine( kw[ "machine" ] )
			if not machine:
				raise ajax.Error, "Unknown machine '%s'" % kw[ "machine" ]
			obj = machine.getProc( int( kw[ "pid" ] ) )

		callback = getattr( obj, method )

		# Marshal arguments to pass to the callback
		cbkw = {}
		for k, v in kw.items():
			if k.startswith( "__" ):
				cbkw[ k[2:] ] = v

		callback( **cbkw )
		if onSuccess:
			return onSuccess
		else:
			return
