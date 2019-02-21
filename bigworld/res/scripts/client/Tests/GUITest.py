"""Some tests for the ashes GUI system. 

	This module has a few functions for testing specific
	features of the GUI system. For more details on each
	one, type help(GUITest.testname).
	
	Available tests are:
	
	mode_conv
	anchors
	game
	
	To execute a test, type GUITest.testname() into the Python console.		
	You can use GUITest.clear() to remove all GUI components from the screen.
"""
import BigWorld, GUI, Math
import random

from functools import partial

BigWorld.camera(BigWorld.CursorCamera()) # Change camera type since FreeCamera eats mouse input
BigWorld.setCursor( GUI.mcursor() )
GUI.mcursor().visible = True


def clear():
	while len(GUI.roots()):
		GUI.delRoot(GUI.roots()[0])
		
#######
		
def _setAllModeCombinations( c, name ):
	setattr( c, name, "PIXEL" )
	setattr( c, name, "LEGACY" )
	setattr( c, name, "PIXEL" )
	setattr( c, name, "CLIP" )
	setattr( c, name, "PIXEL" )
	setattr( c, name, "CLIP" )
	setattr( c, name, "LEGACY" )
	setattr( c, name, "CLIP" )
		
def mode_conv():
	"""
		Tests conversion between position and size modes.
	"""

	def add_conv_test_child( p, hAnchor, vAnchor, pos ):
		c = GUI.Simple("system/maps/col_white.bmp")
		c.colour = (255,0,0,255)
		c.width = 0.1
		c.height = 0.1
		c.materialFX = "SOLID"
		c.horizontalPositionMode = "CLIP"
		c.verticalPositionMode = "CLIP"
		c.position.x = pos[0]
		c.position.y = pos[1]
		c.horizontalAnchor = hAnchor
		c.verticalAnchor = vAnchor
		p.addChild(c)
		
		_setAllModeCombinations( c, "horizontalPositionMode" )
		_setAllModeCombinations( c, "verticalPositionMode" )
		_setAllModeCombinations( c, "widthMode" )
		_setAllModeCombinations( c, "heightMode" )

	clear()
	w = GUI.Window("system/maps/col_white.bmp")
	
	w.position = (0,0.25,1)
	
	_setAllModeCombinations( w, "horizontalPositionMode" )
	_setAllModeCombinations( w, "verticalPositionMode" )
	_setAllModeCombinations( w, "widthMode" )
	_setAllModeCombinations( w, "heightMode" )
	
	add_conv_test_child( w, "LEFT", "TOP", (-1,1) )
	add_conv_test_child( w, "RIGHT", "TOP", (1,1) )
	add_conv_test_child( w, "LEFT", "BOTTOM", (-1,-1) )
	add_conv_test_child( w, "RIGHT", "BOTTOM", (1,-1) )
	add_conv_test_child( w, "CENTER", "CENTER", (0,0) )
	
	GUI.addRoot(w)

#######
	
class _Child(object):
	def __init__( self, component ):
		self.component = component
		self.component.script = self
		
	def handleMouseEnterEvent( self, comp, pos ):
		self.component.parent.hoverLabel.text = "< !! >"
		return False
		
	def handleMouseLeaveEvent( self, comp, pos ):
		self.component.parent.hoverLabel.text = ""
		return False
		
	@staticmethod
	def create( hAnchor, vAnchor, pos ):	
		c = GUI.Simple("system/maps/col_white.bmp")
		c.colour = (0,0,255,255)
		c.horizontalPositionMode = "CLIP"
		c.verticalPositionMode = "CLIP"		
		c.widthMode = "CLIP"
		c.heightMode = "CLIP"
		
		c.horizontalAnchor = hAnchor
		c.verticalAnchor = vAnchor
		
		c.materialFX = "SOLID"
		
		c.width = 0.5
		c.height = 0.5
		
		c.crossFocus = True
		c.moveFocus = True
		c.focus = True		
		
		c.position.x = pos[0]
		c.position.y = pos[1]
		
		return _Child(c)
		

