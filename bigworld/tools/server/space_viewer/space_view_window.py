import wx
import wx.html

import style

import rubber_bander
import cell_talker
import os
import sys
import cStringIO
import math
import string

import socket
import struct
import thread
import time
import copy
import operator
import tempfile

import space_viewer
import replay
import svlogger

import bwsetup; bwsetup.addPath( ".." )
from pycommon.util import MFRectangle
from pycommon.colour import HSV2RGB
from pycommon.cluster import MAX_SERVER_CPU
from pycommon import graph
from pycommon import xmlprefs
from pycommon import dialogs
from pycommon import util

MAX_PAGE_SIZE = 260000.0

class ColourSubMenu( wx.Menu ):
	def __init__( self, frame, propName, varName, default, colours ):
		wx.Menu.__init__( self )
		self.values = {}
		self.varName = varName

		if not hasattr( style, self.varName ):
			setattr( style, self.varName, default )
			checkColour = default
		else:
			checkColour = getattr( style, self.varName )

		for colour in colours:
			id = wx.NewId()
			self.values[id] = colour
			if colour:
				helpStr = "Set %s to %s" % (propName, colour)
			else:
				helpStr = "Hide " + propName

			m = wx.MenuItem( self, id, str(colour), helpStr, wx.ITEM_RADIO )
			self.AppendItem( m )
			m.Check( colour == checkColour )
			wx.EVT_MENU( frame, id, self.onEvent )
			wx.EVT_UPDATE_UI( frame, id, self.onUpdateUI )

	def onEvent( self, event ):
		setattr( style, self.varName, self.values[ event.GetId() ] )

	def onUpdateUI( self, event ):
		if getattr( style, self.varName ) == self.values[ event.GetId() ]:
			event.Check( True )

