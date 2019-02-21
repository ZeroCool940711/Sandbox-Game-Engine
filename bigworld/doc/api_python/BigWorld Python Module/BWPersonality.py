"""The name of this module is actually specified by the personality option in the configuration file.
This module should be implemented by the user. It should contain callback methods that will be called 
when specific events occur. """

def expandMacros( ): 
	"""If present, this method is used to process input on the Python console
	before it is fed to the Python interpreter. It should accept a string and
	return a string formed by processing macros in whatever manner you desire.
	In FantasyDemo, this function is used to expand single character $ expressions
	like '$p' into 'BigWorld.player()'. 

	See the implementation of expandMacros() in fantasydemo/res/scripts/client/FantasyDemo.py
	for a sample implementation. """
	return

def fini( ):
	"""If present, this method is called when the client engine shuts down. Usually,
	this function does any cleanup required before the client exits e.g. log out of the server. """
	return

def handleIMEEvent( event ): 
	"""If present, this method is called whenever the IME system changes
	state internally. The event object can be inspected to determine what
	part actually changed so that the scripts can update the IME interface
	that is currently being shown to the user.
	
	Parameters: event  A PyIMEEvent object.  


	Returns: This function should return False if the event was not handled 
	and True if the key event was handled.  """
	return

def handleKeyEvent( event ): 
	"""If present, this method is called whenever a key is pressed, released,
	or repeated due to operating system auto-repeat settings. 
	Parameters: event  A PyKeyEvent object.  
	Returns: This function should return False if the key event was not handled
	and True if the key event was handled."""
	return

def handleLangChangeEvent( ): 
	"""If present, this method is called whenever the user changes the input
	language for the client (e.g. via the language bar). 
	Returns: This function should return False if the event was not handled
	and True if the event was handled.  """
	return

def handleMouseEvent( event ): 
	"""If present, this method is called whenever the mouse moves. 
	Parameters: event  A PyMouseEvent object.  
	Returns: This function should return False if the mouse event was not 
	handled and True if the mouse event was handled.  """
	return

def init( scriptConfig, engineConfig, preferences, loadingScreenGUI ): 
	"""If present, this method is called when the engine initialises. Usually,
	this function initialises script objects from the configuration data. 
	Parameters: scriptConfig  Is a PyDataSection object refering to the root
	section of script_config.xml.  
	engineConfig  Is a PyDataSection object refering to the root section of
	engine_config.xml.  
	preferences  Is a PyDataSection object refering to the scriptsPreference
	section of the preferences fils (defined in engine_config.xml).  
	loadingScreenGUI  (optional) a GUI.SimpleGUIComponent refering to loading
	screen GUI component shown during the startup procees, if one was defined
	in resources.xml.  """
	return

def onChangeEnvironments( isInside ): 
	"""If present, this method is called when the type of environment that
	the player is in changes. Currently there are only two types of environments: inside and outside. 
	Parameters: isInside  This boolean indicates whether the player
	is in an inside environment. """ 
	return

def onRecreateDevice( ):
	"""If present, this method is called when the Direct3D device is recreated
	e.g. when the screen resolution changes. Usually, this function re-layouts
	the GUI components and recreates any static PyModelRenderer textures. 
	def onStreamComplete( id, desc, data ): 
	If present, this method is called when a download from the proxy triggered 
	with either streamStringToClient() or streamFileToClient() has completed. 

	See Proxy.streamStringToClient in BaseApp Python API documentation.
	See Proxy.streamFileToClient in BaseApp Python API documentation 
	Parameters: id  A unique ID associated with the download.  
	desc  A short string description of the download.  
	data  The downloaded data, as a string.  """
	return

def onStreamComplete( id, desc, data ): 
	"""If present, this method is called when: a) the game time is changed
	manually i.e. not due to the natural progression of time; or b) The ratio
	of game time to real time is changed. 
	Parameters: gameTime  The new game time.  
	secondsPerGameHour  The new number of real time seconds per game time hour.  """
	return

def onTimeOfDayLocalChange( gameTime, secondsPerGameHour ):
	"""If present, this method is called when: a) the game time is changed
	manually i.e. not due to the natural progression of time; or b) The ratio
	of game time to real time is changed. 
	Parameters: gameTime  The new game time.  
	secondsPerGameHour  The new number of real time seconds per game time hour."""  
	return

def start( ): 
	"""If present, this method is called when the engine has initialised. Usually,
	this function starts the game e.g. by bringing up a menu or log-in screen. """
	return

