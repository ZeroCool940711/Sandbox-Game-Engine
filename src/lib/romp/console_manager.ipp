/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// console_manager.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

/**
 *	This method returns the current active console. If no console is active,
 *	NULL is returned.
 */
INLINE XConsole * ConsoleManager::pActiveConsole() const
{
	return pActiveConsole_;
}


/**
 *	This method sets the current active console.
 */
INLINE void ConsoleManager::activate( XConsole * pConsole )
{
	pActiveConsole_ = pConsole;
}


/**
 *	This static method returns the singleton instance of this class.
 */
INLINE ConsoleManager & ConsoleManager::instance()
{
	MF_ASSERT(s_instance_);
	return *s_instance_;
}

// console_manager.ipp
