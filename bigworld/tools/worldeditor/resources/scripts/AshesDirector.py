import WorldEditor
import keys
import GUI
import Locator
import View
import Functor
import ResMgr
from keys import *




class AshesDirector:
	def __init__( self ):
		self.objInfo = ObjInfo()
		pass

	def onStart( self ):
		c = WorldEditor.camera()
		c.speed = WorldEditor.getOptionFloat( "app/cameraSpeed" )
		c.turboSpeed = WorldEditor.getOptionFloat( "app/cameraSpeed2" )

		
		self.alphafunc = Functor.TerrainTextureFunctor()
		self.guiview = View.AlphaGUITextureToolView( self.alphafunc )

		viewmap = ""
		try:
			viewmap = WorldEditor.getOptionString( "tools/alphaTool" )
		except:
			pass
		if viewmap == "": viewmap = "resources/maps/gizmo/disc.bmp"
		
		self.alphatool = WorldEditor.Tool(
			Locator.TerrainToolLocator(),
			View.TerrainTextureToolView( viewmap ),
			self.alphafunc )


		vismap = ""
		try:
			vismap = WorldEditor.getOptionString( "tools/chunkVisualisation" )
		except:
			pass
		if vismap == "": vismap = "resources/maps/gizmo/square.bmp"

		self.chunkvis = View.TerrainChunkTextureToolView( vismap )

		scv = WorldEditor.getOptionString( "tools/showChunkVisualisation" )
		if scv == "true":
			self.alphatool.addView( self.chunkvis, "chunkVisualisation" )

		self.alphatool.addView( self.guiview, "alphaGUI" )
		self.guiview.visible = 1

		self.alphatool.size = WorldEditor.getOptionFloat( "tools/alphaToolSize" )
		self.alphatool.strength = WorldEditor.getOptionFloat( "tools/alphaToolStrength" )

		self.heightfunc = Functor.TerrainHeightFilterFunctor()

		# Create the ecotype tool
		self.ecotool = WorldEditor.Tool(
				Locator.TerrainChunkLocator(),
				View.TerrainChunkTextureToolView( vismap ),
				Functor.HeightPoleEcotypeFunctor() );
		self.ecotool.size = 10000

		# Create the object manipulation tool
		self.objtool = WorldEditor.Tool()
		self.objtool.functor = Functor.ScriptedFunctor(
			ChunkItemFunctor( self.objtool, self.objInfo ) )

		# Make the closed captions commentary viewer
		self.cc = GUI.ClosedCaptions( WorldEditor.getOptionInt( "consoles/numMessageLines" ) )

		self.onResume( 0 )


	def onStop( self ):
		self.onPause()

		del self.cc
		
		return 0

	def onPause( self ):
		WorldEditor.popTool()
		self.cc.visible = 0
		self.cc.delAsView()

	def onResume( self, exitCode ):
		WorldEditor.pushTool( self.alphatool )

		self.cc.addAsView()
		self.cc.visible = 1


	def ownKeyEvent( self, key, modifiers ):
		t = WorldEditor.tool()

		handled = 1

		if key == KEY_RBRACKET:
			if modifiers & MODIFIER_SHIFT:
				t.strength = t.strength * 1.25
				WorldEditor.addCommentaryMsg( "Tool strength %0.1f" % t.strength, 0 )
			else:
				t.size = t.size * 1.25
				WorldEditor.addCommentaryMsg( "Tool size %0.1f" % t.size, 0 )
		elif key == KEY_LBRACKET:
			if modifiers & MODIFIER_SHIFT:
				t.strength = t.strength * 0.8
				WorldEditor.addCommentaryMsg( "Tool strength %0.1f" % t.strength, 0 )
			else:
				t.size = t.size * 0.8
				WorldEditor.addCommentaryMsg( "Tool size %0.1f" % t.size, 0 )

		elif key == KEY_F6:
			scv = WorldEditor.getOptionString( "tools/showChunkVisualisation" )
			if scv == "true":
				self.alphatool.delView( self.chunkvis )
				WorldEditor.setOptionString( "tools/showChunkVisualisation", "false" )
			else:
				self.alphatool.addView( self.chunkvis )
				WorldEditor.setOptionString( "tools/showChunkVisualisation", "true" )

		elif key == KEY_F8:
			WorldEditor.save()

		elif key == KEY_F9:
			if not t.applying:
				if t == self.alphatool:
					if t.functor == self.alphafunc:
						t.functor = self.heightfunc
						self.guiview.visible = 0
						WorldEditor.addCommentaryMsg(
							"Entering height filter mode.  Press LMB to apply", 0 )
					else:
						t.functor = self.alphafunc
						WorldEditor.pushTool( self.ecotool )
						WorldEditor.addCommentaryMsg(
							"Entering ecotype mode.  Press Enter to apply", 0 )
				elif t == self.ecotool:
					WorldEditor.popTool()
					WorldEditor.pushTool( self.objtool )
					WorldEditor.addCommentaryMsg(
						"Entering objt manipln mode. Use LMB to select", 0 )
				else:
					WorldEditor.popTool()
					self.guiview.visible = 1
					WorldEditor.addCommentaryMsg(
						"Entering alpha mode.  Press LMB to apply", 0 )
					

		elif key == KEY_Z:
			if not t.applying and (modifiers & MODIFIER_CONTROL):
				if not (modifiers & MODIFIER_SHIFT):
					what = WorldEditor.undo(0)
					if what:
						WorldEditor.addCommentaryMsg( "Undoing: " + what, 1 )
					WorldEditor.undo()
				else:
					what = WorldEditor.redo(0)
					if what:
						WorldEditor.addCommentaryMsg( "Redoing: " + what, 1 )
					WorldEditor.redo()

		elif key == KEY_I and (modifiers & MODIFIER_CONTROL):
			self.objInfo.browseUp()
			self.objInfo.showBrowse()
		elif key == KEY_K and (modifiers & MODIFIER_CONTROL):
			self.objInfo.browseDown()
			self.objInfo.showBrowse()
		elif key == KEY_J and (modifiers & MODIFIER_CONTROL):
			self.objInfo.browseLeft()
			self.objInfo.showBrowse()
		elif key == KEY_L and (modifiers & MODIFIER_CONTROL):
			self.objInfo.browseRight()
			self.objInfo.showBrowse()

		else:
			handled = 0
		
		return handled

	def onKeyEvent( self, isDown, key, modifiers ):
		handled = WorldEditor.camera().handleKeyEvent( isDown, key, modifiers )

		if not handled and isDown:
			handled = self.ownKeyEvent( key, modifiers )
		if not handled:
			handled = WorldEditor.tool().handleKeyEvent( isDown, key, modifiers )
		return handled

	def onMouseEvent( self, mx, my, mz ):
		handled = 0
		if not handled:
			handled = WorldEditor.tool().handleMouseEvent( mx, my, mz )
		if not handled:
			handled = WorldEditor.camera().handleMouseEvent( mx, my, mz )
		return handled

	def updateState( self, dTime ):
		GUI.update( dTime )
		self.cc.update( dTime )
		WorldEditor.camera().update( dTime )
		if not WorldEditor.tool().applying:
			self.objInfo.overGizmo = WorldEditor.gizmoUpdate( WorldEditor.worldRay() )
		WorldEditor.update( dTime )
		return 1

	def render( self, dTime ):
		WorldEditor.camera().render( dTime )
		WorldEditor.render( dTime )
		GUI.draw()
		return 1



