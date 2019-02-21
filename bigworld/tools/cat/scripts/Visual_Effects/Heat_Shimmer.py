from controls import *

args = \
(
	WatcherCheckBox( "Heat Effect Enabled", "Client Settings/fx/Heat/enable" ),
	
	WatcherFloatSlider( "Speed", "Client Settings/fx/Heat/speed", minMax = (0.0, 320.0) ),
	
	WatcherFloatSlider( "Spread X", "Client Settings/fx/Heat/spread x", minMax = (0.0, 10.0) ),
	WatcherFloatSlider( "Spread Y", "Client Settings/fx/Heat/spread y", minMax = (0.0, 10.0) ),
	
	WatcherFloatSlider( "S Noise Freq", "Client Settings/fx/Heat/S noise freq", minMax = (0.0, 1.0) ),
	WatcherFloatSlider( "T Noise Freq", "Client Settings/fx/Heat/T noise freq", minMax = (0.0, 1.0) ),
	
	WatcherFloatSlider( "U Fix Up", "Client Settings/fx/Heat/u fix up", minMax = (-2.0, 2.0) ),
	WatcherFloatSlider( "V Fix Up", "Client Settings/fx/Heat/v fix up", minMax = (-2.0, 2.0) ),
	
	WatcherCheckBox( "Show Debug Texture", "Client Settings/fx/Heat/debug texture" )
)

commands = \
(
)