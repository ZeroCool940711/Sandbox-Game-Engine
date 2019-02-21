import re
import os
import sys
import logging
import zipfile

import cherrypy
import turbogears
from turbojson import jsonify

import bwsetup; bwsetup.addPath( "../.." );
from pycommon import cluster
from pycommon import log
import format_docs


def alterParams( **kw ):
	path = cherrypy.request.path
	params = dict( cherrypy.request.params )

	# Bugfix for the cluster control watcher page (which seems to be
	# the only code using this utility function).
	if params.has_key( "newval" ):
		del( params["newval"] )

	params.update( kw )
	return turbogears.url( path, **params )


def getSessionUsername():
	idobj = cherrypy.request.identity
	if idobj.anonymous:
		return None
	else:
		return idobj.user.user_name

def getSessionUser():
	idobj = cherrypy.request.identity
	if idobj.anonymous:
		return None
	else:
		return idobj.user

def getSessionUID():
	idobj = cherrypy.request.identity
	if idobj.anonymous:
		return None
	else:
		return idobj.user.id


def getServerUsername():
	idobj = cherrypy.request.identity
	if idobj.anonymous:
		return None
	else:
		return idobj.user.serveruser


def getUser( c, username = None ):
	"""
	Fetch the cluster.User object for the given username, or for the current
	server user if username is None.
	"""

	if not c.getMachines():
		raise turbogears.redirect(
			"/error", msg = "No bwmachined daemons are running")		

	try:
		if not username:
			username = getServerUsername()
		return c.getUser( username )
	except cluster.User.error:
		raise turbogears.redirect(
			"/error", msg = "Couldn't resolve user %s" % username )


# Set up buffered log output
gBufHandler = log.TwinBufferHandler()
gBufHandler.setFormatter( log.CleanFormatter() )
logging.getLogger( "bigworld" ).addHandler( gBufHandler )

# These two convenience functions provide access to debug.py *_MSG output
def getDebugErrors():
	return list( gBufHandler.errors )

def getDebugMessages():
	return list( gBufHandler.messages )

def clearDebugOutput():
	gBufHandler.clear()


# ------------------------------------------------------------------------------
# Section: Decorators
# ------------------------------------------------------------------------------

def unicodeToStr( f ):
	"""
	Converts unicode arguments to strings, useful because SQLObject doesn't deal
	nicely with unicode and doing string == doesn't work with unicode.
	"""

	def execute( *args, **kw ):

		for i in xrange( len( args ) ):
			if type( args[i] ) == unicode:
				if type( args ) == tuple:
					args = list( args )
				args[i] = str( args[i] )

		for k, v in kw.items():
			if type( v ) == unicode:
				kw[ k ] = str( v )

		return f( *args, **kw )

	return execute


def unzip( zipPath, dirName ):
	log.debug( "opening zip file %s" % zipPath )
	zf = zipfile.ZipFile( zipPath )
	extractFile = None

	try:
		infoList = zf.infolist()
		for info in infoList:
			log.debug( "unzipping %s..." % info.filename )

			# is it a dir?
			if info.filename.endswith( '/' ):
				# user and group read, write and executable
				os.umask( 0007 )
				os.mkdir( info.filename )
			else:
				contents = zf.read( info.filename )

				extractFile = open( dirName + os.sep + info.filename, "w" )
				extractFile.write( contents )
				extractFile.close()
				extractFile = None
	finally:
		if extractFile:
			extractFile.close()
		zf.close()


