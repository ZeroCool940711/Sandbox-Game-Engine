import WorldEditor
import keys
import GUI
import Locator
import View
import Functor
import ResMgr
import Math
from BigWorld import setWatcher
from keys import *


# constants
dragBoxColour = 0x80202020
hoverColour = 0xff00ff00
selectColour = 0xffffffff
selectGrowFactor = 0.005   # grow the selection box by 0.5 percent the size of the object
selectTexture = "helpers/maps/dashed_line.tga"


# globals
bd = None
oi = None

# classes
class WorldEditorDirector:
	def __init__( self ):
		global bd
		bd = self
		self.objInfo = ObjInfo()
		global oi
		oi = self.objInfo
		self.modeName = "Object"
		self.terrainModeName = "TerrainTexture"
		self.itemSnapMode = 0
		self.nextTimeDoSelUpdate = 0
		self.currentTerrainFilter = -1
		self.rightMouseButtonDown = 0
		self.mouseMoved = 0
		self.eDown = 0
		self.qDown = 0
		self.avatarMode = 0
		self.modeStack = []
		self.needsChunkVizUpdate = False
		self.currentSpace = ""

		WorldEditor.setNoSelectionFilter( "portal" )
		WorldEditor.setOptionInt( "render/chunk/vizMode", 0 );

	def onStart( self ):

		# create chunk viz
		self.chunkViz = View.TerrainChunkTextureToolView( "resources/maps/gizmo/square.dds" )
		self.chunkViz.numPerChunk = 1

		# create vertex viz
		self.vertexViz = View.TerrainChunkTextureToolView( "resources/maps/gizmo/vertices.dds" )
		self.vertexViz.numPerChunk = 25

		# create terrain mesh viz
		self.meshViz = View.TerrainChunkTextureToolView( "resources/maps/gizmo/quads.dds" )
		self.meshViz.numPerChunk = 25

		# create alpha tool
		self.alphaTool = WorldEditor.Tool()
		self.alphaTool.functor = Functor.TerrainTextureFunctor()
		self.alphaTool.locator = Locator.TerrainToolLocator()
		self.alphaToolTextureView = View.TerrainTextureToolView( "resources/maps/gizmo/alphatool.tga" )
		self.alphaTool.addView( self.alphaToolTextureView, "stdView" )
		self.alphaTool.size = 30
		self.alphaTool.strength = 1000

		# create height filter tool
		self.heightFunctor = Functor.TerrainHeightFilterFunctor()
		self.heightFunctor.index = 0 # this index must match the filters.xml file
		self.heightFunctor.strengthMod = 1
		self.heightFunctor.framerateMod = 1
		self.heightFunctor.constant = 1.0
		self.heightFunctor.falloff = 2

		self.setHeightFunctor = Functor.TerrainSetHeightFunctor()
		self.setHeightFunctor.height = 0
		self.setHeightFunctor.relative = 0
		
		self.heightView = View.TerrainTextureToolView( "resources/maps/gizmo/heighttool.tga" )
		self.setHeightView = View.TerrainTextureToolView( "resources/maps/gizmo/squaretool.dds" )

		self.heightTool = WorldEditor.Tool()
		self.heightTool.locator = Locator.TerrainToolLocator()
		self.heightTool.functor = Functor.TeeFunctor( self.heightFunctor, self.setHeightFunctor, KEY_LCONTROL )
		self.heightToolTextureView = View.TeeView( self.heightView, self.setHeightView, KEY_LCONTROL )
		self.heightTool.addView( self.heightToolTextureView, "stdView" )
		self.heightTool.size = 30

		# create general filter tool
		self.filterTool = WorldEditor.Tool()
		self.filterTool.locator = Locator.TerrainToolLocator()
		self.filterTool.functor = Functor.TerrainHeightFilterFunctor()
		self.filterTool.functor.strengthMod = 1
		self.filterTool.functor.constant = 0.0
		self.filterToolTextureView = View.TerrainTextureToolView( "resources/maps/gizmo/filtertool.tga" )
		self.filterTool.addView( self.filterToolTextureView, "stdView" )
		self.filterTool.size = 30

		# create a hole cutter
		self.holeTool = WorldEditor.Tool()		
		self.holeTool.locator = Locator.TerrainHoleToolLocator()
		view = View.TerrainTextureToolView( "resources/maps/gizmo/squaretool.dds" )
		view.showHoles = True
		self.holeTool.addView( view, "stdView" )
		self.holeTool.functor = Functor.TerrainHoleFunctor()
		self.holeTool.size = 30

		# create the item tool
		self.itemTool = WorldEditor.Tool()
		self.itemToolXZLocator = Locator.ItemToolLocator()
		self.itemTool.locator = Locator.TerrainToolLocator()
		self.itemToolTextureView = View.TerrainTextureToolView( "resources/maps/gizmo/cross.dds" )
		self.itemToolModelView = View.ModelToolView( "resources/models/pointer.model" )
		self.itemToolPlaneView = View.ModelToolView( "resources/models/freepointer.model" )
		self.itemTool.addView( self.itemToolTextureView, "stdView" )
		# This changes our locator to a ChunkItemLocator
		self.itemTool.functor = Functor.ScriptedFunctor( ChunkItemFunctor( self.itemTool, self.objInfo ) )
		# Setup the correct subLocator for the ChunkItemLocator
		self.itemTool.locator.subLocator = self.itemToolXZLocator
		self.itemTool.size = 1

		# Make the closed captions commentary viewer
		self.cc = GUI.ClosedCaptions( WorldEditor.getOptionInt( "consoles/numMessageLines", 5 ) )
		self.cc.addAsView()
		self.cc.visible = 1

		if ( WorldEditor.getOptionInt( "tools/showChunkVisualisation" ) == 1 ):
			WorldEditor.setOptionInt( "render/chunk/vizMode", 1)
		self.enterChunkVizMode()

		self.enterMode( self.modeName, 1 )

		# initialise the mouse move camera
		# load up the start position from space.localsettings
		startPos = (0,1.85,0)
		startDir = (0,0,0)

		dir = WorldEditor.getOptionString( "space/mru0" )
		dirDS = ResMgr.openSection( dir )
		ds = dirDS["space.localsettings"]
		if ds != None:
			startPos = ds.readVector3( "startPosition", startPos )
			startDir = ds.readVector3( "startDirection", startDir )

		m = WorldEditor.camera(0).view
		m.setIdentity()
		m.setRotateYPR( (startDir[2], startDir[1], startDir[0]) )
		m.translation = startPos
		m.invert()
		WorldEditor.camera(0).view = m

		# select the camera as specified in the options
		WorldEditor.changeToCamera( WorldEditor.getOptionInt( "camera/ortho" ) )

		# read the initial item snap mode
		self.updateItemSnaps();

	def onStop( self ):
		self.selEditor = None
		WorldEditor.setCurrentEditors()
		
		# Remove the closed captions commentary viewer
		self.cc.visible = 0
		self.cc.delAsView()
		del self.cc

		# Remove options entries that are messy and transient
		WorldEditor.saveOptions()

		return 0

	def onPause( self ):
		self.cc.visible = 0
		self.cc.delAsView()

		if ( WorldEditor.tool() != None ):
			WorldEditor.popTool()

		pass

	def onResume( self, exitCode ):
		self.cc.addAsView()
		self.cc.visible = 1
		WorldEditor.addCommentaryMsg( "entered edit mode." )

		self.enterMode( self.modeName, 1 )
		
	lightNames = ( "Standard", "Dynamic", "Specular" )
	lockNames = ( "free", "terrain", "obstacle" )
	
	def dragOnSelectMsg( self, state ):
		if state == 1:
			WorldEditor.addCommentaryMsg( "Enabling Drag On Select" )
		else:
			WorldEditor.addCommentaryMsg( "Disabling Drag On Select")
			
	def itemSnapModeMsg( self, state ):
		WorldEditor.addCommentaryMsg( "Entering %s snap mode" % self.lockNames[state] )
		
	def lightingModeMsg( self, state ):
		WorldEditor.addCommentaryMsg( "Lighting Mode: %s" % self.lightNames[state] )
		
	def objectSnapMsg( self, state ):
		if state == 1:
			WorldEditor.addCommentaryMsg( "Enabling Object Snap Grid" )
		else:
			WorldEditor.addCommentaryMsg( "Disabling Object Snap Grid" )
			
	def showBSPMsg( self, state ):
		if state == 1:
			WorldEditor.addCommentaryMsg( "Drawing custom BSPs" )
		else:
			WorldEditor.addCommentaryMsg( "Normal draw mode")

	def ownKeyEvent( self, key, modifiers ):
		
		if key == KEY_B:
			if (modifiers & MODIFIER_CTRL) == 0:
				curr = WorldEditor.getOptionInt( "drawBSP" )
				curr = ( curr + 1 ) % 2
				WorldEditor.setOptionInt( "drawBSP", curr )
				self.showBSPMsg( curr )
		
		elif key == KEY_M:
			curr = WorldEditor.getOptionInt( "dragOnSelect" )
			curr = ( curr + 1 ) % 2
			WorldEditor.setOptionInt( "dragOnSelect", curr )
			self.dragOnSelectMsg( curr )
	
		elif key == KEY_F8:
			curr = WorldEditor.getOptionString( "tools/coordFilter" )
			if curr == "World":
				curr = "Local"	
			elif curr == "Local":
				curr = "View"
			elif curr == "View":
				curr = "World"
			WorldEditor.setOptionString( "tools/coordFilter", curr )	
			WorldEditor.addCommentaryMsg( "Reference Coordinate System: %s" % curr )
		
		elif key == KEY_L:
			if modifiers & MODIFIER_CTRL:
				curr = WorldEditor.getOptionInt( "render/lighting" )
				curr = ( curr + 1 ) % 3
				WorldEditor.setOptionInt( "render/lighting", curr )
				self.lightingModeMsg( curr )
				
		elif key == KEY_G:
			curr = WorldEditor.getOptionInt( "snaps/xyzEnabled" )
			curr = ( curr + 1 )% 2
			WorldEditor.setOptionInt( "snaps/xyzEnabled", curr )
			self.objectSnapMsg( curr )

		elif key == KEY_1 and not modifiers:
			WorldEditor.setToolMode( "Objects" )
			
		elif key == KEY_2 and not modifiers:
			WorldEditor.setToolMode( "TerrainTexture" )
			
		elif key == KEY_3 and not modifiers:
			WorldEditor.setToolMode( "TerrainHeight" )
			
		elif key == KEY_4 and not modifiers:
			WorldEditor.setToolMode( "TerrainFilter" )
			
		elif key == KEY_5 and not modifiers:
			WorldEditor.setToolMode( "TerrainMesh" )
			
		elif key == KEY_6 and not modifiers:
			WorldEditor.setToolMode( "TerrainImpExp" )
			
		elif key == KEY_7 and not modifiers:
			WorldEditor.setToolMode( "Project" )
		
		t = WorldEditor.tool()

		sizeSection = ''
		strengthSection = ''
		minSizeSection = ''
		maxSizeSection = ''
		minStrengthSection = ''
		maxStrengthSection = ''
		if t == self.alphaTool:
			sizeSection = 'terrain/texture/size'
			minSizeSection = 'terrain/texture/minsizelimit'
			maxSizeSection = 'terrain/texture/maxsizelimit'
			strengthSection = 'terrain/texture/strength'
			minStrengthSection = 'terrain/texture/minstrengthlimit'
			maxStrengthSection = 'terrain/texture/maxstrengthlimit'
		elif t == self.heightTool:
			sizeSection = 'terrain/height/size'
			minSizeSection = 'terrain/height/minsizelimit'
			maxSizeSection = 'terrain/height/maxsizelimit'
			strengthSection = 'terrain/height/strength'
			minStrengthSection = 'terrain/height/minstrengthlimit'
			maxStrengthSection = 'terrain/height/maxstrengthlimit'
		elif t == self.filterTool:
			sizeSection = 'terrain/filter/size'
			minSizeSection = 'terrain/filter/minsizelimit'
			maxSizeSection = 'terrain/filter/maxsizelimit'
		elif t == self.holeTool:
			sizeSection = 'terrain/cutRepair/size'
			minSizeSection = 'terrain/cutRepair/minsizelimit'
			maxSizeSection = 'terrain/cutRepair/maxsizelimit'

		if sizeSection:
			size = WorldEditor.getOptionFloat( sizeSection )
			minSize = WorldEditor.getOptionFloat( minSizeSection )
			maxSize = WorldEditor.getOptionFloat( maxSizeSection )
			if key == KEY_RBRACKET:
				if not ( modifiers & MODIFIER_SHIFT ):
					size = size * 1.25 + 1
					if size > maxSize:
						size = maxSize
					t.size = size
					WorldEditor.setOptionFloat( sizeSection, size )
					WorldEditor.addCommentaryMsg( "Tool size %0.1f" % size )
			elif key == KEY_LBRACKET:
				if not ( modifiers & MODIFIER_SHIFT ):
					size = size * 0.8 - 1
					if size < minSize:
						size = minSize
					t.size = size
					WorldEditor.setOptionFloat( sizeSection, size )
					WorldEditor.addCommentaryMsg( "Tool size %0.1f" % size )
		if strengthSection:
			strength = WorldEditor.getOptionFloat( strengthSection )
			minStrength = WorldEditor.getOptionFloat( minStrengthSection )
			maxStrength = WorldEditor.getOptionFloat( maxStrengthSection )
			if key == KEY_RBRACKET and strength >= 0 or key == KEY_LBRACKET and strength < 0:
				if modifiers & MODIFIER_SHIFT:
					if strength >= 0:
						strength = strength * 1.25 + 1
					else:
						strength = strength * 1.25 - 1
					if strength > maxStrength:
						strength = maxStrength
					t.strength = strength
					WorldEditor.setOptionFloat( strengthSection, strength )
					WorldEditor.addCommentaryMsg( "Tool strength %0.1f" % strength )
			elif key == KEY_LBRACKET and strength >= 0 or key == KEY_RBRACKET and strength < 0:
				if modifiers & MODIFIER_SHIFT:
					if strength >= 0:
						strength = strength * 0.8 - 1
					else:
						strength = strength * 0.8 + 1
					if strength < minStrength:
						strength = minStrength
					t.strength = strength
					WorldEditor.setOptionFloat( strengthSection, strength )
					WorldEditor.addCommentaryMsg( "Tool strength %0.1f" % strength )

	def onRightMouse( self ):
		if WorldEditor.tool() != None:
			WorldEditor.tool().handleContextMenu()
			
	def onKeyEvent( self, isDown, key, modifiers ):
		if not WorldEditor.cursorOverGraphicsWnd():
			return 0

		if key == KEY_RIGHTMOUSE:
			if ( not self.rightMouseButtonDown ) and isDown:
				self.rightMouseButtonDown = 1
				self.mouseMoved = 0
			elif self.rightMouseButtonDown and not isDown:
				self.rightMouseButtonDown = 0
				if not self.mouseMoved:
					self.onRightMouse()
		handled = 0
		if self.avatarMode and key == KEY_Q:
			self.qDown = isDown
			self.eDown = 0
			handled = 1
		if self.avatarMode and key == KEY_E:
			self.eDown = isDown
			self.qDown = 0
			handled = 1
		if not handled:
			handled = WorldEditor.camera().handleKeyEvent( isDown, key, modifiers )
		if not handled and isDown:
			handled = self.ownKeyEvent( key, modifiers )
		if not handled and WorldEditor.tool() != None:
			handled = WorldEditor.tool().handleKeyEvent( isDown, key, modifiers )
		if not handled and isDown and key == KEY_LEFTMOUSE and self.objInfo.overGizmo:
			WorldEditor.gizmoClick()
			handled = 1
		return handled

	def handleWheelCameraSpeed( self, mz ):
		# look at the rotator for changing the camera speed
		c = WorldEditor.camera()
		currentValue = WorldEditor.getOptionString( "camera/speed" )
		speeds = ["Slow", "Medium", "Fast", "SuperFast"]

		iSpeed = 0
		if currentValue == speeds[1]:
			iSpeed = 1
		elif currentValue == speeds[2]:
			iSpeed = 2
		elif currentValue == speeds[3]:
			iSpeed = 3

		if mz > 0:
			iSpeed = iSpeed + 1
			if iSpeed > 3:
				iSpeed = 3
		elif mz < 0:
			iSpeed = iSpeed - 1
			if iSpeed < 0:
				iSpeed = 0

		value = speeds[iSpeed]

		handled = 0
		if value != currentValue:
			c = WorldEditor.camera()
			WorldEditor.setOptionString( "camera/speed", value )
			c.speed = WorldEditor.getOptionFloat( "camera/speed/" + value )
			c.turboSpeed = WorldEditor.getOptionFloat( "camera/speed/" + value + "/turbo" )
			handled = 1
			WorldEditor.addCommentaryMsg( "New camera speed %s" % value, 1 )

		return handled

	def onMouseEvent( self, mx, my, mz ):
		handled = 0

		if mx or my:
			self.mouseMoved = 1

		legacyMouse = WorldEditor.getOptionInt( "input/legacyMouseWheel" )
		itemsRotated = 0
		cameraSpeedChanged = False
		
		if legacyMouse != 0:
			# if using legacy mouse
			if WorldEditor.isKeyDown( KEY_MOUSE1 ):
				# Change camera speed with right click
				self.handleWheelCameraSpeed( mz )
				cameraSpeedChanged = True
			elif WorldEditor.tool():
				# handle the tool
				handled = WorldEditor.tool().handleMouseEvent( mx, my, mz )
				itemsRotated = self.itemTool.functor.script.selection.size
		else: 
			# if using new mouse
			if WorldEditor.tool() and mz == 0:
				# handle the tool
				handled = WorldEditor.tool().handleMouseEvent( mx, my, mz )
			elif WorldEditor.isKeyDown( KEY_SPACE ):
				# Change camera speed with space
				self.handleWheelCameraSpeed( mz )
				cameraSpeedChanged = True
			elif ( WorldEditor.isKeyDown( KEY_LSHIFT ) or WorldEditor.isKeyDown( KEY_RSHIFT ) ) and WorldEditor.tool():
				# handle the tool with shift
				handled = WorldEditor.tool().handleMouseEvent( mx, my, mz )
				itemsRotated = self.itemTool.functor.script.selection.size
			elif mz != 0 and \
					( WorldEditor.isKeyDown( KEY_LCONTROL ) or WorldEditor.isKeyDown( KEY_RCONTROL ) ) and \
					self.itemTool.functor.script.selection.size > 0:
				WorldEditor.rotateSnap( self.itemTool.functor.script.selection, mz, self.itemTool.functor.script.mouseRevealer )
				itemsRotated = self.itemTool.functor.script.selection.size

		if not handled:
			handled = WorldEditor.camera().handleMouseEvent( mx, my, mz )
		
		if not handled and ( mz != 0 ) and not itemsRotated and not cameraSpeedChanged:
			# zoom using scroll wheel
			handled = 1
			view = WorldEditor.camera().view
			view.invert()
			mult = mz / 1200.0

			if WorldEditor.isCapsLockOn():
				mult = mult * WorldEditor.camera().turboSpeed
			else:
				mult = mult * WorldEditor.camera().speed
			
			forward = view.applyToAxis( 2 )
			
			view.translation = (
				view.translation[0] + forward[0] * mult,
				view.translation[1] + forward[1] * mult,
				view.translation[2] + forward[2] * mult )
			
			view.invert()
			WorldEditor.camera().view = view

		return handled

	def updateOptions():
		pass

	def updateState( self, dTime ):
		"""This function forces an update to be called in World Editor. 
		Usually called everyframe, however it still recieves a dTime 
		value which informs the update function how much time has passed 
		since the last update call."""

		# detect a change of space, and act accordingly
		if self.currentSpace != WorldEditor.getOptionString( "space/mru0" ):
			self.currentSpace = WorldEditor.getOptionString( "space/mru0" )
			# make sure the chunk viz resolution is set correctly
			self.enterChunkVizMode()

		GUI.update( dTime )
		self.cc.update( dTime )
		WorldEditor.camera().update( dTime )
		if ( WorldEditor.tool() != None and not WorldEditor.tool().applying ):
			self.objInfo.overGizmo = WorldEditor.gizmoUpdate( WorldEditor.worldRay() )

		# update the WorldEditor
		WorldEditor.update( dTime )

		# update tool views
		base = dTime / 5
		self.alphaToolTextureView.rotation += base * (1 + (self.alphaTool.strength / 650))
		self.heightView.rotation += base * (1 + (self.heightTool.strength / 650))
		self.filterToolTextureView.rotation += base

		if self.nextTimeDoSelUpdate == 2:
			self.itemTool.functor.script.restoreOldSelection()
			self.nextTimeDoSelUpdate = 0
		elif self.nextTimeDoSelUpdate == 1:
			self.itemTool.functor.script.clearAndSaveSelection()
			self.nextTimeDoSelUpdate = 2

		if WorldEditor.isInPlayerPreviewMode() and not self.avatarMode:
			WorldEditor.addCommentaryMsg( "entered avatar walkthrough mode." )
			self.avatarMode = 1
		if self.avatarMode and not WorldEditor.isInPlayerPreviewMode():
			self.qDown = 0
			self.eDown = 0
			self.avatarMode = 0
		if self.avatarMode:
			value = WorldEditor.getOptionString( "camera/speed" )
			speed = 1
			if value == "Medium":
				speed = 2
			if value == "Fast":
				speed = 3
			if value == "SuperFast":
				speed = 4
			if self.qDown and WorldEditor.getOptionInt( "graphics/cameraHeight" ) - speed <= 2:
				WorldEditor.setOptionInt( "graphics/cameraHeight", 2 )
			if self.qDown and WorldEditor.getOptionInt( "graphics/cameraHeight" ) > 2:
				WorldEditor.setOptionInt( "graphics/cameraHeight", WorldEditor.getOptionInt( "graphics/cameraHeight" ) - speed )
			if self.eDown:
				WorldEditor.setOptionInt( "graphics/cameraHeight", WorldEditor.getOptionInt( "graphics/cameraHeight" ) + speed )

			WorldEditor.snapCameraToTerrain()

		if WorldEditor.tool() == bd.alphaTool:
			bd.alphaTool.size						= WorldEditor.getOptionFloat  ( "terrain/texture/size" )
			bd.alphaTool.strength					= WorldEditor.getOptionFloat  ( "terrain/texture/strength" )
			bd.alphaTool.functor.displayOverlay		= WorldEditor.getOptionInt    ( "terrain/texture/maskoverlay" )
		
			WorldEditor.setPaintLayer(bd.alphaTool.functor.lastPaintedLayer)
			WorldEditor.setPaintPos(bd.alphaTool.functor.lastPaintedPos)
			if bd.alphaTool.functor.hadEscapeKey != 0:
				WorldEditor.setTerrainPaintEscKey(bd.alphaTool.functor.hadEscapeKey)
				bd.alphaTool.functor.hadEscapeKey = 0

		bd.heightTool.size						= WorldEditor.getOptionFloat ( "terrain/height/size" )
		bd.heightTool.strength					= WorldEditor.getOptionFloat ( "terrain/height/strength" )
		bd.setHeightFunctor.height				= WorldEditor.getOptionFloat ( "terrain/height/height" )
		bd.setHeightFunctor.relative			= WorldEditor.getOptionInt   ( "terrain/height/relative" )
		bd.heightFunctor.falloff				= WorldEditor.getOptionInt   ( "terrain/height/brushFalloff" )
																		
		bd.filterTool.size						= WorldEditor.getOptionFloat ( "terrain/filter/size" )
		bd.filterTool.functor.index				= WorldEditor.getOptionInt   ( "terrain/filter/index" )
																		
		bd.holeTool.size						= WorldEditor.getOptionFloat ( "terrain/cutRepair/size" )
		bd.holeTool.functor.fillNotCut			= WorldEditor.getOptionInt   ( "terrain/cutRepair/brushMode" )
		
		if self.needsChunkVizUpdate:
			self.enterChunkVizMode()
		
		return 1

	def render( self, dTime ):
		"""This function forces World Editor to render everything on the scene. 
		Usually called everyframe, however it still recieves a dTime value which 
		informs the renderer how much time has passed since the last render	call."""
		WorldEditor.camera().render( dTime )
		WorldEditor.render( dTime )
		GUI.draw()
		return 1

	#--------------------------------------------------------------------------
	#	Section - Utilities methods
	#--------------------------------------------------------------------------

	def enterChunkVizMode( self ):
		t = WorldEditor.tool()
		if ( t != None ):
			curr = WorldEditor.getOptionInt( "render/chunk/vizMode" )
			t.delView( "chunkViz" )
			
			vizRes = 0;
			if t.functor == self.alphaTool.functor:
				vizRes = WorldEditor.terrainBlendsRes()
			elif t.functor == self.itemTool.functor or t.functor == self.heightTool.functor or t.functor == self.filterTool.functor:
				vizRes = WorldEditor.terrainHeightMapRes()
			elif t.functor == self.holeTool.functor:
				vizRes = WorldEditor.terrainHoleMapRes()
			
			if vizRes == 0:
				self.needsChunkVizUpdate = True
				vizRes = 1  # set it to some sensible value
			else:
				self.needsChunkVizUpdate = False
			
			if curr == 1:
				t.addView( self.chunkViz, "chunkViz" )
			elif curr == 2:
				self.vertexViz.numPerChunk = vizRes
				t.addView( self.vertexViz, "chunkViz" )
			elif curr == 3:
				self.meshViz.numPerChunk = vizRes
				t.addView( self.meshViz, "chunkViz" )

	def toggleItemSnaps( self ):
		if self.itemSnapMode == 1:
			self.itemSnapMode = 0
		else:
			self.itemSnapMode = 1
		WorldEditor.setOptionInt( "snaps/itemSnapMode", self.itemSnapMode )
		self.updateItemSnaps()

	def updateItemSnaps( self ):
		#this method calculates itemSnapMode based on the
		#entries in the options.xml
		if ( WorldEditor.getOptionInt( "snaps/itemSnapMode" ) == 2 ):
			self.itemSnapMode = 2
		elif ( WorldEditor.getOptionInt( "snaps/itemSnapMode" ) == 1 ):
			self.itemSnapMode = 1
		else:
			self.itemSnapMode = 0
		self.enterItemSnapMode()

	#This method should be called when the item snap type is changed
	def enterItemSnapMode( self ):
		newLoc = None
		t = self.itemTool
		if self.itemSnapMode == 0:
			newLoc = self.itemToolXZLocator
			t.delView( "stdView" )
			t.addView( self.itemToolPlaneView, "stdView" )
			t.size = 1
			WorldEditor.addCommentaryMsg( "Entering free snap mode" )
		elif self.itemSnapMode == 1:
			newLoc = Locator.TerrainToolLocator()
			t.delView( "stdView" )
			t.addView( self.itemToolTextureView, "stdView" )
			t.size = 1
			WorldEditor.addCommentaryMsg( "Entering terrain snap mode" )
		elif self.itemSnapMode == 2:
			newLoc = Locator.ChunkObstacleToolLocator()
			t.delView( "stdView" )
			t.addView( self.itemToolModelView, "stdView" )
			t.size = 1
			WorldEditor.addCommentaryMsg( "Entering obstacle snap mode" )
		else:
			WorldEditor.addCommentaryMsg( "Unknown snap mode" )

		#finally, recreate the functor
		self.itemTool.locator.subLocator = newLoc
	
	# toggle terrain LODing
	def toggleLod( self, state ):
		if ( state == 1 ):
			setWatcher( "Render/Terrain/Terrain2/Do constant LOD", 0 )
		else:
			setWatcher( "Render/Terrain/Terrain2/Do constant LOD", 1 )

	# Called to change to a given mode, and update the tabs in borland in accordance with this
	def changeToMode( self, modeName ):
		self.currentTab = "tab" + modeName
		self.enterMode( modeName )

	# Called to get the current modeName
	def getMode( self ):
		return self.modeName

	#This method changes the top-level editor mode.
	def enterMode( self, modeName, forceUpdate = 0 ):
	
		#print "enterMode - current %s, new %s, terrainMode %s" % (self.modeName, modeName, self.terrainModeName )
	
		if (self.modeName == modeName) and (not forceUpdate):
			return
			
		t = WorldEditor.tool()
		oldTool = t
		
		if t != None and modeName != "Object":
			if self.itemTool.functor.script.selection.size:
				self.itemTool.functor.script.selection.rem( self.itemTool.functor.script.selection )
				self.itemTool.functor.script.selUpdate()

			WorldEditor.popTool()

		# Remove the project or height module if we're coming out of project or height mode
		if self.modeName in ("Project", "Height"):
			self.modeStack.pop()
			self.modeName = self.modeStack[ len( self.modeStack ) - 1 ]
			WorldEditor.pop()

		if ( modeName == "TerrainTexture" ):
			WorldEditor.pushTool( self.alphaTool )
			self.terrainModeName = modeName

		elif ( modeName == "TerrainHeight" ):
			WorldEditor.pushTool( self.heightTool )
			self.terrainModeName = modeName

		elif ( modeName == "TerrainFilter" ):
			WorldEditor.pushTool( self.filterTool )
			self.terrainModeName = modeName

		elif ( modeName == "TerrainHoleCut" ):
			WorldEditor.pushTool( self.holeTool )
			self.terrainModeName = modeName

		elif ( modeName == "Terrain" ):
			self.modeName = modeName
			self.enterMode( self.terrainModeName )
			self.modeStack.append( modeName )
			return

		elif ( modeName == "Object" ):
			WorldEditor.pushTool( self.itemTool )
			self.modeStack.append( modeName )

		elif ( modeName == "Project" ):
			WorldEditor.push( "ProjectModule" )
			self.modeStack.append( modeName )
			
		elif ( modeName == "Height" ):
			WorldEditor.push( "HeightModule" )
			self.modeStack.append( modeName )

		else:
			WorldEditor.addCommentaryMsg( "%s mode not yet implemented" % modeName, 1 )

		self.enterChunkVizMode()

		WorldEditor.addCommentaryMsg( "entered " + modeName + " mode" )
		self.modeName = modeName
		
		newTool = WorldEditor.tool()
		if oldTool != None and oldTool != newTool:
			oldTool.endUsing()
		if newTool != None:
			newTool.beginUsing()

	def hasValidTexture( self ):
		# return self.alphaTool.functor.texture != WorldEditor.getOptionString( "resourceGlue/system/notFoundBmp" )
		return True


	#This method should be called to change the terrain texture.
	def setTerrainAlphaTexture( self, textureName ):
		if textureName:
			self.alphaTool.functor.texture = textureName
		
	def setTerrainTextureUProjection( self, value ):
		self.alphaTool.functor.uProjection = Math.Vector4(value[0], value[1], value[2], value[3])
		
	def setTerrainTextureVProjection( self, value ):
		self.alphaTool.functor.vProjection = Math.Vector4(value[0], value[1], value[2], value[3])
		
	def setImportMaskTopLeft( self, value ):
		self.alphaTool.functor.importMaskTL = Math.Vector2(value[0], value[1])
		
	def setImportMaskBottomRight( self, value ):
		self.alphaTool.functor.importMaskBR = Math.Vector2(value[0], value[1])	
		
	def setTerrainPaintMode( self, value ):
		self.alphaTool.functor.mode = value
		
	def setTerrainPaintBrush( self, value ):
		self.alphaTool.functor.paintBrush = value

	def setSelection( self, group, update ):
		cif = self.itemTool.functor.script
		set_assign( cif.selection, group )
		if (update == 1):
			cif.selUpdate()
			
	def getSelection( self, group ):
		cif = self.itemTool.functor.script
		set_assign( group, cif.selection )

	# called to reset the editors (something special has changed)
	def resetSelUpdate( self, keepSelection = 0 ):
		if keepSelection == 0:
			self.nextTimeDoSelUpdate = 1
		else:			
			self.itemTool.functor.script.selUpdate()



