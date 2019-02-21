import logging

# Import web console modules
from web_console.common import util

from pycommon import cluster, watcher_data_type as WDT, messages
from pycommon import watcher

# Import local modules
import runscript

log = logging.getLogger("runscript")

# -----------------------------------------------------------------------------
# Watcher script class
# -----------------------------------------------------------------------------
class WatcherRunScript( runscript.RunScript ):
	"""
	Class which represents a script exposed through watchers on a cellapp/baseapp.

	Contains functionality for retrieving information about Watcher functions
	as well as executing these scripts.
	"""

	def __init__( self, watcherPath, procType, runType="any", args=None, desc=None ):
		"""
		@param procType     A string which contains the process type that tihs
		                    script should be run on.
							Valid values are: "baseapp" and "cellapp"
		@param watcherPath  The watcher path that needs to be called.
	                        e.g. "command/addGuards"
		@param runType      The method which determines how the script is run.
							Valid values are: "any", "all", or a
							string which contains a comma separated list of
							integers corresponding to.
							e.g. "1,2,34".
							Note that "any" will result in the script being
							called on the least loaded component.
		@param args         A list containing (name, WatcherDataType) tuple
		                    pairs. This list is mostly for user interface
                            purposes and can be used for validating that
                            function arguments are of the right type.
                            Optional parameter.
		@param desc         The script description. Should correspond to the
		                    __doc__ watcher attribute.

		The constructor only requires the minimum amount of information
		to run. That is, only the target and watcherPath is required.
		"""
		scriptName = watcherPath[watcherPath.rfind( '/' ) + 1:]

		runscript.RunScript.__init__( self,
			id     = ":".join( ("watcher", procType, runType, watcherPath) ),
			title  = scriptName,
			code   = None,
			args   = args,
			desc   = desc,
			procType = procType,
			runType = runType,
		)
		self.watcherPath = watcherPath.encode('latin1')

	def execute( self, args, output=None, runType=None ):
		"""
		Runs a watcher script by calling the appropriate method through
		the "forwardTo" watcher on cellappmgr or baseappmgr. The mgr will
		then forward the watcher call to the appropriate group of baseapp
		or cellapps.

		@param args     The list of WatcherDataType objects to send to the
		                function.
		@param output   The RunScriptOutput object that collects the script
		                output for display back at the web page. Note:
		                can be None, hence the: "output and output.method()"
					    code used in this function.
		@param runType  A string which indicates the way in which we choose
		                the processes to run the script on.
						e.g. "all", "any", "1,2,3,15"
						(comma separated component ID list)
		"""
		# TODO: Remove the necessity to build this SET packet 
		# (see comment below)
		wdm = messages.WatcherDataMessage()
		wdm.message = wdm.WATCHER_MSG_SET2
		wdm.count   = 0

		watcherTuple = WDT.WatcherDataTypeTuple( args )
		watcherDestPath = self.getExecutePath( runType )
		log.info( "Executing watcher path: %s", watcherDestPath )
		wdm.addSetRequest( watcherDestPath, watcherTuple )
		try:
			target = self.getTargetProcs()[0]

			# TODO: Replace the call to "sendWatcherMessages" with
			# messages.batchSet() or whatever it's going to be
			# called. This function should be the equivalent of 
			# batchQuery, except for Watcher SET messages instead.

			# Furthermore, we should probably remove the necessity
			# to build a WDM object and just be able to pass Python values
			# directly to this "batchSet" function.

			# e.g. messages.batchSet( target, args )

			results = self.sendWatcherMessages( [target], wdm )
			result = results[target]
			if result[0] == False:
				output and output.addErrorMessage( "Function call did not execute successfully." )
			if result[1]:

				# Generate a nice component name as a prefix
				componentName = ""
				if target.component == "BaseAppMgrInterface":
						componentName = "BaseApp"
				elif target.component == "CellAppMgrInterface":
					componentName = "CellApp"

				# Add the result from each component
				for i in result[1][1]:
					output and output.addResult( "%s %s: %s\n" %
						(componentName, i.value[0].value, i.value[1].value) )

				# display the stdout from each component
				output and output.addOutput( result[1][0].value )
		except runscript.ScriptRunException, e:
			output and output.addErrorMessage( "WebConsole error: %s: %s" % (type(e).__name__, e) )


	def retrieveInfo( self ):
		"""
		Retrieve information from the server about us.
		This will fill in the following attributes in
		our object:
		  - args: Argument list of (name, WatcherDataType type object) pairs
		  - desc: String containing watcher function description. Can contain
		          newlines.
		"""
		watcherArgPath = self.watcherPath + "/__args__"
		watcherDocPath = self.watcherPath + "/__doc__"
		watcherExposePath = self.watcherPath + "/__expose__"

		t = self.getTargetProcs( useForwarder=False )[0]
		r = messages.WatcherDataMessage.batchQuery(
				[watcherArgPath, watcherDocPath, watcherExposePath], [t], 1 )
		types = r[t][watcherArgPath][0][1]
		desc = r[t][watcherDocPath][0][1]
		exposed = r[t][watcherExposePath][0][1]
		self.args = types
		self.desc = desc
		self.runType = exposed

	def getExecutePath( self, runType=None ):
		"""
		Create the Watcher path that we need to call on the baseappmgr or
		cellappmgr.

		The mgr processes have a ForwardingWatcher located at "forwardTo".
		We then need to specify the nature of the forwarding, which we
		get from the runType parameter (or attribute).

		After that, we append the actual watcher path we wish to call on
		the cellapps/baseapps to this path.

		e.g. The watcher path we use in order to run "command/addGuards" on
		     each baseapp would be: "forwardTo/all/command/addGuards". This
		     would be called on the baseapp.

		@param runType A string indicating the nature of the watcher call.
		         e.g. "all" for running on all cellapps
		              "any" for running on any one cellapp (least loaded)
		              "1,2,3" for running on cellapp01, 02 and 03.
		"""
		if runType == None:
			runType = self.runType

		str = watcher.Forwarding.runTypeHintToWatcherPath( runType ) \
				+ "/" + self.watcherPath

		return str


