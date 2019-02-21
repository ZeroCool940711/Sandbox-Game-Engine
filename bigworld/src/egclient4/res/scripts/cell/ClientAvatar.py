"""
The ClientAvatar Cell Entity script.

This entity is the character avatar entity. When it is created on the cell,
it will add a one-second non-repeating timer to call the client method 
fromCell() - see ClientAvatar.def for more details. 

It will also increment the intValue property of the Test user data type.
"""

import BigWorld

# ------------------------------------------------------------------------------
# Section: class ClientAvatar
# ------------------------------------------------------------------------------

class ClientAvatar( BigWorld.Entity ):
	"An Avatar entity for the Client."

	def __init__( self ):
		"""
		Constructor.
		"""
		BigWorld.Entity.__init__( self )

		print "ClientAvatar.__init__:", self.__dict__

		# add a timer - calls back on onTimer when done
		self.addTimer( 1.0, 0.0 )


	def onTimer( self, timerId, userData ):
		"""
		This method is called when a timer associated with this entity is
		triggered.
		"""
		self.allClients.fromCell( "Hello from the cell" )
		self.prop5.intValue += 1


# ClientAvatar.py
