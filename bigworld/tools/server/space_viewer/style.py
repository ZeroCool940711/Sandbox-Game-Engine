# provides an easy to hack around with interface to UI
# stuff in space viewer

import space_viewer

import bwsetup; bwsetup.addPath( ".." )
from pycommon import xmlprefs

prefs = xmlprefs.Prefs( space_viewer.PREFS )

ENTITY_RADIUS = 4
GHOST_ENTITY_RADIUS = 4
ENTITY_SCALE_PERCENT = int( prefs.get( "entityScale" ) or "100" )

# Specify colour for specific entity types.
ENTITY_COLOURS = {
	"Avatar": ((64, 192, 64), 2.0),
	"Guard": ((255, 64, 64), 1.0),
	"Creature": ((255, 128, 0), 1.0),
}

# Specify colours for other entity types that aren't specified in
# ENTITY_COLOURS. Entity type names are hashed into this for their colours.
DEFAULT_COLOUR_PALETTE = [
	(0, 153, 153), 	(0, 107, 107), 	(191, 255, 255), 	(128, 255, 255),
	(255, 102, 0), 	(179, 71, 0), 	(255, 217, 191), 	(255, 179, 128),
	(255, 238, 0), 	(179, 167, 0), 	(255, 251, 191), 	(255, 247, 128),
	(78, 0, 153), 	(55, 0, 107), 	(224, 191, 255), 	(193, 128, 255) ]

def getColourForEntityType( entityTypeName ):
	if ENTITY_COLOURS.has_key( entityTypeName ):
		return ENTITY_COLOURS[entityTypeName][0]
	else :
		return DEFAULT_COLOUR_PALETTE[
			hash( entityTypeName ) % len( DEFAULT_COLOUR_PALETTE ) ]

def getScaleForEntityType( entityTypeName ):
	if ENTITY_COLOURS.has_key( entityTypeName ):
		return ENTITY_COLOURS[entityTypeName][1]
	else :
		return 1.0


def hasAoI( entityID, entityTypeID ):
	return entityTypeID == 0

colourOptions = [
		[ "Cell App ID",         "cellAppIDColour",     "Black" ],
		[ "IP Address",          "ipAddrColour",        "Black" ],
		[ "Cell Load",           "cellLoadColour",      "Gold" ],
		[ "Partition Load",      "partitionLoadColour", None ],
		None,
		[ "Entity Bounds",       "entityBoundsColour",  None ],
		[ "Space Boundary",      "spaceBoundaryColour", "Blue Violet" ],
		[ "Grid",                "gridColour",          "Green" ],
		None,
		[ "Ghost Entity",        "ghostEntityColour",   "Light Grey" ],
		[ "Cell Boundary",       "cellBoundaryColour",  "Blue" ],
	]

colours = [
	None,
	"Black",
	"Blue",
	"Blue Violet",
	"Brown",
	"Cyan",
	"Dark Grey",
	"Dark Green",
	"Gold",
	"Grey",
	"Green",
	"Light Grey",
	"Magenta",
	"Navy",
	"Pink",
	"Red",
	"Sky Blue",
	"Violet",
	"Yellow",
	]

#Sizes of Entities Markers in Percent
sizes = [
	"25",
	"50",
	"75",
	"100",
	"150",
	]

#style.py
