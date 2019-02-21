from sqlobject import *
from datetime import datetime
from turbogears.database import PackageHub
from turbogears import identity
from web_console.common import model

hub = PackageHub( "web_console" )
__connection__ = hub

class ccStartPrefs( model.DictSQLObject ):

	user = ForeignKey( "User" )
	mode = StringCol( length=10 )
	arg = StringCol( length=64 )
	useTags = BoolCol()

	RENAME_COLS = [('uid', 'user_id'),
				   ('uid_id', 'user_id')]


class ccSavedLayouts( model.DictSQLObject ):

	user = ForeignKey( "User" )
	name = StringCol()
	serveruser = StringCol()
	xmldata = StringCol()

	RENAME_COLS = [('uid', 'user_id'),
				   ('uid_id', 'user_id')]


### Custom Watcher Classes
class ccCustomWatchers( model.DictSQLObject ):

	user = ForeignKey( "User" )
	pageName = StringCol( length=128 )
	uniqueIndex = DatabaseIndex( user, pageName, unique=True )


class ccCustomWatcherEntries( model.DictSQLObject ):

	# The Custom Watcher this entry belongs to
	customWatcherPage = ForeignKey( "ccCustomWatchers", cascade=True )

	# The component that contains the watcher value
	componentName = StringCol( length=128 )

	# The id number of the component to display
	# NB: For components that can have multiple instances running,
	#     a value of 0 indicates 'all active processes of this
	#     component type'.
	componentNumber = IntCol( default=0 )

	# The watcher value to query
	watcherPath = StringCol( length=256 )

	# Index to ensure the same watcher value only exists once per
	# custom layout.
	componentWatcherIndex = DatabaseIndex( customWatcherPage, componentName,
					{'column':watcherPath, 'length':256 }, unique=True )


class ccStartProcPrefs( model.DictSQLObject ):

	user = ForeignKey( "User" )
	proc = StringCol( length=16, default = "" )
	machine = StringCol( length=40, default = "" )
	count = IntCol( default = 1 )


ccStartPrefs.createTable( True )
ccSavedLayouts.createTable( True )
ccStartProcPrefs.createTable( True )
ccCustomWatchers.createTable( True )
ccCustomWatcherEntries.createTable( True )
