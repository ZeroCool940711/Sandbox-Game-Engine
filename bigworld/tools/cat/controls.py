import wx
from comm import *
import cPickle
import socket

# -----------------------------------------------------------------------------

class NumericCtrl( wx.TextCtrl ):
	def __init__( self, parent, id, mysize, isFloat = False,
					limits = (0, 0), useLimits = False):
		wx.TextCtrl.__init__( self, parent, id, size=mysize )
		self.isFloat = isFloat
		self.min = limits[0]
		self.max = limits[1]
		self.useLimits = useLimits
		self.inOnText = False
		wx.EVT_CHAR( self, self.onChar )
		wx.EVT_TEXT( self, id, self.onText )
		wx.EVT_SET_FOCUS( self, self.onSetFocus )
		wx.EVT_KILL_FOCUS( self, self.onKillFocus )

	# ------------------------------

	def verifyLimitsAndSet( self, value ):
		if not self.useLimits:
			self.SetValue( value )
			return;

		if self.isFloat:
			number = float( value )
			if number < float( self.min ):
				number = float( self.min )
			elif number > float( self.max ):
				number = float( self.max )
			value = number
		else:
			number = int( value )
			if number < int( self.min ):
				number = int( self.min )
			elif number > int( self.max ):
				number = int( self.max )
		self.SetValue( str(number) )

	# ------------------------------

	def setValue( self, value ):
		self.SetValue( str(value) )
		#self.verifyLimitsAndSet( self.GetValue() )
		self.applyMakeup()

	# ------------------------------

	def getValue( self ):
		return self.GetValue()
		
	# ------------------------------

	def onChar( self, event ):
		if event.GetKeyCode() == 13: # Was enter pressed?
			self.killFocus() # Update the field correctly
		event.Skip() # ignore the char event to allow onText
		
	# ------------------------------

	def onText( self, event ):
		
		# return immediately if recursively called
		if self.inOnText:
			self.inOnText = False
			return

		# make sure the text is a number
		val = self.GetValue()
		if len( val ) == 0:
			return

		# examine entered index
		insertIndex = self.GetInsertionPoint()

		enteredIndex = insertIndex - 1
		if enteredIndex < 0:
			enteredIndex = 0

		charEntered = val[ enteredIndex ]

		# how many decimals
		numDecimals = val.count( "." )
		if charEntered.isdigit():
			self.SetInsertionPoint( insertIndex )
			return
		if enteredIndex == 0 and charEntered == "-":
			self.SetInsertionPoint( insertIndex )
			return
		if self.isFloat and charEntered == "." and numDecimals == 1:
			self.SetInsertionPoint( insertIndex )
			return

		# else, not valid, remove the new digit
		newVal = ""
		for i in range( 0, enteredIndex ):
			newVal = newVal + val[i]
		if insertIndex < len( val ):
			for i in range( insertIndex, len( val ) ):
				newVal = newVal + val[i]
				
		# to prevent infinite recurssion
		self.inOnText = True
		self.SetValue( newVal )

		# keep the cursor at the same positon
		self.SetInsertionPoint( enteredIndex )

	# ------------------------------

	def applyMakeup( self ):
		value = self.GetValue()
		if len( value ) == 0:
			value = "0"
		if self.isFloat:
			try:
					value = "%g" % float( value )
			except ValueError,e:
				print "Error converting to float:", e, value
				value = "0.0"
			# make sure there is a decimal there
			numDecimals = value.count( "." )
			if numDecimals == 0:
				value = value + ".0"
		self.SetValue( value )

	# ------------------------------

	def onSetFocus( self, event ):
		val = self.GetValue()
		# select all text
		self.SetSelection( -1, -1 )
		#event.Skip()

	# ------------------------------

	def killFocus( self ):
		val = self.GetValue()
		if val == "-" or val == "." or val == "-.": # Dodgy special values
			val = "0"	
		self.verifyLimitsAndSet( val )
		self.applyMakeup()
		self.SetSelection( 0, 0 )
	
	# ------------------------------
	
	def onKillFocus( self, event ):
		self.killFocus()
		event.Skip()


