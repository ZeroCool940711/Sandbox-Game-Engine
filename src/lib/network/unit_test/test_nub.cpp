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

#include "network/nub.hpp"

namespace
{

struct Fixture
{
	Fixture()
	{
		pNub_ = new Mercury::Nub();
	}

	~Fixture()
	{
		delete pNub_;
		pNub_ = NULL;
	}

	Mercury::Nub* pNub_;
};

TEST_F( Fixture, Nub_testConstruction )
{
	// Test that it has correctly opened a socket.
	CHECK( pNub_->socket() != -1 );
}

};

// test_nub.cpp
