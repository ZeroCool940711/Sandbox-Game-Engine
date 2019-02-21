import telnetlib
import re

# Import standard modules
from datetime import datetime
from StringIO import StringIO
import os
import fnmatch
import logging

# Import third party modules
from turbogears.controllers import (RootController, expose, error_handler,
                                    flash, redirect, validate)
from turbogears import identity
import turbogears
from sqlobject import AND, OR, SQLObject
import sqlobject
import cherrypy
import simplejson

# Import BigWorld modules
import bwsetup; bwsetup.addPath( "../.." )
from web_console.common import ajax
from web_console.common import module
from web_console.common import util
from web_console.common.model import User
from pycommon import cluster, watcher_data_type as WDT

# Import local modules
import model
#import runscript
#import runscript_db

log = logging.getLogger( "runscript"  )

# --------------------------------------------------------------------------
# Section: Python console
# --------------------------------------------------------------------------
class Console( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
#		self.addPage( "Run scripts", "runscript" )
		#self.addPage( "Edit scripts", "managescript" )
		self.addPage( "Python Console", "list" )
		self.addPage( "Help", "help", popup=True )

		# Reload scripts from file
		try:
			self.loadScriptsFromDir()
			self.removeDeletedScripts()
			self.loadScriptFail = False
		except Exception, e:
			self.loadScriptFail = True

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: Python Console" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="console" )

	# Request Processing
	@identity.require(identity.not_anonymous())
	@ajax.expose
	def process_request(self, line, host, port):
		tn = telnetlib.Telnet( str(host), str(port) )

		# Read the connection info from the component
		tn.read_until( ">>> " )

		# do the line write / read
		result = self.process_line(tn, line)

		# Close the session
		tn.close()

		# Prefix the command with a python console indicator, and
		# strip off the trailing console marks as well as the
		# \r\n that telnet has sent through
		result = ">>> " + result[:-6]

		return dict( more=False, output=result )


	def process_line(self, tn, line):
		line = line + "\r\n"

		# Write out the command
		tn.write( str( line ) )

		# Read back the response (including the command executed)
		(index, regex, result) = tn.expect( [ ">>> ", "\.\.\. " ] )

		return result


	# Multiline processing
	@identity.require(identity.not_anonymous())
	@ajax.expose
	def process_multiline_request(self, block, host, port):
		(tn, output) = self.connect( host, port )

		if tn:
			# Force a final line after the text block to ensure
			# any indented code is executed by the interpreter
			block = block + "\n"

			lines = [line for line in block.split('\n')]

			output = ""
			for line in lines:
				output = output + self.process_line( tn, line )

			# Close the session
			tn.close()

			# Prefix the command with a python console indicator, and
			# strip off the trailing console marks as well as the
			# \r\n that telnet has sent through
			output = ">>> " + output[:-6]

		return dict( more=False, output=output )


	def connect(self, host, port):
		tn = None
		result = None
		try:
			tn = telnetlib.Telnet( str(host), str(port) )

			# Read the connection info from the component
			result = tn.read_until( ">>> " )

			return (tn, result[:-6])
		except:
			return (tn, result)


	## Main console
	@identity.require( identity.not_anonymous() )
	@expose( template="console.templates.console" )
	def console( self, host, port, process, tg_errors=None ):
		tn, banner = self.connect( host, port )
		if tn:
			tn.close()
		banner = re.sub( "\r\n", " - ", banner)
		return dict( hostname = host, port = port,
					 process_label = process, banner = banner )


	@identity.require( identity.not_anonymous() )
	@expose( template="console.templates.list" )
	def list( self, user = None ):

		c = cluster.Cluster()
		user = util.getUser( c, user )

		# Generate list of processes that have a python console
		procs = sorted( [p for p in user.getProcs() if
						 p.name in ("cellapp","baseapp","bots")] )

		# Generate mapping for python ports
		ports = {}
		procsToRemove = set()
		for p in procs:
			port = p.getWatcherValue( "pythonServerPort" )
			if port:
				ports[ p ] = port
			else:
				#procs.remove( p )
				procsToRemove.add( p )

		for p in procsToRemove:
			procs.remove( p )

		# Generate labels
		labels = {}
		for p in procs:
			labels[ p ] = "%s on %s" % (p.label(), p.machine.name)

		return dict( procs = procs, ports = ports, labels = labels,
					 user = user )

#	# --------------------------------------------------------------------------
#	# Section: Runscript
#	# --------------------------------------------------------------------------
#	@identity.require( identity.not_anonymous() )
#	@expose( template="console.templates.runscript" )
#	def runscript( self, id=None ):
#		"""
#		WebConsole page which presents an interface for select a script to run
#		and actually running it.
#
#		@param id    ID of a script to execute straight away.
#		             TODO: Get it working again, as it's currently not.
#					 This will be done in runscript.kid and runscript.js.
#
#					 However you'll need to pass an argument value list
#					 to for scripts which take arguments. The alternative
#					 is not to have the script execute straight away, but
#					 simply just to show up, ready for the user to enter
#					 argument values and execute.
#		"""
#		c = cluster.Cluster()
#		user = util.getUser( c )
#		# Scripts are provided in a tree structure 
#		categories = runscript.getCategories()
#		return dict(
#			categories = categories,
#			runNow = None,
#			serverRunning = user.serverIsRunning()
#		)
#
#	# XXX: This functionality has been disabled until python console
#	#      improvements are implemented. eg: save console history /
#	#	   send to all components
#	#@identity.require( identity.not_anonymous() )
#	#@expose( template="console.templates.managescript" )
#	#def managescript( self ):
#	#	"""
#	#	WebConsole page which lists scripts that can be edited,
#	#	deleted, copied, etc.  Also links to the "createscript" page.
#	#	"""
#	#	scripts = runscript_db.getScripts( userOnly=False )
#	#	user = util.getSessionUser()
#	#	myScripts = [(s, self.getScriptActions(s))
#	#			for s in scripts if s.id == user.id]
#	#	otherScripts = [(s, self.getScriptActions(s))
#	#			for s in scripts if s.id != user.id]
#	#	return {"myScripts": myScripts, "otherScripts":otherScripts}
#
#	@identity.require( identity.not_anonymous() )
#	@expose( )
#	def deletescript( self,id=None ):
#		"""
#		Remove a script from the database.
#
#		@param id    The string ID of the script to remove. Should only accept
#		             'db' type ids.
#		"""
#		raise NotImplemented
#
#	@identity.require( identity.not_anonymous() )
#	@expose( allow_json=True )
#	def executescript( self, id=None, args=None, runType=None ):
#		"""
#		AJAX interface for executing a script.
#
#		@param id    The string ID of the script that needs to be run.
#		@param args  The JSON encoded list of arguments of the format:
#			         [(value, WatcherTypeEnum), ...]
#			         e.g. [(100, 1), ('hello', 5)]
#		             The WatcherTypeEnum will be used to parse the value from
#					    string format to a more appropriate format.
#		@param runType  The method by which the server will be run. Typically
#		                mirrors the RunScript.runType attribute, but can be
#						overridden from the webpage. Possible values are:
#						"all", "any", or a comma-delimited list of ints
#		"""
#		args = simplejson.loads( args )
#		script = runscript.getScript( id );
#		if runType != None:
#			runType = runType.encode( "latin1" )
#
#		argList = []
#		argCount = 0
#		canExecute = True
#		output = runscript.RunScriptOutput()
#		for value, valueType in args:
#			argCount += 1
#
#			if not len(value) and valueType != WDT.WATCHER_TYPE_STRING:
#				output.addErrorMessage( "Argument %d: No value provided.",
#					argCount )
#				canExecute = False
#				continue
#
#			try:
#				# TODO:
#				#
#				# More sensible conversion of the argument values from
#				# strings to WatcherDataTypes
#				#
#				# e.g. At the moment a WatcherDataTypeBool is created simply
#				# by passing the string through "bool()", which returns
#				# True on nonempty strings regardless of content.
#				#
#				wdt = WDT.WDTRegistry.getClass( valueType )( \
#					value.encode('latin1') )
#				argList.append( wdt )
#			except Exception, e:
#				output.addErrorMessage( "Argument %d: Couldn't convert \"%s\"" \
#					" to %s: %s",
#					argCount, value, WDT.toString( valueType ), e )
#				canExecute = False
#
#
#		if canExecute:
#			try:
#				script.execute( argList, output, runType )
#			except Exception, e:
#				output.addErrorMessage( "Error running script: %s: %s",
#					type(e), e )
#				raise
#		#print "Output: %s" % (output)
#		return {"output": output}
#
#	@identity.require( identity.not_anonymous() )
#	@expose( allow_json=True )
#	def scriptinfo( self, id=None ):
#		"""
#		AJAX interface for retrieving information about a script.
#		Returns a dictionary containing the following key/values:
#			- title: Script title
#			- procType: Type of component the script runs on
#			- runType: Method by which the script should be run.
#			- desc: Description of the script.
#			- args: JSON encoded argument list
#
#		@param id   The string ID of the script.
#		"""
#		script = runscript.getScript( id )
#		script.retrieveInfo()
#		return {
#			"title": script.title,
#			"procType": script.procType,
#			"runType": script.runType,
#			"desc": script.desc,
#			"args": script.argsToStringList( script.args ),
#		}
#
#	@identity.require( identity.not_anonymous() )
#	@expose( allow_json=True )
#	def getscripts( self, type, category ):
#		"""
#		AJAX interface for retrieving a list of scripts belonging to a
#		category.
#
#		@param type      The source of the scripts that the category belongs to
#		@param category  The name of the category from we want the list of
#		                 scripts
#		"""
#		type = type.encode("latin1")
#		category = category.encode("latin1")
#		scriptList = runscript.getScriptsFromCategory( type, category )
#		scripts = [{"id":s.id, "title":s.title} for s in scriptList]
#		return {"scripts": scripts}
#
#	@identity.require( identity.not_anonymous() )
#	@expose( template="console.templates.createscript" )
#	def createscript( self, mode="create", **kwargs ):
#		"""
#		WebConsole page allowing the user to create a new database script.
#
#		@param mode      "submit" if we're submitting data for script creation
#		                 or "create" if we just want the form in order to enter
#					     the data.
#		@param kwargs    All the data entered in the form on the webpage.
#		"""
#		dct = {
#			"error":[],
#			"action":"add",
#			"validProcTypes": runscript.RunScript.validProcTypes,
#			"validRunTypes": runscript.RunScript.validRunTypes,
#			"dest":"createscript"}
#
#		if mode == "submit":
#			# User has submitted data
#			data, errors = runscript.RunScript.processScriptData( kwargs )
#			if errors:
#				dct["error"] = errors
#				dct.update( kwargs )
#				log.debug( "Error, returning dict: %s", kwargs )
#				return dct
#			else:
#				try:
#					runscript_db.DbRunScript.createScript( data['title'],
#						data['code'], data['args'], data['worldReadable'],
#						data['lockCells'], data['procType'], data['runType'],
#						desc=data['desc'] )
#						#desc=data['desc'], tags=data['tags'] )
#				except sqlobject.dberrors.DuplicateEntryError, e:
#					dct["error"] = ["The name '%s' is already taken, please " \
#						"choose a different name" % (data['title'])]
#				except Exception, e:
#					dct["error"] = [str(e)]
#
#				if dct["error"]:
#					dct.update( kwargs )
#					log.debug( "Error, returning dict: %s", dct )
#					return dct
#
#				raise redirect( "managescript" )
#
#		elif mode == "create":
#			# User has not submitted data
#			dct.update( self.blankScriptDict() )
#			return dct
#
#		raise Exception( "Unknown mode type: %s" % (mode) )
#
#	@identity.require( identity.not_anonymous() )
#	@expose( template="console.templates.viewscript" )
#	def viewscript( self, id=None, triedEdit=None ):
#		""" View a script """
#		raise NotImplemented
#
#	@expose()
#	@identity.require( identity.not_anonymous() )
#	def copyscript( self, id ):
#		""" Make a copy of the script """
#		raise NotImplemented
#
#
#	@identity.require( identity.not_anonymous() )
#	@expose( template="console.templates.createscript" )
#	def editscript( self, id, mode="edit", **kwargs ):
#		"""
#		WebConsole page allowing the user to edit an existing database script.
#		This page uses the same kid template as "createscript"
#
#		@param mode      "submit" if we're submitting data for script creation
#		                 or "edit" if we just want the form in order to enter
#					     the data.
#		@param kwargs    All the data entered in the form on the webpage.
#		"""
#		dct = {
#			"error":[],
#			"action":"edit",
#			"validProcTypes": runscript.RunScript.validProcTypes,
#			"validRunTypes": runscript.RunScript.validRunTypes,
#			"dest":"editscript?id=%s" % (id) }
#
#		rs = runscript.getScript(id)
#		#print rs
#
#		if mode == "submit":
#			# User has submitted data
#			data, errors = runscript.RunScript.processScriptData( kwargs )
#			if errors:
#				dct["error"] = errors
#				dct.update( kwargs )
#				log.debug( "Error, returning dict: %s", kwargs )
#				return dct
#			else:
#				try:
#					rs.update( data )
#				except sqlobject.dberrors.DuplicateEntryError, e:
#					dct["error"] = ["The name '%s' is already taken, please " \
#						"choose a different name" % (data['title'])]
#				except Exception, e:
#					dct["error"] = [str(e)]
#
#				if dct["error"]:
#					dct.update( kwargs )
#					log.debug( "Error, returning dict: %s", dct )
#					return dct
#
#				raise redirect( "managescript" )
#
#		elif mode == "edit":
#			# User has not submitted data
#			dct.update( rs.getDict() )
#			dct["jsArgs"] = runscript.RunScript.argsToJS( rs.args )
#			return dct
#
#		raise Exception( "Unknown mode type: %s" % (mode) )
#
#	# --------------------------------------------------------------------------
#	# Section: Runscript utility functions
#	# --------------------------------------------------------------------------
#	def getScriptActions( self, script ):
#		""" Creates an appropriate ActionMenuOptions object for a script.  """
#		user = util.getSessionUser()
#		actions = util.ActionMenuOptions()
#
#		actions.addRedirect( "Run",
#				turbogears.url( "/console/runscript", id = script.id ),
#				help = "Run this script" )
#
#		if script.user == user:
#			actions.addRedirect( "Edit",
#				"/console/editscript?action=edit&id=%s" % (script.id),
#				help="Edit this script" )
#
#			actions.addScript( "Delete",
#				"""
#				var goAhead=confirm(
#					"Are you sure you want to delete script '%s'?");
#				if (goAhead) {window.location="/console/deletescript?&id=%s";}
#				""" % (script.title, script.id),
#				help="Delete this script" )
#
#			actions.addRedirect( "Duplicate",
#					turbogears.url( "/console/dupscript", id = script.id ),
#					help = "Make a copy of this script" )
#
#		if script.user != user:
#			actions.addRedirect( "View source",
#					"/console/viewscript?id=%s" % (script.id),
#					help="View script source" )
#
#		return actions
#
#	def blankScriptDict( self ):
#		""" Returns a blank script dictionary """
#		return dict( title="", worldReadable=False, lockCells=False,
#			procType="cellapp", runType="any", code="", desc="", jsArgs=[])
#
#
#
#	def runWatcherScript( self, procName, watcher, code, output=None ):
#		c = cluster.Cluster()
#		user = util.getUser( c ) # Cluster.User object
#		proc = user.getProc( procName )
#		if proc == None:
#			raise ScriptRunException( "No %s available to run the script. Is the server running?" % \
#				(procName) )
#
#		if output: output.info( "Retrieved %s on %s(%s)" , proc.label(),
#			proc.machine.name, proc.machine.ip )
#
#		# Check we have processes to run it on
#		childProcs = {"baseappmgr": "baseapp", "cellappmgr": "cellapp"}
#		procs = user.getProcs( childProcs[procName] )
#		if not procs:
#			output.warning( "There are no %ss, so this script will " \
#				"probably have no effect.", childProcs[procName] )
#
#		code = code.replace('\r\n', '\n')
#		result = proc.setWatcherValue( watcher, code )
#		if output: output.info( "Executed script!" )
#		return result

	# --------------------------------------------------------------------------
	# Section: Misc
	# --------------------------------------------------------------------------
	@identity.require( identity.not_anonymous() )
	@expose()
	def index( self ):
		raise redirect( "list" )
