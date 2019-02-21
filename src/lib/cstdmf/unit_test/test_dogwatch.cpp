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

#include "cstdmf/cstdmf.hpp"

#if ( ENABLE_DOG_WATCHERS )

const char * TEST_WATCH_NAME = "Test Dogwatch";

// Do something that will take time but not get optimised out.
uint32 ShortDelay()
{
	static volatile uint32 value = 1;
	for ( uint32 i = 0; i < 1000; i++ )
		value += i;
	return value;
}

namespace
{
struct Fixture
{
	Fixture()
	{
		new CStdMf;
		watch_ = new DogWatch( TEST_WATCH_NAME );
	}

	~Fixture()
	{
		delete watch_;
		watch_ = NULL;
		delete CStdMf::pInstance();
	}

	DogWatch* watch_;
};


TEST_F( Fixture, DogWatch_testStart )
{	
	CHECK_EQUAL( TEST_WATCH_NAME, watch_->title() );

	// Should be able to call start a couple of times with no problems
	watch_->start();
	watch_->start();

	// It will be deleted here without being stopped, that should be ok too.
}

TEST_F( Fixture, DogWatch_testRead )
{
	// Should be able to read without it being started
	uint64 value = watch_->slice();

	// It should have value 0
	CHECK_EQUAL( 0UL, value );

	// Start and read
	watch_->start();
	ShortDelay();

	// It should should still be zero before stopping.
	value = watch_->slice();
	CHECK_EQUAL( 0UL, value );
}

TEST_F( Fixture, DogWatch_testStop )
{
	// Test normal stop
	watch_->start();
	ShortDelay();
	watch_->stop();

	// should be able to read a stopped dogwatch
	uint64 value = watch_->slice();

	// Test for non-zero value
	CHECK( value > 0 );
}
} // namespace

#endif // ENABLE_DOG_WATCHERS