def deepCopy( destSect, srcSect ):
	destSect.asString = srcSect.asString
	for x in srcSect.values():
		nd = destSect.createSection( x.name )
		deepCopy( nd, x )
	return destSect

def deepCopyTemplate( destSect, srcSect ):
	destSect.asString = srcSect.asString
	gotAmbient = False;
	for x in srcSect.values():
		if x.name == "ambientLight":
			if gotAmbient:
				continue
			gotAmbient = True
		nd = destSect.createSection( x.name )
		deepCopy( nd, x )
	return destSect


#
# Set functions, for manipulating sets of ChunkItemRevealers
# (ie, what we can select in the editor)
#

# return a copy of a
def set_copy( a ):
	s = WorldEditor.ChunkItemGroup()
	s.add( a )
	return s

# make a equal to b
def set_assign( a, b ):
	a.rem( a )
	a.add( b )

# remove all elements from a
def set_clear( a ):
	a.rem( a )

# add all the elements in b that aren't in a to a
def set_union( a, b ):
	a.rem( b )
	a.add( b )

# return a new set containing all the elements that are in both a and b
def set_intersection_new( a, b ):
	aonly = set_copy( a )
	aonly.rem( b )

	int = set_copy( a )
	int.rem( aonly )

	return int

# return a new set containing all the elements in a that aren't in b
def set_difference_new( a, b ):
	diff = set_copy( a )
	diff.rem( b )
	return diff

