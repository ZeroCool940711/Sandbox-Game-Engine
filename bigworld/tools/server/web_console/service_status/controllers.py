import logging
import cherrypy
import turbogears
import sqlobject

from turbogears import controllers, expose, redirect
from turbogears import validate, validators, identity
from turbogears import widgets
from turbojson import jsonify

# Standard python modules
from StringIO import StringIO
import os
import re
import random
import threading
import traceback

# BigWorld modules
import bwsetup; bwsetup.addPath( "../.." )
from pycommon import cluster
from pycommon import uid as uidmodule
from pycommon import log
from pycommon import async_task
import pycommon.util
from web_console.common import util
from web_console.common import module

class PageLink:
	def __init__( self, name, url, status = "fail"):
		self.name = name
		self.url = url
		self.status = status

pageLinks = [PageLink(name = 'FD Server',
				url = 'fdserver'),
			PageLink(name = 'Continuous Server Builds - trunk',
				url = 'bldlog?revision=Trunk'),
			PageLink(name = 'Continuous Server Builds - 1.8-current',
				url = 'bldlog?revision=1.8-current'),
#			PageLink(name = 'C++ API Document Builds',
#				url = 'cppbld'),
			PageLink(name = 'Docbook HTML Builds',
				url = 'docbook?type=chunk'),
			PageLink(name = 'Docbook PDF Builds',
				url = 'docbook?type=pdf')]

autoDocMods = {'baseappmgr':'', 'baseapp':'', 'cellappmgr':'', 'cellapp':'',
		'client':'', 'dbmgr':'', 'loginapp':'', 'reviver':'', 'updater':''}

class ServiceStatus( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		self.userid = "demo"
		self.addPage( "All Services", "" )
		self.addPage( "Help", "help", popup=True )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: ServiceStatus Help" )

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="service_status" )

	@identity.require( identity.not_anonymous() )
	@expose( template="service_status.templates.status" )
	def index( self, **kw ):
		if self.checkFdserver() == 'error':
			pageLinks[0].status = 'fail'
			pageLinks[0].url = '/log'
		else:
			pageLinks[0].url = 'fdserver'
			pageLinks[0].status = self.checkFdserver()
		pageLinks[1].status = self.checkServerBuild( "Trunk" )
		pageLinks[2].status = self.checkServerBuild( "1.8-current" )
#		pageLinks[3].status = self.checkCppDocs()
		pageLinks[3].status = self.checkDocBuilds( "chunk" )
		pageLinks[4].status = self.checkDocBuilds( "pdf" )
		return dict( links = pageLinks )

	# List fd server processes
	@identity.require(identity.not_anonymous())
	@expose( template="service_status.templates.procs" )
	def fdserver( self ):
		c = cluster.Cluster.get( user = self.userid )
		procs = sorted( c.getProcs() )
		return dict( status = 'ok', procs = procs )

	# List Auto Document Builds Status
	@identity.require(identity.not_anonymous())
	@expose( template="service_status.templates.docs" )
	def cppbld( self ):
		for m in autoDocMods:
			rc = os.path.isfile("/home/build/public_html/current/gendox_%s.log" % m)
			if rc == False:
				autoDocMods[m] = 'fail'
				continue
			num = os.popen("tail /home/build/public_html/current/gendox_%s.log"
						"| grep 'Generating page index' | wc -l" % m)
			for n in num:
				if n == 0:
					autoDocMods[m] = 'fail'
				else:
					autoDocMods[m] = 'ok'

		date = ''
		str = os.popen("ls -l /home/build/public_html/current/gencfg_loginapp.log | awk '{print $6\" \"$7}'")
		for s in str:
			date = s
		return dict( mods = autoDocMods, date = date )

	# Show Auto Document Builds Log
	@identity.require(identity.not_anonymous())
	@expose( template="service_status.templates.log" )
	def doclog( self, module ):
		msg = self.readlog("/home/build/public_html/current/warnings_%s.txt" % module)
		if msg == '':
			return dict( data = 'No warning log.')
		return dict( data = msg )

	# List fd server processes
	@identity.require(identity.not_anonymous())
	@expose( template="service_status.templates.log" )
	def bldlog( self, revision ):
		msg = self.readlog("/home/build/.build.%s.failed" % revision)
		if msg == '':
			return dict( data = 'No build fail log.')
		return dict( data = msg )

	# show docbook nightly builds logs
	@identity.require(identity.not_anonymous())
	@expose( template="service_status.templates.log" )
	def docbook( self, type ):
		fn = "/home/build/.build.logs/docbook-%s.log" % type
		msg = self.readlog( fn )
		if msg == '':
			return dict( data = "No docbook %s buils log." % fn)
		return dict( data = msg )

	def readlog( self, fn ):
		data = ''
		rc = os.path.isfile( fn )
		if rc == True:
			logfile = open( fn )
			data = logfile.read()
			logfile.close()
		return data

	def checkDocBuilds( self, type ):
		rc = os.path.isfile("/home/build/.build.logs/docbook-%s.log" % type)
		if rc == False:
			return 'fail'
		return "ok"

	def checkCppDocs( self ):
		for m in autoDocMods:
			rc = os.path.isfile("/home/build/public_html/current/gendox_%s.log" % m)
			if rc == False:
				return 'fail'
			num = os.popen("tail /home/build/public_html/current/gendox_%s.log"
						"| grep 'Generating page index' | wc -l" % m)
			for n in num:
				if n == 0:
					return 'fail'
				break
		return 'ok'

	# Checking continuous server building logs
	def checkServerBuild( self, revision ):
		fn = "/home/build/.build.%s.failed" % revision
		rc = os.path.isfile( fn )
		if rc == False:
			return 'ok'

		if os.stat(fn)[6] == 0:
			return 'ok'

		return 'fail'

	# Checking fd server processes
	def checkFdserver( self ):
		c = cluster.Cluster.get( user = self.userid )
		me = c.getUser( self.userid )
		loginApps = c.getProcs( "loginapp" )
		if (loginApps and loginApps[0].statusCheck()) or \
				me.serverIsRunning():
			return 'ok'
		return 'fail'