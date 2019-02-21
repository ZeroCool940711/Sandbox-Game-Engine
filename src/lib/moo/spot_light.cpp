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

#include "spot_light.hpp"

#ifndef CODE_INLINE
#include "spot_light.ipp"
#endif

namespace Moo {

/**
 *	Constructor
 */
SpotLight::SpotLight( )
: position_( 0, 0, 0 ),
  direction_( 0, 0, 1 ),
  innerRadius_( 0 ),
  outerRadius_( 0 ),
  cosConeAngle_( 1 ),
  colour_( 0, 0, 0, 0 ),
  terrainTimestamp_( 0xffffffff )
#ifdef EDITOR_ENABLED
  , multiplier_( 1.f )
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position_;
	worldDirection_ = direction_;

	worldInnerRadius_ = innerRadius_;
	worldOuterRadius_ = outerRadius_;
}

/**
 *	Constructor
 */
SpotLight::SpotLight( const Colour& colour, const Vector3& position, const Vector3& direction,
	 float innerRadius, float outerRadius, float cosConeAngle )
: position_( position ),
  direction_( direction ),
  innerRadius_( innerRadius ),
  outerRadius_( outerRadius ),
  cosConeAngle_( cosConeAngle ),
  colour_( colour )
#ifdef EDITOR_ENABLED
  , multiplier_( 1.f )
#endif
{
	// give the worldTransformed attributes default values.
	worldPosition_ = position;
	worldDirection_ = direction;
	worldInnerRadius_ = innerRadius;
	worldOuterRadius_ = outerRadius;
}

/**
 *  Transform the the light to worldspace. The worldTransform of the light must
 *  be an orthogonal matrix with uniform scale for the light to scale properly.
 */
void SpotLight::worldTransform( const Matrix& transform )
{
	// transform light position
	XPVec3TransformCoord( &worldPosition_, &position_, &transform );

	// calculate the scale of the matrix by calculating the scale of the
	// first row of the world transform matrix (only works on orthogonal
	// uniform scale matrices)
	float scale = XPVec3Length( &Vector3( transform.m[ 0 ] ) ) ;

	//scale radiuses
	worldInnerRadius_ = innerRadius_ * scale;
	worldOuterRadius_ = outerRadius_ * scale;

	// transform the direction, assumes that transform is uniformally scaled and orthogonal.
	XPVec3TransformNormal( &worldDirection_, &direction_, &transform );
	XPVec3Normalize( &worldDirection_, &worldDirection_ );

}

/**
 * Create the vertex shader constants for a terrain light.
 *
 */
void SpotLight::createTerrainLight( float lightScale )
{
	terrainLight_[ 0 ] = Vector4( worldPosition_.x * lightScale,
		worldPosition_.y * lightScale,
		worldPosition_.z * lightScale, 0 );
	terrainLight_[ 1 ] = Vector4( colour_.r * multiplier(), colour_.g * multiplier(), colour_.b * multiplier(), colour_.a * multiplier() );

	float outer = worldOuterRadius_ * lightScale;
	float inner = worldInnerRadius_ * lightScale;

	terrainLight_[ 2 ] = Vector4( outer, 1.f / ( outer - inner ), cosConeAngle_, 0 );
	terrainLight_[ 3 ] = Vector4( worldDirection_.x , worldDirection_.y, worldDirection_.z, 0 );
}

/**
 *	Destructor
 */
SpotLight::~SpotLight()
{
}

}

// spot_light.cpp
