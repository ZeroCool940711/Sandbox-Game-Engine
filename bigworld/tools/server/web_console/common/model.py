from sqlobject import *
from datetime import datetime
import md5
import re
import MySQLdb
import turbogears
from turbogears.database import PackageHub
from turbogears import identity
from pycommon import log

hub = PackageHub( "web_console" )
__connection__ = hub

"""
Notes when using SQLObject:
===============================
In SQLObject 0.7.3, it did some very bad things when you have foreign key
attributes which ended in "id". For example, with an attribute named
"uid", SQLObject 0.7.3 would do the following:

- Create a table named "uid"
- Create an object attribute named "u" which returns the object being pointed
  to. This attribute also used for assigning objects.
  Example: object.u = User("hello")
- Create an object attribute named "uid" which returned the primary key of the
  object being pointed to. This is meant to be used in queries.

In SQLObject 0.8.0, the following happens:
- Create a table named "uid_id"
- Create an object attribute named "uid" which returned the object being
  pointed to.
- Create an object attribute name "uidID" which returned the primary key of the
  object being pointed to.

The different behaviour causes problems when moving from SQLObject 0.7.3 to 0.8.
This means we should follow this rule: Foreign key attributes must not end in
the characters "id", regardless of case. The behaviour for both SQLObjects
when we have foreign keys not ending in "id" is equivalent to 0.8's behaviour.


How to use SQLObject foreign keys
=================================
Here are examples for a class called "Obj" which has an attribute of "user"
which in turn is a foreign key to a "User" class:

bobUser = User("bob")
- Assigning: obj.user = bobUser
- Querying: obj.select( Obj.q.userID == bobUser.q.id )
- Accessing: ourUser = obj.user
- Creating: obj = Obj( user=bobUser )
"""

class DictSQLObjectType( declarative.DeclarativeMeta ):
	"""
	Metaclass to add each subclass of DictSQLObject to a list.  We use this list
	to enforce correct database schemas in enforceSchemas().
	"""

	s_classes = []

	def __init__( cls, *args, **kw ):
		declarative.DeclarativeMeta.__init__( cls, *args, **kw )
		DictSQLObjectType.s_classes.append( cls )


class DictSQLObject( SQLObject ):
	"""
	Fairly trivial subclassing of SQLObject with operators defined so that
	calling dict() on an instance of this class will work as you'd expect.
	"""

 	__metaclass__ = DictSQLObjectType

	def keys( self ):
		return self.sqlmeta.columns.keys()

	def __len__( self ):
		return len( self.sqlmeta.columns )

	def __getitem__( self, key ):
		return getattr( self, key )

	def __iter__( self ):
		return ( (c, getattr( self, c )) for c in \
			self.sqlmeta.columns.iterkeys() )


	# This is circumventing the TurboGears and SQLObject database connections
	# here. This can be removed once `tg-admin sql update` works.
	@staticmethod
	def verifySchemas():

		m = re.search( "//([\w\.\-]+):(.+)@([\w\.\-]+):(\d+)/([\w\$]+)",
				   turbogears.config.get( "sqlobject.dburi" ) )
		try:
			user, password, host, port, dbname = m.groups()

		except (AttributeError, ValueError):
			log.error( "Could not parse MySQL database URI.")
			log.error( "Not verifying that database is in sync with model defs." )
			return False

		# Set up connection to the database
		db = MySQLdb.connect( host = host, port = int( port ), db = dbname,
							  user = user, passwd = password )
		cursor = db.cursor()

		# Iterate over classes with VERIFY_COLS lists
		for cls in DictSQLObjectType.s_classes:
			cls.createTable( True )
			cursor.execute( "describe %s;" % cls.sqlmeta.table )
			cols = dict( [x[:2] for x in cursor.fetchall()] )

			if hasattr( cls, "VERIFY_COLS" ):
				# Get a list of cols already defined in that table

				for name, type in cls.VERIFY_COLS:

					if name not in cols:
						cursor.execute( "alter table %s add %s %s;" %
										(cls.sqlmeta.table, name, type) )
						cursor.fetchall()
						log.info( "Added missing column %s to %s" %
								  (name, cls.sqlmeta.table) )

					if name in cols and cols[ name ] != type:
						log.critical( "Existing field with wrong type: "
									  "%s is %s, should be %s" %
									  (name, cols[ name ], type) )

			if hasattr( cls, "RENAME_COLS" ):
				for columnName, newName in cls.RENAME_COLS:
					if columnName in cols:
						log.info( "Renaming column from %s to %s", columnName, newName )

						if newName not in cols:
							sql = "ALTER TABLE %s CHANGE %s %s %s" % \
								(cls.sqlmeta.table, columnName, newName, cols[columnName])
							cursor.execute(sql)
						else:
							log.critical( "Column %s already exists, can't rename", newName )

