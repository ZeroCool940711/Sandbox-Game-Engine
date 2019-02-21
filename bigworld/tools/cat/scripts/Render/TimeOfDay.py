from controls import *

args = \
(
	FloatSlider( "Time_Of_Day_24hr",
		updateCommand = """
		tod = BigWorld.getWatcher(\"Client Settings/Time of Day\")
		todPair = tod.split( ":" )
		float( todPair[0] ) + float( todPair[1] ) / 60.0
		""",
		setCommand = "BigWorld.setWatcher(\"Client Settings/Time of Day\", float( Time_Of_Day_24hr ))",
		minMax = (0, 23.99)
		),
	WatcherInt( "Secs_Per_Hour", "Client Settings/Secs Per Hour" ),
)

commands = \
(
)