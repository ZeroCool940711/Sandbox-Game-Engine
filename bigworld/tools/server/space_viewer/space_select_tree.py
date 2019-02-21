import re
import string
import socket
import struct

import wx

from cell_app_mgr_talker import CellAppMgrTalker
import space_view_window

import bwsetup; bwsetup.addPath( ".." )
from pycommon import cluster

class SpacesTreeView( wx.TreeCtrl ):
	"""
	Represents user / space tree view.
	"""

	def __init__( self, parent, id ):

		wx.TreeCtrl.__init__( self, parent, id )
		self.makeTree()
		self.spaceSelectFrame = parent

	def makeTree( self ):

		self.cluster = cluster.Cluster()

		# Set up a mapping from uid -> Process for each cellappmgr
		self.cellappmgrs = {}
		for p in self.cluster.getProcs( "cellappmgr" ):
			self.cellappmgrs[ p.uid ] = p

		# create root node
		self.rootNode = \
			self.AddRoot( "Users (" +
						  str( len( self.cellappmgrs.keys() ) ) + ")"  )

		# create node for each user on LAN
		names = []
		for uid in self.cellappmgrs.keys():
			names.append( self.cluster.getUser( uid ).name )

		for name in sorted( names ):
			itm = self.AppendItem( self.rootNode, name )
			self.SetItemHasChildren( itm, True )

		self.Toggle( self.rootNode )

		wx.EVT_TREE_ITEM_EXPANDING( self, self.GetId(), self.onExpanding )
		wx.EVT_TREE_ITEM_COLLAPSED( self, self.GetId(), self.onCollapsed )

		wx.EVT_LEFT_DCLICK( self, self.OnDClick )

	def refresh( self ):
		self.DeleteAllItems()
		self.makeTree()

	def onCollapsed( self, event ):

		if event.GetItem() != self.rootNode:
			self.DeleteChildren( event.GetItem() )
			self.SetItemHasChildren( event.GetItem(), True )


	def onExpanding( self, event ):

		for (uid, proc) in self.cellappmgrs.items():
			user = self.cluster.getUser( uid )
			if user.name == self.GetItemText( event.GetItem() ):
				spaces = proc.getWatcherData( "spaces" )
				for wd in spaces.getChildren():
					toc = self.AppendItem( event.GetItem(),
										   "Space: " + wd.name )
					self.SetPyData( toc, (int( wd.name ),
										  user.uid) )

	def OnDClick( self, event ):

		pt = event.GetPosition()
		item, flags = self.HitTest( pt )
		parent = self.GetItemParent( item )

		# if double click is on a space item open space view.
		if self.GetItemParent( item ) != self.rootNode:
			if item != self.rootNode:
				spaceID, uid = self.GetPyData( item )
				spaceViewWindow = space_view_window.\
								  SpaceViewWindow.createLoggerAndView(
									self, spaceID, uid )
				spaceViewWindow.Show( True )


if __name__ == '__main__':
	print "must run from space_viewer.py or sv.py"

# space_select_tree.py