# -----------------------------------------------------------------------------


class Arg:
	def __init__( self, name, updateCommand, setCommand ):
		self.name = name.replace( " ", "_" )
		self.nameGUI = name.replace( "_", " " )
		self.comms = None
		self.updateCommand = updateCommand
		self.setCommand = setCommand
		# must nominate a control for checking whether its value has changed
		# this is usually a text entry control
		self.control = None
		self.recordedValue = None

	# ------------------------------

	def createControl( self, comms ):
		self.comms = comms
		# base does not do any creating

	# ------------------------------

	def getName( self ):
		return self.name

	def getNameGUI( self ):
		return self.nameGUI

	# ------------------------------

	def getValue( self ):
		return ""

	def fill( self ):
		return

	# ------------------------------

	def recordValue( self ):
		self.recordedValue = self.control.GetValue()

	def valueChanging( self ):
		if self.recordedValue == None:
			return False
		try:
			 (a, b) = self.control.GetSelection()
			 if a != b:
			 	return True
		except StandardError:
			pass

		return self.control.GetValue() != self.recordedValue

	# ------------------------------

	def update( self, force = False ):
		if not force and self.valueChanging():
			return

		updCommand = self.updateCommand.strip()
		if updCommand != "":
			commandLines = updCommand.split( "\n" )
			numberLines = len( commandLines )
			response = None #""
			if numberLines > 1:
				for i in range( numberLines - 1):
					self.comms.post( commandLines[i] )
				response = self.comms.ask( commandLines[ numberLines - 1 ] )
			else:
				response = self.comms.ask( updCommand )

			if response != "":
				# connection i alive
				self.updateValue( response )
			else:
				start = updCommand.find( "\"" ) + 1
				end = updCommand.rfind( "\"" )
				print "Unable to find watcher \"", updCommand[start:end], "\". Changing this value will have no effect."

	# ------------------------------

	def postValue( self ):
		if self.getValue() == "":
			return

		strVal = self.getName() + " = " + str( self.getValue() )
		self.comms.post( strVal )
		self.recordValue()

	# ------------------------------

	def postSetCommand( self ):
		setCommand = self.setCommand
		if setCommand == "":
			return

		# make sure have the correct last value
		self.recordValue()

		# set the parameter
		val = str( self.getValue() )
		if type( val ) == str:
			val = "\"" + val + "\""
		strVal = self.getName() + " = " + str(self.getValue())
		self.comms.post( strVal )
		commandLines = setCommand.split( "\n" )
		for line in commandLines:
			self.comms.post( line )


# -----------------------------------------------------------------------------

class FixedText( Arg ):
	def __init__( self, text = "" ):
		Arg.__init__( self, text, "", "" )
		self.text = text

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.text )
		self.box.Add( self.label, 0, wx.ALIGN_LEFT | wx.ALL, 5 )

		self.panel.SetSizer( self.box )
		return self.panel

# -----------------------------------------------------------------------------

class Text( Arg ):
	def __init__( self, name, default = None, updateCommand = "",
					setCommand = "", readOnly = False ):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.readOnly = readOnly
		self.default = default

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the text edit
		tStyle = wx.TE_LEFT
		if self.readOnly:
			tStyle = tStyle | wx.TE_READONLY
		controlID = wx.NewId()
		self.control = wx.TextCtrl( ctrlParent, controlID, size=(80,-1),
							style = tStyle )
		if self.default != None:
			self.setValue( self.default )

		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
		wx.EVT_TEXT_ENTER( self.control, controlID, self.onTextEnter )
		wx.EVT_KILL_FOCUS( self.control, self.onTextEnter )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onTextEnter( self, event ):
		self.postSetCommand()

	# ------------------------------

	def setValue( self, val ):
		self.control.SetValue( val )
		self.recordValue()

	def getValue( self ):
		return "'" + self.control.GetValue() + "'"

	def updateValue( self, strVal ):
		# Remove trailing and leading quotation marks
		# so that the stringvalue is a proper string
		strVal = strVal.strip("\'\"")
		self.setValue( strVal )