# return true is b is a subset of a
# ie, all of b's elements also exist in a
def set_issubset( a, b ):
	# TODO: JWD 23/9/2003: Shouldn't this be '== b.size'?
	return set_intersection_new( a, b ).size



# This is a helper class for passing state between the module and
#  the object manipulation functor
class ObjInfo:
	def __init__( self ):
		self.overGizmo = 0
		self.shellMode = 0
		self.tabName = ""

	def showBrowse( self ):
		if not self.getBrowsePath() in ["", "unknown file"]:
			WorldEditor.addCommentaryMsg( self.getBrowsePath() )

	def getBrowsePath( self ):
		#get model file out of the options.xml
		return WorldEditor.getOptionString( "itemEditor/browsePath" )

	def setBrowsePath( self, name ):
		print "setBrowsePath", name
		WorldEditor.setOptionString( "itemEditor/browsePath", name )

	def setShellMode( self, state ):
		if self.shellMode == state:
			return
		self.shellMode = state
		
	def setObjectTab( self, tabName ):
		if self.tabName != "":
			# store the current selection
			currentFilter = WorldEditor.getOptionString( "tools/selectFilter" )
			prevSectionName = "object/" + self.tabName + "/selectFilter"
			WorldEditor.setOptionString( prevSectionName, currentFilter )
		self.tabName = tabName
		currSectionName = "object/" + self.tabName + "/selectFilter"
		return WorldEditor.getOptionString( currSectionName )


