CAT - Client Access Tool
-------------------------------------------------------------------------------

CAT provides a graphical interface to change watcher values and run Python
console commands on a remote or local client. It does so via the Remote 
Console service provided by the client.

How To Use
-------------------------------------------------------------------------------
- You need Python 2.5 and wxPython installed.
- Add the Python 2.5 installation directory to the PATH environment variable
  if needed.
- Open the client, and then CAT. 
- Input the client's computer name and IP address into CAT and click OK.

To make a new script
-------------------------------------------------------------------------------
0.
  The script must live in the directory bigworldtech/bigworld/tools/cat/scripts 
  or a subdirectory.Scripts in subdirectories are put in a separate branch in 
  the tree control. Note the each script directory must contain an empty file 
  named: "__init__.py".

1.
  Insert this line at the of the script:
  from controls import *

2.
  Define variables though define a tuple that contains instances of CC* classes.
  - Each class must have a name parameter. 
    This name must be unique to this script.  The name parameter refers to the
    value currently held by the control.
  - Each class can also have a 'updateCommand' parameter.  
    This is a command that is run on the client python server to obtain the
    current value of the parameter, the return value is interpreted by each
    CCArg class.  The return value type must match the type used in the class.
    
  Classes available are:
  - CCText
  - CCStaticText
  - CCInt: 
    optional argument 'maxMin' If the maxMin tuple does not contain identical
    values, these boundary conditions are observer for values entered into the 
    control.
  - CCFloat: 
    optional argument 'maxMin' If the maxMin tuple does not contain identical 
    values, these boundary conditions are observer for values entered into the 
    control.
  - CCList: 
    optional argument 'updateFillsList'.  If true, the list box will be filled
    with the list of values returned by the updateCommand parameter.
    (usually updateCommand sets the current value)
  - CCEnum: 
    required argument 'choiceTuples', the first element being the string to 
    show in the list box, the second element being what is used in the when
    called by the command discussed in (3).
  - CCBool
  - CCCheckBox: 
    required argument 'setCommand', this command is sent to the python server 
    whenever the checkbox selection has changed.  Refer to the value of the 
    checkbox via its name.
  - CCIntSlider: 
    required argument 'setCommand', this command is sent to the python server 
    whenever the slider position changes.  Refer to the value of the slider 
    via its name.
  
3.
  Create a list of commands.  Each command is instantiated as a button.
  The commands are a list of tuples of strings.  The first element in the 
  tuple is the name of the button, the second element in the tuple is the 
  python command.
  
Enjoy!
