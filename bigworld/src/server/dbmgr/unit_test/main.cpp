/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "unit_test_lib/unit_test.hpp"
#include "cstdmf/memory_tracker.hpp"

int main( int argc, char* argv[] )
{
#ifdef ENABLE_MEMTRACKER
	MemTracker::instance().setReportOnExit( true );
	MemTracker::instance().setCrashOnLeak( true );
#endif
	return BWUnitTest::runTest( "dbmgr", argc, argv );
}

// main.cpp