# -----------------------------------------------------------------------------


class StaticText( Text ):
	def __init__( self, name, default = None, updateCommand = ""):
		Text.__init__( self, name, default = default, updateCommand = updateCommand, readOnly = True )


# -----------------------------------------------------------------------------


class Int( Arg ):
	def __init__( self, name, default = 0, updateCommand = "",
					setCommand = "", minMax = (0, 0) ):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.minMax = minMax
		self.default = default

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the text edit
		isBounded = self.minMax[0] != self.minMax[1]
		controlID = wx.NewId()
		self.control = NumericCtrl( ctrlParent, controlID, (80,-1),
									limits = self.minMax,
									useLimits = isBounded )
		if self.default:
			self.setValue( str(self.default) )

		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
		wx.EVT_TEXT_ENTER( self.control, controlID, self.onTextEnter )
		wx.EVT_KILL_FOCUS( self.control, self.onKillFocus )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onTextEnter( self, event ):
		self.postSetCommand()

	def onKillFocus( self, event ):
		self.onTextEnter( event )
		self.control.onKillFocus( event )

	# ------------------------------

	def setValue( self, val ):
		self.control.setValue( val )
		self.recordValue()

	def getValue( self ):
		return self.control.getValue()

	def updateValue( self, strVal ):
		self.setValue( strVal )


# -----------------------------------------------------------------------------


class Float( Arg ):
	def __init__( self, name, default = 0.0, updateCommand = "",
					setCommand = "", minMax = (0, 0) ):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.minMax = minMax
		self.default = default

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the text edit
		isBounded = self.minMax[0] != self.minMax[1]
		controlID = wx.NewId()
		self.control = NumericCtrl( ctrlParent, controlID, (80,-1), True,
									limits = self.minMax,
									useLimits = isBounded )
		if self.default:
			self.setValue( str(self.default) )
		self.box.Add( self.control, 1, wx. ALIGN_CENTRE | wx.ALL, 5 )
		wx.EVT_TEXT_ENTER( self.control, controlID, self.onTextEnter )
		wx.EVT_KILL_FOCUS( self.control, self.onKillFocus )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onTextEnter( self, event ):
		self.postSetCommand()

	def onKillFocus( self, event ):
		self.onTextEnter( event )
		self.control.onKillFocus( event )

	# ------------------------------

	def setValue( self, val ):
		self.control.setValue( val )
		self.recordValue()

	def getValue( self ):
		return self.control.getValue()

	def updateValue( self, strVal ):
		self.setValue( strVal )

# -----------------------------------------------------------------------------


class List( Arg ):
	def __init__( self, name, choicesList, default = None, updateCommand = "", fillCommand = ""):
		Arg.__init__( self, name, updateCommand, "" )
		self.choicesList = choicesList
		self.fillCommand = fillCommand.strip()
		self.default = default

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( parent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the combo
		self.control = wx.ComboBox( parent, wx.NewId(), size=(80,-1),
							choices = self.choicesList,
							style = wx.CB_DROPDOWN | wx.CB_READONLY )
		if self.default != None:
			self.setValue( self.default )

		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )

		return self.box

	# ------------------------------

	def setValue( self, val ):
		# this function takes the key value
		self.control.SetValue( val )
		self.recordValue()

	def getValue( self ):
		val = self.control.GetValue()
		if type( val ) == str:
			val = "\"" + str( val ) + "\""
		return val

	# ------------------------------

	def updateValue( self, val ):
		self.setValue( val )

	# ------------------------------

	def fill( self ):
		if self.fillCommand == "":
			return

		commandLines = self.fillCommand.split( "\n" )
		numberLines = len( commandLines )
		resopnse = ""
		if numberLines > 1:
			for i in range( numberLines - 1):
				self.comms.post( commandLines[i] )
			response = self.comms.ask( "cPickle.dumps( " + commandLines[ numberLines - 1 ] + " )" )
		else:
			response = self.comms.ask( "cPickle.dumps( " + self.fillCommand + " )" )

		if response != "":
			response = response.strip( "\"" )
			response = response.replace( "\\n", "\n" )
			valList = cPickle.loads( response )
			self.control.Clear()
			for v in valList:
				self.control.Append( v )