class SpaceViewWindow( wx.Frame ):

	"""
	Main Space View Frame.
	"""

	def __init__( self, parent, replayer, stopLoggerOnExit = True ):

		# This is the interface we collect server data through (don't store any
		# references to *Talker objects in this module)
		self.replayer = replayer

		wx.Frame.__init__( self, parent, -1,
			self.replayer.cmt.getTitle(),
			size=(600,600),
			style=wx.DEFAULT_FRAME_STYLE | wx.NO_FULL_REPAINT_ON_RESIZE )

		# Graph overlay
		self.graph = None

		# Whether or not we'll stop the logger when this window is destroyed
		self.stopLoggerOnExit = stopLoggerOnExit

		# create status bar
		self.statusBar = self.CreateStatusBar()
		self.statusBar.SetFieldsCount( 4 )
		# self.statusBar.SetStatusText( "Bounding Box: ", 0 )
		self.statusBar.SetStatusText( "Cursor Position: ", 1 )
		self.statusBar.SetStatusText( "Entities: 0   Ghosts: 0", 2 )

		# create menu bar
		ID_HELP = wx.NewId()
		ID_EXIT = wx.NewId()
		ID_CLOSEWINDOW = wx.NewId()
		ID_CLOSE_WINDOW_AND_LOGGER = wx.NewId()
		self.ID_UPDATE_ENTITIES = wx.NewId()
		ID_ZOOMIN = wx.NewId()
		ID_ZOOMOUT = wx.NewId()
		ID_DEL_CELL = wx.NewId()
		ID_DEL_CELL_APP = wx.NewId()
		self.ID_CELL_FREQ = wx.NewId()
		self.ID_CELLAPPMGR_FREQ = wx.NewId()
		self.ID_ZOOM_TO_SPACE_BOUNDS = wx.NewId()

		fileMenu = wx.Menu()
		fileMenu.Append( ID_CLOSE_WINDOW_AND_LOGGER,
						 "Close Window and Terminate Logger" )
		fileMenu.Append( ID_CLOSEWINDOW, "Close Window\tCTRL+Q" )

		viewMenu = wx.Menu()

		viewMenu.AppendCheckItem( self.ID_UPDATE_ENTITIES, "Update Entities" )
		viewMenu.AppendSeparator()
		viewMenu.Append( self.ID_CELL_FREQ, "Set Cell App Update Frequency..." )
		viewMenu.Append( self.ID_CELLAPPMGR_FREQ,
				"Set Cell App Manager Update Frequency..." )
		viewMenu.AppendSeparator()
		viewMenu.Append( ID_ZOOMIN, "Zoom In\tPGUP" )
		viewMenu.Append( ID_ZOOMOUT, "Zoom Out\tPGDN" )
		viewMenu.Append( self.ID_ZOOM_TO_SPACE_BOUNDS,
						 "Zoom to Space Bounds\tCTRL+HOME" )
		viewMenu.AppendSeparator()

		# Create menu items for image overlays
		self.imageOverlayMenus = self.OverlayMenuItems(
			self, viewMenu,	"Image", self.onDisplayImageOverlay,
			self.onDisplayRecentImageOverlay, self.onRemoveImageOverlay,
			lambda x: "%s @ %s" % (os.path.basename( x["file"] ), x["coords"]) )

		self.graphOverlayMenus = self.OverlayMenuItems(
			self, viewMenu,	"Graph", self.onDisplayGraphOverlay,
			self.onDisplayRecentGraphOverlay, self.onRemoveGraphOverlay,
			lambda x: "%s * %s" % (os.path.basename( x["file"] ), x["scale"]) )

		colourMenu = wx.Menu()

		for option in style.colourOptions:
			if option:
				menu = ColourSubMenu( self,
						option[0], option[1], option[2], style.colours )
				colourMenu.AppendMenu( wx.NewId(), option[0], menu )
			else:
				colourMenu.AppendSeparator()

		# Relative colouring stuff
		colourMenu.AppendSeparator()

		relativeColouringId = wx.NewId()
		self.relativeColouringItem = colourMenu.AppendCheckItem(
			relativeColouringId, "Relative Colouring\tCTRL+R" )
		# backwards compatibility with 2.4 wx lib
		if self.relativeColouringItem is None:
			self.relativeColouringItem = colourMenu.FindItemById(
				relativeColouringId )

		self.sizeMenu = wx.Menu()
		self.id2size = {}
		self.size2id = {}
		for size in style.sizes:
			id = wx.NewId()
			self.id2size[id] = size
			self.size2id[size] = id
			self.sizeMenu.Append( id,  size+" %",
				"Set the scale to "+size+"%", wx.ITEM_RADIO )
			wx.EVT_MENU( self, id, self.onSetEntityScale )

		self.ID_CUSTOM_SCALE = wx.NewId()
		self.sizeMenu.Append( self.ID_CUSTOM_SCALE, "Custom Scale...",
			   "Set a custom scale percanetage for the entity markers",
			   wx.ITEM_RADIO )
		wx.EVT_MENU( self, self.ID_CUSTOM_SCALE, self.onCustomEntityScale )

		self.updateScaleChecks() # This updates the selected size checks

		self.visibleMenu = wx.Menu()
		self.ID_SHOW_ALL = wx.NewId()
		self.visibleMenu.Append( self.ID_SHOW_ALL, "Show All",
			   "Show all the entity types" )
		self.ID_HIDE_ALL = wx.NewId()
		self.visibleMenu.Append( self.ID_HIDE_ALL, "Hide All",
			   "Hide all the entity types" )
		self.visibleMenu.AppendSeparator()

		utilMenu = wx.Menu()
		utilMenu.Append( ID_DEL_CELL, "Retire Selected Cell",
			   "Remove the selected cell from this space" )
		utilMenu.Append( ID_DEL_CELL_APP, "Stop Selected Cell App",
			   "Stop the application that has the selected cell" )

		# The menu with the warping commands in it
		warpMenu = wx.Menu()

		helpMenu = wx.Menu()
		helpMenu.Append( ID_HELP, "Help..." )

		self.menuBar = wx.MenuBar()
		self.menuBar.Append( fileMenu, "&File" )
		self.menuBar.Append( viewMenu, "&View" )
		self.menuBar.Append( colourMenu, "&Colour" )
		self.menuBar.Append( self.sizeMenu, "Entity &Size" )
		self.menuBar.Append( self.visibleMenu, "&Visible Entities")
		self.menuBar.Append( utilMenu, "&Util" )
		self.menuBar.Append( warpMenu, "&Warp" )
		self.menuBar.Append( helpMenu, "Help" )
		self.SetMenuBar( self.menuBar )

		wx.EVT_MENU( self, ID_CLOSEWINDOW, self.onCloseWindow )
		wx.EVT_MENU( self, ID_CLOSE_WINDOW_AND_LOGGER,
			self.onCloseWindowAndLogger )
		wx.EVT_MENU( self, ID_EXIT, self.onExitNow )
		wx.EVT_MENU( self, ID_DEL_CELL, self.onDelCell )
		wx.EVT_MENU( self, ID_DEL_CELL_APP, self.onDelCellApp )

		wx.EVT_MENU( self, ID_ZOOMIN, self.onZoomInTB )
		wx.EVT_MENU( self, ID_ZOOMOUT, self.onZoomOutTB )
		wx.EVT_MENU( self, self.ID_ZOOM_TO_SPACE_BOUNDS,
			self.onZoomToSpaceBounds )
		wx.EVT_MENU( self, self.ID_UPDATE_ENTITIES, self.onUpdateEntities )
		wx.EVT_MENU( self, self.ID_CELL_FREQ, self.onChangeCellUpdateFrequency )
		wx.EVT_MENU( self, self.ID_CELLAPPMGR_FREQ,
			self.onChangeCellAppMgrUpdateFrequency )
		wx.EVT_MENU( self, ID_HELP, self.onMouseControlsHelp )

		self.window = self.getSpaceViewPanel( self, -1, self )

		viewMenu.Check( self.ID_UPDATE_ENTITIES,
			self.window.surface.shouldUpdateEntities )

		ID_WARP = wx.NewId()
		warpMenu.Append( ID_WARP, "Warp To Time...\tCTRL+W" )
		wx.EVT_MENU( self, ID_WARP, self.window.surface.onWarp )

		ID_WARP_LOG = wx.NewId()
		warpMenu.Append( ID_WARP_LOG, "Warp To Log Event...\tCTRL+L" )
		wx.EVT_MENU( self, ID_WARP_LOG, self.window.surface.onWarpLog )

		self.window.surface.zoomToSpaceBounds()

	def getSpaceViewPanel( self, parent, id, frame ):
		return SpaceViewPanel( parent, id, frame )

	def onMouseControlsHelp( self, event ):
		wx.InitAllImageHandlers()
		helpctrl = wx.html.HtmlHelpController()
		helpctrl.AddBook( 'help/space_viewer.hhp', True )
		helpctrl.DisplayContents()
		self.helpctrl = helpctrl

	#@staticmethod
	def createLoggerAndView( parent, *args ):
		"""
		To create a new logger for a live server, pass the spaceID and UID as
		*args.  To create a logger to replay a log, pass the filename of the CMT
		db file as *args.
		"""

		# Get temp filename to write listener address to
		fd, tmp = tempfile.mkstemp()
		os.close( fd ); os.unlink( tmp )

		# Args is (cmtDBFilename)
		if len( args ) == 1:
			common = args[0].replace( ".cmt.db", "" )
			os.spawnl( os.P_NOWAIT, util.interpreter(), util.interpreter(),
					   svlogger.FULLPATH,
					   "-r", common,
					   "-w", tmp,
					   "-k" )

		# Args are (spaceID, UID)
		elif len( args ) == 2:
			os.spawnl( os.P_NOWAIT, util.interpreter(), util.interpreter(),
					   svlogger.FULLPATH,
					   "-s", str( args[0] ),
					   "-u", str( args[1] ),
					   "-w", tmp,
					   "-k" )

		else:
			raise RuntimeError, \
				  "You must pass either 2 or 3 args to createLoggerAndView()"

		# Read the tempfile until we get an address out of it (have to wait for
		# logger to start up and write it)
		while True:
			try:
				ip, port = open( tmp ).read().split( ":" )
				port = int( port )
				os.unlink( tmp )
				break
			except:
				time.sleep( 0.2 )

		# If the address is invalid, then the logger hasn't started up
		if ip == "none":
			return None

		# Make replayer
		replayer = replay.Replayer( (ip, port) )

		# Make viewer window
		return SpaceViewWindow( parent, replayer )
	createLoggerAndView = staticmethod( createLoggerAndView )

	# --------------------------------------------------------------------------
	# Section: Image Overlay Stuff
	# --------------------------------------------------------------------------

	class OverlayMenuItems( object ):
		"""A class to manage a pair of menu items.  The first is for adding some
		   kind of overlay to the display, and the second records a history of
		   all the recent overlays."""

		def __init__( self, window, parentMenu, name,
					  onAddCallback, onRecentCallback, onRemoveCallback,
					  itemStrFunc, maxSize = 5 ):

			self.parentMenu = parentMenu
			self.window = window
			self.name = name
			self.maxSize = maxSize
			self.onRecentCallback = onRecentCallback
			self.onRemoveCallback = onRemoveCallback

			# This is a function to convert an entry in the recentItems list
			# into a string for display in the recent items menu
			self.itemStrFunc = itemStrFunc

			# The top level menu everything resides in
			self.topMenu = wx.Menu()
			parentMenu.AppendMenu( wx.NewId(),
								   "%s Overlay" % name,
								   self.topMenu )

			# Add overlay menu item
			ID_ADD_OVERLAY = wx.NewId()
			self.topMenu.Append( ID_ADD_OVERLAY,
								 "Display &%s Overlay...\tCTRL+%s" % \
								 (name, name[0]) )
			wx.EVT_MENU( window, ID_ADD_OVERLAY, onAddCallback )

			# Remove overlay menu item
			ID_REMOVE_OVERLAY = wx.NewId()
			self.removeItem = self.topMenu.Append(
				ID_REMOVE_OVERLAY, "Remove %s Overlay" % name )
			if not self.removeItem:
				# wxWindows 2.4 backcompat
				self.removeItem = self.topMenu.FindItemById( ID_REMOVE_OVERLAY )
			self.removeItem.Enable( False )
			wx.EVT_MENU( window, ID_REMOVE_OVERLAY, self.onRemoveItemClicked )

			# Recent overlays submenu
			self.recentItems = self.getRecentItems()
			self.recentItemsMenu = wx.Menu()
			self.recentItemsId = wx.NewId()
			self.topMenu.AppendMenu( self.recentItemsId,
									 "Recent %s Overlays" % name,
									 self.recentItemsMenu )

			# A mapping from menu event IDs to the item they're for
			self.ids2items = {}

			# Populate menu with history
			self.populateRecentItems()

		def populateRecentItems( self ):

			# Remove all former entries in the recent items menu
			for mi in self.recentItemsMenu.GetMenuItems():
				self.recentItemsMenu.Remove( mi.GetId() )

			# Put the up-to-date ones in
			for item in self.recentItems:
				itemId = wx.NewId()

				# Make shortcut key for most recent mapping
				if item is self.recentItems[0]:
					self.recentItemsMenu.Append( \
						itemId, "%s\tCTRL+SHIFT+%s" %
						(self.itemStrFunc( item ),self.name[0]) )
				else:
					self.recentItemsMenu.Append( \
						itemId, self.itemStrFunc( item ) )

				wx.EVT_MENU( self.window, itemId, self.onRecentItemClicked )
				self.ids2items[ str( itemId ) ] = item

			# Enable or disable menu as appropriate
			self.topMenu.Enable( self.recentItemsId,
									bool( self.recentItems ) )

		def getRecentItems( self ):
			"""Reads the recent items from the prefs file."""

			prefs = xmlprefs.Prefs( space_viewer.PREFS )

			# Find the relevant node
			recentNode = prefs.getNode( "recent%sOverlays" % self.name, True )
			items = []

			# Go through each recent item entry
			for itemNode in recentNode.childNodes:

				# Turn the XML attributes into a hash and put it in the
				# return list
				item = {}
				for k, v in itemNode.attributes.items():
					item[ k ] = v
				items.append( item )

			return items

		def saveRecentItems( self ):
			"""Saves recent items to the prefs file."""

			prefs = xmlprefs.Prefs( space_viewer.PREFS )

			# Delete the existing history list if it exists
			node = prefs.getNode( "recent%sOverlays" % self.name, True )
			for child in node.childNodes[:]:
				node.removeChild( child )
				child.unlink()

			# Append current items
			for item in self.recentItems:
				itemNode = prefs.doc.createElement( "item" )
				for k, v in item.items():
					itemNode.setAttribute( k, v )
				node.appendChild( itemNode )

			# Save the prefs to disk
			prefs.save()

		def addToRecentItems( self, item ):

			# Enable the remove overlay menu item since we have an overlay now
			self.removeItem.Enable( True )

			# If the record is already in there, remove it from the list before
			# we add it in again in a second time
			if item in self.recentItems:
				self.recentItems.remove( item )

			# If it isn't and the list is full, then pop the last one off
			elif len( self.recentItems ) > self.maxSize:
				self.recentItems.pop()

			# Now add the new record to the top of the list
			self.recentItems.insert( 0, item )

			# Update the menu to reflect the change
			self.populateRecentItems()

			# Update the prefs to reflect the change
			self.saveRecentItems()

		def removeRecentItem( self, item ):
			if item in self.recentItems:
				self.recentItems.remove( item )
				self.populateRecentItems()
				self.saveRecentItems()

		def onRecentItemClicked( self, event ):
			self.removeItem.Enable( True )
			self.onRecentCallback( self.ids2items[ str( event.GetId() ) ] )

		def onRemoveItemClicked( self, event ):
			self.removeItem.Enable( False )
			self.onRemoveCallback( event )

	def onDisplayRecentImageOverlay( self, item ):

		imageFile = item[ "file" ]
		coords = map( lambda x: int( x ), item[ "coords" ].split( " " ) )

		# Create the master image in the surface
		self.setMasterImageOverlay( imageFile, coords )

		# Force redraw
		self.window.surface.hasZoomed = True

	def onDisplayImageOverlay( self, event ):

		prefs = xmlprefs.Prefs( space_viewer.PREFS )
		imageDir = prefs.get( "imageDir" ) or os.path.abspath( "." )
		imageFile = wx.FileSelector( "Choose an image to overlay", imageDir )
		if not imageFile: return
		prefs.set( "imageDir", os.path.dirname( imageFile ) )

		# Try to use coords from cellTalker for mapping
		try:
			coords = [ self.replayer.spaceBounds[x] for x in
					   [0, 2, 3, 5] ]

		# Where there is no cellTalker (i.e. in svreplay) we will pop up a
		# dialog asking for the coordinates
		except:
			# Ask user for coords to map image to
			coordPrefs = (prefs.get( "imageCoords" ) or
				"0 0 100 100").split( " " )
			coords = dialogs.GetMultipleTextDialog(
				self, "Coords to map image onto",
				[ { "label": "Min X", "default": coordPrefs[0] },
				  { "label": "Min Y", "default": coordPrefs[1] },
				  { "label": "Max X", "default": coordPrefs[2] },
				  { "label": "Max Y", "default": coordPrefs[3] } ] ).\
				  getSimpleResults()

			if not coords: return

			# Save coords pref
			coordStr = string.join( coords, " " )
			prefs.set( "imageCoords", coordStr )
			prefs.save()

		# Convert coords to ints
		coords = [ int( c ) for c in coords ]

		# Create the master image in the surface
		self.setMasterImageOverlay( imageFile, coords )

	def setMasterImageOverlay( self, imageFile, coords ):

		# Swap y values for wx coord system
		wxcoords = copy.copy( coords )
		(wxcoords[1], wxcoords[3]) = (wxcoords[3], wxcoords[1])

		# Recent menus entry
		item = { "file": imageFile,
				 "coords": string.join( map( lambda x: str(x), coords ), " " ) }

		# Try to create wx.Image
		image = wx.Image( imageFile )
		if not image.Ok():
			self.imageOverlayMenus.removeRecentItem( item )
			return

		# Set recent item entry
		self.imageOverlayMenus.addToRecentItems( item )

		# Set masterImage field
		self.window.surface.masterImage = self.window.surface.MappedImage(
			self.window.surface, image, wxcoords )

	def onRemoveImageOverlay( self, event ):
		self.window.surface.masterImage = None

	def onDisplayGraphOverlay( self, event ):

		prefs = xmlprefs.Prefs( space_viewer.PREFS )
		graphDir = prefs.get( "graphDir" ) or os.path.abspath( "." )
		graphFile = wx.FileSelector(	"Choose a graph file", graphDir )
		if not graphFile: return
		prefs.set( "graphDir", os.path.dirname( graphFile ) )

		scaleDefault = prefs.get( "graphScale" ) or "1.0"
		scale = wx.GetTextFromUser( "scalePos",
								   "Enter a scale factor:",
								   scaleDefault )
		if not scale: return

		prefs.set( "graphScale", scale )
		scale = float( scale )
		prefs.save()

		self.setGraphOverlay( graphFile, scale )

	def onDisplayRecentGraphOverlay( self, item ):

		graphFile = item[ "file" ]
		scale = float( item[ "scale" ] )

		self.setGraphOverlay( graphFile, scale )

	def onRemoveGraphOverlay( self, event ):
		self.graph = None

	def setGraphOverlay( self, graphFile, scale ):

		# Recent items entry
		item = {"file": graphFile, "scale": str( scale )}

		try:
			# Create graph
			g = graph.Graph.parse( graphFile )
			g.scale( scale )

			# Set recent items entry
			self.graphOverlayMenus.addToRecentItems( item )

			# Actually set the graph field to enable drawing
			self.graph = g

		except:

			# If failed, remove it from the recent items menu
			self.graphOverlayMenus.removeRecentItem( item )

	# --------------------------------------------------------------------------
	# End Section: Overlay Stuff
	# --------------------------------------------------------------------------

	def onZoomInTB( self, event ):
		self.window.surface.zoomIn()

	def onZoomOutTB( self, event ):
		self.window.surface.zoomOut()

	def onZoomToSpaceBounds( self, event ):
		self.window.surface.zoomToSpaceBounds()

	def onChangeCellUpdateFrequency( self, event ):

		if not self.replayer.online:
			wx.MessageBox( "Can't change log interval when replaying a log!",
						  "Error", wx.ICON_ERROR )
			return

		newint = wx.GetTextFromUser(
			"New Cell Sample Interval (seconds)",
			"Change Log Interval",
			str( self.replayer.ctInterval ) )

		if newint:
			interval = max( 0.2, float( newint ) )
			self.replayer.ctInterval = interval
			self.replayer.setIntervals()

	def onChangeCellAppMgrUpdateFrequency( self, event ):

		if not self.replayer.online:
			wx.MessageBox( "Can't change log interval when replaying a log!",
						  "Error", wx.ICON_ERROR )
			return

		newint = wx.GetTextFromUser(
			"New CellAppMgr Sample Interval (seconds)",
			"Change Log Interval",
			str( self.replayer.cmtInterval ) )

		if newint:
			interval = max( 0.2, float( newint ) )
			self.replayer.cmtInterval = interval
			self.replayer.setIntervals()

	def updateScaleChecks( self ):
		checked = False
		for size in style.sizes:
			if int(size) == style.ENTITY_SCALE_PERCENT:
				checked = True
				self.sizeMenu.Check( self.size2id[size], True )
		if not checked:
			self.sizeMenu.Check( self.ID_CUSTOM_SCALE, True )

	def onCustomEntityScale( self, event ):
		db = wx.TextEntryDialog( self,
				"New Entity Marker Percent Size (1-1000%)",
				"Change Entity Marker Percent Size",
				str( float( style.ENTITY_SCALE_PERCENT ) ) )
		db.ShowModal()
		# TODO: warn user if invalid number.
		tmpNum = float( db.GetValue() )
		if tmpNum < 1:
			tmpNum = 1
		elif tmpNum > 1000:
			tmpNum = 1000
		style.ENTITY_SCALE_PERCENT = tmpNum
		self.updateScaleChecks()

	def onSetEntityScale( self, event ):
		style.ENTITY_SCALE_PERCENT = int( self.id2size[ event.GetId() ] )

		# Remember this to avoid those big stupid dots
		prefs = xmlprefs.Prefs( space_viewer.PREFS )
		prefs.set( "entityScale", str( style.ENTITY_SCALE_PERCENT ) )
		prefs.save()

		self.updateScaleChecks()

	def onDelCell( self, event ):
		self.window.surface.onDelCell( event )

	def onDelCellApp( self, event ):
		self.window.surface.onDelCellApp( event )

	def onUpdateEntities( self, event ):
		self.window.surface.shouldUpdateEntities = event.Checked()

	def onExitNow( self, event ):
		self.GetParent().Destroy()

	def onCloseWindowAndLogger( self, event ):

		self.replayer.stopLogger()
		self.Destroy()
		self.window.surface.drawTimer.Stop()

	def onCloseWindow( self, event ):
		self.Destroy()
		self.window.surface.drawTimer.Stop()

		if self.stopLoggerOnExit:
			self.replayer.stopLogger()

