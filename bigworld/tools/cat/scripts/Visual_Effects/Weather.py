from controls import *

args = \
(
	FixedText( "Note: These settings only affect the client if not\nconnected to the server (when online the weather\nsystem overrides the parameters)."),

	WatcherFloatSlider( "Wind Velocity", "Client Settings/Clouds/wind y", minMax = (-320.0, 0.0) ),
	WatcherFloatSlider( "Effective Drop Size", "Client Settings/Rain/area", minMax = (0.5, 10.0) ),
	Divider(),
	WatcherFloatSlider( "Clear Weighting", "Client Settings/Weather/CLEAR/propensity", minMax = (0.0, 10.0) ),
	Divider(),
	WatcherFloatSlider( "Cloud Weighting", "Client Settings/Weather/CLOUD/propensity", minMax = (0.0, 10.0) ),
	WatcherFloatSlider( "Cover", "Client Settings/Weather/CLOUD/arg0", minMax = (0.0, 1.0) ),
	WatcherFloatSlider( "Cohesion", "Client Settings/Weather/CLOUD/arg1", minMax = (0.0, 1.0) ),
	Divider(),
	WatcherFloatSlider( "Rain Weighting", "Client Settings/Weather/RAIN/propensity", minMax = (0.0, 10.0) ),
	WatcherFloatSlider( "Darkness", "Client Settings/Weather/RAIN/arg0", minMax = (0.0, 1.0) ),
	WatcherFloatSlider( "Cohesion", "Client Settings/Weather/RAIN/arg1", minMax = (0.0, 1.0) ),
	Divider(),
	WatcherFloatSlider( "Storm Weighting", "Client Settings/Weather/STORM/propensity", minMax = (0.0, 10.0) ),
)

commands = \
(
)