# This makes sure you have the right Dojo version extracted
def verifyDojo():

	jsRoot = bwsetup.appdir + "/static/js"
	dojoDir = jsRoot + "/dojo"

	zips = [f for f in os.listdir( jsRoot )
			if re.match( "dojo.*zip", f )]

	if not zips:
		log.critical( "No dojo archive found in %s", jsRoot )

	elif len( zips ) > 1:
		log.critical( "Multiple Dojo zips found, please delete all but one" )

	dojoZip = zips[0]

	getVersion = lambda s: map( int, s.split( "." ) )
	unGetVersion = lambda a: ".".join( map( str, a ) )

	try:
		zipVer = getVersion( re.search( "dojo.*?([\d\.]+).*?\.zip", dojoZip ).\
				  group( 1 ) )
	except:
		log.critical( "Couldn't determine Dojo version from %s", dojoZip )


	def install():
		oldcwd = os.getcwd()
		os.chdir( jsRoot )
		try:
			unzip( dojoZip, '.' )
		except Exception, e:
			print str( e )
			log.critical( "Couldn't extract %s" % dojoZip )


		try:
			os.rename( dojoZip.replace( ".zip", "" ), "dojo" )
		except Exception, e:
			log.critical( "Couldn't rename extracted dojo\n%s", e )

		try:
			open( "dojo/VERSION", "w" ).write( unGetVersion( zipVer ) )
		except Exception, e:
			log.critical( "Couldn't write Dojo version to VERSION\n%s", e )

		# We need to re-execute this program because for some reason, extracting
		# the Dojo archive confuses the auto-reloader or something and we will
		# get a database error if we try to proceed after extracting.
		# Re-executing the startup script seems to avoid this.
		os.chdir( oldcwd )
		print sys.argv[0]
		if os.name == "posix":
			os.execv( sys.argv[0], sys.argv )
		else:
			return

	if "dojo" not in os.listdir( jsRoot ):
		install()
		return

	try:
		dojoVer = getVersion( open( "%s/VERSION" % dojoDir ).read() )
	except Exception, e:
		log.critical( "Dojo Toolkit version unknown\n%s", e )

	for i in xrange( len( zipVer ) ):
		if zipVer[i] > dojoVer[i]:
			newDir = dojoDir + "-%s" % unGetVersion( dojoVer )
			log.warning( "Dojo is out of date, renaming old ver -> %s", newDir )
			os.rename( dojoDir, newDir )
			install()
			return


def addContentsToHTML():
	"""
	This method generates a table of contents for each file named *.notoc.html
	in the web_console tree.
	"""

	wcroot = os.path.abspath( bwsetup.appdir + "/.." )

	for path in os.popen( "find %s -name '*.notoc.html'" % wcroot ):

		path = path.strip()
		dir, fname = os.path.split( path )
		rawpath = "%s/%s" % (dir, fname.replace( ".notoc.html", ".html" ))

		if not os.path.exists( rawpath ) or \
		   os.stat( path ).st_mtime > os.stat( rawpath ).st_mtime:
			format_docs.process( path, open( rawpath, "w" ) )
			log.notice( "Generated TOC for %s",
						path.replace( wcroot + "/", "" ) )


class ActionMenuOptions( object ):
	"""
	This is a helper class that must be passed to actionMenu() in
	web_console.common.templates.common.kid.  It has support for option groups
	and can assign XML ids to them.  Actual options can either be redirects or
	javascript calls.
	"""

	class Group( object ):
		def __init__( self, name, id ):
			self.name = name
			self.id = id
			self.options = []


	def __init__( self ):
		self.groups = {}
		self.groupOrder = []


	def addGroup( self, name, id = "" ):
		group = self.Group( name, id )
		self.groups[ name ] = group
		self.groupOrder.append( group )
		return group


	def addRedirect( self, label, href, params = {}, help = "",
					 group = "Action..." ):
		"""
		Add a menu option that will do a redirect when clicked.  The URL can be
		provided entirely inline in the 'href' argument, or the base URL can be
		passed as 'href' and 'params' will be appended as querystring params.
		"""

		try:
			group = self.groups[ group ]
		except KeyError:
			group = self.addGroup( group )

		if params:
			href = turbogears.url( href, **params )

		group.options.append( (label, "window.location = '%s'" % href, help) )


	def addScript( self, label, script, args = None, help = "",
				   group = "Action..." ):
		"""
		Add a menu option that is a JavaScript call.  The call can be provided
		entirely inline in the 'script' argument (with 'args' left as None), or
		if 'args' is passed, the call will be interpreted as 'script( *args )'.
		"""

		try:
			group = self.groups[ group ]
		except KeyError:
			group = self.addGroup( group )

		# If javascript args have been provided, transform the script into a
		# function call
		if args is not None:
			script = ("%s(" % script) + \
					 ",".join( map( jsonify.encode, args ) ) + ")"

		group.options.append( (label, script, help) )
