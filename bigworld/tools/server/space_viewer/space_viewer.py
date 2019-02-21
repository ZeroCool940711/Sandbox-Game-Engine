#!/usr/bin/env python

"""
Space Viewer allows the user to view the position of entities and cell
boundaries in a BigWorld game space.
"""

import sys
import os
import platform
import socket
import anydbm
import cPickle as pickle

PREFS = "%s/prefs.xml" % os.path.dirname( __file__ )

from space_select_window import *
import space_view_window
import replay
from optparse import OptionParser

import bwsetup; bwsetup.addPath( ".." )
from pycommon import cluster
from pycommon import uid as uidmodule

import wx

USAGE = "%proc [options]\n" + __doc__.rstrip()

class MyApp( wx.App ):

	def __init__( self, options ):
		self.options = options
		wx.App.__init__( self, 0 )

	def OnInit(self):

		window = None
		version = map( int, platform.python_version_tuple()[:2] )

		if version[0] + version[1]/10.0 < 2.4:
			print "WARNING: You should be using python 2.4.1 or later " + \
				  "... some functionality may break"

		# Replaying a log
		if self.options.replay:
			window = space_view_window.SpaceViewWindow.createLoggerAndView(
				None, self.options.replay )
			window.Show( True )
			return True

		if self.options.uid and not self.options.spaceid:
			print "When specifying a UID, you must also specify a SpaceID"
			return False

		elif self.options.spaceid and not self.options.uid:
			if os.name == "posix":
				self.options.uid = str( os.getuid() )
			else:
				print "When specifying a spaceID, you must also specify a UID"
				return False

		# Connecting to an already running logger
		if self.options.connect:

			if ":" in self.options.connect:
				(ip, port) = self.options.connect.split( ":" )
			else:
				ip = socket.gethostname()
				port = self.options.connect

			port = int( port )
			replayer = replay.Replayer( (ip, port) )
			window = space_view_window.SpaceViewWindow( None, replayer, False )
			window.Show( True )

		# Create a temporary logger and connect a viewer to it
		elif self.options.uid and self.options.spaceid:

			window = space_view_window.SpaceViewWindow.createLoggerAndView(
				None, self.options.spaceid, self.options.uid )
			if window:
				window.Show( True )

		else:
			spaceSelectWindow = SpaceSelectWindow( None, -1, "Space Viewer",
							wx.DefaultPosition, wx.Size(200,200) )
			spaceSelectWindow.Show( True )

		# Select the first cell if --autoselect given
		if self.options.autoselect and window:
			window.window.surface.changeToNextCell()

		return True

def main():
	opt = OptionParser( USAGE )
	opt.add_option( "-u", "--uid", dest="uid",
					help="Specify user id" )
	opt.add_option( "-s", "--space", dest="spaceid", type="int",
					help="Specify space id" )
	opt.add_option( "-c", "--connect", dest = "connect",
					help = "Address to connect to svlogger on, if connecting "
					"to an already running logger" )
	opt.add_option( "-r", "--replay", dest = "replay", metavar = "PREFIX",
					help = "Prefix of log to replay. If your logs are written "
					"to /tmp/svlog-john/svlog-005.*, then the prefix is "
					"'/tmp/svlog-john/svlog-005'")
	opt.add_option( "--autoselect", dest = "autoselect", action="store_true",
					help = "Automatically select a cell on startup. A space "
					"must be selected with -s when this option is passed" )
	opt.add_option( "-d", "--dump-db", dest = "dumpdb",
					help = "Display all values from a log .db file" )
	options, args = opt.parse_args()

	if options.uid:
		options.uid = uidmodule.getuid( options.uid )

	if options.dumpdb:
		try:
			db = anydbm.open( options.dumpdb )
			print options.dumpdb
			for k, v in db.items():
				print "%s: %s" % (k, pickle.loads( v ))
			return 0
		except anydbm.error:
			print "Unknown or invalid database: %s" % options.dumpdb
			return 1

	app = MyApp( options )
	app.MainLoop()


if __name__ == "__main__":
	sys.exit( main() )

# space_viewer.py
