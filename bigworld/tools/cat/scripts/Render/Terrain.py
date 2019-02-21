from controls import *

args = \
(
	WatcherCheckBox( "Draw Terrain", "Render/Terrain/draw"),
	Divider(),	
	WatcherFloatSlider( "Tiling Frequency", "Render/Terrain/Terrain1/texture tile frequency", minMax = (0.0, 1.0) ),
	WatcherFloatSlider( "Specular Diffuse amount", "Render/Terrain/Terrain1/specular diffuse amount", minMax = (0.0, 1.0) ),
	WatcherFloatSlider( "Specular Multiplier", "Render/Terrain/Terrain1/specular multiplier", minMax = (0.0, 1.0) ),
	WatcherIntSlider( "Specular Power", "Render/Terrain/Terrain1/specular power", minMax = (1, 10) ),
)

commands = \
(
)
 