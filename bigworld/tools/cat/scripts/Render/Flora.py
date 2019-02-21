from controls import *

args = \
(
	WatcherCheckBox( "Use Flora Texture", "Client Settings/Flora/Use flora texture"),
	WatcherCheckBox( "Cull", "Client Settings/Flora/Cull"),
	WatcherFloatSlider( "Noise Factor", "Client Settings/Flora/Noise Factor", minMax = (0.0, 0.5) ),
	WatcherCheckBox( "Show Debug Bounding Box", "Client Settings/Flora/Debug BB"),
)

commands = \
(
	( "No Flora" 		, 	"BigWorld.floraVBSize(0)" ),
	( "Sparse Flora (256KB)", 	"BigWorld.floraVBSize(1024*256)" ),
	( "Standard Flora (1MB)", 	"BigWorld.floraVBSize(1024*1024)" ),
	( "Dense Flora (5MB)", 		"BigWorld.floraVBSize(1024*1024*5)" ),
)
