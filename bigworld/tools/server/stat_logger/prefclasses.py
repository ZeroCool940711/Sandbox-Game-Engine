import itertools
import constants

# Classes to represent preferences

# -----------------------------------------------------------------------------
# Section: Base class
# -----------------------------------------------------------------------------

class Pref:
	def __init__( self, name, dbId ):
		self.dbId = dbId
		self.name = name
		self.parent = None

	def setParent( self, parent ):
		self.parent = parent

# -----------------------------------------------------------------------------
# Section: PrefTree
# -----------------------------------------------------------------------------

class PrefTree:
	def __init__( self ):
		self.procPrefs = {}
		self.procOrder = []

		self.machineStatPrefs = {}
		self.machineStatOrder = []
		self.machineStatPrefsById = {}

		self.allProcStatPrefs = {}
		self.allProcStatOrder = []
		self.allProcStatPrefsById = {}

		# window prefs are ordered
		self.windowPrefs = []

	def addProcPref( self, procPref ):
		self.procPrefs[ procPref.name ] = procPref
		procPref.setParent( self )
		self.procOrder.append( procPref.name )

	def addWindowPref( self, windowPref ):
		self.windowPrefs.append( windowPref )
		windowPref.setParent( self )

	def addMachineStatPref( self, statPref ):
		self.machineStatPrefs[ statPref.name ] = statPref
		self.machineStatOrder.append( statPref.name )
		if statPref.dbId != None:
			self.machineStatPrefsById[ statPref.dbId ] = statPref
		statPref.setParent( self )

	def procPrefByName( self, processPrefName ):
		return self.procPrefs[processPrefName]

	def addAllProcStatPref( self, statPref ):
		self.allProcStatPrefs[ statPref.name ] = statPref
		self.allProcStatPrefsById[ statPref.dbId ] = statPref
		self.allProcStatOrder.append( statPref.name )
		statPref.setParent( self )

	def allProcStatPrefById( self, dbId ):
		return self.allProcStatPrefsById[ dbId ]

	def allProcStatPrefByName( self, name ):
		return self.allProcStatPrefs[ name ]

	def iterAllProcStatPrefs( self ):
		return iter( self.allProcStatPrefs[statName]
			for statName in self.allProcStatOrder )

	def iterProcPrefs( self ):
		return iter( self.procPrefs[procName] for procName in self.procOrder )

	def iterMachineStatPrefs( self ):
		return iter( self.machineStatPrefs[statName]
			for statName in self.machineStatOrder )

	def machineStatPrefByName( self, name ):
		return self.machineStatPrefs[name]

	def machineStatPrefById( self, dbId ):
		return self.machineStatPrefsById[ dbId ]

	def display( self ):
		print str( self )

	def __str__( self ):
		out = ''
		out += "=========================================\n"
		out += "    Printing preference tree\n"
		out += "=========================================\n"
		for p in self.iterProcPrefs():
			out += "  Process \"%s\"\n" % (p.name)
			for s in p.iterStatPrefs():
				out += "    Statistic \"%s\"\n\n" % (s.name)
		out += "Special process class: <Machines>\n"
		for s in self.iterMachineStatPrefs():
			out += "   Statistic \"%s\"\n" % (s.name)
		out += "Special process class: <All Processes>\n"
		for s in self.iterAllProcStatPrefs():
			out += "   Statistic \"%s\"\n" % (s.name)
		out += "=========================================\n"
		out += "        End preference tree\n"
		out += "=========================================\n\n"
		return out

# -----------------------------------------------------------------------------
# Section: Pref implementations
# -----------------------------------------------------------------------------
class ProcessPref( Pref ):
	def __init__( self, name, matchtext, dbId = None ):
		Pref.__init__( self, name, dbId )
		self.matchtext = matchtext
		self.statPrefs = {}
		self.idsToPrefs = {}
		self.statOrder = []
		self.tableName = None

	def statPrefById( self, dbId ):
		if self.idsToPrefs.has_key( dbId ):
			return self.idsToPrefs[ dbId ]
		return self.parent.allProcStatPrefById( dbId )

	def statPrefByName( self, name ):
		if not self.statPrefs.has_key( name ):
			return self.parent.allProcStatPrefByName( name )
		return self.statPrefs[name]

	def addStatPref( self, statPref ):
		self.statPrefs[ statPref.name ] = statPref
		self.idsToPrefs[ statPref.dbId ] = statPref
		self.statOrder.append( statPref.name )
		statPref.setParent( self )

	def iterStatPrefs( self ):
		return iter( self.statPrefs[statName] for statName in self.statOrder )

	def iterAllStatPrefs( self ):
		if self.parent:
			return itertools.chain(
				self.parent.iterAllProcStatPrefs(),
				self.iterStatPrefs()
			)
		else:
			raise Exception(
				"Process preference not attached to preference tree!" )

	def __str__( self ):

		return '{' + \
			", ".join( str( statPref )
				for statPref in self.statPrefs.values() ) + \
		'}'

class StatPref( Pref ):
	def __init__( self, name, valueAt, maxAt, consolidate,
			dbId = None, colour = None, show = None, description = None, 
			type = "FLOAT" ):
		Pref.__init__( self, name, dbId )
		self.valueAt = valueAt
		self.maxAt = maxAt
		self.type = type
		self.consolidate = consolidate
		self.colour = colour
		self.show = show
		self.description = description
		self.columnName = None

	def consolidateColumn( self, tableName=None ):
		if tableName:
			tableName += "."
		else:
			tableName = ""

		if self.consolidate == constants.CONSOLIDATE_AVG:
			return "AVG( %s%s ) AS avg_%s" % \
				(tableName, self.columnName, self.columnName)
		elif self.consolidate == constants.CONSOLIDATE_MAX:
			return "MAX( %s%s ) AS max_%s" % \
				(tableName, self.columnName, self.columnName)
		elif self.consolidate == constants.CONSOLIDATE_MIN:
			return "MIN( %s%s ) AS min_%s" % \
				(tableName, self.columnName, self.columnName)
		else:
			raise ValueError, "invalid consolidate function"


	def __str__( self ):
		return "<%d:%s>" % (self.dbId, self.columnName)

	__repr__ = __str__


class WindowPref( Pref ):
	"""
	Preference class for aggregation windows, defined by the number of tick
	samples held and the number of base ticks between the ticks in this window.
	"""
	def __init__( self, samples, samplePeriodTicks, dbId = None ):
		Pref.__init__( self, None, dbId )
		self.samples = samples
		self.samplePeriodTicks = samplePeriodTicks
		self.parent = None

	def setParent( self, parent ):
		self.parent = parent

	def __str__( self ):
		return "<wp%d>" % (self.samplePeriodTicks)

	__repr__ = __str__

# -----------------------------------------------------------------------------
# Section: General options
# -----------------------------------------------------------------------------
class Options:
	def __init__( self ):
		self.setDefaults()

	def setDefaults( self ):
		self.dbHost = "localhost"
		self.dbUser = "bigworld"
		self.dbPass = "bigworld"
		self.dbPort = 3306
		self.uid = None
		self.sampleTickInterval = 2000

	def display( self ):
		print "====================================="
		print "    Global options"
		print "====================================="
		print "dbHost: %s" % (self.dbHost)
		print "dbUser: %s" % (self.dbUser)
		print "dbPass: %s" % (self.dbPass)
		print "dbPort: %s" % (self.dbPort)
		print "uid: %s" % self.uid
		print "sampleTickInterval: %d" % (self.sampleTickInterval)

# prefclasses.py
