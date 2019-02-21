/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 *	cooperative_moo.cpp
 *	Class to help make Moo windowed apps cooperative with other DX apps
 */

#include "pch.hpp"
#include "cooperative_moo.hpp"
#include "moo/render_context.hpp"
#include "resmgr/bwresource.hpp"
#include <Psapi.h>
#include <shlwapi.h>


// Statics

/// Guard to prevent calls before the class is inited.
/*static*/ bool CooperativeMoo::s_inited_ = false;

/// Flag used in begin/endPaint to know if we have temporarily acquired DX.
/*static*/ bool CooperativeMoo::s_wasPaused_ = false;

/// Variable to store the current mode of operation
/*static*/ CooperativeMoo::MODE CooperativeMoo::s_mode_ =
													CooperativeMoo::AUTO;

/// This is used to know if there are other DX apps running.
/*static*/ bool CooperativeMoo::s_otherAppsRunning_ = false;

/// This is used check for other processes every N milliseconds, instead of
/// every frame, because it's a bit expensive.
static const uint64 ENUM_PROCESSES_INTERVAL_MS = 1000; // ms
/*static*/ uint64 CooperativeMoo::s_lastCheckTime_ = 0;

/// This variable caches the current exe's name.
/*static*/ std::string CooperativeMoo::s_thisAppName_;

/// This vector contains the list of apps we want to cooperate with.
/*static*/ std::vector< std::string > CooperativeMoo::s_otherApps_;


// Methods


/**
 *	This method initialises the defaults, and must be called before using it.
 *
 *	@param pConfigSection	Optional data section to read the defaults from.
 *	@return	True if initialisation was successful, false otherwise.
 */
/*static*/ bool CooperativeMoo::init( DataSectionPtr pConfigSection /*=NULL*/ )
{
	MF_ASSERT( !s_inited_ );

	// Get the current app exe name.
	char thisAppFile[MAX_PATH];
	memset( thisAppFile, 0, sizeof( thisAppFile ) );
	GetModuleFileName( NULL, thisAppFile, MAX_PATH );
	s_thisAppName_ = BWResource::getFilename( thisAppFile );

	if (!pConfigSection)
	{
		// If pConfigSection is NULL, read the default config file expected.
		char configFile[MAX_PATH];
		char resolvedName[MAX_PATH];
		::GetCurrentDirectory( sizeof( configFile ), configFile );
		strcat( configFile, "\\..\\misc\\common_options.xml" );
		if ( !PathCanonicalize( resolvedName, configFile) )
		{
			strcpy( resolvedName, configFile);
			ERROR_MSG("Could not find canonical path for common_options.xml\n");
		}
		DataSectionPtr pCommonSection = BWResource::openSection( resolvedName );
		if (pCommonSection)
		{
			pConfigSection = pCommonSection->openSection( "cooperative" );
		}
	}

	// Read the configuration info if possible.
	bool addDefaultApps = false;
	if (pConfigSection)
	{
		std::string sectionMode = pConfigSection->readString( "mode", "AUTO" );
		if (sectionMode == "AUTO")
		{
			s_mode_ = CooperativeMoo::AUTO;
		}
		else if (sectionMode == "ON")
		{
			s_mode_ = CooperativeMoo::ON;
		}
		else if (sectionMode == "OFF")
		{
			s_mode_ = CooperativeMoo::OFF;
		}
		else
		{
			ERROR_MSG( "CooperativeMoo::init: Error reading file %s, mode '%s'"
				" is not a valid mode. Only AUTO, ON or OFF are valid.\n",
				pConfigSection->sectionName().c_str(), sectionMode.c_str() );
		}

		DataSectionPtr pAppsSection = pConfigSection->openSection( "apps" );
		if (pAppsSection)
		{
			// Read apps we want to cooperate with.
			std::vector< DataSectionPtr > sections;
			pAppsSection->openSections( "app", sections );
			for (std::vector< DataSectionPtr >::iterator it = sections.begin();
				it != sections.end(); ++it)
			{
				s_otherApps_.push_back( (*it)->asString() );
			}
		}
		else if (s_mode_ == CooperativeMoo::AUTO)
		{
			// No <apps> section, and mode is Auto, so complain!
			ERROR_MSG( "CooperativeMoo::init: No <app> sections inside "
				"<apps>, couldn't read application names we need to "
				"cooperate with. Using defaults.\n" );
			addDefaultApps = true;
		}
	}
	else
	{
		// No config file found, so complain and say that we are using the
		// default values.
		ERROR_MSG( "CooperativeMoo::init: No configuration file found. Using "
			"defaults.\n" );
		addDefaultApps = true;
	}

	if (addDefaultApps)
	{
		// Use these known apps as the default if non specified.
		s_otherApps_.push_back( "worldeditor*.exe" );
		s_otherApps_.push_back( "modeleditor*.exe" );
		s_otherApps_.push_back( "particleeditor*.exe" );
		s_otherApps_.push_back( "3dsmax.exe" );
		s_otherApps_.push_back( "maya.exe" );
	}

	s_inited_ = true;
	return true;
}


/**
 *	This method sets the current mode.
 *
 *	@param newMode	The new cooperative mode to use.
 */
/*static*/ void CooperativeMoo::mode( CooperativeMoo::MODE newMode )
{
	s_mode_ = newMode;
}


/**
 *	This method gets the current mode.
 *
 *	@return The current cooperative mode.
 */
/*static*/ CooperativeMoo::MODE CooperativeMoo::mode()
{
	return s_mode_;
}


/**
 *	This method checks the running processes and updates the internal state of
 *	the class if there are any apps that we need to cooperate running or not.
 */