class ChunkItemFunctor:
	def __init__( self, tool, oi ):
		# set up the tool we are part of
		self.movementLocator = tool.locator
		self.mouseLocator = Locator.ChunkItemLocator(
			tool.locator )
		self.mouseRevealer = self.mouseLocator.revealer
		self.selection = WorldEditor.ChunkItemGroup()
		
		# leftMouseDown is used to detect if the mouse was pressed inside the 3D
		# view, used when doing a marquee drag select
		
		self.leftMouseDown = 0
		
		self.dragging = 0
		
		self.clickX = 0
		self.clickY = 0
		
		# This const is used when dragOnSelect and when selecting with marquee.
		# This value is relatively big because onMouseEvent's dx and dy parameters
		# are in a sub-pixels scale.
		self.dragStartDelta = 15

		self.mouseView = View.ChunkItemBounds( self.mouseRevealer, hoverColour )
		self.selView = View.ChunkItemBounds( self.selection,
			selectColour, selectGrowFactor, selectTexture )

		tool.locator = self.mouseLocator

		tool.addView( self.selView )
		tool.addView( self.mouseView )

		# store a reference to the object info class
		self.objInfo = oi

		self.oldSelection = WorldEditor.ChunkItemGroup()
		self.currentSpace_ = ""

	def addChunkItem( self ):
		import UIAdapter
		bp = self.objInfo.getBrowsePath()
		UIAdapter.brwObjectUalItemSelect( bp, 0 )
		if len(bp)>7 and bp[-7:] == ".prefab":
			self.addChunkPrefab()
		elif self.objInfo.shellMode:
			self.addChunkShell()
		else:
			self.addChunkModel()
			
	def chunkItemAdded( self, name ):
		WorldEditor.addItemToHistory( self.objInfo.getBrowsePath(), "FILE" );
		WorldEditor.addCommentaryMsg( "Added " + name )

	def addChunkPrefab( self ):
		group = WorldEditor.loadChunkPrefab( self.objInfo.getBrowsePath(), self.mouseLocator.subLocator )
		if ( group != None ):
			self.chunkItemAdded( self.objInfo.getBrowsePath() );

	def addChunkModel( self ):
		#If we are adding a model file, then simply
		#add the resource name to the chunk at the locator.
		bp = self.objInfo.getBrowsePath()
		if len(bp)>6 and bp[-6:] == ".model":
			d = ResMgr.DataSection( "model" )
			d.writeString( "resource", bp )
			group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator, 2 )
			if ( group != None ):
				self.chunkItemAdded( d.name );
		if len(bp)>4 and bp[-4:] == ".spt":
			d = ResMgr.DataSection( "speedtree" )
			d.writeString( "spt", bp )
			d.writeInt( "seed", 1 )
			group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator, 2 )
			if ( group != None ):
				self.chunkItemAdded( d.name );


		# if it's a .xml file in the particles directory, add a ChunkParticles chunk item
		if bp.find("particles/") != -1 and (len(bp)>4 and bp[-4:] == ".xml"):
			d = ResMgr.DataSection( "particles" )
			d.writeString( "resource", bp )
			group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator, 2 )
			if ( group != None ):
				self.chunkItemAdded( d.name );

		#XML files represent completely generic additions to the chunk,
		#and so all the information must be deep-copied and added
		#to the chunk.
		elif len(bp)>4 and bp[-4:] == ".xml":
			s = ResMgr.openSection( bp ).values()[0]
			if ( s != None ):
				d = ResMgr.DataSection( s.name )
				deepCopy( d, s )
				group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator )
				if ( group != None ):
					self.chunkItemAdded( d.name );

		# If it's a .py file, add an entity with the same name
		elif len(bp)>3 and bp[-3:] == ".py":
			d = ResMgr.DataSection( "entity" )
			d.writeString( "type", bp[ bp.rfind("/")+1 : -3 ] )
			group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator )
			if ( group != None ):
				self.chunkItemAdded( d.name );

		# If it's a .def file, add an entity with the same name
		elif len(bp)>4 and bp[-4:] == ".def":
			if bp.find("user_data_object") == -1:
				d = ResMgr.DataSection( "entity" )
				d.writeString( "type", bp[ bp.rfind("/")+1 : -4 ] )
			else:
				d = ResMgr.DataSection( "UserDataObject" )
				d.writeString( "type", bp[ bp.rfind("/")+1 : -4 ] )
			group = WorldEditor.createChunkItem( d, self.mouseLocator.subLocator )
			if ( group != None ):
				self.chunkItemAdded( d.name );


	def addChunkShell( self ):

		try:
			(pChunkSection,chunkName) = WorldEditor.createInsideChunkDataSection()

			if ( pChunkSection == None ):
				WorldEditor.addCommentaryMsg( "Could not create data section", 1 )
			else:
				# fill in the model information
				pChunkSection.writeString( "shell/resource", \
					self.objInfo.getBrowsePath() )

				# open the model data section
				pModelSection = ResMgr.openSection( self.objInfo.getBrowsePath() )
				if ( not pModelSection ):
					WorldEditor.addCommentaryMsg( \
							"Could not open model file. Shell not added", 1 )
				else:
					# check to see whether the model is nodefull, if so, warn the user
					testSection = pModelSection["nodefullVisual"]
					if testSection != None:
						WorldEditor.addCommentaryMsg( \
							"warning: model is nodefull, shell not statically lit", 1 )

					# create initial data
					WorldEditor.chunkFromModel( pChunkSection, pModelSection )

					# copy from the template
					pTemplateSection = ResMgr.openSection( self.objInfo.getBrowsePath() + ".template" )
					if pTemplateSection:
						deepCopyTemplate( pChunkSection, pTemplateSection )

					group = WorldEditor.createChunk( pChunkSection, \
						chunkName, \
						self.mouseLocator.subLocator )

					if ( group != None ):
						self.chunkItemAdded( self.objInfo.getBrowsePath() );

		except Exception, e:
			WorldEditor.addCommentaryMsg( e.args[0], 1 )

	# key event entry point
	def onKeyEvent( self, (isDown, key, modifiers), tool ):
		if not WorldEditor.cursorOverGraphicsWnd():
			return 0

		handled = 0
		if not isDown and key == KEY_LEFTMOUSE:
			self.dragging = 0

		if isDown:
			if key == KEY_LEFTMOUSE:
				self.leftMouseDown = 1
				self.onLeftMouse()
				handled = 1
			if key == KEY_MIDDLEMOUSE:
				self.onMiddleMouse()
				handled = 1
			elif key == KEY_RETURN:
				self.addChunkItem()
			elif key == KEY_DELETE:
				if self.selection.size:
					WorldEditor.deleteChunkItems( self.selection )
			elif key == KEY_ESCAPE:
				# clear the selection
				set_clear( self.selection )
				self.selUpdate()
			#elif key == KEY_R and self.objInfo.shellMode and self.selection.size:
			elif key == KEY_R and self.selection.size:
				WorldEditor.recreateChunks( self.selection )
				WorldEditor.addCommentaryMsg( "Recreated chunks" )

				# clear the selection
				set_clear( self.selection )
				self.selUpdate()
