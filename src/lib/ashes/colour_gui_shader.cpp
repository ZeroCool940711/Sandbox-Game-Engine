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

#include "colour_gui_shader.hpp"
#include "simple_gui_component.hpp"
#include "pyscript/script.hpp"
#include "math/colour.hpp"

#include "cstdmf/debug.hpp"
DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )

#ifndef CODE_INLINE
#include "colour_gui_shader.ipp"
#endif

#pragma warning(disable:4786)	// remove "truncated identifier" warnings from STL



// Python statics

PY_TYPEOBJECT( ColourGUIShader )

PY_BEGIN_METHODS( ColourGUIShader )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ColourGUIShader )
	/*~	attribute ColourGUIShader.start
	 *
	 *	The start attribute is the colour to associate with a value of zero.
	 *	The colour of the shader is linearly interpolated between the start,
	 *	middle and end attributes, using the value attribute.  When value is
	 *	between 0.0 and 0.5, it is interpolated between start and middle.
	 *
	 *	The value is a Vector4, with the components (red, green, blue, alpha),
	 *	each between 0 and 255.  Alpha is ignored.
	 *
	 *	@type	Vector4
	 */
	PY_ATTRIBUTE( start )
	/*~	attribute ColourGUIShader.middle
	 *
	 *	The middle attribute is the colour to associate with a value of 0.5.
	 *	The colour of the shader is linearly interpolated between the start,
	 *	middle and end attributes, using the value attribute.  When value is
	 *	between 0.0 and 0.5, it is interpolated between start and middle.  When
	 *	value is between 0.5 and 1.0, it is interpolated between middle and end.
	 *
	 *	The value is a Vector4, with the components (red, green, blue, alpha),
	 *	each between 0 and 255.  Alpha is ignored
	 *
	 *	@type	Vector4
	 */
	PY_ATTRIBUTE( middle )
	/*~	attribute ColourGUIShader.end
	 *
	 *	The end attribute is the colour to associate with a value of 1.0.  The
	 *	colour of the shader is linearly interpolated between the start, middle
	 *	and end attributes, using the value attribute.  When value is between
	 *	0.5 and 1.0, it is interpolated between middle and end.
	 *
	 *	The value is a Vector4, with the components (red, green, blue, alpha),
	 *	each between 0 and 255.  Alpha is ignored
	 *
	 *	@type	Vector4
	 */
	PY_ATTRIBUTE( end )
	/*~ attribute ColourGUIShader.value
	 *
	 *	The value attribute is used to linearly interpolate the colour of the
	 *	shader between the start, middle and end colours. Values between 0.0
	 *	and 0.5 are interpolated between start and middle, values between 0.5
	 *	and 1.0 are interpolated between middle and end.
	 *	
	 *	If speed is non-zero, then when value is assigned to, the colour takes
	 *	speed seconds to move from its	current colour to the interpolated
	 *	colour chosen by the new value.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( value )
	/*~ attribute ColourGUIShader.speed
	 *
	 *	This attribute specifies how many seconds the colour of the shader
	 *	takes to adjust when the value attribute is assigned to.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( speed )
	/*~ attribute ColourGUIShader.colourProvider
	 *
	 *	This attribute can be assigned a ColourProvider, which then specifies
	 *	the colour for the shader, overriding the colour derived using value to
	 *	interpolate between start, middle and end.  By default, colourProvider 
	 *	is None.
	 *
	 *	@type	ColourProvider
	 */
	PY_ATTRIBUTE( colourProvider )
PY_END_ATTRIBUTES()

/*~ function GUI.ColourShader
 *
 *	This function creates a new ColourGUIShader.  This is used to apply colours
 *	to gui components.
 *
 *	@return		a new ColourGUIShader.
 */
PY_FACTORY_NAMED( ColourGUIShader, "ColourShader", GUI )

SHADER_FACTORY( ColourGUIShader )


/**
 *	Constructor
 */
ColourGUIShader::ColourGUIShader( PyTypePlus * pType )
:GUIShader( pType ),
	desiredT_(0),
	speed_(0),
	currentT_( 1.f ),
	oldT_( 1.f ),
	timer_( 0.f ),
	animation_(),
	colourProvider_( NULL )
{
	constants_[0].set( 255.f, 0.f, 0.f, 255.f );
	constants_[1].set( 255.f, 255.f, 0.f, 255.f );
	constants_[2].set( 0.f, 255.f, 0.f, 255.f );

	resetAnimation();
}


/**
 *	Destructor
 */
ColourGUIShader::~ColourGUIShader()
{
}


/**
 *	Get an attribute for python
 */
PyObject * ColourGUIShader::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return GUIShader::pyGetAttribute( attr );
}


/**
 *	Set an attribute for python
 */
int ColourGUIShader::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return GUIShader::pySetAttribute( attr, value );
}



/**
 *	Static python factory method
 */
PyObject * ColourGUIShader::pyNew( PyObject * args )
{
	return new ColourGUIShader();
}



/**
 *	This method processes a GUI component, applying this shader to its vertices
 *
 *	@param component	The component to process
 *	@param dTime		The delta time for the current frame of the application
 */
bool
ColourGUIShader::processComponent( SimpleGUIComponent& component, float dTime )
{
	//calculate the current colour
	Vector4 currentColour;
	uint32 colourMask;

	if (colourProvider_) {
		colourProvider_->tick( dTime );
		colourProvider_->output( currentColour );
		colourMask = 0xffffffff;
	} else {
		//calculate the current t-value
		timer_ += dTime;
		float dt;
		if ( speed_ > 0.f )
			dt = timer_ / speed_;
		else
			dt = 1.f;

		if ( dt > 1.f )
			dt = 1.f;
		if ( dt < 0.f )
			dt = 0.f;

		currentT_ = oldT_ + ( ( desiredT_ - oldT_ ) * dt );

		currentColour = animation_.animate( currentT_ );
		colourMask = 0x00ffffff;
	}

	uint32 col = Colour::getUint32( currentColour );

	//apply to component
	component.runTimeColour( (col & colourMask) | (component.runTimeColour() & ~colourMask) );

	return true;
}


/**
 *	Load method
 */
bool ColourGUIShader::load( DataSectionPtr pSect )
{
	if (!GUIShader::load( pSect )) return false;

	constants_[0] = pSect->readVector4( "start", constants_[0] );
	constants_[1] = pSect->readVector4( "middle", constants_[1] );
	constants_[2] = pSect->readVector4( "end", constants_[2] );
	desiredT_ = pSect->readFloat( "value", desiredT_ );
	speed_ = pSect->readFloat( "speed", speed_ );

	currentT_ = desiredT_;
	this->value( desiredT_ );
	this->resetAnimation();

	return true;
}


/**
 *	Save method
 */
void ColourGUIShader::save( DataSectionPtr pSect )
{
	GUIShader::save( pSect );

	pSect->writeVector4( "start", constants_[0] );
	pSect->writeVector4( "middle", constants_[1] );
	pSect->writeVector4( "end", constants_[2] );
	pSect->writeFloat( "value", desiredT_ );
	pSect->writeFloat( "speed", speed_ );
}

// colour_gui_shader.cpp