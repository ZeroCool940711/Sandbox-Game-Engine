from controls import *

args = \
(
	WatcherCheckBox( "Draw bounding boxes", "SpeedTree/Bounding boxes" ),
	WatcherCheckBox( "Use batched rendering", "SpeedTree/Batched rendering" ),
	WatcherCheckBox( "Draw speedtrees", "SpeedTree/Draw trees" ),
	WatcherCheckBox( "Draw leave geometry", "SpeedTree/Draw leaves" ),
	WatcherCheckBox( "Draw branch geometry", "SpeedTree/Draw branches" ),
	WatcherCheckBox( "Draw fronds", "SpeedTree/Draw fronds" ),
	WatcherCheckBox( "Draw billboards", "SpeedTree/Draw billboards" ),
	WatcherCheckBox( "Use optimised billboards ", "SpeedTree/Optimise billboards" ),
	WatcherCheckBox( "Draw with texturing", "SpeedTree/Texturing" ),
	WatcherCheckBox( "Play wind animation", "SpeedTree/Play animation" ),
	WatcherFloat( "Leaf rock far plane", "SpeedTree/Leaf Rock Far Plane", (0, 10000) ),
	WatcherFloat( "LOD Mode", "SpeedTree/LOD Mode", (-2, 1) ),
	WatcherFloat( "LOD near distance", "SpeedTree/LOD near", (0, 10000) ),
	WatcherFloat( "LOD far distance", "SpeedTree/LOD far", (0, 10000) )
)

commands = \
(
)