# -----------------------------------------------------------------------------


class Enum( Arg ):
	def __init__( self, name, choiceTuples, updateCommand = "", setCommand = ""):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.choiceTuples = choiceTuples	# this is a list of tuples

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the combo
		userChoices = []
		for pair in self.choiceTuples:
			userChoices.append( pair[0] )
		comboID = wx.NewId()
		self.control = wx.ComboBox( ctrlParent, comboID, size=(80,-1),
							choices = userChoices,
							style = wx.CB_DROPDOWN | wx.CB_READONLY )
		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
		wx.EVT_COMBOBOX( ctrlParent, comboID, self.onEvent )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onEvent( self, event ):
		self.postSetCommand()

	# ------------------------------

	def setValue( self, val ):
		# this function takes the key value
		self.control.SetValue( val )
		self.recordValue()

	def getValue( self ):
		# this function returns the actual value, bit confusing
		curValue = self.control.GetValue()
		for pair in self.choiceTuples:
			if pair[0] == curValue:
				val = pair[1]
				if type( val ) == str:
					val = "\"" + str( val ) + "\""
				return val
		# should never get here
		return None

	# ------------------------------

	def updateValue( self, strVal ):
		# type sensitive comparison
		# assume the items are of correct type
		if type( eval(strVal) ) == type( 1.0 ):
			# print "float"
			for item in self.choiceTuples:
				if item[1] == float( strVal ):
					self.setValue( item[ 0 ] )
					break
		elif type( eval(strVal) ) == type( 1 ):
			# print "int"
			for item in self.choiceTuples:
				if item[1] == int( strVal ):
					self.setValue( item[ 0 ] )
					break
		else:
			# print "string"
			for item in self.choiceTuples:
				if str( item[1] ) == strVal:
					self.setValue( item[ 0 ] )
					break


# -----------------------------------------------------------------------------


class Bool( Arg ):
	def __init__( self, name, updateCommand = ""):
		Arg.__init__( self, name, updateCommand, "" )
		self.choicesDict = { "False" : 0, "True" : 1 }

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( parent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the combo
		self.control = wx.ComboBox( parent, wx.NewId(), size=(80,-1),
							choices = self.choicesDict.keys(),
							style = wx.CB_DROPDOWN | wx.CB_READONLY | 
								wx.CB_SORT )
		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )

		return self.box

	# ------------------------------

	def setValue( self, val ):
		# this function takes the key value
		self.control.SetValue( val )
		self.recordValue()

	def getValue( self ):
		# this function returns the actual value, bit confusing
		return self.choicesDict[ self.control.GetValue() ]

	# ------------------------------

	def updateValue( self, strVal ):
		choiceItems = self.choicesDict.items()
		for item in choiceItems:
			if str( item[1] ) == strVal:
				self.setValue( item[ 0 ] )
				break


# -----------------------------------------------------------------------------


class CheckBox( Arg ):
	def __init__( self, name, updateCommand, setCommand ):
		Arg.__init__( self, name, updateCommand, setCommand )

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the checkbox
		checkID = wx.NewId()
		self.control = wx.CheckBox( ctrlParent, checkID, self.getNameGUI() )
		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )
		wx.EVT_CHECKBOX( ctrlParent, checkID, self.onClick )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onClick( self, event ):
		self.postSetCommand()

	# ------------------------------

	def setValue( self, val ):
                val = eval( val )
                if val == "true":
                  val = 1
                elif val != "True":
                  val = 0
		if type( val ) is str:
			val = eval( val )
		self.control.SetValue( int( val ) )
		self.recordValue()

	def getValue( self ):
		return self.control.GetValue()

	def updateValue( self, strVal ):
		self.setValue( strVal )


# -----------------------------------------------------------------------------