# identity models.
class Visit( SQLObject ):
	class sqlmeta:
		table = "visit"

	visit_key = StringCol(length=40, alternateID=True,
						alternateMethodName="by_visit_key")
	created = DateTimeCol(default=datetime.now)
	expiry = DateTimeCol()

	def lookup_visit(cls, visit_key):
		try:
			return cls.by_visit_key(visit_key)
		except SQLObjectNotFound:
			return None
	lookup_visit = classmethod(lookup_visit)



class VisitIdentity( DictSQLObject ):
	visit_key = StringCol( length=40, alternateID=True,
						  alternateMethodName="by_visit_key" )
	user_id = IntCol()


class Group( DictSQLObject ):
	"""
	An ultra-simple group definition.
	"""

	# names like "Group", "Order" and "User" are reserved words in SQL
	# so we set the name to something safe for SQL
	class sqlmeta:
		table="tg_group"

	group_name = UnicodeCol( length=16, alternateID=True,
							 alternateMethodName="by_group_name" )
	display_name = UnicodeCol( length=255 )
	created = DateTimeCol( default=datetime.now )

	# collection of all users belonging to this group
	users = RelatedJoin( "User", intermediateTable="user_group",
						 joinColumn="group_id", otherColumn="user_id" )

	# collection of all permissions for this group
	permissions = RelatedJoin( "Permission", joinColumn="group_id",
							   intermediateTable="group_permission",
							   otherColumn="permission_id" )


class User( DictSQLObject ):
	"""
	Reasonably basic User definition. Probably would want additional attributes.
	"""
	# names like "Group", "Order" and "User" are reserved words in SQL
	# so we set the name to something safe for SQL
	class sqlmeta:
		table="tg_user"

	# This retains the underscore because identity and catwalk both use user_name
	user_name = UnicodeCol( length=32, alternateID=True,
						   alternateMethodName="by_user_name" )
	password = UnicodeCol( length=32 )
	serveruser = UnicodeCol( length=32, default="demo" )

	# groups this user belongs to
	groups = RelatedJoin( "Group", intermediateTable="user_group",
						  joinColumn="user_id", otherColumn="group_id" )

	def _get_permissions( self ):
		perms = set()
		for g in self.groups:
			perms = perms | set(g.permissions)
		return perms

	def _set_password( self, cleartext_password ):
		"Runs cleartext_password through the hash algorithm before saving."
		hash = identity.encrypt_password(cleartext_password)
		self._SO_set_password(hash)

	def set_password_raw( self, password ):
		"Saves the password as-is to the database."
		self._SO_set_password(password)


class Permission( DictSQLObject ):
	permission_name = UnicodeCol( length=16, alternateID=True,
								 alternateMethodName="by_permission_name" )
	description = UnicodeCol( length=255 )

	groups = RelatedJoin( "Group",
						intermediateTable="group_permission",
						 joinColumn="permission_id",
						 otherColumn="group_id" )

Visit.createTable( True )
VisitIdentity.createTable( True )
Group.createTable( True )
User.createTable( True )
Permission.createTable( True )


# Initialise users and groups which should always exist
def setupAdminGroup():
	results = list( Group.select( Group.q.group_name == "admin" ) )
	if not results:
		grouprec = Group( group_name = "admin",
						  display_name = "admin" )
	else:
		grouprec = results[0]


def setupAdminUser( name, password, serveruser ):
	# Retrieve admin group
	results = list( Group.select( Group.q.group_name == "admin" ) )
	adminGroup = results[0]

	# print "Setting up admin user: %s" % (name )
	# Make sure admin account is set up
	results = list( User.select( User.q.user_name == name ) )
	if not results:
		userrec = User(
			user_name = name,
			password = password,
			serveruser = serveruser )
	else:
		userrec = results[0]

	if userrec not in adminGroup.users:
		adminGroup.addUser( userrec )

setupAdminGroup()
setupAdminUser( "admin", "admin", "root" )
