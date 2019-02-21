from controls import *

args = ( \
	CheckBox( "First_Person", updateCommand = "BigWorld.camera().firstPerson",
			setCommand = "BigWorld.camera().firstPerson = First_Person" ),
	CheckBox( "Reverse_View", updateCommand = "BigWorld.camera().reverseView",
			setCommand = "BigWorld.camera().reverseView = Reverse_View" ),
	WatcherCheckBox( "Invert_Mouse", "Client Settings/Mouse Inverted" ),
	WatcherFloatSlider( "Field_Of_View", "Render/Fov", (0.1, 3.0) ),
	FloatSlider( "Camera_Distance",
		updateCommand = "BigWorld.camera().pivotMaxDist",
		setCommand = "BigWorld.camera().pivotMaxDist = Camera_Distance",
		minMax = (0.01, 200.0) ),

	WatcherFloatSlider( "Camera_Mass", "Client Settings/Camera Mass", (1.0, 100.0) ),
	WatcherFloatSlider( "Orientation_Damper", "Client Settings/Orientation Damper", (0.01, 2.0) ),
)

commands = \
(
)