/*static*/ void CooperativeMoo::tick()
{
	MF_ASSERT( s_inited_ );

	if (s_mode_ == CooperativeMoo::ON ||
		s_mode_ == CooperativeMoo::OFF)
	{
		// No need to check, we already know what to do (not auto mode).
		return;
	}

	uint64 intervalInStamps = ENUM_PROCESSES_INTERVAL_MS * stampsPerSecond() / 1000;
	if (timestamp() - s_lastCheckTime_ > intervalInStamps)
	{
		s_lastCheckTime_ = timestamp();
		DWORD procs[1024];
		DWORD procSizeReturned = 0;

		// Get process list
		if (!EnumProcesses( procs, sizeof( procs ), &procSizeReturned ))
		{
			const char * errMsg = "CooperativeMoo::tick: Failed to detect\n";
			MF_ASSERT_DEBUG( 0 && errMsg );
			ERROR_MSG( errMsg );
			return;
		}

		// Check if there are any apps we want to cooperate with in the process
		// list, skipping our process name.
		s_otherAppsRunning_ = false;

		uint32 numProcs = procSizeReturned / sizeof( DWORD );
		for (uint32 i = 0; i < numProcs; ++i)
		{
			if (procs[i] != 0)
			{
				HANDLE procHandle = OpenProcess(
									PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
									0,
									procs[i] );

				char procName[256];
				memset( procName, 0, sizeof( procName ) );
				GetModuleBaseName( procHandle, 0, procName, 256 );
				CloseHandle( procHandle );

				if (strcmp( s_thisAppName_.c_str(), procName ) != 0 &&
					isCooperativeApp( procName ))
				{
					s_otherAppsRunning_ = true;
					break;
				}
			}
		}
	}

	if (!s_otherAppsRunning_)
	{
		// No other apps running that we care about, so get resources back.
		if (Moo::rc().paused())
		{
			Moo::rc().resume();
		}
	}
}


/**
 *	This method gets called before responding to a WM_PAINT message.
 *
 *	@return True if it could gather back the DX resources, false otherwise.
 */
/*static*/ bool CooperativeMoo::beginOnPaint()
{
	tick();
	if (!needsToCooperate())
	{
		return true;
	}
	
	if (AfxGetMainWnd()->IsIconic())
	{
		return false;
	}
	
	s_wasPaused_ = Moo::rc().paused();
	if (s_wasPaused_)
	{
		Moo::rc().resume();
	}

	return !Moo::rc().paused();
}


/**
 *	This method gets called while responding to a WM_PAINT message, after
 *	rendering, and must be called only if beginOnPaint returned true.
 */
/*static*/ void CooperativeMoo::endOnPaint()
{
	if (!needsToCooperate())
	{
		return;
	}

	if (AfxGetMainWnd()->IsIconic())
	{
		return;
	}

	if (s_wasPaused_)
	{
		Moo::rc().pause();
	}
}


/**
 *	This method gets called before rendering in the main loop, to gather the
 *	DX resources if possible.
 *
 *	@param isWindowActive	Tells CooperativeMoo if the main win is active.
 *	@param minTextureMemMB	Minimum amount of free memory needed to render.
 *	@return		True if resources could be gathered and rendering is allowed.
 */
/*static*/ bool CooperativeMoo::canUseMoo(
								bool isWindowActive, uint32 minTextureMemMB )
{
	tick();
	if (AfxGetMainWnd()->IsIconic())
	{
		// Never allow rendering if minimised.
		deactivate();
		Sleep( 100 );
		return false;
	}

	if (!needsToCooperate())
	{
		// If we don't need to cooperate with others, allow rendering always.
		return true;
	}

	if (!isWindowActive)
	{
		// We need to cooperate, and the app is in the background, so stop.
		deactivate();
		Sleep( 100 );
		return false;
	}

	// Ok, so there's no reason not to render. Let's check if the device is
	// paused, and if so, try to gather it again.

	if (!Moo::rc().paused())
	{
		return true;
	}

	CWaitCursor wait;

	if (Moo::rc().checkDevice() &&
		Moo::rc().getAvailableTextureMem() <= minTextureMemMB * 1024*1024)
	{
		// not enough free video memory to resume the context, so fail
		deactivate();
		Sleep( 500 );
		return false;
	}
	Moo::rc().resume();

	return !Moo::rc().paused();
}


/**
 *	This method gets called if activate failed to ensure that DX resources are
 *	freed (some resources might have been gathered during activate).
 */
/*static*/ void CooperativeMoo::deactivate()
{
	if (!needsToCooperate())
	{
		return;
	}

	Moo::rc().pause();
}


/**
 *	This internal method checks the current states to see if we need to check
 *	it there are other apps running that we want to cooperate with.
 *
 *	@return		True if we need to cooperate, false to keep DX resources.
 */
/*static*/ bool CooperativeMoo::needsToCooperate()
{
	if (s_mode_ == CooperativeMoo::OFF ||
		(s_mode_ == CooperativeMoo::AUTO && !s_otherAppsRunning_))
	{
		return false;
	}

	return true;
}


/**
 *	This method looks receives a process name and looks it up in the internal
 *	apps vector too see if it's one of the apps we want to cooperate with.
 *
 *	@param procName		Process name of the app we want to check.
 *	@return		True if it's an app we need to cooperate with, false otherwise.
 */
/*static*/ bool CooperativeMoo::isCooperativeApp( const std::string & procName )
{
	for (std::vector< std::string >::iterator it = s_otherApps_.begin();
		it != s_otherApps_.end(); ++it)
	{
		if (PathMatchSpec( procName.c_str(), (*it).c_str() ))
		{
			return true;
		}
	}

	return false;
}
