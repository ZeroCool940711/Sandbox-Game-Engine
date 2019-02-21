def callableOnGhost( f ):
	"""
	This is a decorator that should be added to methods that can validly be
	called on ghost entities.

	For example:

	import bwdecorators
	class Table( BigWorld.Entity ):
		@bwdecorators.callableOnGhost
		def getArea( self ):
			return self.width * self.height
	"""

	f.isGhost = True
	return f

# bwdecorators.py
