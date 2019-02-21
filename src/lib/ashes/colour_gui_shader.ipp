/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// colour_gui_shader.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


/**
 *	This method sets all the constants
 */
INLINE void ColourGUIShader::constants( const Vector4& c1, const Vector4& c2,
	const Vector4& c3, float param, float speed )
{
	oldT_ = currentT_;
	timer_ = 0.f;

	constants_[0] = c1;
	constants_[1] = c2;
	constants_[2] = c3;
	desiredT_ = param;
	speed_ = speed;

	resetAnimation();
}


/**
 *	Sets start
 */
INLINE void ColourGUIShader::start( const Vector4 & v )
{
	constants_[0] = v;
	this->resetAnimation();
}

/**
 *	Sets middle
 */
INLINE void ColourGUIShader::middle( const Vector4 & v )
{
	constants_[1] = v;
	this->resetAnimation();
}

/**
 *	Sets end
 */
INLINE void ColourGUIShader::end( const Vector4 & v )
{
	constants_[2] = v;
	this->resetAnimation();
}

/**
 *	Sets value
 */
INLINE void ColourGUIShader::value( float f )
{
	oldT_ = currentT_;
	timer_ = 0.f;
	desiredT_ = f;
}


/**
 *	Resets the internal colour animation class.
 *	Automatically called whenever our colour constants change
 */
INLINE void ColourGUIShader::resetAnimation()
{
	animation_.reset();

	animation_.addKey( 0.f, constants_[0] );
	animation_.addKey( 0.5f, constants_[1] );
	animation_.addKey( 1.f, constants_[2] );
}


/*~ function ColourGUIShader.reset
 *
 *	This method resets the colour of the shader to be
 *	exactly the destination.  It bypasses any inherent
 *	animation to the target colour.
 */
/**
 *	This method resets the colour of the shader to be
 *	exactly the destination.  It bypasses any inherent
 *	animation to the target colour.
 */
INLINE void	ColourGUIShader::reset()
{
	oldT_ = desiredT_;
	currentT_ = desiredT_;
	timer_ = 0.f;
}


/**
 *	This method retrieves the immediate colour of the shader.
 */
INLINE Vector4 ColourGUIShader::currentColour()
{
	return animation_.animate( currentT_ );
}


// colour_gui_shader.ipp