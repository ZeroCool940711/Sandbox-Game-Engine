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

#ifndef MODEL_ANIMATION_HPP
#define MODEL_ANIMATION_HPP

#include "forward_declarations.hpp"
#include "math/forward_declarations.hpp"
#include "moo/forward_declarations.hpp"



/**
 *	Inner class to represent the base Model's animation
 */
class ModelAnimation : public ReferenceCount
{
public:
	ModelAnimation();
	virtual ~ModelAnimation();

	virtual bool valid() const;

	virtual void tick( float dtime, float otime, float ntime );
	virtual void play( float time = 0.f, float blendRatio = 1.f,
		int flags = 0 ) = 0;

	virtual void flagFactor( int flags, Matrix & mOut ) const;
	virtual const Matrix & flagFactorBit( int bit ) const;

	virtual uint32 sizeInBytes() const;

	virtual Moo::AnimationPtr getMooAnim();


	float	duration_;
	bool	looped_;
};



#endif // ANIMATION_HPP
