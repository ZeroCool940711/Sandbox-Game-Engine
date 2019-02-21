/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"

#include "pyscript/script.hpp"
#include "console_manager.hpp"
#include "xconsole.hpp"
#include "geometrics.hpp"

#ifndef CODE_INLINE
#include "console_manager.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

ConsoleManager* ConsoleManager::s_instance_ = NULL;


// -----------------------------------------------------------------------------
// Section: ConsoleManager
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ConsoleManager::ConsoleManager() : 
	pActiveConsole_( NULL ),
	darkenBackground_(true)
{
}


/**
 *	Destructor.
 */
ConsoleManager::~ConsoleManager()
{
	fini();
}

/**
 *	Free allocated data.
 */
void ConsoleManager::fini()
{
	// TODO:PM We may need to be careful about consoles that are added twice
	// with different key bindings.

	ConsoleSet::iterator iter = consoleSet_.begin();

	while (iter != consoleSet_.end())
	{
		delete *iter;

		iter++;
	}
	consoleSet_.clear();
}


/**
 *	Create instance of this object.
 */
/*static*/ void ConsoleManager::createInstance()
{
	if (s_instance_ == NULL)
	{
		s_instance_ = new ConsoleManager();
	}
}


/**
 *	Destroy the instance of this object.
 */
/*static*/ void ConsoleManager::deleteInstance()
{
	delete s_instance_;
	s_instance_ = NULL;
}


/**
 *	This method adds a console to be managed. The console manager owns the input
 *	console and is responsible for deleting it.
 *
 *	@param pConsole		The console to be managed.
 *	@param label		The label to associate with the console.
 *	@param key			The keyboard key to associate with the console.
 *	@param modifiers	The modifiers that must be pressed with 'key'.
 */
void ConsoleManager::add( XConsole * pConsole,
		const char * label,
		KeyEvent::Key key,
		uint32 modifiers )
{
	if (key != KeyEvent::KEY_NONE)
	{
		keyedConsoles_[ key | (modifiers << 24) ] = pConsole;
	}

	if (label != NULL)
	{
		labelledConsoles_[ label ] = pConsole;
	}

	// Consoles should currently only be added once.
	MF_ASSERT( consoleSet_.find( pConsole ) == consoleSet_.end() );

	consoleSet_.insert( pConsole );

	pConsole->init();
}


/**
 *	This method activates the console with the associated label.
 */
void ConsoleManager::activate( const char * label )
{
	LabelledConsoles::iterator iter = labelledConsoles_.find( label );

	MF_ASSERT( iter != labelledConsoles_.end() );

	if (iter != labelledConsoles_.end())
	{
		if (pActiveConsole_ != iter->second )
		{
			this->deactivate();
			this->activate( iter->second );
			pActiveConsole_->activate( false );
		}
	}
}


/**
 *	This method toggles the activation of the console with the associated
 *	label.
 */
void ConsoleManager::toggle( const char * label )
{
	LabelledConsoles::iterator iter = labelledConsoles_.find( label );

	MF_ASSERT( iter != labelledConsoles_.end() );

	if (iter != labelledConsoles_.end())
	{
		if (pActiveConsole_ == iter->second )
		{
			pActiveConsole_->activate( true );
			this->deactivate();
		}
		else
		{
			this->deactivate();
			this->activate( iter->second );
			pActiveConsole_->activate( pActiveConsole_ != iter->second );
		}
	}
}


/**
 *	This method draws the active console, if necessary.
 */
void ConsoleManager::draw( float dTime )
{
	if (pActiveConsole_ != NULL)
	{
		if (this->darkenBackground() && pActiveConsole_->allowDarkBackground())
		{
			Geometrics::drawRect( 
				Vector2::zero(), 
				Vector2( Moo::rc().screenWidth(), Moo::rc().screenHeight() ),
				0xc0202020 );
		}
		pActiveConsole_->draw( dTime );
	}
}


/**
 *	This method returns the console associated with the input label.
 */
