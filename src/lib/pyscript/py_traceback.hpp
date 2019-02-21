/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PY_TRACEBACK_HPP
#define PY_TRACEBACK_HPP

#import "network/nub.hpp"

namespace Script
{
	void initExceptionHook( Mercury::Nub * pNub );
}

#endif // PERSONALITY_HPP