class _ParentAnchorWindow(object):
	def __init__( self, component ):
		self.component = component
		self.component.script = self

	def handleMouseEnterEvent( self, comp, pos ):
		#print "W1Script.handleMouseEnterEvent", comp, pos
		self.component.hoverLabel.text = "!!"
		return False
		
	def handleMouseLeaveEvent( self, comp, pos ):
		#print "W1Script.handleMouseLeaveEvent", comp, pos
		self.component.hoverLabel.text = ""
		return False
	
	@staticmethod	
	def create( hAnchor, vAnchor, pos, colour ):
		c = GUI.Window("system/maps/col_white.bmp")
		c.widthMode = "PIXEL"
		c.heightMode = "PIXEL"
		c.width = 200
		c.height = 200
		c.widthMode = "CLIP"
		c.heightMode = "CLIP"
		
		c.colour = colour
		c.horizontalAnchor = hAnchor
		c.verticalAnchor = vAnchor
		
		c.position.x = pos[0]
		c.position.y = pos[1]
		
		c.materialFX = "BLEND"
		
		c.crossFocus = True
		c.moveFocus = True
		c.focus = True
		
		c.addChild( _Child.create( "LEFT",   "TOP", (-1, 1) ).component )
		c.addChild( _Child.create( "CENTER", "TOP", ( 0, 1) ).component )
		c.addChild( _Child.create( "RIGHT",  "TOP", ( 1, 1) ).component )
		
		c.addChild( _Child.create( "LEFT",   "CENTER", (-1, 0) ).component )
		c.addChild( _Child.create( "CENTER", "CENTER", ( 0, 0) ).component )
		c.addChild( _Child.create( "RIGHT",  "CENTER", ( 1, 0) ).component )
		
		c.addChild( _Child.create( "LEFT",   "BOTTOM", (-1,-1) ).component )
		c.addChild( _Child.create( "CENTER", "BOTTOM", ( 0,-1) ).component )
		c.addChild( _Child.create( "RIGHT",  "BOTTOM", ( 1,-1) ).component )
		
		label = GUI.Text( "%s, %s" % (hAnchor, vAnchor) )
		label.font = "default_smaller.font"
		label.verticalAnchor = "BOTTOM"
		label.horizontalPositionMode = "CLIP"
		label.verticalPositionMode = "CLIP"
		label.position.y = -0.8
		label.colour = (100, 100, 100, 255)
		c.addChild( label, "label" )
		
		hoverLabel = GUI.Text("")
		hoverLabel.colour = (255,128,64,255)		
		c.addChild( hoverLabel, "hoverLabel" )
		
		return _ParentAnchorWindow(c)
		
		
#######
		