XConsole * ConsoleManager::find( const char * name ) const
{
	LabelledConsoles::const_iterator iter = labelledConsoles_.find( name );

	return iter != labelledConsoles_.end() ? iter->second : NULL;
}

/**
 *	Given a console, this determines the name of that console. Returns "" if not found.
 */
const std::string & ConsoleManager::consoleName( XConsole * console ) const
{
	for (LabelledConsoles::const_iterator iter = labelledConsoles_.begin();
		iter != labelledConsoles_.end(); iter++)
	{
		if (iter->second == console)
		{
			return iter->first;
		}
	}

	static std::string s_emptyString = "";
	return s_emptyString;
}


// -----------------------------------------------------------------------------
// Section: InputHandler overrides
// -----------------------------------------------------------------------------

/**
 *	This method handles key events.
 */
bool ConsoleManager::handleKeyEvent( const KeyEvent & event )
{
	bool handled = false;

	KeyedConsoles::iterator iter =
		keyedConsoles_.find( event.key() | (event.modifiers() << 24) );

	if (iter != keyedConsoles_.end())
	{
		if (event.isKeyDown())
		{
			bool isReactivate = (iter->second == pActiveConsole_);

			if( !isReactivate )
				this->deactivate();

			pActiveConsole_ = iter->second;

			pActiveConsole_->activate( isReactivate );
		}

		handled = true;
	}
	else if (pActiveConsole_ != NULL)
	{
		InputHandler * pHandler = pActiveConsole_->asInputHandler();

		if (pHandler)
		{
			handled = pHandler->handleKeyEvent( event );
		}

		if (!handled &&
			event.key() == KeyEvent::KEY_ESCAPE &&
			event.isKeyDown())
		{
			this->deactivate();
			handled = true;
		}
	}

	return handled;
}


/**
 *	This method handles axis events.
 */
bool ConsoleManager::handleAxisEvent( const AxisEvent & event )
{
	bool handled = false;

	if (pActiveConsole_ != NULL)
	{
		InputHandler * pHandler = pActiveConsole_->asInputHandler();

		if (pHandler)
		{
			handled = pHandler->handleAxisEvent( event );
		}
	}

	return handled;
}


/**
 *	This method clears the active console.
 */
void ConsoleManager::deactivate()
{
	if (pActiveConsole_ != NULL) 
	{
		pActiveConsole_->deactivate();
		pActiveConsole_ = NULL;
	}
}


/**
 *	Returns true if the a dark background is to be 
 *	drawn underneat the current active console.
 */
bool ConsoleManager::darkenBackground() const
{
	return this->darkenBackground_;
}


/**
 *	Sets wether a dark background is to be 
 *	drawn underneat the current active console.
 */
void ConsoleManager::setDarkenBackground(bool enable)
{
	this->darkenBackground_ = enable;
}


/*~	function BigWorld.darkenConsoleBackground
 *
 *	Sets the dark-console-background flag. When true, a dark background will be 
 *	rendered underneat the current active console (if any), making the console 
 *	text easier to read. If false, console text will be rendered directly over
 *	the current contents of the color buffer. Some console may inhibit the 
 *	renderting of the dark background (e.g. the histogram console).
 *	
 *	@param	enable	(optional) new value to be assigned to the 
 *					dark-console-background flag.
 *
 *	@return			the current value (before the call is processed).
 *
 */
PyObject * py_darkenConsoleBackground(PyObject * args)
{
	bool currentValue = ConsoleManager::instance().darkenBackground();
	if (PyTuple_Size(args) > 0)
	{
		bool enable = false;
		if (!PyArg_ParseTuple(args, "b", &enable))
		{
			PyErr_SetString( PyExc_TypeError, "BigWorld.darkenConsoleBackground: "
				"Argument parsing error: Expected a boolean" );
			return NULL;
		}
		ConsoleManager::instance().setDarkenBackground(enable);
	}
	return Script::getData(currentValue);
}
PY_MODULE_FUNCTION( darkenConsoleBackground, BigWorld )

// console_manager.cpp
