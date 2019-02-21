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

#include "cstdmf/memory_tracker.hpp"
#include "unit_test_lib/unit_test.hpp"

int main( int argc, char* argv[] )
{
#ifdef ENABLE_MEMTRACKER
	MemTracker::instance().setCrashOnLeak( true );
#endif

	int ret = BWUnitTest::runTest( "cstdmf", argc, argv );

	DebugFilter::fini(); // prevent singleton leak.

	return ret;
}

// main.cpp
