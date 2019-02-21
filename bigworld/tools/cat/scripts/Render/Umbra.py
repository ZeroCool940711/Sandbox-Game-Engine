from controls import *

args = \
(
	WatcherCheckBox( "Enable", "Render/Umbra/enabled" ),
	WatcherCheckBox( "Use occlusion culling", "Render/Umbra/occlusionCulling" ),
	WatcherCheckBox( "Use depth only pass", "Render/Umbra/depthOnlyPass" ),
	WatcherCheckBox( "Flush trees", "Render/Umbra/flushTrees" ),

	WatcherCheckBox( "Draw occlusion queries", "Render/Umbra/depthOnlyPass" ),
	WatcherCheckBox( "Draw test models", "Render/Umbra/drawTestModels" ),
	WatcherCheckBox( "Draw write models", "Render/Umbra/drawWriteModels" ),
	WatcherCheckBox( "Draw object bounds", "Render/Umbra/drawObjectBounds" ),
	WatcherCheckBox( "Draw silhouettes", "Render/Umbra/drawSilhouettes" ),
	WatcherCheckBox( "Draw voxels", "Render/Umbra/drawVoxels" ),
)

commands = \
(
)