class SpaceViewPanel( wx.Panel ):

	"""
	This class is no longer needed - but I've kept it here since it's likely
	it will be needed in the future.
	"""

	def __init__( self, parent, id, frame ):
		wx.Panel.__init__( self, parent, id )

		# create view surface
		self.surface = self.getSpaceViewSurface( self, -1, frame )

		# put surface in window
		sizer = wx.BoxSizer( wx.VERTICAL )
		sizer.Add( self.surface, 1, wx.EXPAND )
		self.SetAutoLayout( True )
		self.SetSizer( sizer )
		self.Layout()

	def getSpaceViewSurface( self, parent, id, frame ):
		return SpaceViewSurface( parent, id, frame )

class BufferedWindow( wx.Window ):

	"""
	Implements double buffering functionality
	Derive from this class and override the draw method.
	"""

	def __init__( self, parent, id, pos=wx.DefaultPosition,
					size=wx.DefaultSize, style=wx.NO_FULL_REPAINT_ON_RESIZE ):

		wx.Window.__init__( self, parent, id, pos, size, style )

		wx.EVT_PAINT( self, self.onPaint )
		wx.EVT_SIZE( self, self.onSize )

	# place holder: to be overriden
	def draw( self, dc ):
		pass

	def onSize( self, event ):
		self.Width, self.Height = self.GetClientSizeTuple()
		self.buffer = wx.EmptyBitmap( self.Width, self.Height )

		# Force re-scale of image overlay if appropriate
		self.hasZoomed = True

		self.updateDrawing()

	def onPaint( self, event ):
		dc = wx.BufferedPaintDC(self, self.buffer)

	def updateDrawing( self ):
		dc = wx.BufferedDC( wx.ClientDC(self), self.buffer )
		self.draw( dc )


