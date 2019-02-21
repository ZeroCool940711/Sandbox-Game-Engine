import BigBang
import keys
import GUI
import Locator
import View
import Functor
import ResMgr
from keys import *


browsePath = []
browseSect = None
browseIdx = 0

class TestScript:
	def __init__( self ):
		pass

	def onStart( self ):
		self.onResume( 0 )


	def onStop( self ):
		self.onPause()
		return 0

	def onPause( self ):
		pass

	def onResume( self, exitCode ):
		pass


	def ownKeyEvent( self, key, modifiers ):
		handled = 0
		return handled

	def onKeyEvent( self, isDown, key, modifiers ):
		handled = 0
		return handled

	def onMouseEvent( self, mx, my, mz ):
		handled = 0
		return handled

	def updateState( self, dTime ):
		GUI.update( dTime )
		BigBang.update( dTime )
		BigBang.camera().update( dTime )
		return 1

	def render( self, dTime ):
		BigBang.camera().render( dTime )
		BigBang.render()
		GUI.draw()
		return 1