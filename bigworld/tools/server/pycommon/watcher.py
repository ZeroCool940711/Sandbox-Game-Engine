#!/usr/bin/env python

class Constants( object ):
	# Watcher protocol data types
	TYPE_UNKNOWN    = 0
	TYPE_INT        = 1
	TYPE_UINT       = 2
	TYPE_FLOAT      = 3
	TYPE_BOOL       = 4
	TYPE_STRING     = 5
	TYPE_TUPLE      = 6
	TYPE_TYPE       = 7

	# Exposure hints for watcher forwarding
	EXPOSE_WITH_ENTITY = 0
	EXPOSE_ALL = 1
	EXPOSE_WITH_SPACE = 2
	EXPOSE_LEAST_LOADED = 3


class Forwarding( object ):

	forwardPaths = {}
	forwardPaths[ Constants.EXPOSE_WITH_ENTITY ]  = "forwardTo/withEntity"
	forwardPaths[ Constants.EXPOSE_ALL ]          = "forwardTo/all"
	forwardPaths[ Constants.EXPOSE_WITH_SPACE ]   = "forwardTo/withSpace"
	forwardPaths[ Constants.EXPOSE_LEAST_LOADED ] = "forwardTo/leastLoaded"


	def __init__( self ):
		pass

	@staticmethod
	def runTypeHintToWatcherPath( runType ):
		try:
			return Forwarding.forwardPaths[ int(runType) ]
		except:
			raise TypeError( "Unable to map '%s' to a known forwarding path." \
							% str(runType) )

