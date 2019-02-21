from controls import *

args = \
(
	WatcherFloatEnum( "Shadow Quality", "Render/shadows/quality",
		(	( "Low",		0 ),
			( "Mid",	1 ),
			( "High",	2 )
		) ),
	WatcherFloatSlider( "Shadow Intensity", "Render/shadows/intensity", minMax = (0.0, 1.0) ),
	WatcherFloat( "Shadow Distance", "Render/shadows/distance" ),
	WatcherFloat( "Shadow Fade Start", "Render/shadows/fadeStart" )
	
)

commands = \
(
)