def anchors():
	"""
		Tests anchoring, relative to screen and within windows.
		It also checks the mouse hit tests (by displaying some
		text when the mouse hovers over each component).
	"""
	clear()
			
	GUI.addRoot( _ParentAnchorWindow.create( "LEFT",   "TOP", (-1, 1), (0,0,0,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "CENTER", "TOP", (0,1),   (0,0,128,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "RIGHT",  "TOP", (1,1),   (0,255,0,255) ).component )
	
	GUI.addRoot( _ParentAnchorWindow.create( "LEFT",   "CENTER", (-1,0), (0,255,255,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "CENTER", "CENTER", (0,0),  (255,0,0,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "RIGHT",  "CENTER", (1,0),  (255,0,255,255) ).component )
	
	GUI.addRoot( _ParentAnchorWindow.create( "LEFT",   "BOTTOM", (-1,-1),  (255,255,0,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "CENTER", "BOTTOM", (0,-1),  (255,255,255,255) ).component )
	GUI.addRoot( _ParentAnchorWindow.create( "RIGHT",  "BOTTOM", (1,-1), (128,128,128,255) ).component )
	

#######

class _GameSquare:

	def __init__( self, component ):
		self.component = component
		self.component.script = self
		
		self.velocity = Math.Vector2( random.random()*2-1, random.random()*2-1 )
		self.velocity.normalise()
		self.velocity *= 0.25
		
		self.lastUpdateTime = BigWorld.time()
		self.gameStarted = False
		
		BigWorld.callback( 0.01, self.update )
		BigWorld.callback( 3, self.startGame )
		
	def startGame( self ):
		self.gameStarted = True
		self.lastUpdateTime = BigWorld.time()
		self.gameStartTime = BigWorld.time()
		BigWorld.callback( 0.01, self.update )
		BigWorld.callback( 7, self.speedUp )
		
	def update( self ):
		dt = float(BigWorld.time() - self.lastUpdateTime)
		self.lastUpdateTime = BigWorld.time()
		
		self.component.position.x += float(self.velocity.x) * float(dt)
		self.component.position.y += float(self.velocity.y) * float(dt)
		
		#print "blah", self.component.position, self.velocity
		
		if self.component.position.x < -1:
			self.velocity.x = -self.velocity.x
			self.component.position.x = -1
		elif self.component.position.x  > 1 - self.component.width:
			self.velocity.x = -self.velocity.x
			self.component.position.x = 1.0 - self.component.width
		elif self.component.position.y < -1 + self.component.height:
			self.velocity.y = -self.velocity.y
			self.component.position.y = -1 + self.component.height
		elif self.component.position.y > 1:
			self.velocity.y = -self.velocity.y
			self.component.position.y = 1.0
		
		if self.gameStarted:
			BigWorld.callback( 0.001, self.update )
		

	def speedUp( self ):
		BigWorld.callback( 5, self.speedUp )
		self.velocity = self.velocity.scale( 1.4 )
			
		
	def handleMouseEnterEvent( self, comp, pos ):
		if self.gameStarted:
			#print "YOU LOSE!"
			
			t = GUI.Text("YOU LOSE! Time: %.2f sec." % (BigWorld.time() - self.gameStartTime))
			t.colour = (255,0,0,255)
			t.position.z = 0.5
			GUI.addRoot(t)
			GUI.reSort()
			BigWorld.callback( 5, clear )
			
			for root in GUI.roots():
				if isinstance(root.script, _GameSquare):
					root.script.gameStarted = False
		
		return False
		
	def handleMouseLeaveEvent( self, comp, pos ):
		#self.component.parent.label.text = ""
		return False
		
	@staticmethod	
	def create():
		c = GUI.Simple("system/maps/col_white.bmp")
		c.horizontalPositionMode = "LEGACY"
		c.verticalPositionMode = "LEGACY"
		c.horizontalAnchor = "LEFT"
		c.verticalAnchor = "TOP"
		
		c.materialFX = "SOLID"
		
		c.widthMode = "CLIP"
		c.heightMode = "CLIP"
		
		c.width = random.random()
		c.height = random.random()
		
		c.focus = True
		c.moveFocus = True
		c.crossFocus = True
		
		c.colour = ( int(random.random()*127), 
					 int(random.random()*127), 
					 int(random.random()*127), 
					 255 )
					 
		c.position = (random.random()*2-1, random.random()*2-1, 1)
		
		#print c, c.position, c.width, c.height, c.colour[0]
		
		return _GameSquare(c)
		
		
def _removeComponent( component ):
	GUI.delRoot( component )
			
def _gameStarted( label ):
	label.text = "GO!"
	BigWorld.callback( 1, partial(_removeComponent, label) )

def game():
	"""
		Small game where the objective is to not touch the boxes
		with the mouse for as long as possible :). Probably not
		very challenging...
		
		Note: Since hit testing in the GUI system is only ever done
		when moving the mouse, the hit testing is a little flakey.
	"""
	clear()

	for i in range(10):
		GUI.addRoot( _GameSquare.create().component )
		
	t = GUI.Text("Get ready...")
	t.colour = (255,0,0,255)
	t.position.z = 0.5
	
	GUI.addRoot(t)
	BigWorld.callback( 3, partial(_gameStarted, t) )
	
	GUI.reSort()
		
	
	



#######

# Test bug 22596, easier than recreating the symptom in bug 22518

class _OrderStackingBlock():
	def __init__( self, component, colour ):
		self.component = component
		self.component.script = self
		self.name = colour

	def handleMouseClickEvent( self, comp, pos ):
		#print "W1Script.handleMouseEnterEvent", comp, pos
		self.component.hoverLabel.text = self.name
		BigWorld.callback( 2, self.clearClick )
		return True


	def clearClick( self ):
		self.component.hoverLabel.text = ""

	
	@staticmethod
	def create( position, colour ):
		c = GUI.Simple( "system/maps/col_%s.bmp" % colour )
		c.materialFX = "SOLID"
		c.size = ( 0.5, 0.5 )
		c.position = position
		c.focus = True

		hoverLabel = GUI.Text("")
		hoverLabel.colour = (255,255,255,255)		
		hoverLabel.position = position
		hoverLabel.position.x *= 2
		hoverLabel.position.y *= 2
		hoverLabel.position.z = 0.0
		c.addChild( hoverLabel, "hoverLabel" )
		
		return _OrderStackingBlock( c, colour )


class _OrderChangingButton():
	def __init__( self, component, backComponent ):
		self.component = component
		self.component.script = self
		self.target = backComponent

	def handleMouseClickEvent( self, comp, pos ):
		self.target.position.z = 0.2
		self.target.parent.reSort()
		return True

	@staticmethod
	def create( backComponent ):
		c = GUI.Text( "Click here to move black box forward" )
		c.position = ( 0.0, -0.8, 0.0 )
		c.focus = True
		return _OrderChangingButton( c, backComponent )


def localReSort():
	"""
		Tests that parent.reSort() on a component is sufficient
		to correct the order in which the GUI searchs for the
		component under the cursor.
	"""
	clear()
	ourRoot = _OrderStackingBlock.create( ( 0.0, 0.0, 0.0 ), "white" ).component
	GUI.addRoot( ourRoot )

	red = _OrderStackingBlock.create( ( 0.0, -0.2, 0.5 ), "red" ).component
	ourRoot.addChild( red )

	yellow =_OrderStackingBlock.create( ( 0.2, 0.0, 0.6 ), "yellow" ).component
	ourRoot.addChild( yellow )

	black =_OrderStackingBlock.create( ( -0.2, 0.0, 0.7 ), "black" ).component
	ourRoot.addChild( black )

	GUI.addRoot( _OrderChangingButton.create( black ).component )