class IntSlider( Arg ):
	def __init__( self, name, updateCommand, setCommand, minMax ):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.minValue = minMax[0]
		self.maxValue = minMax[1]

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the slider
		self.slider = wx.Slider( ctrlParent, wx.NewId(),
									self.maxValue - self.minValue,
									self.minValue,
									self.maxValue )
		self.box.Add( self.slider, 1, wx.ALIGN_CENTRE | wx.ALL )
		wx.EVT_SCROLL( ctrlParent, self.onScroll )

		# the control entry
		textID = wx.NewId()
		self.control = NumericCtrl( ctrlParent, textID, (40,-1),
								limits = (self.minValue, self.maxValue),
								useLimits = True )
		self.box.Add( self.control, 0, wx.ALIGN_CENTRE | wx.ALL )
		wx.EVT_TEXT_ENTER( self.control, textID, self.onTextEnter )
		wx.EVT_KILL_FOCUS( self.control, self.onKillFocus )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def onTextEnter( self, event ):
		self.slider.SetValue( int( self.control.GetValue() ) )
		self.postSetCommand()

	def onKillFocus( self, event ):
		self.onTextEnter( event )
		self.control.onKillFocus( event )

	def onScroll( self, event ):
		self.control.setValue( str( self.slider.GetValue() ) )
		self.onKillFocus( event )
		self.postSetCommand()

	# ------------------------------

	def setValue( self, val ):
		if type(val) == str:
			val = val.strip("\'\"")

		try:
			floatVal = float(val)
		except ValueError:
			print "The slider could not be set to invalid value:", str(val)
			floatVal = 0
		
		self.slider.SetValue( int( floatVal ) )
		self.control.setValue( floatVal )
		self.recordValue()

	def getValue( self ):
		return self.slider.GetValue()

	def updateValue( self, strVal ):
		self.setValue( strVal )


# -----------------------------------------------------------------------------


class FloatSlider( Arg ):
	def __init__( self, name, updateCommand, setCommand, minMax ):
		Arg.__init__( self, name, updateCommand, setCommand )
		self.minValue = minMax[0]
		self.maxValue = minMax[1]
		self.multi = 100.0
		self.minValueSlider = int( minMax[0] * self.multi )
		self.maxValueSlider = int( minMax[1] * self.multi )

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )

		self.panel = wx.Panel( parent, -1, size = (100, 28) )
		ctrlParent = self.panel
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the label
		self.label = wx.StaticText( ctrlParent, -1, self.getNameGUI() )
		self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

		# the slider
		self.slider = wx.Slider( ctrlParent, wx.NewId(),
									int(self.maxValueSlider - 
										self.minValueSlider),
									self.minValueSlider,
									self.maxValueSlider )
		self.box.Add( self.slider, 1, wx.ALIGN_CENTRE | wx.ALL )
		wx.EVT_SCROLL( ctrlParent, self.onScroll )

		# the control entry
		textID = wx.NewId()
		self.control = NumericCtrl( ctrlParent, textID, (40,-1),
								limits = (self.minValue, self.maxValue),
								useLimits = True, isFloat = True )
		self.box.Add( self.control, 0, wx.ALIGN_CENTRE | wx.ALL )
		wx.EVT_TEXT_ENTER( self.control, textID, self.onTextEnter )
		wx.EVT_KILL_FOCUS( self.control, self.onKillFocus )

		self.panel.SetSizer( self.box )
		return self.panel

	# ------------------------------

	def getSliderValue( self ):
		return float(self.slider.GetValue()) / float(self.multi)

	def setSliderValue( self, val ):
		if type(val) == str:
			val = val.strip("\'\"")

		try:
			fVal = float( val ) * self.multi
		except ValueError:
			print "unable to retrieve float value for ", self.name 
			fVal = 0
		self.slider.SetValue( int( fVal ) )

	# ------------------------------

	def onTextEnter( self, event ):
		self.setSliderValue( self.control.getValue() )
		self.postSetCommand()

	def onKillFocus( self, event ):
		self.onTextEnter( event )
		self.control.onKillFocus( event )

	def onScroll( self, event ):
		self.control.setValue( str( self.getSliderValue() ) )
		self.postSetCommand()

	# ------------------------------

	def setValue( self, val ):
		self.setSliderValue(val)
		self.control.setValue( eval(val) )
		self.recordValue()

	def getValue( self ):
		return self.getSliderValue()

	def updateValue( self, strVal ):
		self.setValue( strVal )


