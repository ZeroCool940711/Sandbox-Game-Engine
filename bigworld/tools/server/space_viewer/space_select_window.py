#!/usr/bin/env python

import string
import sys
import os
import string

import wx

from space_select_tree import *
import space_viewer
import space_view_window
import svlogger

import bwsetup; bwsetup.addPath( ".." )
from pycommon import xmlprefs

class SpaceSelectWindow( wx.Frame ):

	""" The main window """

	def __init__( self, parent, id, title, pos, size ):

		wx.Frame.__init__( self, parent, id, title, pos, size,
			style = wx.DEFAULT_FRAME_STYLE | wx.HSCROLL )

		#set up tree.
		treeID = wx.NewId()
		self.treeWindow = SpacesTreeView( self, treeID )
		newfont = wx.Font( 10, wx.ROMAN, wx.NORMAL, wx.NORMAL, False, 
			"Courier" )
		self.treeWindow.SetFont( newfont )

		ID_REFRESH = wx.NewId()
		ID_EXIT = wx.NewId()
		ID_REPLAY_LOG = wx.NewId()

		menu1 = wx.Menu()
		menu1.Append( ID_REFRESH, "Refresh" )
		menu1.Append( ID_REPLAY_LOG, "Replay Log ...\tCTRL+R" )
		menu1.AppendSeparator()
		menu1.Append( ID_EXIT, "Exit Viewer\tCTRL+Q" )

		menuBar = wx.MenuBar()
		menuBar.Append( menu1, "&File" );

		self.SetMenuBar( menuBar )

		wx.EVT_MENU( self, ID_EXIT, self.onExitNow )
		wx.EVT_MENU( self, ID_REFRESH, self.onRefresh )
		wx.EVT_MENU( self, ID_REPLAY_LOG, self.onReplayLog )

	def onExitNow( self, event ):
		self.Destroy()

	def OnSize( self, event ):
		self.outputWindow.Refresh()
		self.treeWindow.Refresh()

	def onRefresh( self, event ):
		self.treeWindow.refresh()

	def onReplayLog( self, event ):
		prefs = xmlprefs.Prefs( space_viewer.PREFS )
		logDir = prefs.get( "logDir" ) or svlogger.DEFAULT_LOGDIR
		cmtdb = wx.FileSelector(
			"Select the CMT database (.db) file for the log you wish to replay",
			logDir )

		prefs.set( "logDir", os.path.dirname( cmtdb ) )

		win = space_view_window.\
			  SpaceViewWindow.createLoggerAndView( self, cmtdb )
		win.Show( True )

if __name__ == '__main__':
	print "must run from space_viewer.py or sv.py"
