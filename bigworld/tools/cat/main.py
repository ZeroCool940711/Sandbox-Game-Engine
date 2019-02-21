# main.py

import wx
import wx.html
from wx.lib.scrolledpanel import ScrolledPanel as wxScrolledPanel
from wx.lib.intctrl import IntCtrl as wxIntCtrl

from comm import *
from options import *
import time
import pickle
import socket
import string
import sys

ID_ABOUT = 101
ID_EXIT = 102
ID_CHOOSE_CLIENT = 103
ID_XBCHOOSER_IP = 110
ID_XBCHOOSER_NAME = 111
ID_INITIAL = 120

version_str = "$Id: main.py 71385 2008-04-28 01:11:19Z paulm $"


comms = None
mainWindow = None

def connectToLast():
	mainWindow.connectToLast()


class exAboutWindow( wx.Window ):
	def __init__( self, parent, id, frame, buildInfo ):
		self.parent = parent
		wx.Window.__init__( self, parent, id, wx.Point( 0, 0 ), 
			wx.Size( parent.parent.pict.GetWidth(),
				parent.parent.pict.GetHeight() ) 
		)
		self.buffer = wx.EmptyBitmap( parent.parent.pict.GetWidth(),
			parent.parent.pict.GetHeight() )
		self.info = buildInfo;
		wx.EVT_LEFT_DOWN( self, self.onButton )
		wx.EVT_PAINT( self, self.onPaint )

	# place holder: to be overriden
	def draw( self, dc ):
		pass

	def onPaint( self, event ):
		dc = wx.BufferedPaintDC( self, self.buffer )
		dc.DrawBitmap( self.parent.parent.pict, 0, 0 )
		dc.SetTextForeground( wx.Colour( 128, 128, 128 ) );
		dc.DrawText( self.info, 70, 310 )

	def updateDrawing( self ):
		dc = wx.BufferedDC( wx.ClientDC( self ), self.buffer )
		self.draw( dc )

	def onButton( self, event ):
		self.parent.parent.Close( True )

class exAboutPanel( wx.Panel ):
	def __init__( self, parent, id, frame, buildNum, buildDate, buildTime ):
		self.parent = parent
		wx.Panel.__init__( self, parent, -1, wx.Point( 0, 0 ), 
			wx.Size( parent.pict.GetWidth(),
				parent.pict.GetHeight() ), 
			wx.NO_BORDER )

		aboutStr = "Version 1.9.1: built " + buildDate + " at " + buildTime

		self.about = exAboutWindow( self, -1, frame, aboutStr )

class AboutBox ( wx.Frame ):
	def __init__( self, parent, ID, buildNum, buildDate, buildTime ):
		self.pict = wx.Bitmap( "aboutbox.bmp",  wx.BITMAP_TYPE_BMP )
		wx.Frame.__init__( self,parent,ID, '', 
			wx.Point( 0, 0 ),
			wx.Size( self.pict.GetWidth(), self.pict.GetHeight() ),
			wx.STAY_ON_TOP | wx.SIMPLE_BORDER )
		wx.Frame.Centre( self, wx.BOTH )
		panel = exAboutPanel( self, -1, self, buildNum, buildDate, buildTime )

#---------------------------------------------------------------------------

