"""
This provides similar functionality to bwconfig.hpp.
Typical usage would be:

import bwconfig

bwconfig.get( "baseApp/secondaryDB/enable", False )
"""

import os
import libxml2


def get( xpath, default = None ):
	"""
	This method returns a configuration value. The default
	is returned if not found.
	"""
	xpath = cleanXPath( xpath )

	for config in configChain:
		nodes = config[1].xpathEval( xpath )

		if nodes:
			return nodes[0].getContent().strip()

	return default


def set( xpath, value ):
	"""
	This method sets a configuration. Non-existing nodes on
	the xpath are created.
	"""
	xpath = cleanXPath( xpath )

	# First in the chain is authoritive
	configFile = configChain[0]
	doc = configFile[1]

	# Get leaf node
	nodes = doc.xpathEval( xpath )
	if nodes:
		node = nodes[0]
	else:
		# Walk xpath, creating non existing nodes
		node = doc.children
		parts = xpath.split( "/" )

		partialXPath = ""

		for part in parts:
			partialXPath += "/" + part 
			nodes = doc.xpathEval( partialXPath )

			if nodes:
				node = nodes[0]
			else:
				node = node.addChild( libxml2.newNode( part ) )

	node.setContent( value )

	doc.saveFile( configFile[0] )


def cleanXPath( xpath ):
	"""
	This method cleans up a xpath to libxml2 format.
	"""
	xpath = "root/" + xpath

	# No leading '/'
	if xpath[0] == "/":
		xpath = xpath[1:]

	# No trialing '/'
	if xpath[-1] == "/":
		xpath = xpath[:-1]

	return xpath


def resPaths():
	"""
	This method returns the paths on the resource tree.
	"""
	paths = []

	f = file( os.environ['HOME'] + "/.bwmachined.conf", "r" )
	lines = f.readlines()
	f.close()

	for l in lines:
		l = l.strip()
		if (len( l ) > 0) and (not l.startswith( '#' )):
			l = l.replace( "\n", "" )
			try:
				paths = l.split( ";" )[1].split(':')
			except Exception, e:
				raise Exception( 'Malformed line in .bwmachined.conf: ' + l )
			break

	return paths


def find( resource, paths = None ):
	"""
	This method returns the full path of a file in a resource
	tree. None is returned if not found.
	"""
	if paths == None:
		paths = resPaths()

	for p in paths:
		p = p + os.sep + resource
		if os.access( p, os.F_OK ):
			return p

	return None


def parseChain():
	"""
	This method parses the configuration file chain and returns
	a list of (filename, libxml2.Document) tuples ordered by
	chain position.
	"""
	configChain = []

	filename = find( "server/bw.xml" )

	while filename:
		doc = libxml2.parseFile( filename )
		configChain.append( (filename, doc) )

		# Check for parent
		nodes = doc.xpathEval( "/root/parentFile" )

		if nodes:
			filename = nodes[0].getContent().strip()
			filename = find( filename )
		else:
			filename = None

	return configChain


configChain = parseChain()