#			elif key == KEY_T:
#				WorldEditor.recalcTerrainShadows()
#				WorldEditor.addCommentaryMsg( "Recalced shadows" )
			elif key == KEY_C:
				if self.selection.size > 0:
					WorldEditor.cloneChunkItems( self.selection, bd.itemTool.locator.subLocator )
					WorldEditor.addCommentaryMsg( "Cloned Selection" )
		else:
			if key == KEY_LEFTMOUSE:
				self.leftMouseDown = 0

		return handled


	# update entry point
	def update( self, dTime, tool ):
		if self.currentSpace_ != WorldEditor.getOptionString( "space/mru0" ):
			self.currentSpace_ = WorldEditor.getOptionString( "space/mru0" )
			set_clear( self.selection )
			self.selUpdate()
		if self.objInfo.overGizmo:
			self.mouseView.revealer = None
		else:
			self.mouseView.revealer = self.mouseRevealer

		# TODO: Check if mouse button down and threshold crossed.
		# If so start a drag with the current object
		pass

	def startDragSelect( self ):
		# add a drag select tool, which will pop itself and set our
		# selection when done.
		nt = WorldEditor.Tool()
		nt.locator = bd.itemTool.locator.subLocator
		nt.functor = Functor.ScriptedFunctor( DragSelectFunctor(nt, self) )
		WorldEditor.pushTool( nt )
		
	def dragDeltaExceeded( self ):
		return abs( self.clickX ) > self.dragStartDelta or abs( self.clickY ) > self.dragStartDelta

	def onMouseEvent( self, (dx,dy,dz), tool ):
		if dz != 0 \
			and ( WorldEditor.isKeyDown( KEY_LSHIFT ) \
				  or WorldEditor.isKeyDown( KEY_RSHIFT ) \
				  or WorldEditor.getOptionInt( "input/legacyMouseWheel" ) != 0 ) \
			and self.selection.size:
			rotateTool = WorldEditor.Tool()
			rotateTool.functor = Functor.WheelRotator()
			rotateTool.locator = Locator.OriginLocator()

			rotateTool.handleMouseEvent( dx, dy, dz )

			# Add the mousewheel rotate tool, it'll automatically pop itself
			WorldEditor.pushTool( rotateTool )

		if not WorldEditor.isKeyDown( KEY_MOUSE0 ):
			# just to make sure that leftMouseDown has a consistent value
			self.leftMouseDown = 0
		
		if self.dragging:
			self.clickX += dx
			self.clickY += dy
			if self.dragDeltaExceeded():
				self.dragging = 0
				try:
					nt = WorldEditor.Tool()
					nt.locator = bd.itemTool.locator.subLocator
					nt.functor = Functor.MatrixMover()

					#if undoName != None:
					#	nt.functor.setUndoName( undoName )

					# and then push it onto the stack. it'll pop itself off when done
					WorldEditor.pushTool( nt )
				except ValueError:
					# probably tried to drag a terrain block or something
					# else that can't be moved around.
					pass
		
		elif self.leftMouseDown:
			self.clickX += dx
			self.clickY += dy
			if self.dragDeltaExceeded():
				# not moving through DragOnSelect, so must be a marquee selection.
				self.startDragSelect()
				
		return 0

	def onContextMenu( self, tool ):
		"""This function opens an item's context menu. It is usually called
		when the user right clicking on an item in the world with the mouse."""
		if self.mouseRevealer.size == 1:
			WorldEditor.rightClick( self.mouseRevealer )
		return 1
			
	def onLeftMouse( self ):
		self.clickX = 0
		self.clickY = 0
		
		# first see if there's a gizmo in the house
		if self.objInfo.overGizmo:
			# if so, let it take care of things
			WorldEditor.gizmoClick()
			return

		if not self.mouseRevealer.size:
			# nothing to click on, start a marquee selection. This will take
			# care of clearing the selection if it's only a click.
			self.startDragSelect()
			return

		# Check if control is held down, it indicates that we want to toggle
		# what's pointed to in/out of the selection
		if (WorldEditor.isKeyDown( KEY_LCONTROL ) or WorldEditor.isKeyDown( KEY_RCONTROL )):
			# if we're pointing at a subset of the selection
			if set_issubset( self.selection, self.mouseRevealer):
				# remove the subset
				self.selection.rem( self.mouseRevealer )
				self.selUpdate()
				return
			else:
				# add the mouseRevealer to the selection
				set_union( self.selection, self.mouseRevealer )
				self.selUpdate()
				return
		else:
			# if the selection is totally different to what's under the mouse
			# specifically, if we're only pointing at a subset of the
			# selection, we don't want to set the selection to that, we want
			# to drag it instead (which happens below )
			#if not set_intersection_new( self.selection, self.mouseRevealer).size:
			if not set_issubset( self.selection, self.mouseRevealer ):
				# set the selection to what's under the mouse
				set_assign( self.selection, self.mouseRevealer )
				self.selUpdate()



		# nothing under the mouse, bail
		if not self.selection.size:
			return

		if not WorldEditor.getOptionInt( "dragOnSelect" ):
			if not WorldEditor.isKeyDown( KEY_V ):
				return

		# ok, it's drag time
		self.dragging = 1

	# The middle mouse button is used for autosnap
	def onMiddleMouse( self ):
		# ensure that we both have a selection and something under the mouse
		if not self.selection.size or not self.mouseRevealer.size:
			return

		# ensure that we're in shell mode
		if not WorldEditor.isChunkSelected():
			return

		# If v is held down, clone and snap the shell under the cursor
		if WorldEditor.isKeyDown( KEY_V ):
			group = WorldEditor.cloneAndAutoSnap( self.mouseRevealer, self.selection )
			if ( group != None ):
				set_assign( self.selection, group )
				self.selUpdate()
			else:
				WorldEditor.addCommentaryMsg( "No matching portals", 2 )

			return

		# if the selection is different to what's under the mouse,
		if set_difference_new( self.selection, self.mouseRevealer ).size:
			# auto snap the shells together
			if not WorldEditor.autoSnap( self.selection, self.mouseRevealer ):
				WorldEditor.addCommentaryMsg( "No matching portals" )

	def selUpdate( self ):
		try:
			# tell big bang what the current selection is
			WorldEditor.revealSelection( self.selection )

			if self.selection.size:

				self.selEditor = WorldEditor.ChunkItemEditor( self.selection )

				WorldEditor.setCurrentEditors( self.selEditor )
				#if hasattr(self.selEditor, "description"):
				#	print "Selected a", str(self.selEditor.description)
				#else:
				#	print "Selected a group"

				# inform the user of stats about the selection
				#if ( self.objInfo.shellMode == 1 and self.selection.size > 1):
				#	WorldEditor.showChunkReport( self.selection )
			else:
				self.selEditor = None
				WorldEditor.setCurrentEditors()
		except EnvironmentError, e:
			WorldEditor.addCommentaryMsg( e.args[0], 1 )			

	def clearAndSaveSelection( self ):
		set_assign( self.oldSelection, self.selection )
		set_clear( self.selection )
		self.selUpdate()