class ControlPanel( wxScrolledPanel ):
	def __init__( self, parent, id, script ):
		wxScrolledPanel.__init__( self, parent, id )
		self.script = script

		# create the vertical sizer
		self.sizerV = wx.BoxSizer( wx.VERTICAL )

		# add info if available
		if dir( self.script ).count( "info" ) > 0:
			data = self.script.info.replace( "\n", " " )
			text = wx.TextCtrl( self, -1, "", size = ( 80, 60 ),
								style = wx.ALIGN_LEFT | wx.TE_MULTILINE |
								wx.VSCROLL | wx.STATIC_BORDER )
			text.SetValue( data.strip() )
			text.SetBackgroundColour( self.GetBackgroundColour() )
			self.sizerV.Add(text, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | 
				wx.ALL, 5 )

		# add elements
		for arg in script.args:
			global comms
			control = arg.createControl(self, comms)
			# note: initial value is set on the first update
			self.sizerV.Add( control, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | 
				wx.ALL, 5 )

		if len( script.commands ) > 0 and len( script.args ) > 0:
			# add seperator
			line = wx.StaticLine(self, -1, size=(20,-1), style=wx.LI_HORIZONTAL)
			self.sizerV.Add(line, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | 
				wx.RIGHT | wx.TOP, 5 )

		cID = ID_INITIAL
		for command in script.commands:
			# add a button to send the command
			buttonBox = wx.BoxSizer( wx.HORIZONTAL )
			button = wx.Button( self, cID, command[0], size=(80,-1) )
			buttonBox.Add( button, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
			self.sizerV.Add( buttonBox, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | 
				wx.ALL, 5 )
		wx.EVT_BUTTON( self, cID, self.onClickCommandButton )
		cID = cID + 1

		self.SetSizer( self.sizerV )
		self.SetAutoLayout( True )
		self.SetupScrolling()

	#------------------------------

	def onClickCommandButton( self, event ):
		# send the local variables for the command
		for arg in self.script.args:
			arg.postValue()

		# send the command (need to append "\r" at end of each line
		commandName = event.GetEventObject().GetLabel()
		# find the appropriate command
		commandText = ""
		for command in self.script.commands:
			if command[0] == commandName:
				commandText = command[1]

		commandLines = commandText.split( "\n" )
		global comms
		for line in commandLines:
			comms.post( line )

	#------------------------------

	def envSetup( self ):
		global comms
		# do any script specific imports
		if dir( self.script ).count( "envSetup" ) > 0:
			commandLines = self.script.envSetup.split( "\n" )
			for line in commandLines:
				comms.post( line )

	#------------------------------

	def fillControls( self ):
		# update the args that need updating
		for arg in self.script.args:
			arg.fill()

	#------------------------------

	def updateControls( self, force = False ):
		# update the args that need updating
		for arg in self.script.args:
			arg.update( force )


#---------------------------------------------------------------------------


class ClientChooser( wx.Dialog ):
	def __init__( self, parent, settings ):
		wx.Dialog.__init__( self, parent, -1, "Client Chooser",
							size=wx.Size(350, 200) )
		self.settings = settings

		# new tag name (to remove references to xbox)
		oldData = settings.read( "xboxChoices" )
		if oldData != "":
			# rewrite the data with new tag name
			settings.write( "clientChoices", oldData )
			settings.erase( "xboxChoices" )

		self.selections = {}
		data = settings.read( "clientChoices" )
		if data != "":
			data = data.replace( "*nl_", "\n" )
			self.selections = pickle.loads( data )
		clientChoices = []
		if len( self.selections ) > 0:
			clientChoices = self.selections.keys()
		else:
			clientChoices = ["Local Host"]

		sizer = wx.BoxSizer( wx.VERTICAL )

		box = wx.BoxSizer( wx.HORIZONTAL )
		label = wx.StaticText(self, -1, "Previous choices:")
		box.Add( label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		if len( clientChoices ):
			comboInitial = clientChoices[0]
		else:
			comboInitial = ""

		self.combo = wx.ComboBox( self, -1, size=(160,-1),
							value = comboInitial,
							choices = clientChoices,
							style = wx.CB_DROPDOWN | wx.CB_READONLY )

		box.Add( self.combo, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		sizer.Add( box, 0, wx.ALIGN_CENTER_VERTICAL | wx.ALL, 5 )

		line = wx.StaticLine( self, -1, size=(20,-1), style=wx.LI_HORIZONTAL )
		sizer.Add( line, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT | 
			wx.TOP, 5 )

		box = wx.BoxSizer( wx.HORIZONTAL )
		label = wx.StaticText( self, -1, "Or enter new client details:" )
		box.Add( label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		sizer.Add(box, 0, wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT | 
			wx.TOP, 5 )

		box = wx.BoxSizer( wx.HORIZONTAL )
		label = wx.StaticText( self, -1, "Computer name:" )
		box.Add( label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		self.name = wx.TextCtrl( self, ID_XBCHOOSER_NAME, "", size=(80,-1) )
		box.Add( self.name, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
		sizer.Add( box, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | 
			wx.RIGHT, 5 )
		self.name.SetValue( "" )  # Make sure this is empty by default

		box = wx.BoxSizer( wx.HORIZONTAL )
		label = wx.StaticText( self, -1, "IP address:" )
		box.Add( label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		self.ip = ( 
			wxIntCtrl( self, min = 0, max = 255, 
				limited = True, size = (30, -1) ),
			wxIntCtrl( self, min = 0, max = 255, 
				limited = True, size = (30, -1) ),
			wxIntCtrl( self, min = 0, max = 255, 
				limited = True, size = (30, -1) ),
			wxIntCtrl( self, min = 0, max = 255, 
				limited = True, size = (30, -1) ))
		for a in self.ip:
			box.Add( a, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )

		sizer.Add( box, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | 
			wx.RIGHT | wx.BOTTOM, 5 )

		line = wx.StaticLine( self, -1, size=(20,-1), style=wx.LI_HORIZONTAL )
		sizer.Add( line, 0, wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT | 
			wx.TOP, 5 )

		box = wx.BoxSizer( wx.HORIZONTAL )

		btn = wx.Button( self, wx.ID_OK, " OK " )
		btn.SetDefault()
		box.Add( btn, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		btn = wx.Button( self, wx.ID_CANCEL, " Cancel " )
		box.Add( btn, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )
		sizer.Add( box, 0, wx.ALIGN_CENTER_VERTICAL | wx.ALL, 5 )

		self.SetSizer( sizer )
		self.SetAutoLayout( True )
		sizer.Fit( self )

		icon = wx.Icon( "cat.ico", wx.BITMAP_TYPE_ICO )
		self.SetIcon( icon )

	#------------------------------

	def getNameAndIP( self ):
		nameString = self.name.GetValue().strip()
		ipString = ""
		for a in self.ip:
			ipString = ipString + str( a.GetValue() ) + "."
		ipString = ipString.rstrip( "." )

		if nameString != "" and ipString != "0.0.0.0":
			self.selections[ nameString ] = ipString
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( nameString, ipString )
		elif nameString != "":
			self.selections[ nameString ] = ""
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( nameString, "" )
		elif ipString != "0.0.0.0":
			if ipString == "127.0.0.1":
				hostName = "Local Host"
			else:
				hostName = ipString
			self.selections[ hostName ] = ipString
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( "", ipString )

		# otherwise look in the combo box
		if self.combo.GetValue() == "":
			return ( "", "" )

		if  self.selections.has_key( self.combo.GetValue() ):
			return ( self.combo.GetValue(), str( self.selections[ self.combo.GetValue() ] ) )

		if self.combo.GetValue() == "Local Host":
			self.selections[ "Local Host" ] = "127.0.0.1"
			# add to the options
			data = pickle.dumps( self.selections )
			data = data.replace( "\n", "*nl_" )
			self.settings.write( "clientChoices", data )
			return ( "", "127.0.0.1")

		return ( "", "" )


#---------------------------------------------------------------------------


class UpdateTimer( wx.Timer ):
    def __init__( self, target, dur=1000 ):
        wx.Timer.__init__( self )
        self.target = target
        self.Start( dur )

    def Notify(self):
        """Called every timer interval"""
        if self.target:
            self.target.onAutoUpdate()
            self.target.displayConnectionStatus()


#---------------------------------------------------------------------------


class MainWindow( wx.Frame ):
	def __init__( self, parent, id, title ):
		wx.Frame.__init__( self,parent,wx.ID_ANY, title, size = (500,532),
			style = wx.DEFAULT_FRAME_STYLE ) #| wx.NO_FULL_REPAINT_ON_RESIZE )
		global mainWindow
		mainWindow = self

		wx.EVT_CLOSE( self, self.onClose )

		# statusbar at the window bottom
		self.CreateStatusBar()

		# setting up the menu.
		filemenu= wx.Menu()
		filemenu.Append( ID_CHOOSE_CLIENT, "&Choose &client...",
			" Choose an client to interface to" )
		filemenu.AppendSeparator()
		filemenu.Append( ID_EXIT, "E&xit"," Terminate the program" )

		helpmenu= wx.Menu()
		helpmenu.Append( ID_ABOUT, "&About CAT...", 
			" Information about this program" )

		# creating the menubar.
		menuBar = wx.MenuBar()
		menuBar.Append( filemenu, "&File" ) # Adding the "filemenu" to the 
											# MenuBar
		menuBar.Append( helpmenu, "&Help" )
		self.SetMenuBar( menuBar )  # Adding the MenuBar to the Frame content.

		# connect the events
		wx.EVT_MENU( self, ID_ABOUT, self.onAbout )
		wx.EVT_MENU( self, ID_EXIT, self.onExit )
		wx.EVT_MENU( self, ID_CHOOSE_CLIENT, self.onChooseClient )

		# create a toolbar
		toolbar = self.CreateToolBar( wx.TB_HORIZONTAL | wx.NO_BORDER | 
			wx.TB_FLAT | wx.TB_TEXT | wx.NO_FULL_REPAINT_ON_RESIZE )
		buttonID = wx.NewId()
		button = wx.Button( toolbar, buttonID, "Reconnect" )
		toolbar.AddControl( button )
		wx.EVT_BUTTON( self, buttonID, self.onReconnect )

		buttonID = wx.NewId()
		button = wx.Button( toolbar, buttonID, "Disconnect" )
		toolbar.AddControl( button )
		wx.EVT_BUTTON( self, buttonID, self.onDisconnect )

		toolbar.AddSeparator()
		buttonID = wx.NewId()
		self.updateButton = wx.Button( toolbar, buttonID, "Update" )
		toolbar.AddControl( self.updateButton )
		wx.EVT_BUTTON( self, buttonID, self.onUpdate )

		toolbar.AddSeparator()

		checkID = wx.NewId()
		self.updateCheckBox = wx.CheckBox( toolbar, checkID, "Auto Update" )
		self.updateCheckBox.SetValue( False )
		toolbar.AddControl( self.updateCheckBox )
		wx.EVT_CHECKBOX( toolbar, checkID, self.onClickAutoUpdate )

#		autoUpdateOnBmp = wx.Bitmap( "auto_update.bmp", wx.BITMAP_TYPE_BMP )
#		autoUpdateOffBmp = wx.Bitmap( "update.bmp", wx.BITMAP_TYPE_BMP )
#		updateBmp = wx.Bitmap( "update.bmp", wx.BITMAP_TYPE_BMP )
#		toolbar.AddTool( 10, autoUpdateOffBmp, autoUpdateOnBmp, "Auto Update", 
#			"Periodically update the displayed controls." )
#		toolbar.AddSimpleTool( 20, updateBmp, "Auto Update", 
#			"Periodically update the displayed controls." )
#		wx.EVT_TOOL( self, 20, self.onClickAutoUpdate)

		toolbar.Realize()
		self.SetToolBar( toolbar )

		# split the window into two
		self.splitter = wx.SplitterWindow(self, -1, style=wx.NO_3D | wx.SP_3D)

		# load the options file
		self.settings = Options( "localsettings.xml" )
		self.options = Options( "options.xml" )

		# create an update timer
		self.timer = UpdateTimer(self, 2000) # update once every 2 seconds
		if self.settings.read( "autoUpdate" ) == "True":
			self.updateCheckBox.SetValue( True )
			self.onClickAutoUpdate( None )

		# setup any additional search paths
		paths = self.options.readTags( "additionalSearchPath" )
		for path in paths:
			# insert at the beginning (for efficiency later)
			sys.path[ :0] = [ os.environ[ "MF_ROOT" ] + "/" + path ]

		# establish telnet connection
		global comms
		comms = CommTelnet()
		self.connectToLast()

		# create a tree control
		tID = wx.NewId()
		self.tree = wx.TreeCtrl( self.splitter, tID )

		# populate tree
		self.panels = {}

		treeRoot = self.tree.AddRoot( "Control Menu" )

		# load the scripts into the tree
		self.loadScripts( treeRoot )

# TODO: watchers interface
#		branch = self.tree.AppendItem( root, "watchers" )
		self.tree.Expand( treeRoot )

		# handle events
		wx.EVT_TREE_SEL_CHANGED( self, tID, self.onTreeSelectionChange )
		wx.EVT_TREE_ITEM_ACTIVATED( self, tID, self.onTreeItemActivated )

		# display window
		self.emptyPanel = wx.Panel( self.splitter, wx.NewId() )
		self.splitter.SplitVertically(self.tree, self.emptyPanel, 190)
		self.splitter.SetMinimumPaneSize(20)

		icon = wx.Icon( "cat.ico",wx.BITMAP_TYPE_ICO )
		self.SetIcon( icon )

		self.Show( True )

	#------------------------------

	def connectToLast( self ):
		# find an client and connect
		oldData = self.settings.read( "lastXBox" )
		if oldData != "":
			# rewrite the data with new tag name
			self.settings.write( "lastClient", oldData )
			self.settings.erase( "lastXBox" )

		clientName = self.settings.read( "lastClient" )
		clientIP = ""
		if clientName == "":
			clientName, clientIP = self.runClientChooser()

		self.connectTo( (clientName, clientIP) )

	#------------------------------

	def displayConnectionStatus( self ):
		global comms
		if comms != None:
			if comms.connected():
				self.SetTitle( "CAT - " + self.settings.read( "lastClient" ) )
				return
		self.SetTitle( "CAT (not connected)" )

	#------------------------------

	def runClientChooser( self ):
		# ask user to choose an client
		dlg = ClientChooser( self, self.settings )
		dlg.CenterOnScreen()
		retVal = dlg.ShowModal()
		if retVal != wx.ID_OK:
			return ("", "")

		clientName, clientIP = dlg.getNameAndIP()
		dlg.Destroy()

		if clientName == "" and clientIP == "":
			dlg = wx.MessageDialog( self, "No client specified to connect to.\n Please specify the client computer name or title IP address via the file menu.",
								  "Error", wx.OK | wx.ICON_ERROR )
			dlg.ShowModal()
			dlg.Destroy()
			# ask again
			return self.runClientChooser()
		else:
			return (clientName, clientIP)

	#------------------------------

	def connectTo( self, clientInfo ):
		clientName = clientInfo[0]
		clientIPString = clientInfo[1]

		if clientName == "" and clientIPString == "":
			return

		# set up the telnet connection
		global comms
		comms.disconnect()
		if clientName != "" and clientName != "Unknown" and clientName != "Local Host" and clientName != clientIPString:
			if comms.connect( clientName, 50001 ):
				self.settings.write( "lastClient", clientName )
				return

		if clientIPString != "":
			if clientName != "Unknown" and clientName != "Local Host" and clientName != clientIPString:
				print "Finding client via name failed, trying ip address."
		 	if comms.connect( clientIPString, 50001 ):
				self.settings.write( "lastClient", clientIPString )
				return

		dlg = wx.MessageDialog( self, "Could not connect to specified client",
							  "Error", wx.OK | wx.ICON_ERROR )
		dlg.ShowModal()
		dlg.Destroy()


	#------------------------------

	def formatName( self, name ):
		# ensure the first letter is capitalised
		fname = name[0]
		fname.upper()
		# put a space before capitals
		for i in range( 1, len( name ) ):
			ch = name[i]
			if ch.isupper():
				if name[i-1].islower():
					fname = fname + " "
			fname = fname + ch
		# remove underscores
		return fname.replace( "_", " " )


	def loadScriptDir( self, treeState, scDir, branch, currBranch ):
		if not os.path.exists( scDir ):
			print "INFO: cannot load script directory:", scDir
			return

		# insert at the beginning (for efficient searching)
		sys.path[ :0] = [ scDir ]

		# sort the dir list into files and directories
		dirlist = os.listdir( scDir )
		files = {}
		directories = []
		for name in dirlist:
			if name in [ "CVS", ".svn" ]:
				continue
			scriptsDir = scDir + "/" + name
			if os.path.isdir( scriptsDir ):
				directories.append( name )
			else:
				if name[ -3: ] == ".py":
					files[ name[ : -3 ] ] = True
				elif name[ -4: ] == ".pyc":
					files[ name[ : -4 ] ] = True

		for name in files.keys():
			# determine whether extension is .py
			moduleName = name
			#determine whether this is a loadable script
			if moduleName[ : 2] != "__":
				scr = __import__( moduleName, {}, {}, "s1" )
				scName = self.formatName( moduleName )
				self.tree.AppendItem( branch, scName )
				# create panel for each scripts
				panel = ControlPanel( self.splitter, -1, scr )
				panel.Show( False )
				self.panels[ scName ] = panel

		for name in directories:
			scriptsDir = scDir + "/" + name
			if os.path.isdir( scriptsDir ):
				# make a new tree branch
				scDirName = self.formatName( name )
				branchName = currBranch + "/" + scDirName
				if self.name2branch.has_key( branchName ) == 0:
					scriptBranch = self.tree.AppendItem( branch, scDirName )
					self.name2branch[ branchName ] = scriptBranch
				else:
					scriptBranch = self.name2branch[ branchName ]
				self.loadScriptDir( treeState, scriptsDir, scriptBranch, branchName  )

		branchName = self.tree.GetItemText( branch )
		if treeState.has_key( branchName ):
			if treeState[ branchName ]:
				self.tree.Expand( branch )
		else:
			self.tree.Collapse( branch )


	def loadScripts( self, treeRoot ):
		self.name2branch = {} # This map will be used to determine if a branch already exists.
		if sys.platform == "win32":
			sep = ';'
		else:
			sep = ':'
		optionsDirs = self.options.readTags( "scriptDirectory" )
		addDirs = []
		if "BW_RES_PATH" in os.environ:
			for resDir in string.split( os.environ["BW_RES_PATH"], sep ):
				scriptDir = os.path.abspath( \
					os.path.join( resDir, "..", "tools", "cat", "scripts" ) )
				addDirs.append( scriptDir )
		for dir in optionsDirs:
			scriptDir = os.path.abspath( \
				os.path.join( os.environ["MF_ROOT"], dir ) )
			addDirs.append( scriptDir )
		treeState = self.getTreeExpandedState()
		for dir in addDirs:
			print "loading from", dir
			self.loadScriptDir( treeState, dir, treeRoot, "" )

	#------------------------------

	def checkConnection( self ):
		global comms
		if not comms.checkConnection():
			return False

		if not comms.hasReconnected():
			return True

		# setup the env again
		for item in self.panels.values():
			item.envSetup()

		# fill the controls
		panel = self.splitter.GetWindow2()
		if id(panel) != id(self.emptyPanel):
			panel.fillControls()

		return True

	#------------------------------

	def recordTreeExpandedState( self ):
		cookie = 0
		root = self.tree.GetRootItem()
		try:
			(child, cookie) = self.tree.GetFirstChild(root, cookie)
		except:
			(child, cookie) = self.tree.GetFirstChild( root )

		state = {}
		while child.IsOk():
			if self.tree.ItemHasChildren( child ):
				state[ self.tree.GetItemText(child) ] = \
					self.tree.IsExpanded( child )
			(child, cookie) = self.tree.GetNextChild(root, cookie)
		# add to the options
		data = pickle.dumps( state )
		data = data.replace( "\n", "*nl_" )
		self.settings.write( "treeExpandedState", data )

	def getTreeExpandedState( self ):
		state = {}
		data = self.settings.read( "treeExpandedState" )
		if data != "":
			data = data.replace( "*nl_", "\n" )
			state = pickle.loads( data )
		return state

	#------------------------------

	def onUpdate( self, event = None, force = True ):
		if not self.checkConnection():
			return

		panel = self.splitter.GetWindow2()
		if id(panel) != id(self.emptyPanel):
			panel.updateControls( force )

	def onAutoUpdate( self ):
		if self.updateCheckBox.GetValue():
			self.onUpdate( force = False )

	#------------------------------

	def onClickAutoUpdate( self, event ):
		if self.updateCheckBox.GetValue():
			self.settings.write( "autoUpdate", "True" )
		else:
			self.settings.write( "autoUpdate", "False" )

	#------------------------------

	def onTreeSelectionChange( self, event ):
		# switch the displayed panel
		item = event.GetItem()

		# ignore if not a script
		if item == self.tree.GetRootItem():
			return

		# ignore if not under the scripts folder
#		ok = False
#		itemID = self.tree.GetItemParent( item )
#		while itemID != self.tree.GetRootItem():
#			if self.tree.GetItemText(itemID).rfind( "scripts" ) != -1:
#				ok = True
#				break
#			itemID = self.tree.GetItemParent( itemID )
#		if not ok:
#			return

		oldPanel = self.splitter.GetWindow2()

		# if a directory, set to empty panel
		if self.tree.GetChildrenCount( item, False ) != 0:
			newPanel = self.emptyPanel
		else:
			itemText = self.tree.GetItemText(item)

			newPanel = self.panels[ itemText ]
			if self.checkConnection():
				newPanel.fillControls()
				newPanel.updateControls( True )

		if id(newPanel) != id(oldPanel):
			self.splitter.ReplaceWindow( oldPanel, newPanel )
			oldPanel.Show( False )
			newPanel.Show( True )

	#------------------------------

	def onTreeItemActivated( self, event ):
		self.onUpdate()

	#------------------------------

	def onAbout( self, event ):
		"""
		Show the About dialog.
		"""

		"""
		# create and show a message dialog box
		d= wx.MessageDialog( self,
			"CAT -- Client Access Tool.\n"
			"Part of BigWorld Technology. Version " + version_str.split()[2],
			"About CAT", wx.OK )
		d.ShowModal()
		# destroy dialog when finished.
		d.Destroy()
		"""



		frame = AboutBox( None, -1, 
			version_str.split()[2], version_str.split()[3], 
			version_str.split()[4] )
		frame.Show( True )

		"""
		dialog = imagebrowser.ImageDialog ( None )
		# Show the dialog
		dialog.ShowModal()
		# Destroy the dialog
		dialog.Destroy()
		"""

	#------------------------------

	def onExit( self, event ):
		# close the frame.
		self.Close( True )

	#------------------------------

	def onClose( self, event ):
		print "closing.."
		# stop the timer
		self.timer.Stop()

		# remember the current state of the tree
		self.recordTreeExpandedState()

		# save the options
		self.settings.writeAndClose()

		# end the telnet connection
		global comms
		ans = comms.disconnect()

		# really close
		self.Destroy()
		print "closed."

	#------------------------------

	def onChooseClient( self, event ):
		self.connectTo( self.runClientChooser() )

	#------------------------------

	def onDisconnect( self, event ):
		global comms
		comms.disconnect()

	#------------------------------

	def onReconnect( self, event ):
		global comms
		comms.disconnect()
		self.connectToLast()


#---------------------------------------------------------------------------


class catApp( wx.App ):
	def OnInit( self ):
		# run the program
		frame = MainWindow(None, -1, "CAT")
		self.SetTopWindow(frame)
		return True

	def OnExit( self ):
		pass


#---------------------------------------------------------------------------

# run the application
app = catApp( 0 )
app.MainLoop()