class SpaceViewSurface( BufferedWindow ):

	"""
	The space view surface.
	Responsible for drawing itself, world / physical coord
	transforms and handling mouse / keyboard / scrollbar events.
	"""

	# Code is set up to accomodate different modes. This is no longer used, but
	# all the mechanincs for doing it is still here.
	DRAGGING = 1
	SELECTING = 2
	ZOOM_SELECTING = 3

	def __init__( self, parent, id, frame ):

		BufferedWindow.__init__( self, parent, id ) #, style=wx.VSCROLL |
													#	wx.HSCROLL )

		self.cellFont = wx.Font( 14, wx.ROMAN, wx.NORMAL, wx.BOLD )

		self.lastMoveXPos = 0
		self.lastMoveYPos = 0

		self.lostConnection = 0

		self.xTickOffsetCounter = 0  # arbitrary.
		self.yTickOffsetCounter = 0  # arbitrary.

		self.frame = frame
		self.statusBar = frame.statusBar
		self.replayer = frame.replayer

		self.shouldUpdateEntities = True

		self.rubberBand = rubber_bander.RubberBander( )
		self.movementSave = (0,0)
		self.pageStack = []

		# space bb and world bb in km.
		self.xPageSize = MAX_PAGE_SIZE / 1.0
		self.xPosition = -MAX_PAGE_SIZE / 2.0
		self.yPosition = MAX_PAGE_SIZE / 2.0

		# Image overlay master copy and current zoomed section
		self.masterImage = self.currImage = None
		self.hasZoomed = False

		self.id2name = {} # This is the menu num to name map
		self.entityVisible = {} # This is the entity visibility map
		self.entityVisible["Entity"] = True # Make the default visible

		# Set up menu to toggle visiblity of different entity types
		if self.replayer.ct and self.replayer.ct.getNumTypeNames() != 0:

			for i in range( self.replayer.ct.getNumTypeNames() ):

				id = wx.NewId()
				self.id2name[id] = self.replayer.ct.getTypeName( i )

				self.frame.visibleMenu.Append(
					id, self.id2name[id],
					"Toggle the visibility of the " +
					self.id2name[id] + " entities." ,
					wx.ITEM_CHECK )

				# All entities visible by default
				self.entityVisible[ self.id2name[id] ] = True
				self.frame.visibleMenu.Check( id, True )
				wx.EVT_MENU( self.frame, id, self.onToggleVisible )

			wx.EVT_MENU( self.frame, self.frame.ID_SHOW_ALL, self.onShowAll )
			wx.EVT_MENU( self.frame, self.frame.ID_HIDE_ALL, self.onHideAll )

		# Remove the visible menu if there is no entity list availible
		# (i.e. using an old cellApp)
		else:
			self.frame.menuBar.Remove(
				self.frame.menuBar.FindMenu( "Visible Entities" ) )

		self.frame.menuBar.Refresh()

		self.inTempSelect = 0
		self.defaultMode = self.DRAGGING
		self.rememberMode = self.DRAGGING
		self.setMode( self.DRAGGING )

		self.onSize( None )
		self.SetFocus()

		# set up timer. retrieve data from Cell App Manager and cells every
		# second.
		self.drawTimer = wx.PyTimer( self.onDrawTimer )
		self.drawTimer.Start( self.frame.replayer.displayInterval * 1000 )
		self.updateDrawing()

		wx.EVT_LEFT_DOWN( self, self.onLMouseDown )
		wx.EVT_LEFT_UP( self, self.onLMouseUp )
		wx.EVT_RIGHT_UP( self, self.onRMouseUp )
		wx.EVT_MIDDLE_DOWN( self, self.onMMouseDown )
		wx.EVT_MIDDLE_UP( self, self.onMMouseUp )
		wx.EVT_RIGHT_DOWN( self, self.onRMouseDown )
		wx.EVT_RIGHT_DCLICK( self, self.onRMouseDown )
		wx.EVT_MOTION( self, self.onMouseMove )

		wx.EVT_KEY_DOWN( self, self.onKeyDown )

		wx.EVT_MOUSEWHEEL( self, self.onMouseWheel )

	def onToggleVisible ( self, event ):
		id = event.GetId()
		name = self.id2name[id]
		visible = self.frame.visibleMenu.IsChecked( id );
		self.entityVisible[name] = visible # set the entity visibility

	def setAllVisible( self, visible):
		for i in xrange( self.cellTalker.getNumTypeNames() ):
			name = self.cellTalker.getTypeName( i )
			self.entityVisible[ name ] = visible
			self.frame.visibleMenu.Check(
				self.frame.visibleMenu.FindItem(name), visible )

	def onShowAll( self, event ):
		self.setAllVisible( True )

	def onHideAll( self, event ):
		self.setAllVisible( False )

	def onLMouseUp( self, event):
		if self.leftToTravel > 0:
			self.onRMouseDown( event )

		self.setMode( self.defaultMode )

	def onMMouseDown( self, event ):
		self.rubberBand.onMouseEvent( event )
		self.setMode( self.ZOOM_SELECTING )

	def onMMouseUp( self, event ):

		if self.mode == self.ZOOM_SELECTING:

			# rubber band class
			self.rubberBand.onMouseEvent( event )

			# world coordinate description of
			rb = self.rubberBand
			x1 = self.iTransformX( min(rb.m_stpoint.x,rb.m_endpoint.x) )
			y1 = self.iTransformY( min(rb.m_stpoint.y,rb.m_endpoint.y) )
			x2 = self.iTransformX( max(rb.m_stpoint.x,rb.m_endpoint.x) )
			y2 = self.iTransformY( max(rb.m_stpoint.y,rb.m_endpoint.y) )

			self.zoomToRect( x1, y1, x2, y2 )
			self.hasZoomed = True

			# update statusbar (get rid of width/height info).
			self.statusBar.SetStatusText( "Current Pos: (%.1f, %.1f)" %
					(self.iTransformX( event.GetX() ),
						 self.iTransformY( event.GetY() ) ), 1 )

		self.setMode( self.defaultMode )

	def zoomToRect( self, x1, y1, x2, y2 ):
		xw = x2 - x1
		yw = y1 - y2

		if xw != 0 and yw != 0:
			self.pageStack.append( (self.xPosition, self.yPosition, self.xPageSize) )

			self.xPosition = x1
			self.yPosition = y1
			self.xPageSize = xw

			if ( xw / self.Width > yw / self.Height ):
				self.xPageSize = xw
				nyw = -self.iTransformY( self.Height ) + self.yPosition
				ywdiff = nyw - yw
				self.yPosition += ywdiff / 2.0
			else:
				if self.Height != 0:
					self.xPageSize = float(yw) * float(self.Width) / float(self.Height)
					nxw = self.iTransformX( self.Width ) - self.xPosition
					xwdiff = nxw - xw
					self.xPosition -= xwdiff / 2.0

		self.updateDrawing()

	def onMouseWheel( self, event ):
		x = self.lastMoveXPos
		y = self.lastMoveYPos

		if event.GetWheelRotation() > 0:
			self.zoom(x, y, 12.0/10.0)

		elif event.GetWheelRotation() < 0:
			self.zoom(x, y, 10.0/12.0)


	def setMode( self, mode ):

		self.mode = mode

		# show correct cursor type.
		if mode == self.DRAGGING:
			self.SetCursor( wx.StockCursor(wx.CURSOR_HAND) )
			# How sensitive it is to decide between select and drag.
			self.leftToTravel = 3

		elif mode == self.SELECTING:
			self.SetCursor( wx.StockCursor(wx.CURSOR_ARROW) )

		else:
			self.SetCursor( wx.StockCursor(wx.CURSOR_CROSS) )

	def onRMouseDown( self, event ):

		if not self.lostConnection:
			self.changeCurrentCell( self.iTransformX(event.GetX()),
									self.iTransformY(event.GetY()) )
			self.setMode( self.SELECTING )


	def onLMouseDown( self, event ):

		self.movementSave = ( event.GetX(), event.GetY() )
		self.setMode( self.DRAGGING )

	def onMouseMove( self, event ):
		self.lastMoveXPos = event.GetX()
		self.lastMoveYPos = event.GetY()

		# default status bar text (if not selecting)
		x = self.iTransformX( event.GetX() )
		y = self.iTransformY( event.GetY() )
		sbText = "Current Pos: (%.1f, %.1f)" % (x, y)

		# handle drag
		if self.mode == self.DRAGGING and event.m_leftDown:
			# calculate new window - don't let user scroll outside +-13km boundary.
			gx = event.GetX()
			gy = event.GetY()

			dx = gx - self.movementSave[0]
			dy = gy - self.movementSave[1]

			self.leftToTravel -= (abs(dx) + abs(dy))
			if self.leftToTravel > 0:
				return
			self.leftToTravel = 0

			self.movementSave = ( gx, gy )
			d2 = self.iTransformDist( dx, dy )
			xPos = self.xPosition - d2[0]
			yPos = self.yPosition - d2[1]
			( newXPos, newYPos, newXPageSize ) = \
				self.makeNearestValidWindow( xPos, yPos, self.getXPageSize(), self.getYPageSize() )

			# update tick mark counters
			( pdx, pdy ) = self.transformDist( self.xPosition - newXPos, self.yPosition - newYPos )
			self.xTickOffsetCounter += pdx
			self.yTickOffsetCounter += pdy

			# finally update drawing.
			self.xPosition = newXPos
			self.yPosition = newYPos
			self.xPageSize = newXPageSize
			self.updateDrawing()

		# handle selection
		elif self.mode == self.ZOOM_SELECTING:
			self.rubberBand.onMouseEvent( event )
			self.updateDrawing()
			if self.rubberBand.middleClicked:
				r = self.iTransformDist( self.rubberBand.w, self.rubberBand.h )
				sbText = "Width/Height: (%.1f, %.1f)" % ( r[0], r[1] )

		self.statusBar.SetStatusText( sbText, 1 )


	def onRMouseUp( self, event ):
		self.setMode( self.defaultMode )

	def onKeyDown( self, event ):

		# Macro for seeking
		def seek( n ):
			wx.BeginBusyCursor()

			oldrt = self.frame.replayer.realtime
			self.frame.replayer.seek( n )
			if not oldrt and self.frame.replayer.realtime:
				self.statusBar.SetStatusText( "Showing real-time", 0 )
			elif oldrt and not self.frame.replayer.realtime:
				self.statusBar.SetStatusText( "Showing history", 0 )

			wx.EndBusyCursor()

		k = event.GetKeyCode()

		if k == wx.WXK_PRIOR: # PGUP
			self.zoomOut()

		elif k == wx.WXK_NEXT: # PGDN
			self.zoomIn()

		elif k == wx.WXK_TAB:
			if event.ShiftDown():
				self.changeToPrevCell()
			else:
				self.changeToNextCell()

		elif k == wx.WXK_RIGHT:
			seek( 1 )

		elif k == wx.WXK_LEFT:
			seek( -1 )

		elif k == wx.WXK_DOWN:
			seek( 60 )

		elif k == wx.WXK_UP:
			seek( -60 )

		elif k == wx.WXK_END:
			seek( 2**31 ) # That really oughta be enough (25000 days or so)

		elif k == wx.WXK_HOME:
			seek( -2**31 )

		self.updateDrawing()

	def onDrawTimer( self ):

		if self.lostConnection or not self.replayer.online:
			return

		try:
			# Time how long the network and draw updates take
			starttime = time.time()
			self.replayer.update()
			self.updateDrawing()
			elapsed = time.time() - starttime

			# If they took longer than the display interval, bump it
			if elapsed > self.frame.replayer.displayInterval:
				self.frame.replayer.displayInterval = elapsed * 1.5
				print "WARNING: Display lagging; bumped draw interval to %.2f" % \
					  self.frame.replayer.displayInterval

		except socket.error:
			self.lostConnection = 1
			dialog = wx.MessageDialog(
				self, "Lost connection to Cell App Manager. " +
				"Window no longer live.", "Error", wx.OK )
			dialog.ShowModal()

		# Check that our timer is in sync with the displayInterval
		if self.drawTimer.GetInterval() != \
		   self.frame.replayer.displayInterval * 1000:
			self.drawTimer.Start( self.frame.replayer.displayInterval * 1000 )

	def updateDrawing( self ):

		BufferedWindow.updateDrawing( self )

		# Update bounding box on status bar.
		if False:
			self.statusBar.SetStatusText( "Bounding Box: (%.0f, %.0f) -> (%.0f,%.0f)" %
											( self.xPosition, self.yPosition,
											  self.xPosition+self.getXPageSize(),
											  self.yPosition-self.getYPageSize() ), 0 )

		# Update number of entities on status bar.
		if self.replayer.ct:
			self.statusBar.SetStatusText(
				"Entities: %d   Ghosts: %d" %
				(len( self.replayer.ct.entityData ),
				 len(self.replayer.ct.ghostEntityData)), 2 )

		# Otherwise show total entities for server
		else:
			self.statusBar.SetStatusText(
				"Total Entities: %d" % self.replayer.cmt.numEntities,
				2 )

		# Update time
		timestr = time.strftime( "%H:%M:%S %d-%m-%Y",
								 time.localtime( self.replayer.dispTime ) )
		self.statusBar.SetStatusText( timestr, 3 )

	def changeCurrentCell( self, worldX, worldY ):
		cell = self.replayer.cmt.cellAt( worldX, worldY )
		self.changeCurrentCellTo( cell )

	def changeToNextCell( self, offset = 1 ):
		cells = self.replayer.cmt.cells
		try:
			curr = self.replayer.ct.addr
			index = cells.keys().index( curr )
			index = (index + offset) % len(cells)
		except:
			index = 0
		self.changeCurrentCellTo( cells.values()[ index ] )

	def changeToPrevCell( self ):
		self.changeToNextCell( -1 )

	def changeCellFrom( self, cellID ):
		cells = self.cellAppMgrTalker.cells
		for (id, data) in cells.items():
			if id != cellID:
				self.changeCurrentCellTo( data )
				return 1
		return 0

	def changeCurrentCellTo( self, cell ):

		self.replayer.selectCell( cell.addr )
		self.statusBar.SetStatusText( "Toggled display for cell %d" %
									  cell.appID, 0 )
		self.replayer.update()
		self.updateDrawing()

	def makeNearestValidWindow( self, xPos, yPos, xPageSize, yPageSize ):

		if xPageSize > MAX_PAGE_SIZE:
			xPageSize = MAX_PAGE_SIZE

		if yPageSize > MAX_PAGE_SIZE:
			yPageSize = MAX_PAGE_SIZE

		return ( xPos, yPos, xPageSize )


	def zoomIn( self ):
		self.zoom( 0.5*self.Width, 0.5*self.Height, 2.0 )


	def zoomOut( self ):
		self.zoom( 0.5*self.Width, 0.5*self.Height, 0.5 )


	def zoom( self, x, y, mag ):
		newXPageSize = self.getXPageSize() / mag
		if newXPageSize > MAX_PAGE_SIZE:
			return
		extraBit = ( self.getXPageSize() - newXPageSize ) * float(x) / float(self.Width)
		newXPos = self.xPosition + extraBit

		newYPageSize = self.getYPageSize() / mag
		extraBit = ( self.getYPageSize() - newYPageSize ) * float(y) / float(self.Height)
		newYPos = self.yPosition - extraBit

		self.xPosition = newXPos
		self.yPosition = newYPos
		self.xPageSize = newXPageSize
		self.yPageSize = newYPageSize

		self.hasZoomed = True

		self.updateDrawing()

	def zoomToSpaceBounds( self, bounds=None ):

		if bounds == None:
			bounds = self.getSpaceBounds()

		width = bounds[3] - bounds[0]
		height = bounds[5] - bounds[2]

		if (width > 0) and (height > 0):
			bufferRatio = 0.1
			xBuf = width * bufferRatio
			yBuf = height * bufferRatio

			self.zoomToRect( bounds[0] - xBuf, bounds[5] + yBuf,
					bounds[3] + xBuf, bounds[2] - yBuf )

	def getSpaceBounds( self ):
		return self.replayer.spaceBounds

	def onDelCell( self, event ):
		self.replayer.deleteCell( self.replayer.ct )

	def onDelCellApp( self, event ):
		self.replayer.deleteCellApp( self.replayer.ct )

	def warpToTime( self, timestr ):

		tgttime = float( time.mktime( time.strptime( timestr ) ) )
		self.replayer.seek( tgttime - self.replayer.reqTime )

	def onWarp( self, event ):

		evttime = wx.GetTextFromUser(
			"Enter the time you wish to warp to", "Warp",
			str( time.ctime( self.replayer.dispTime ) ) )

		if evttime:
			self.warpToTime( evttime )

		self.updateDrawing()

	def onWarpLog( self, event ):

		# If we haven't already read a command log, the user chooses one
		if not hasattr( self, "cmdlog" ):
			prefs = xmlprefs.Prefs( space_viewer.PREFS )
			self.cmdlog = wx.FileSelector( "Choose a timestamped log",
										  prefs.get( "cmdlogdir" ) or \
										  os.getcwd() )
			if self.cmdlog:
				prefs.set( "cmdlogdir", os.path.dirname( self.cmdlog ) )
				prefs.save()

		if self.cmdlog:

			# Open the command log and read out all the entries
			f = open( self.cmdlog, "r" )
			lines = map( lambda l: l.rstrip(), f.readlines() )
			f.close()

			# Ask the user which event they want to warp to
			diag = wx.SingleChoiceDialog( self, "Warp",
										 "Choose an event to warp to",
										 lines )
			diag.ShowModal()

			# Extract the timestamp for that event and warp to it
			if diag.GetStringSelection():
				sel = diag.GetStringSelection()
				evttime = util.extractTime( sel )
				if evttime:
					self.warpToTime( evttime )
				else:
					wx.MessageBox( "'%s' does not contain a time!" % sel,
								  "Error" )
		else:
			del self.cmdlog

		self.updateDrawing()

	def draw( self, dc ):

		dc.BeginDrawing()
		dc.SetBackground( wx.Brush( "WHITE" ) )
		dc.Clear()

		# Set title correctly
		self.GetParent().GetParent().SetTitle( self.replayer.cmt.getTitle() )

		# Draw the image overlay if required (y is inverted, x is not)
		if self.masterImage:

			# The extents visible in the viewport
			w, h = self.GetSizeTuple()
			viewExtents = [ self.iTransformX( 0 ),
							self.iTransformY( 0 ),
							self.iTransformX( w ),
							self.iTransformY( h ) ]

			self.drawImage( dc, viewExtents )

			#cs = self.frame.imagecoords
			#self.drawBitmap( dc, self.frame.bitmap, cs[0], cs[3],
			#				 cs[2]-cs[0], cs[1]-cs[3] )

		# draw grid
		if self.replayer.ct and style.gridColour:
			cell = self.replayer.cmt.cells[ self.replayer.ct.addr ]
			dc.SetPen( wx.Pen( style.gridColour ) )
			gridResolution = self.replayer.ct.gridResolution
			chunkBounds = cell.chunkBounds
			spaceBounds = self.getSpaceBounds()

			# Get the bounds in world coordinates
			worldX1 = max( chunkBounds.minX, spaceBounds[0] )
			worldX2 = min( chunkBounds.maxX, spaceBounds[3] )
			worldY1 = max( chunkBounds.minY, spaceBounds[2] )
			worldY2 = min( chunkBounds.maxY, spaceBounds[5] )

			# Convert to screen
			offset = -1
			screenX1 = max( -offset,              self.transformX( worldX1 ) )
			screenX2 = min( self.Width + offset,  self.transformX( worldX2 ) )
			screenY1 = max( -offset,              self.transformY( worldY2 ) )
			screenY2 = min( self.Height + offset, self.transformY( worldY1 ) )

			x = worldX1

			while x <= worldX2:
				screenX = self.transformX( x )

				if screenX >= 0:
					if screenX >= self.Width:
						break
					dc.DrawLine( screenX, screenY1, screenX, screenY2 )

				x += gridResolution


			y = worldY1

			while y <= worldY2:
				screenY = self.transformY( y )

				if screenY <= self.Height:
					if screenY <= 0:
						break
					dc.DrawLine( screenX1, screenY, screenX2, screenY )

				y += gridResolution

		# Draw the graph on if it exists
		if self.frame.graph:

			dc.SetPen( wx.Pen( "GREY" ) )
			dc.SetBrush( wx.Brush( "GREY" ) )

			for v in self.frame.graph.vs.values():

				# Draw edges
				for n in v.ns:
					self.drawLine( dc, v.pos.x, v.pos.z, n.pos.x, n.pos.z )

				# Draw vertex
				r = v.getRadius()
				dc.DrawCircle( self.transformX( v.pos.x ),
							   self.transformY( v.pos.z ),
							   self.transformDist( r, r )[0] )

		# draw axes
		if self.Width !=0:

			minPageDimension = min( self.getXPageSize(), self.getYPageSize() )
			exponent = math.floor( math.log10(minPageDimension*0.8 ) )
			dist = math.pow( 10, exponent )

			pixEquals = 1
			if self.Width != 0:
				pixEquals = self.xPageSize / float(self.Width)

			barLength = dist / pixEquals

			if exponent >= 3.0:
				text = "1" + ((int(exponent) - 3) * "0") + "km"
			elif exponent >= 0.0:
				text = "1" + (int(exponent) * "0") + "m"
			else:
				text = "0." + ((-1 - int(exponent)) * "0") + "1m"

			dc.SetPen( wx.Pen( "BLACK" ) )
			dc.SetBrush( wx.Brush( "LIGHT GREY" ) )

			dc.SetTextForeground( "BLACK" )
			dc.DrawText( "tick distance: " + text, 20, self.Height - 35 )

			# draw x axis
			tickPosition = 14 + operator.mod( self.xTickOffsetCounter, barLength )
			dc.DrawLine( 14, self.Height - 14, self.Width - 14, self.Height - 14 )
			while tickPosition < self.Width - 20:
				dc.DrawLine( int(tickPosition), self.Height - 14,
					int(tickPosition), self.Height - 9 )
				tickPosition += barLength

			# draw yAxis
			tickPosition = self.Height - 14 + \
				operator.mod( self.yTickOffsetCounter, barLength )
			dc.DrawLine( 14, 14, 14, self.Height - 14 )
			while tickPosition > 14:
				if tickPosition < self.Height - 14:
					dc.DrawLine( 9, int(tickPosition), 14, int(tickPosition) )
				tickPosition -= barLength

		# draw cell boundary lines
		# Note: the rect is inflated so that the boundaries are clipped
		# off by the window pane (ie so the cells will draw as unbounded rects
		wclip = MFRectangle(
				self.iTransformX( -3 ),
				self.iTransformY( self.Height + 3 ),
				self.iTransformX( self.Width + 3 ),
				self.iTransformY( -3 ) )

		class EdgeDrawer:
			def __init__( self, surface, dc, shouldDrawLoad ):
				self.surface = surface
				self.dc = dc
				self.shouldDrawLoad = shouldDrawLoad

			def visitInterval( self, branch, pt1, pt2 ):
				# print "visitInterval", pt1, pt2
				self.surface.drawLine( self.dc,
						pt1[0], pt1[1], pt2[0], pt2[1] )

				if self.shouldDrawLoad:
					x0 = (pt1[0] + pt2[0])/2
					y0 = (pt1[1] + pt2[1])/2
					txt = "%.3f" % branch.load
					try:
						self.dc.DrawText( txt,
								self.surface.transformX(x0)+5,
								self.surface.transformY(y0) )
					except:
						pass

		if style.cellBoundaryColour:
			dc.SetPen( wx.Pen( style.cellBoundaryColour ) )

			oldFont = dc.GetFont()
			shouldDrawLoad = (style.partitionLoadColour != None)
			if shouldDrawLoad:
				dc.SetFont( self.cellFont )
				dc.SetTextForeground( style.partitionLoadColour )

			self.replayer.cmt.visit( wclip,
					EdgeDrawer( self, dc, shouldDrawLoad ) )

			dc.SetFont( oldFont )

		# draw space boundary
		if style.spaceBoundaryColour:
			dc.SetPen( wx.Pen( style.spaceBoundaryColour ) )
			dc.SetBrush( wx.Brush( "WHITE", wx.TRANSPARENT ) )
			sb = self.getSpaceBounds()
			offset = 1
			rectX0 = max( -offset, self.transformX( sb[0] ) )
			rectX1 = min( self.Width + offset, self.transformX( sb[3] ) )
			rectY0 = max( -offset, self.transformY( sb[5] ) )
			rectY1 = min( self.Height + offset, self.transformY( sb[2] ) )
			try:
				# Draw with lines so that the space bounds lines up better with
				# the grid rectangles.
				# dc.DrawRectangle( rectX0, rectY0,
				#		rectX1 - rectX0, rectY1 - rectY0 )
				dc.DrawLine( rectX0, rectY0, rectX0, rectY1 )
				dc.DrawLine( rectX0, rectY0, rectX1, rectY0 )
				dc.DrawLine( rectX0, rectY1, rectX1, rectY1 )
				dc.DrawLine( rectX1, rectY0, rectX1, rectY1 )
			except OverflowError:
				pass

		self.closestEntityX = 0
		self.closestEntityY = 0
		self.closestEntityDistSqr = 25*25 # Need to be within 25 metres to show details
		self.closestEntityID = -1
		self.closestEntityType = -1
		self.closestEntityPixelSize = 0

		if self.replayer.ct:

			# draw ghost entities
			if style.ghostEntityColour:
				dc.SetPen( wx.Pen( style.ghostEntityColour ) )
				dc.SetBrush( wx.Brush( "WHITE" ) )
				for i in self.replayer.ct.ghostEntityData:
					self.drawGhostEntity( dc, i[0], i[1] )

			# draw entites
			dc.SetPen( wx.Pen( wx.Colour( 50, 50, 0 ) ) )
			for i in self.replayer.ct.entityData:

				# Since there is no guarantee that the CT has a type mapping
				# during the viewer's init, we catch unmapped typenames now
				typeName = self.replayer.ct.getTypeName( i[2] )
				if not self.entityVisible.has_key( typeName ):
					self.entityVisible[ typeName ] = True

				if self.entityVisible[ typeName ]:
					self.drawEntity( dc, i[0], i[1], i[3], i[2] )

		#IGOR - old tooltip info goes here

		# draw centres
		class CentreDrawer:
			def __init__( self, surface, dc ):
				self.surface = surface
				self.dc = dc

				# Calculate min and max loads if we're doing relative colouring
				if self.surface.frame.relativeColouringItem.IsChecked():
					self.maxload = max( [ c.load for c in
										  self.surface.replayer.cmt.cells.values() ] )
					self.minload = min( [ c.load for c in
										  self.surface.replayer.cmt.cells.values() ] )

			def visitCell( self, cell, rect ):
				s = self.surface
				# x = (rect.minX + rect.maxX)/2
				# y = (rect.minY + rect.maxY)/2
				self.surface.drawCellCentre( self.dc, rect, cell, self )
				ct = self.surface.replayer.ct

				if ct and cell.addr == ct.addr and style.cellBoundaryColour:
					oldPen = dc.GetPen()
					dc.SetBrush( wx.Brush( "WHITE", wx.TRANSPARENT ) )
					dc.SetPen( wx.Pen( style.cellBoundaryColour, 3,
								wx.LONG_DASH ) )
					x1 = s.transformX( rect.minX )
					x2 = s.transformX( rect.maxX )
					y1 = s.transformY( rect.minY )
					y2 = s.transformY( rect.maxY )
					try:
						newX1 = max( x1, 0 )
						newY1 = min( y1, self.surface.Height )
						newX2 = min( x2, self.surface.Width )
						newY2 = max( y2, 0 )

						oldX1 = max( x1, -5 )
						oldY1 = min( y1, self.surface.Height + 5 )
						oldX2 = min( x2, self.surface.Width + 5 )
						oldY2 = max( y2, -5 )

						dc.DrawRectangle( newX1, newY1,
								newX2 - newX1, newY2 - newY1 )
						dc.SetPen( wx.Pen( style.cellBoundaryColour, 3 ) )
						# dc.DrawRectangle( x1, y1, x2 - x1, y2 - y1 )
						dc.DrawRectangle( oldX1, oldY1,
								oldX2 - oldX1, oldY2 - oldY1 )
					except OverflowError:
						pass
					dc.SetPen( oldPen )

				# Draw the entity bounds
				if style.entityBoundsColour:
					b = cell.entityBounds
					spaceBounds = self.surface.getSpaceBounds()

					# Convert to screen
					x1 = max( -1,           s.transformX( b.minX ) )
					x2 = min( s.Width + 1,  s.transformX( b.maxX ) )
					y1 = max( -1,           s.transformY( b.maxY ) )
					y2 = min( s.Height + 1, s.transformY( b.minY ) )

					dc.SetPen( wx.Pen( style.entityBoundsColour ) )
					dc.SetBrush( wx.Brush( "GREY", wx.TRANSPARENT ) )
					try:
						self.dc.DrawRectangle( x1, y1, x2-x1, y2-y1 )
					except OverflowError:
						pass

		self.replayer.cmt.visit( wclip, CentreDrawer( self, dc ) )

		# draw rubber band
		# TODO: it would be better to use use XOR drawing out of the main draw
		# method for this because it can be a bit slow to update on a slow
		# machine. However, getting the timing right with the double buffering
		# is tricky.
		if self.mode == self.ZOOM_SELECTING and \
									self.rubberBand.middleClicked:
			rb = self.rubberBand
			dc.SetBrush( wx.Brush( "WHITE", wx.TRANSPARENT ) )
			dc.SetPen( wx.Pen( "DARK GREY" ) )
			dc.DrawRectangle( rb.m_stpoint.x, rb.m_stpoint.y, rb.w, rb.h )

		# If there is an entity close to the cursor then draw it

		if self.closestEntityID != -1:

			offset = self.closestEntityPixelSize / math.sqrt(2.0) + 1;

			name = self.replayer.ct.getTypeName( self.closestEntityType )

			x = int(10.0 * self.closestEntityX) / 10.0; # Trim x-pos to 1dp
			y = int(10.0 * self.closestEntityY) / 10.0; # Trim y-pos to 1dp

			desc = name + " #" + str(self.closestEntityID) + " (" + str(x) + "," + str(y) + ")"

			(width,height) = dc.GetTextExtent(desc)

			dc.SetBrush( wx.Brush( "WHITE", wx.SOLID ) )
			dc.SetPen( wx.Pen( "BLACK" ) )

			dc.DrawRectangle(
			self.transformX(self.closestEntityX) + offset,
			self.transformY(self.closestEntityY) + offset,
			width + 4,
			height + 2 )

			dc.SetTextForeground( "BLACK" )

			dc.DrawText(
				name +
				" #" + str(self.closestEntityID) +
				" (" + str(x) + "," + str(y) + ")",
				self.transformX(self.closestEntityX) + offset + 2,
				self.transformY(self.closestEntityY) + offset
				)

		# Print warning if no cell selected (i.e. entities aren't being drawn)
		if not self.replayer.ct:
			dc.SetFont( wx.Font( 12, wx.ROMAN, wx.NORMAL, wx.NORMAL ) )
			dc.SetTextForeground( "#BB0000" )

			text = "select cell to show entities"
			w, h = dc.GetTextExtent( text )
			dc.DrawText( text, self.Width-w-20, self.Height-35 )

		# Draw geometry mappings
		geometryMappingVPos = 10;
		for type, matrix, path in self.replayer.spaceGeometryMappings:
			dc.SetFont( wx.Font( 12, wx.ROMAN, wx.NORMAL, wx.NORMAL ) )
			dc.SetTextForeground( "#A020F0" )

			text = path # + " at " + str( matrix ) + ", type " + str( type )
			w, h = dc.GetTextExtent( text )
			dc.DrawText( text, 20, geometryMappingVPos )
			geometryMappingVPos += h

		dc.EndDrawing()

	def drawEntity( self, dc, x0, y0, entityID, entityTypeID ):
		entityName = self.replayer.ct.getTypeName( entityTypeID )
		colour = style.getColourForEntityType( entityName )
		scale = style.getScaleForEntityType( entityName )
		dc.SetBrush( wx.Brush( wx.Colour(colour[0], colour[1], colour[2]) ) )
		try:
			pixelSize = style.ENTITY_SCALE_PERCENT * scale * style.ENTITY_RADIUS / 100

			if pixelSize < 1:
				pixelSize = 1
			elif pixelSize > 100:
				pixelSize = 100

			dc.DrawCircle(
				self.transformX(x0),
				self.transformY(y0),
				pixelSize )

			cx = self.iTransformX( self.lastMoveXPos )
			cy = self.iTransformY( self.lastMoveYPos )

			dx = cx - x0
			dy = cy - y0
			distSqr = dx*dx + dy*dy

			if distSqr < self.closestEntityDistSqr:
				self.closestEntityX = x0
				self.closestEntityY = y0
				self.closestEntityDistSqr = distSqr
				self.closestEntityID = entityID
				self.closestEntityType = entityTypeID
				self.closestEntityPixelSize = pixelSize

		except OverflowError:
			pass

	def drawGhostEntity( self, dc, x0, y0 ):
		try:
			scale = style.ENTITY_SCALE_PERCENT
			pixelSize = scale * style.GHOST_ENTITY_RADIUS / 100

			if pixelSize < 1:
				pixelSize = 1
			elif pixelSize > 100:
				pixelSize = 100

			dc.DrawCircle(
				self.transformX(x0),
				self.transformY(y0),
				pixelSize )
		except OverflowError:
			pass

	def drawCellCentre( self, dc, rect, cell, centreDrawer ):
		xMin = max( 0, self.transformX( rect.minX ) )
		yMin = max( 0, self.transformY( rect.maxY ) )
		xMax = min( self.Width, self.transformX( rect.maxX ) )
		yMax = min( self.Height, self.transformY( rect.minY ) )

		x = (xMin + xMax)/2
		y = (yMin + yMax)/2

		oldFont = dc.GetFont()
		dc.SetFont( self.cellFont )

		# Colour the cell's load
		if self.frame.relativeColouringItem.IsChecked():
			if len( self.frame.replayer.cmt.cells ) > 1:
				perceivedload = (cell.load - centreDrawer.minload) / \
								(centreDrawer.maxload - centreDrawer.minload)
			else:
				perceivedload = 0.5

		else:
			perceivedload = min( cell.load, MAX_SERVER_CPU ) / MAX_SERVER_CPU

		rgb = HSV2RGB( (1-perceivedload)/2.0, 1, 1 )
		loadColour = wx.Colour( *rgb )

		if cell.isOverloaded or cell.isRetiring or \
			   len( self.replayer.cmt.cells ) < 10:
			text = (
				("%d" % cell.appID, style.cellAppIDColour),
				(socket.inet_ntoa( struct.pack( "<I", cell.addr[0] ) ),
				 style.ipAddrColour ),
				("%.3f" % cell.load, loadColour ),
				("Retiring", (None, "RED")[ cell.isRetiring ] ),
				("Overloaded", (None, "RED")[ cell.isOverloaded ]))

		else:
			text = [ ("%.3f" % cell.load, loadColour ) ]

		for (t, colour) in text:
			if colour:
				dc.SetTextForeground( colour )
				(w, h) = dc.GetTextExtent( t )
				try:
					dc.DrawText( t, x - w/2, y - h/2 )
				except OverflowError:
					pass
				y += h

		dc.SetFont( oldFont )

	def drawLine( self, dc, x0, y0, x1, y1 ):
		try:
			dc.DrawLine( self.transformX(x0), self.transformY(y0),
						 self.transformX(x1), self.transformY(y1) )
		except:
			pass

	def drawRect( self, dc, x0, y0, w, h ):
		(w, h) = self.transformDist( w, h )
		dc.DrawRectangle( self.transformX(x0), self.transformY(y0), w , h )

	def drawImage( self, dc, viewExtents ):

		# Macro to restrict the first set of coords to within the box defined by
		# the second set
		def intersect( xs, ys ):
			fs = ( max, min, min, max )
			return [ fs[i]( xs[i], ys[i] )
					 for i in xrange( len( fs ) ) ]

		# Macro to determine if any of the first set of coords is outside the
		# second set of coords
		def outside( xs, ys ):
			testvals = (-1, 1, 1, -1)
			for i in xrange( len( testvals ) ):
				if cmp( xs[i], ys[i] ) == testvals[i]:
					return True
			return False

		# Macro to expand a set coords by the given ratio
		def expand( xs, amt ):
			w, h = (xs[2] - xs[0], xs[3] - xs[1])
			amt = float( amt )
			return (xs[0] - (amt-1)/2 * w,
					xs[1] - (amt-1)/2 * h,
					xs[2] + (amt-1)/2 * w,
					xs[3] + (amt-1)/2 * h)

		# Compute the area covered by some coords
		def area( xs ):
			return (xs[2]-xs[0]) * (xs[1]-xs[3])

		# Work out max world coords to draw
		drawCoords = intersect( self.masterImage.coords, viewExtents )

		# If we don't actually have a currImage, or if the zoom level has
		# changed, or if the any one of the drawCoords exceeds any one of the
		# world coords the currImage is mapped to, we need to resample
		if not self.currImage or \
				outside( drawCoords, self.currImage.coords ) or \
				self.hasZoomed:
			self.hasZoomed = False

			# Blow out the coords slightly (to give us some hysteresis for
			# panning) while still staying within the master coord limits
			hystCoords = intersect( expand( drawCoords, 1.5 ),
									self.masterImage.coords )

			self.currImage = self.masterImage.getSubImage( hystCoords )

		# Draw the current image
		if self.currImage:
			self.currImage.draw( dc )

	# transform world coordinate -> physical coordinate.
	def transformX( self, x ):
		return int( (float(x)-self.xPosition)/self.getXPageSize() *
				self.Width )

	def transformY( self, y ):
		return int( (self.yPosition-float(y))/self.getYPageSize() *
				self.Height )

	def transformDist( self, dx, dy ):
		dx2 = int( dx / self.getXPageSize() * self.Width )
		dy2 = int( -dy / self.getYPageSize() * self.Height )
		return (dx2, dy2)

	# transform physical coordinate -> world coordinate
	def iTransformX( self, x ):
		return float(x)/float(self.Width) * self.getXPageSize() + self.xPosition

	def iTransformY( self, y ):
		return self.yPosition - (float(y))/float(self.Height) * self.getYPageSize()

	def iTransformDist( self, dx, dy ):
		dx2 = float(dx)/float(self.Width) * self.getXPageSize()
		dy2 = -float(dy)/float(self.Height) * self.getYPageSize()
		return (dx2, dy2)


	def getXPageSize( self ):
		return self.xPageSize

	def setXPageSize( self, l ):
		self.xPageSize = l

	def getYPageSize( self ):
		if self.Width != 0:
			return float(self.xPageSize) * float(self.Height) / float(self.Width)
		else:
			return self.xPageSize

	def setYPageSize( self, l ):
		if self.Height == 0:
			self.xPageSize = l
		else:
			self.xPageSize = self.Width / self.Height * l

	def getDimsForCoords( self, coords ):
		return self.transformDist( coords[2] - coords[0],
								   coords[3] - coords[1] )

	class MappedImage( object ):
		"""An image logically mapped to world coords."""

		def __init__( self, surface, image, coords ):
			"""This constructor should only ever be called directly for creating the
			   master image.  All scaled/sliced images should be created using
			   getSubImage()."""

			self.surface = surface
			self.image = image
			self.coords = coords
			self.bitmap = None

		def getDims( self ):
			"""Returns the pixel dimensions of the actual image stored within
			   this object."""
			return (self.image.GetWidth(), self.image.GetHeight() )

		def getWindowCoords( self ):
			"""Returns the (x,y,w,h) window coords that this image will be drawn
			   to."""

			return (self.surface.transformX( self.coords[0] ),
					self.surface.transformY( self.coords[1] )) + \
					self.surface.transformDist(
				self.coords[2] - self.coords[0],
				self.coords[3] - self.coords[1] )


		def getSubImage( self, coords ):
			"""Return the subimage of this image taken from the given world
			   coords."""

			# Window dimensions of scaled image
			dims = self.surface.transformDist( coords[2] - coords[0],
											   coords[3] - coords[1] )

			# If the coords are the same but the dims are different, we just
			# need to rescale
			if coords == self.coords and dims != self.getDims():
				return self.surface.MappedImage( self.surface,
												 self.image.Scale( *dims ),
												 coords )

			# If the coords are different we need to subimage
			if coords != self.coords:

				# Width and height in world coords of the source image
				mywh = (self.coords[2] - self.coords[0],
						self.coords[3] - self.coords[1])

				# Width and height in pixels of source image
				mydims = self.getDims()

				# (x,y,w,h) in world coords to slice from the source image
				wsubregion = (coords[0] - self.coords[0],
							  coords[1] - self.coords[1],
							  coords[2] - coords[0],
							  coords[3] - coords[1])

				# (x,y,w,h) in pixel to slice from the source image
				psubregion = (int( wsubregion[0] / mywh[0] * mydims[0] ),
							  int( wsubregion[1] / mywh[1] * mydims[1] ),
							  int( wsubregion[2] / mywh[0] * mydims[0] ),
							  int( wsubregion[3] / mywh[1] * mydims[1] ))

				rect = wx.Rect( *psubregion )
				im = self.image.GetSubImage( rect ).Scale( *dims )
				return self.surface.MappedImage( self.surface, im, coords )

		def draw( self, dc ):

			# If we don't have a bitmap, make one now
			if not self.bitmap:
				self.bitmap = self.image.ConvertToBitmap()

			# Window coords we're drawing to
			x, y = (self.surface.transformX( self.coords[0] ),
					self.surface.transformY( self.coords[1] ))

			# Draw it to the right coordinates
			dc.DrawBitmap( self.bitmap, x, y )


if __name__ == '__main__':
	print "must run from space_viewer.py or sv.py"