# This is a helper class for passing state between the module and
#  the object manipulation functor
class ObjInfo:
	def __init__( self ):
		self.overGizmo = 0

		self.browsePath = []
		self.browseSect = ResMgr.root._objects
		self.browseIdx = 0

	def showBrowse( self ):
		WorldEditor.addCommentaryMsg( self.getBrowsePath() )

	def getBrowsePath( self ):
		as = ""
		for i in self.browsePath:
			as += i.name
			as += "/"
		as += self.browseSect.name
		if self.browseIdx < len(self.browseSect.keys()):
			as += "/" + self.browseSect.keys()[ self.browseIdx ]

		return as

	def browseUp( self ):
		self.browseIdx = (self.browseIdx + 1) % len(self.browseSect.keys())

	def browseDown( self ):
		self.browseIdx = (self.browseIdx - 1) % len(self.browseSect.keys())

	def browseLeft( self ):
		if len(self.browsePath):
			self.browseSect = self.browsePath[-1]
			self.browsePath = self.browsePath[:-1]
			self.browseIdx = 0

	def browseRight( self ):
		if len(self.browseSect.keys()):
			self.browsePath.append( self.browseSect )
			self.browseSect = self.browseSect.values()[ self.browseIdx ]
			self.browseIdx = 0







class ChunkItemFunctor:
	def __init__( self, tool, oi ):
		# set up the tool we are part of
		self.mouseLocator = Locator.ChunkItemLocator(
			Locator.TerrainToolLocator() )
		self.mouseRevealer = self.mouseLocator.revealer
		self.selection = WorldEditor.ChunkItemGroup()

		self.mouseView = View.ChunkItemBounds( self.mouseRevealer, 0xff0000ff )
		self.selView = View.ChunkItemBounds( self.selection, 0xff00ff00 )

		tool.locator = self.mouseLocator
		tool.addView( self.mouseView )
		tool.addView( self.selView )

		# store a reference to the object info class
		self.objInfo = oi



	# key event entry point
	def onKeyEvent( self, (isDown, key, modifiers), tool ):
		handled = 0
		if isDown:
			if key == KEY_LEFTMOUSE:
				self.onLeftMouse()
				handled = 1
			elif key == KEY_INSERT:
				bp = self.objInfo.getBrowsePath()
				if len(bp)>6 and bp[-6:] == ".model":
					d = ResMgr.DataSection( "model" )
					d.writeString( "resource", bp )
					WorldEditor.createChunkItem( d, self.mouseLocator.subLocator )
			elif key == KEY_DELETE:
				if self.selection.size:
					WorldEditor.deleteChunkItem( self.selection )
					self.selection.rem( self.selection )

		return handled
	

	# update entry point
	def update( self, dTime, tool ):
		if self.objInfo.overGizmo:
			self.mouseView.revealer = None
		else:
			self.mouseView.revealer = self.mouseRevealer

		# TODO: Check if mouse button down and threshold crossed.
		# If so start a drag with the current object
		pass

	def onLeftMouse( self ):
		# first see if there's a gizmo in the house
		if self.objInfo.overGizmo:
			# if so, let it take care of things
			WorldEditor.gizmoClick()
			return

		# we have to compare what's under the cursor to our selection

		# if there's no selection then (for now) set it to that
		#  and do nothing more
		if not self.selection.size:
			self.selection.add( self.mouseRevealer )
			self.selUpdate()
			return

		# if there's a selection but nothing under the mouse,
		#  clear the selection
		if self.selection.size and not self.mouseRevealer.size:
			self.selection.rem( self.selection )
			self.selUpdate()
			return

		# if the selection is different to what's under the mouse,
		#  change the selection
		diff = WorldEditor.ChunkItemGroup()
		diff.add( self.selection )
		diff.rem( self.mouseRevealer )	# cumbersome I know
		if diff.size:
			self.selection.rem( self.selection )
			self.selection.add( self.mouseRevealer )
			self.selUpdate()
			return

		# ok the selection and what's under the mouse are the same,
		#  start a drag of the selection

		# first make the tool
		nt = WorldEditor.Tool()
		nt.locator = self.mouseLocator
		nt.functor = Functor.MatrixMover( self.selection )

		# and then push it onto the stack. it'll pop itself off when done
		WorldEditor.pushTool( nt )


	def selUpdate( self ):
		if self.selection.size:
			if WorldEditor.isKeyDown( KEY_C ):
				self.selEditor = WorldEditor.ChunkEditor( self.selection )
			else:
				self.selEditor = WorldEditor.ChunkItemEditor( self.selection )
			WorldEditor.setCurrentEditors( self.selEditor )
			print "Selected a", self.selEditor.description
		else:
			self.selEditor = None
			WorldEditor.setCurrentEditors()


