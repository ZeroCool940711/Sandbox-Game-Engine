import BigBang
import keys
import GUI
import Locator
import View
import Functor
import ResMgr
from keys import *

cm = None

class ChunkManager:
	"""This module creates and manages spaces, and all the chunk
	files therein."""

	def __init__( self ):
		global cm
		cm = self

		self.newUniverseName = "cz"
		self.newSpaceName = "demo"
		self.numChunks = 50
		self.minX = -25
		self.maxX = 25
		self.minZ = -25
		self.maxZ = 25

		self.bo = BigBang.opts
		self.oldFarPlane = 500
		self.oldFog = 1
		self.oldCameraSpeed = 200
		self.oldCameraSpeedTurbo = 400
		pass

	def onStart( self ):
		self.cc = GUI.ClosedCaptions( BigBang.opts._consoles._numMessageLines.asInt )
		self.cc.addAsView()
		self.cc.visible = 1

		self.chunkTool = BigBang.Tool()
		self.chunkLocator = Locator.OriginLocator()
		self.chunkViz = View.TerrainChunkTextureToolView( "resources/maps/gizmo/squareTool.dds" )
		self.chunkViz.numPerChunk = 1

		self.chunkTool.locator = self.chunkLocator
		self.chunkTool.addView( self.chunkViz, "chunkViz" )
		self.chunkTool.functor = None
		self.chunkTool.size = 5000

		#2000 m. far plane
		self.oldFarPlane = BigBang.farPlane()
		BigBang.farPlane( 2000.0 )

		#no fog
		self.oldFog = BigBang.opts._render._misc._drawFog.asFloat
		BigBang.opts._render._misc._drawFog.asFloat = self.oldFog

		#fast camera
		c = BigBang.camera()
		self.oldCameraSpeed = c.speed
		self.oldCameraSpeedTurbo = c.turboSpeed
		c.speed = 500
		c.turboSpeed = 1000

		self.onResume( 0 )

	def onStop( self ):
		#restore settings
		BigBang.farPlane( self.oldFarPlane )
		BigBang.opts._render._misc._drawFog.asFloat = self.oldFog
		c = BigBang.camera()
		c.speed = self.oldCameraSpeed
		c.turboSpeed = self.oldCameraSpeedTurbo

		self.onPause()
		del self.cc

		return 0

	def onPause( self ):
		self.cc.visible = 0
		self.cc.delAsView()
		if BigBang.tool() == self.chunkTool:
			BigBang.pushTool( self.chunkTool )

	def onResume( self, exitCode ):
		self.cc.addAsView()
		self.cc.visible = 1
		BigBang.pushTool( self.chunkTool )
		BigBang.addCommentaryMsg( "entered chunk management mode." )

	def ownKeyEvent( self, key, modifiers ):
		handled = 0
		if key == KEY_LBRACKET:
			if modifiers & MODIFIER_SHIFT:
				self.chunkTool.size -= 1000
			else:
				self.chunkTool.size -= 100
			if self.chunkTool.size < 100:
				self.chunkTool.size = 100
			self.numChunks = self.chunkTool.size / 100
			BigBang.addCommentaryMsg( \
				"will create %0.0f x %0.0f chunks" % (self.numChunks, self.numChunks) )
		elif key == KEY_RBRACKET:
			if modifiers & MODIFIER_SHIFT:
				self.chunkTool.size += 1000
			else:
				self.chunkTool.size += 100
			self.numChunks = self.chunkTool.size / 100
			BigBang.addCommentaryMsg( \
				"will create %0.0f x %0.0f chunks" % (self.numChunks, self.numChunks) )
		elif key == KEY_RETURN:
			self.numChunks = self.chunkTool.size / 100
			self.create()

		return handled

	def onKeyEvent( self, isDown, key, modifiers ):
		if not BigBang.cursorOverGraphicsWnd():
			return 0
		
		handled = BigBang.camera().handleKeyEvent( isDown, key, modifiers )

		if not handled and isDown:
			handled = self.ownKeyEvent( key, modifiers )
		if not handled and BigBang.tool() != None:
			handled = BigBang.tool().handleKeyEvent( isDown, key, modifiers )
		return handled

	def onMouseEvent( self, mx, my, mz ):
		return BigBang.camera().handleMouseEvent( mx, my, mz )

	def updateState( self, dTime ):		
		GUI.update( dTime )
		self.cc.update( dTime )
		BigBang.update( dTime )
		BigBang.camera().update( dTime )
		BigBang.fudgeOrthographicMode( 1500.0, 0.01 )
		return 1

	def render( self, dTime ):
		BigBang.camera().render( dTime )
		BigBang.render( dTime )
		GUI.draw()
		return 1

	def create( self ):
		if self.newUniverseName == "none":
			BigBang.addCommentaryMsg( "New Universe Name not set", 2 )
			return
		if self.newSpaceName == "none":
			BigBang.addCommentaryMsg( "New Space Name not set", 2 )
			return

		self.createUniverse( self.newUniverseName )
		self.createSpace( self.newUniverseName, self.newSpaceName )
		fullPathName = "universes/" + self.newUniverseName + "/" + self.newSpaceName + "/"
		self.createChunks( fullPathName, self.numChunks, self.numChunks )
		pass


	def createNewUniverseSettings( self, universePath ):
		settingsFile = ResMgr.root.createSection( universePath + "universe.settings" )
		if settingsFile != None:
			settingsFile.save()
			BigBang.addCommentaryMsg( "created universe.settings" )
			return settingsFile
		else:
			BigBang.addCommentaryMsg( "error creating universe.settings", 2 )
		return None

	def createNewSpaceSettings( self, spacePath ):
		settingsFile = ResMgr.root.createSection( spacePath + "space.settings" )
		if settingsFile != None:
			settingsFile.writeString( "timeOfDay", "environments/sky.xml" )
			settingsFile.writeString( "skyGradientDome", "environments/sky.xml" )
			settingsFile.save();
			BigBang.addCommentaryMsg( "created space.settings" )
			return settingsFile
		else:
			BigBang.addCommentaryMsg( "error creating space.settings", 2 )
		return None

	def createUniverse( self, universeName ):
		settingsFile = None
		pathName = "universes/" + universeName + "/"

		settingsFile = ResMgr.openSection( pathName + "universe.settings" )
		if not settingsFile:
			settingsFile = self.createNewUniverseSettings( pathName )
			if settingsFile != None:
				BigBang.addCommentaryMsg( "created new universe" )

	def createSpace( self, universeName, spaceName ):
		settingsFile = None
		pathName = "universes/" + universeName + "/"

		settingsFile = ResMgr.openSection( pathName + "universe.settings" )
		if settingsFile == None:
			settingsFile = self.createNewUniverseSettings( pathName )

		if settingsFile != None:
			#create a space entry in the universes file
			spaceSect = settingsFile.has_key( spaceName )
			if spaceSect == 0:
				settingsFile.writeString( spaceName, "" )
				settingsFile.save()

			#create the space folder
			pathName += spaceName + "/"
			settingsFile = ResMgr.openSection( pathName + "space.settings" )
			if settingsFile == None:
				settingsFile = self.createNewSpaceSettings( pathName )
				if settingsFile != None:
					BigBang.addCommentaryMsg( "created new space" )

	def chunkName( self, x, y ):
		return "%04x%04xo" % (x&0xffff,y&0xffff)

	def writePortal( self, sect, idx, x, z ):
		ps = sect.createSection( "portal" )
		
		if idx == 0:
			ps.writeString( "chunk", self.chunkName(x-1,z) )
			ps.writeVector3( "uAxis", (0.0,1.0,0.0) )
			ps.writeVector3s( "point",
				((-200.000000,0.000000,0.000000),
				 (200.000000,0.000000,0.000000),
				 (200.000000,100.000000,0.000000),
				 (-200.000000,100.000000,0.000000)))
		elif idx == 1:
			ps.writeString( "chunk", self.chunkName(x+1,z) )
			ps.writeVector3( "uAxis", (0.0,0.0,1.0) )
			ps.writeVector3s( "point",
				((0.000000,-200.000000,0.000000),
				 (100.000000,-200.000000,0.000000),
				 (100.000000,200.000000,0.000000),
				 (0.000000,200.000000,0.000000)))
		elif idx == 2:
			ps.writeString( "chunk", "earth" )
			ps.writeVector3( "uAxis", (0.0,0.0,1.0) )
			ps.writeVector3s( "point",
				((0.0,0.0,0.0),
				(100.0,0.0,0.0),
				(100.0,100.0,0.0),
				(0.0,100.0,0.0)))
		elif idx == 3:
			ps.writeString( "chunk", "heaven" )
			ps.writeVector3( "uAxis", (1.0,0.0,0.0) )
			ps.writeVector3s( "point",
				((0.0,0.0,0.0),
				(100.0,0.0,0.0),
				(100.0,100.0,0.0),
				(0.0,100.0,0.0)))
		elif idx == 4:
			ps.writeString( "chunk", self.chunkName(x,z-1) )
			ps.writeVector3( "uAxis", (1.0,0.0,0.0))
			ps.writeVector3s( "point",
				((0.000000,-200.000000,0.000000),
				 (100.000000,-200.000000,0.000000),
				 (100.000000,200.000000,0.000000),
				 (0.000000,200.000000,0.000000)))
		elif idx == 5:
			ps.writeString( "chunk", self.chunkName(x,z+1) )
			ps.writeVector3( "uAxis", (0.0,1.0,0.0) )
			ps.writeVector3s( "point",
				((-200.000000,0.000000,0.000000),
				 (200.000000,0.000000,0.000000),
				 (200.000000,100.000000,0.000000),
				 (-200.000000,100.000000,0.000000)))


	def writeBoundary( self, sect, x, y, idx ):
		bs = sect.items()[idx][0]

		if idx == 0:
			#right
			bs.writeVector3( "normal", (1.0,0.0,0.0) )
			bs.writeFloat( "d", 0.0 )
			if x != self.minX:
				self.writePortal( bs,idx,x,y )
		elif idx == 1:
			#left
			bs.writeVector3( "normal", ( -1.0, 0.0, 0.0 ) )
			bs.writeFloat( "d", -100.0 )
			if x != self.maxX:
				self.writePortal( bs,idx,x,y )
		elif idx == 2:
			#earth
			bs.writeVector3( "normal", (0.0,1.0,0.0) )
			bs.writeFloat( "d", -200.0 )
			self.writePortal( bs,idx,x,y )

		elif idx == 3:
			#heaven
			bs.writeVector3( "normal", (0.0,-1.0,0.0) )
			bs.writeFloat( "d", -200.0 )
			self.writePortal( bs,idx,x,y )
			
		elif idx == 4:
			#rear
			bs.writeVector3( "normal", ( 0.0, 0.0, 1.0 ) )
			bs.writeFloat( "d", 0.0 )
			if y != self.minZ:
				self.writePortal( bs,idx,x,y )
		elif idx == 5:
			#fore
			bs.writeVector3( "normal", ( 0.0, 0.0, -1.0 ) )
			bs.writeFloat( "d", -100.0 )
			if y != self.maxZ:
				self.writePortal( bs,idx,x,y )

	def writeTerrain( self, sect, x, y ):
		sect.writeString( "terrain/resource", self.chunkName(x,y)[0:-1]+".terrain" )

	def writeBoundsAndTransform( self, sect, x, z ):
		x *= 100
		z *= 100
		sect.writeVector3( "transform/row0", (1.0,0.0,0.0) )
		sect.writeVector3( "transform/row1", (0.0,1.0,0.0) )
		sect.writeVector3( "transform/row2", (0.0,0.0,1.0) )
		sect.writeVector3( "transform/row3", (x,0.0,z) )
		sect.writeVector3( "boundingBox/min", (x,-200.0,z) )
		sect.writeVector3( "boundingBox/max", (x+100.0,200.0,z+100.0) )

	def createChunk( self, pathName, x, y ):
		theChunk = pathName + self.chunkName(x,y)
		#create chunk file
		fullChunkName = theChunk + ".chunk"
		sect = ResMgr.openSection( fullChunkName )
		if sect == None:
			sect = ResMgr.root.createSection( fullChunkName )
			if sect != None:
				sect.writeStrings( "boundary", ("","","","","","") )
				for i in xrange( 0, 6 ):
					self.writeBoundary( sect,x,y,i ) 
				self.writeTerrain( sect,x,y )
				self.writeBoundsAndTransform( sect,x,y )
				sect.save()
				BigBang.addCommentaryMsg( "created " + fullChunkName )
		else:
			BigBang.addCommentaryMsg( "chunk exists :  - skipping" + fullChunkName, 1 )

		#create terrain file
		fullTerrainName = theChunk[0:-1] + ".terrain"
		BigBang.createBlankTerrainFile( fullTerrainName )		
		BigBang.addCommentaryMsg( "created " + fullTerrainName )

	def createChunks( self, pathName, m, n ):
		if pathName[-1] != "/":
			pathName += "/"

		w = m / 2
		h = n / 2
		self.minX = -w
		self.maxX = w - 1
		self.minZ = -h
		self.maxZ = h - 1

		for x in xrange( -w, w ):
			for y in xrange( -h, h ):
				self.createChunk( pathName, x, y )

		#now, write the bounds into the space.settings file
		ss = ResMgr.openSection( pathName + "space.settings" )
		if ss != None:
			ss.writeInt( "bounds/minX", self.minX )
			ss.writeInt( "bounds/maxX", self.maxX )
			ss.writeInt( "bounds/minY", self.minZ )
			ss.writeInt( "bounds/maxY", self.maxZ )
			ss.save()