#		WorldEditor.addCommentaryMsg( "clearAndSaveSelection" )

	def restoreOldSelection( self ):
		set_assign( self.selection, self.oldSelection )
		self.selUpdate()
#		WorldEditor.addCommentaryMsg( "restoreOldSelection" )



class DragSelectFunctor:
	def __init__( self, tool, cif ):
		# set up the tool we are part of
		self.movementLocator = tool.locator
		self.mouseLocator = Locator.ChunkItemFrustumLocator(
			tool.locator )
		self.mouseRevealer = self.mouseLocator.revealer

		tool.locator = self.mouseLocator
		tool.addView( View.DragBoxView( self.mouseLocator, dragBoxColour ) )
		tool.addView( View.ChunkItemBounds( self.mouseRevealer, hoverColour ) )
		tool.addView( View.ChunkItemBounds( cif.selection,
			selectColour, selectGrowFactor, selectTexture ) )

		self.chunkItemFunctor = cif

	# update entry point
	def update( self, dTime, tool ):
		# must use update to check for this, key events aren't reliable
		if not WorldEditor.isKeyDown( KEY_LEFTMOUSE ):
			if (WorldEditor.isKeyDown( KEY_LCONTROL ) or WorldEditor.isKeyDown( KEY_RCONTROL )):
				# add the set
				set_union( self.chunkItemFunctor.selection, self.mouseRevealer )
			elif WorldEditor.isKeyDown( KEY_LALT ) or WorldEditor.isKeyDown( KEY_RALT ):
				# remove the set
				self.chunkItemFunctor.selection.rem( self.mouseRevealer )
			else:
				# set the selection to our mouse revaler
				set_assign( self.chunkItemFunctor.selection, self.mouseRevealer )

			if not WorldEditor.isKeyDown( KEY_LALT ) and not WorldEditor.isKeyDown( KEY_RALT ):
				# not removing, so also add whatever is under the cursor
				set_union( self.chunkItemFunctor.selection, self.chunkItemFunctor.mouseRevealer )

			self.chunkItemFunctor.selUpdate()

			WorldEditor.popTool( )

	def onMouseEvent( self, (dx,dy,dz), tool ):
		return 0

