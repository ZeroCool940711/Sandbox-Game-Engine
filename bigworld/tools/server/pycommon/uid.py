"""
Classes etc for uid<->username mapping
"""
import string
import os
import sys
import traceback

import cluster
import log

# -----------------------------------------------------------------------------
# Section: Exported interface
# -----------------------------------------------------------------------------

def getuid( name = None, machine = None ):
	return query( "uid", name, machine )

def getname( uid = None, machine = None ):
	return query( "name", uid, machine )

def query( ret, name = None, machine = None ):
	"""
	Resolve name <-> uid, or get effective uid with no args.
	"""

	# Kwargs to pass to cluster.Cluster() to avoid spamming as much as possible
	kw = {}

	# Convert name to int if possible
	try:
		name = int( name )
		kw[ "uid" ] = name
	except:
		pass

	if name is None:
		if os.name == "posix":
			name = os.getuid()
			kw[ "uid" ] = name
		else:
			name = os.getenv( 'UID' )
			if not name:
				log.info( 'UID environment parameter not set' )
				name = 0
			else:
				name = int( name )
			kw[ "uid" ] = name

	user = cluster.Cluster.get( **kw ).getUser( name, machine )
	if user:
		if ret == "uid":
			return user.uid
		else:
			return user.name
	else:
		log.critical( "Couldn't resolve %s to a uid" % name )

def getall( machines = [] ):
	"""
	Get a list of all users on the cluster.
	"""

	return cluster.Cluster.get().getAllUsers( machines )
