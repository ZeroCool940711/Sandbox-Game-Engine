/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifdef _MSC_VER 
#pragma once
#endif

#ifndef DYE_PROP_SETTING_HPP
#define DYE_PROP_SETTING_HPP


#include "math/vector4.hpp"


/**
 *	Inner class to represent a dye property setting
 */
class DyePropSetting
{
public:
	DyePropSetting() {}
	DyePropSetting( int i, const Vector4 & v ) : index_( i ), value_( v ) {}

	enum
	{
		PROP_TEXTURE_FACTOR,
		PROP_UV
	};

	int			index_;
	Vector4		value_;
};



#endif // DYE_PROP_SETTING_HPP
