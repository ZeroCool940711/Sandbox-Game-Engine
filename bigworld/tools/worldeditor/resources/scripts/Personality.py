import WorldEditor

# Personality is a way of collecting game specific information for each space.
# On save, the Personality module named in the worldeditor options.xml
# and correspondingly defined in res\entities\editor is called.
# e.g. the Personality module could record the locations of particular types
#      of entities or count the number of particular guards, any space related
#      information that the game may need


personalityModule = 0

def getPersonalityModule():
	global personalityModule

	if personalityModule != 0: return personalityModule

	personalityModule = None
	try:
		personalityName = WorldEditor.opts._personality.asString
	except:
		return

	personalityModule = __import__( personalityName )
	return personalityModule

def callPersonalityFunction( fnname ):
	try:
		fn = getattr( getPersonalityModule(), fnname )
	except:
		return
	fn()

def preFullSave():
	callPersonalityFunction( "preFullSave" )

def preQuickSave():
	callPersonalityFunction( "preQuickSave" )