# -----------------------------------------------------------------------------


class Divider( Arg ):
	def __init__( self, name = "" ):
		Arg.__init__( self, name, "", "" )

	# ------------------------------

	def createControl( self, parent, comms ):
		Arg.createControl( self, comms )
		self.box = wx.BoxSizer( wx.HORIZONTAL )

		# the line
		self.control = wx.StaticLine( parent, -1, size=(20,-1),
							style=wx.LI_HORIZONTAL )
		self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )

		if len( self.name ) > 0:
			# the label
			self.label = wx.StaticText( parent, -1, self.getNameGUI() )
			self.box.Add( self.label, 0, wx.ALIGN_CENTRE | wx.ALL, 5 )

			# the line
			self.control = wx.StaticLine( parent, -1, size=(20,-1),
								style = wx.LI_HORIZONTAL )
			self.box.Add( self.control, 1, wx.ALIGN_CENTRE | wx.ALL, 5 )

		return self.box


# -----------------------------------------------------------------------------
# Specialisations for watchers


class WatcherText( Text ):
	def __init__( self, name, watcherName ):
		name = name.replace( " ", "_" )
		updateCommand = "BigWorld.getWatcher(\"" + watcherName + "\")"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", str(" + name + "))"
		Text.__init__( self, name, updateCommand = updateCommand,
						setCommand = setCommand )

class WatcherStaticText( StaticText ):
	def __init__( self, name, watcherName ):
		name = name.replace( " ", "_" )
		updateCommand = "print BigWorld.getWatcher(\"" + watcherName + "\")"
		StaticText.__init__( self, name, updateCommand = updateCommand )

class WatcherInt( Int ):
	def __init__( self, name, watcherName, minMax = (0, 0) ):
		name = name.replace( " ", "_" )
		updateCommand = "int( BigWorld.getWatcher(\"" + watcherName + "\") )"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", str(" + name + ") + \"f\")"
		Int.__init__( self, name, updateCommand = updateCommand,
						setCommand = setCommand, minMax = minMax )


class WatcherFloat( Float ):
	def __init__( self, name, watcherName, minMax = (0, 0) ):
		name = name.replace( " ", "_" )
		updateCommand = "float( BigWorld.getWatcher(\"" + watcherName + "\") )"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", str(" + name + ") + \"f\")"
		Float.__init__( self, name, updateCommand = updateCommand,
						setCommand = setCommand, minMax = minMax )


class WatcherFloatEnum( Enum ):
	def __init__( self, name, watcherName, nameValueTuples ):
		name = name.replace( " ", "_" )
		updateCommand = "float( BigWorld.getWatcher(\"" + watcherName + "\") )"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", float(" + name + "))"
		Enum.__init__( self, name, nameValueTuples, updateCommand = updateCommand,
						setCommand = setCommand )



class WatcherCheckBox( CheckBox ):
	def __init__( self, name, watcherName ):
		name = name.replace( " ", "_" )
		updateCommand = "BigWorld.getWatcher(\"" + watcherName + "\")"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", float(" + name + "))"
		CheckBox.__init__( self, name, updateCommand, setCommand )


class WatcherIntSlider( IntSlider ):
	def __init__( self, name, watcherName, minMax ):
		name = name.replace( " ", "_" )
		updateCommand = "BigWorld.getWatcher(\"" + watcherName + "\")"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", str(" + name + ") + \"f\")"
		IntSlider.__init__( self, name, updateCommand, setCommand, minMax )


class WatcherFloatSlider( FloatSlider ):
	def __init__( self, name, watcherName, minMax ):
		name = name.replace( " ", "_" )
		updateCommand = "BigWorld.getWatcher(\"" + watcherName + "\")"
		setCommand = "BigWorld.setWatcher(\"" + watcherName + "\", str(" + name + ") + \"f\")"
		FloatSlider.__init__( self, name, updateCommand, setCommand, minMax )


#__controls.py
