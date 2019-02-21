import BigWorld
import Math
import math
import random
from functools import partial

def teleportNext( timer, destinationList ):
	dest = destinationList[0]
	BigWorld.player().tryToTeleport( dest )
	if len(destinationList) > 1:
		BigWorld.callback( timer, partial(teleportNext, timer, destinationList[1:] ) )
	else:
		print "teleport test finished"

def testTeleport():
	d = []
	
	d.append( "spaces/highlands/demo2" )
	d.append( "spaces/highlands/demo3" )
	d.append( "spaces/highlands/demo4" )
	
	d.append( "spaces/arctic/demo1" )
	d.append( "spaces/arctic/demo2" )
	d.append( "spaces/arctic/demo3" )
	d.append( "spaces/arctic/demo4" )
	
	d.append( "spaces/desert/demo1" )
	d.append( "spaces/desert/demo2" )
	d.append( "spaces/desert/demo3" )
	d.append( "spaces/desert/demo4" )
	
	d.append( "spaces/highlands/demo1" )
	
	BigWorld.callback( 0.1, partial(teleportNext, 20, d) )