# -----------------------------------------------------------------------------
# General functions
# -----------------------------------------------------------------------------

def getCategories( user=None ):
	"""
	Retrieve the set of categories that watcher scripts use.

	For watcher scripts, categories are defined by any watcher directory entries
	under the "commmand" top level directory on any baseapp/cellapp. It is
	assumed that each baseapp/cellapp contains the same set of Watcher
	functions (in normal use they shouldn't carry different functions,
	although sometimes the server can get into a bad state where it does).
	"""
	if not user:
		c = cluster.Cluster()
		user = util.getUser( c )
	path = "command"
	procs = []
	cellapp = user.getProc("cellapp")
	baseapp = user.getProc("baseapp")
	procs = [p for p in (baseapp, cellapp) if p != None]
	dirs = [""]
	if procs:
		response = messages.WatcherDataMessage.batchQuery( [path], procs, 2 )
		#print "Response", response
		for p, listing in response.iteritems():
			#print "Listing:", listing
			if path in listing:
				dirs.extend( [l[0] for l in listing[path] if l[3] == \
					messages.WatcherDataMessage.WATCHER_MODE_DIR] )
	#print "Categories:", dirs
	return dirs


def getScripts( category, user=None ):
	"""
	Retrieve a list of watcher scripts from a baseapp/cellapp (the first
	baseapp/cellapp found is the one queried).

	If a category is specified, then we retrieve Watcher functions in the
	directory corresponding to that category. If category is a blank string,
	then get the list of Watcher functions directly under the top level
	"command" directory.

	NOTE: The resulting script objects do NOT contain the following attributes:
		- args (the argument list consisting of (name, WatcherDataType) pairs)
		- desc (the description string)

	To get these attributes you need to call the "retrieveInfo" method on the
	watcher object which will then fill in these attributes.

	@param category The category for which we want the list of scripts.
                    Corresponds to a directory under the top level "command"
					directory on the baseapp/cellapp.
	@param user     cluster.User object for who we want the scripts.
	"""
	if not user:
		c = cluster.Cluster()
		user = util.getUser( c )
		# Scan baseappmgrs, cellappmgrs, baseapp, cellapp

	path = "command"
	if category:
		path += "/" + category

	proc = user.getProc("cellapp")
	cellAppWatchers = []
	if proc:
		result = messages.WatcherDataMessage.batchQuery( [path], [proc], 2 )
		#print "RESULT:", repr(result)
		for t in result[proc].get( path, [] ):
			if t[3] == messages.WatcherDataMessage.WATCHER_MODE_CALLABLE:
				cellAppWatchers.append( WatcherRunScript(
					"%s/%s" % (path, t[0]), "cellapp", "any" ) )

	proc = user.getProc("baseapp")
	baseAppWatchers = []
	if proc:
		result = messages.WatcherDataMessage.batchQuery( [path], [proc], 2 )
		#print "RESULT2:", repr(result)
		for t in result[proc].get( path, [] ):
			if t[3] == messages.WatcherDataMessage.WATCHER_MODE_CALLABLE:
				cellAppWatchers.append( WatcherRunScript(
					"%s/%s" % (path, t[0]), "baseapp", "any" ) )

	return cellAppWatchers + baseAppWatchers


def getScript( id ):
	"""
	Given an ID string, create a RunScript script object that we can then
	run methods on. No remote watcher queries are made as a result of this
	function.

	NOTE: The resulting script objects do NOT contain the following attributes:
		- args (the argument list consisting of (name, WatcherDataType) pairs)
		- desc (the description string)

	To get these attributes you need to call the "retrieveInfo" method on the
	watcher object which will then fill in these attributes.

	@param id	The ID is of the format "watcher:<proctype>:<runtype>:<watcherPath>"
				e.g. "watcher:baseapp:any:command/numEntities"
				e.g. "watcher:baseapp:1,2:command/addGuards"
				e.g. "watcher:cellapp:all:command/createStorm"
	"""
	_, procType, runType, path = id.split( ":", 3 )
	return WatcherRunScript( path, procType=procType, runType=runType )



runscript.registerScriptLoader( "watcher", getScripts, getScript, getCategories )
