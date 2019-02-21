from sqlobject import *
from datetime import datetime
from turbogears.database import PackageHub
from turbogears import identity

import bwsetup; bwsetup.addPath( "../.." )
from web_console.common import model
from message_logger import bwlog

hub = PackageHub( "web_console" )
__connection__ = hub

def getSeverityList():
	return ",".join( [s for s, n in sorted( bwlog.SEVERITY_LEVELS.items(),
											key = lambda (s,n): n )] )

class lvSavedQuery( model.DictSQLObject ):

	user = ForeignKey( "User" )

	# This is the user-assignable name for the query
	name = StringCol( length=256 )

	# Some of these fields that you would expect to be ints are strings because
	# they are text input fields in the HTML form and they can therefore be ""
	time = StringCol( length=128, default="(server startup)" )
	periodCount = StringCol( length=32, default="" )
	periodUnits = StringCol( length=32, default="seconds" )
	periodDirection = StringCol( length=32, default="to present" )
	host = StringCol( length=32, default="" )
	serveruser = StringCol( length=32, default="" )
	pid = StringCol( length=16, default="" )
	appid = StringCol( length=16, default="" )
	procs = StringCol( length=256,
					   default=",".join( bwlog.BASE_COMPONENT_NAMES ) )
	severity = StringCol( length=256, default = getSeverityList() )
	message = StringCol( default="" )
	exclude = StringCol( default="" )
	caseSens = BoolCol( default=True )
	regex = BoolCol( default=False )
	interpolate = BoolCol( default=True )
	showMask = IntCol( default = bwlog.SHOW_ALL & ~bwlog.SHOW_USER &
					   ~bwlog.SHOW_PID & ~bwlog.SHOW_APPID )
	live = BoolCol( default=False )
	autoHide = BoolCol( default=False )
	context = StringCol( length=16, default = "" )

	VERIFY_COLS = [ ("context", "varchar(16)"),
					("exclude", "text") ]
	RENAME_COLS = [ ("uid", "user_id"),
					("uid_id", "user_id"),
					("user", "serveruser") ]


class lvAnnotation( model.DictSQLObject ):

	user = ForeignKey( "User" )
	message = StringCol()
