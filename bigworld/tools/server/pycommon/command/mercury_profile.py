"""
Mercury profile module.

This module contains logic for retrieving and displaying per-message-type
statistics from processes.

See the printProfile() module function below.
"""

# Python < 2.5 compatibility, can't relative import
#from .. import messages
import sys
import os
sys.path.append( os.path.dirname( __file__ ) + "/../.." )

from pycommon import messages

class _FieldType( object ):
	"""
	This class represents a field type object, which defines one of the
	columns in the resulting table output.
	"""
	def __init__( self, name, short, format ):
		self.name = name
		self.short = short
		self.format = format

FIELD_TYPES = {}
FIELD_ORDER = []


def _declareFieldType( name, short, format="%s" ):
	"""
	Declare a field type.

	@param name 	the name of the watcher value
	@param short 	a short name for the field, this is displayed as part
					of the header when printing out stats
	@param format 	the format string which is used to print the value
	"""
	global FIELD_TYPES
	global FIELD_ORDER
	FIELD_ORDER.append( name )
	FIELD_TYPES[name] = _FieldType( name, short, format )

# Mercury message type statistic field declarations

_declareFieldType( "id", "id", "%d" )
_declareFieldType( "name", "nm", "%s" )
_declareFieldType( "bytesReceived", "br", "%d" )
_declareFieldType( "messagesReceived", "mr", "%d" )
_declareFieldType( "maxBytesReceived", "max br", "%d" )
_declareFieldType( "avgMessageLength", "aml", "%.01f" )
_declareFieldType( "avgBytesPerSecond", "abps", "%.01f")
_declareFieldType( "avgMessagesPerSecond", "amps", "%.01f" )

def _collectNubStatistics( proc, nubPath, skipUnused=True ):
	"""
	Collect nub statistics for a path, return them as a list (indexed by
	interface ID) of dictionaries containing mappings of statistic names
	to values.

	@param proc 		the process
	@param nubPath 		the watcher path to the nub watcher directory
	@param skipUnused 	skip the unused interfaces (those that have not
						received any messages)
	"""

	global FIELD_TYPES
	global FIELD_ORDER

	outStats = []
	maxFieldLens = dict( [(fieldName, len( fieldType.short ))
		for fieldName, fieldType in FIELD_TYPES.items()] )


	for ifaceID in xrange( 256 ):
		# batch up the interface statistics watcher directory retrieval
		paths = [nubPath + "/interfaceByID/" + str( ifaceID ) + "/" + fieldName
			for fieldName in FIELD_ORDER]

		results = messages.WatcherDataMessage.batchQuery( paths, [proc], 1.0 )

		# need to process the results into a more useful form, dictionary of
		# the field names to their values
		results = dict( [(path[path.rindex( "/" ) + 1:] , valueInfo[0][1])
			for path, valueInfo in results[proc].items()] )

		# skip unused interfaces if we can
		if skipUnused and not results["messagesReceived"]:
			continue

		ifaceStats = dict( id=ifaceID )
		for fieldName, fieldType in FIELD_TYPES.items():
			fieldValue = results[fieldName]
			# id can be mangled, e.g. for script message ifaces
			if fieldName == "id":
				fieldValue = ifaceID
			ifaceStats[fieldName] = fieldValue

			formatted = fieldType.format % fieldValue
			maxFieldLens[fieldName] = max( len( formatted ),
				maxFieldLens.get( fieldName, len( fieldType.short ) ) )

		outStats.append( (ifaceID, ifaceStats) )

	return outStats, maxFieldLens

def _sortNubStatistics( fieldName, stats, descending=True ):
	"""
	Sort the statistics list in-place according to the given field name.

	@param fieldName 	the field name of the sort key
	@param stats 		the list of stats to sort
	@param descending 	if True, do descending sort otherwise ascending
	"""

	def cmpFn( v1, v2 ):
		ifaceID1, ifaceStats1 = v1
		ifaceID2, ifaceStats2 = v2
		out = cmp( ifaceStats1[fieldName], ifaceStats2[fieldName] )
		if descending:
			return -out
		else:
			return out

	stats.sort( cmpFn )


def getProcStats( processes, skipUnused = False ):
	"""
	Retrieve a list of process statistics for the given processes.

	@param processes 	a list of processes to profile
	@return 			a list of tuples of the form:
							(process, nubLabel, (ifaceStats, maxFieldLens))
						where
							process is a Process object,
							nubLabel is the string label of the nub on that
							process (either "Internal Nub" or "External Nub",
							ifaceStats is an ordered list of interface
							statistics, indexed by interface ID, each is a
							dictionary with keys corresponding to entries in
							FIELD_TYPES,
							maxFieldLens is a dictionary with an entry for each
							type in FIELD_TYPES, and the maximum string length
							of the statistic as formatted by the type in
							FIELD_TYPES.
	"""
	procStats = []
	for proc in processes:
		procStats.append( (proc, "Internal Nub",
			_collectNubStatistics( proc, "nub", skipUnused )) )

		# if it's external, there are two nubs to collect from
		if proc.name in ["baseapp", "loginapp"]:
			procStats.append( (proc, "External Nub",
				_collectNubStatistics( proc, "nubExternal", skipUnused )) )
	return procStats


def printProfile( processes, sortBy, descending, skipUnused ):
	"""
	Mercury profile console output function.

	@param processes 	a list of processes to profile
	@param sortBy 		sort key to use, must be one of FIELD_ORDER
	@param descending 	use descending sort
	@param skipUnused 	skip unused interfaces (those that have not received
						any messages)
	"""
	global FIELD_TYPES
	global FIELD_ORDER

	if sortBy not in FIELD_ORDER:
		raise ValueError, "sort must be one of %s" % ", ".join( FIELD_ORDER )

	procStats = getProcStats( processes, skipUnused )

	for proc, label, (stats, maxFieldLens) in procStats:
		print "%(name)s - %(nubName)s\n" % \
			dict( name=proc.label(), nubName=label )

		_sortNubStatistics( sortBy, stats, descending )

		# print the header
		print " ".join( ["%*s" %
				(maxFieldLens[fieldName], FIELD_TYPES[fieldName].short)
			for fieldName in FIELD_ORDER] )
		for ifaceID, ifaceStats in stats:
			print " ".join( [("%*" + FIELD_TYPES[fieldName].format[1:]) %
					(maxFieldLens[fieldName], ifaceStats[fieldName] )
				for fieldName in FIELD_ORDER] )
		print

# mercury_profile.py
