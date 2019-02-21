"""The Scaleform module provides access to the Scaleform integration, if it is enabled. """



class PyMovieDef(PyObjectPlus):
	"""This class wraps Scaleform's Movie Definition and provides lifetime control
	and python access to its API. 
	
	A Movie Definition contains the resources for an .swf file, and allows creation
	of instanced views of the movie. 
	
	Movie Definitions can also be loaded for the sole purpose of providing fonts for
	other movies and for flash text components."""
	
	def MovieDef():	
		"""Factory function to create and return a Scaleform Movie Definition."""
		return	
	
	def createInstance( self, bool ):
		"""This function creates an instance of the movie definition.
		* It returns a PyMovieView object. 
		Parameters: bool  [optional] Specifies whether the frame1 tags of
		the * swf movie are executed or not during the createInstance call.
		* Default: true"""
		return
	def setAsFontMovie( self ):
		"""Set this movie as the font source for the DrawTextManager, 
		which is * used by FlashTextComponents to draw with."""
		return
	



class PyMovieView(PyObjectPlus):
	"""This class wraps Scaleform's Movie View and provides lifetime control
	and python access to its API. 
	
	A Movie View is an instanced view of a Movie Definition, and can only be
	created via the PyMovieDef.createInstance() method. """
	
	backgroundAlpha = None	# Sets the background color alpha applied to the movie clip. * Set to 0.0f if you do not want the movie to render its background * (ex. if you are displaying a hud on top of the game scene). * The default value is 1.0f, indication full opacity. Type: Read-Write Float.  
	scaleMode = None   	# This attribute stores the movie view scale mode, * which describes how the movie should display when the * viewing dimensions are not the same as the original * flash movie size. 
	userData = None		# This attribute stores the User data object associated * with the movie. The data can be any object.
	visible = None		# Sets the visibility state of the movie view. Movie Views that * are invisible do not draw, and do not advance. 
	
	
	def gotoFrame( self, Integer ):
		"""This function moves the playahead to the specified frame.
		Note that * calling this function may cause ActionScript tags
		attached to the * frame to be bypassed. 
		Parameters: Integer  The frame number to go to."""
		return
	
	def gotoLabeledFrame( self, String, Integer ):
		"""This function moves the playahead to the frame with the given
		label. * If the label is not found, the function will return false. 
		Parameters: String  The frame label to go to.  
		Integer  Offset from the label.  
				
		Returns: Boolean Whether or not the label was found."""
		return
	
	def handleKeyEvent( self, event ):
		"""Call this method to pass a key event into the underlying flash
		movie. If the * movie handles the event, this method will return True. 
		Parameters: event  A PyKeyEvent object.  
		

		Returns: True if the event was handled, False if it was not."""
		return
	
	def handleMouseButtonEvent( self, event ):
		"""Call this method to pass a mouse button event into the underlying
		flash movie. If the * movie handles the event, this method will return True. 
		Parameters: event  A PyKeyEvent object.  
		
		
		Returns: True if the event was handled, False if it was not."""
		
		return
	
	def handleMouseEvent( self, event ):
		"""Call this method to pass a mouse move event into the underlying flash
		movie. If the * movie handles the event, this method will return True. 
		Parameters: event  A PyMouseEvent object.  
		
		
		Returns: True if the event was handled, False if it was not."""
		return
	
	def hitTest( self, position ):
		"""This method is used to determine if a given screen space position
		intersects with * any geometry in the Flash movie (based on the last rendered viewport).
		It is useful * for allowing mouse events to pass through transparent parts of a Flash based HUD. 
		Parameters: position  Position in clip space.  


		Returns: True if the position intersected with movie geometry, False if not."""
		
		return
	
	def invoke( self ):
		"""This function calls a function in the movie's ActionScript.
		It takes a string name * of the ActionScript method to call, and any number of 
		arguments to be passed through. * If the second argument is a list, then that
		will be used as the argument list. 

		* For example: 
		
		* @{ * view.invoke( "_root.functionName" ) # Call function with no parameters 
		* view.invoke( "_root.functionName", 1, 2, 3 ) # Call function with three args
		* view.invoke( "_root.functionName", [1,2,3] ) # This call is equivalent * @}"""
		
		return
	
	def restart( self ):
		"""This function restarts the flash movie."""
		return
	
	def setExternalInterfaceCallback( self, Callable ):
		"""This function sets up the callback function used * to handle ActionScript's external interface command. 
		Parameters: Callable  A callable Python object (e.g. a method or * function).
		This will be passed two parameters: * the command name, and a tuple containing the * command arguments."""
		return
	
	def setFSCommandCallback( self, Callable ):
		"""This function sets up the callback function used * to handle ActionScript's fscommand. 
		Parameters: Callable  A callable Python object (e.g. a method or * function).
		This will be passed two parameters: * the command name, and a tuple containing the * command arguments."""
		return
	
	def setFocussed( self ):
		"""This function selects this movie as having IME focus, and * finalises IME support for the previously focussed movie. 

		* Once focussed, the movie will internally tell IME which * dynamic text fields are focussed, and IME will appear * accordingly."""
		return
	
	def setPause( self, Boolean ):
		"""This function sets the pause mode of the movie. 
		Parameters: Boolean  Flag specifying whether or not to pause the movie."""
		return