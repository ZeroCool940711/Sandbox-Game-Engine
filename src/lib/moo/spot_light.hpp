/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SPOT_LIGHT_HPP
#define SPOT_LIGHT_HPP

#include "moo_math.hpp"
#include <cstdmf/stdmf.hpp>
#include <cstdmf/smartpointer.hpp>
#include "cstdmf/vectornodest.hpp"

class BoundingBox;

namespace Moo {

class SpotLight;
typedef SmartPointer< SpotLight > SpotLightPtr;

/**
* This class represents an spot light with a colour, position, direction and a
* circular frustum.
*/
class SpotLight : public SafeReferenceCount
{
public:
	SpotLight();
	SpotLight( const Colour& colour, const Vector3& position, 
		const Vector3& direction, float innerRadius, float outerRadius, 
		float cosConeAngle );
	~SpotLight();

	const Vector3&		position( ) const;
	void				position( const Vector3& position );
	const Vector3&		direction( ) const;
	void				direction( const Vector3& direction );
	float				innerRadius( ) const;
	void				innerRadius( float innerRadius );
	float				outerRadius( ) const;
	void				outerRadius( float outerRadius );

	float				cosConeAngle( ) const;
	void				cosConeAngle( float cosConeAngle );
	const Colour&		colour( ) const;
	void				colour( const Colour& colour );

	void				worldTransform( const Matrix& transform );

	const Vector3&		worldPosition( ) const;
	const Vector3&		worldDirection( ) const;
	float				worldInnerRadius( ) const;
	float				worldOuterRadius( ) const;

	bool				intersects( const BoundingBox& worldSpaceBB ) const;

	Vector4*			getTerrainLight( uint32 timestamp, float lightScale_ );

#ifdef EDITOR_ENABLED
	float				multiplier() const { return multiplier_; }
	void				multiplier( float m ) { multiplier_ = m; }
#else//EDITOR_ENABLED
	float				multiplier() const { return 1.0f; }
#endif

private:
	void				createTerrainLight( float lightScale );

	Vector3				position_;
	Vector3				direction_;
	float				innerRadius_;
	float				outerRadius_;
	float				cosConeAngle_;
	Colour				colour_;

	Vector3				worldPosition_;
	Vector3				worldDirection_;
	float				worldInnerRadius_;
	float				worldOuterRadius_;

	uint32				terrainTimestamp_;
	Vector4				terrainLight_[4];

#ifdef EDITOR_ENABLED
	float				multiplier_;
#endif

};
}

typedef VectorNoDestructor< Moo::SpotLightPtr > SpotLightVector;

#ifdef CODE_INLINE
#include "spot_light.ipp"
#endif




#endif // SPOT_LIGHT_HPP
