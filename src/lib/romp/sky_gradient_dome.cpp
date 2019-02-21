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

#include "sky_gradient_dome.hpp"
#include "moo/material.hpp"
#include "math/colour.hpp"
#include "resmgr/dataresource.hpp"
#include "resmgr/datasection.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/render_context.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/texture_exposer.hpp"
#include "moo/visual_manager.hpp"
#include "fog_controller.hpp"
#include "time_of_day.hpp"
#include "moo/vertex_declaration.hpp"

#include "moo/fog_helper.hpp"


static AutoConfigString s_mfmName( "environment/skyDomeMaterial" );
static AutoConfigString s_visualName( "environment/skyDomeVisual" );

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

#ifndef CODE_INLINE
#include "sky_gradient_dome.ipp"
#endif

static float s_SGDFogMultiplier = 1.f;

SkyGradientDome::SkyGradientDome():
 fullOpacity_( false ),
 inited_( false ),
 modulateColour_( 1.f, 1.f, 1.f ), 
 emitterID_( 0 ),
 mieEffect_( 0.3f ),
 turbidityOffset_( 0.2f ),
 turbidityFactor_( 0.1f ),
 vertexHeightEffect_( 1.f ),
 sunHeightEffect_( 1.f ),
 effectiveTurbidity_( 0.2f ),
 power_( 4.f ),
 pRayleighMap_( NULL )
{
	textureAlphas_.reset();
	textureAlphas_.loop( true, 24.f );

	skyDome_ = Moo::VisualManager::instance()->get( s_visualName );
	DataSectionPtr pMaterial = BWResource::openSection( s_mfmName );
	material_ = new Moo::EffectMaterial;
	material_->load( pMaterial );	
	parameters_.effect(material_->pEffect()->pEffect());

	fogEmitter_.innerRadius( 10000.f );
	fogEmitter_.outerRadius( 10001.f );
	fogEmitter_.colour_ = 0;
	fogEmitter_.position_ = Vector3::zero();
	fogEmitter_.maxMultiplier_ = 1.0f;
	fogEmitter_.nearMultiplier_ = 0.5f;
	fogEmitter_.localised_ = false;

	static bool watchingSkyGradient = false;
	if (!watchingSkyGradient)
	{
		MF_WATCH( "Client Settings/std fog/near", *this,
			MF_ACCESSORS( float, SkyGradientDome, nearMultiplier ),
			"Multiplier for near fog amount.  Higher values push the fog start "
			"value back towards where the fog end value is." );

		MF_WATCH( "Client Settings/std fog/far", *this,
			MF_ACCESSORS( float, SkyGradientDome, farMultiplier ),
			"Multiplier for far fog amount.  Higher values mean more overall fog" );

		MF_WATCH( "Client Settings/Sky Dome2/mie amount",
			mieEffect_ ,
			Watcher::WT_READ_WRITE,
			"Overall Mie scattering amount");

		MF_WATCH( "Client Settings/Sky Dome2/mie power",
			power_,
			Watcher::WT_READ_WRITE,
			"Mathematical power of Mie scattering effect." );

		MF_WATCH( "Client Settings/Sky Dome2/vertex height effect",
			vertexHeightEffect_,
			Watcher::WT_READ_WRITE,
			"Vertex height contribution to Mie scattering effect." );

		MF_WATCH( "Client Settings/Sky Dome2/sun height effect",
			sunHeightEffect_,
			Watcher::WT_READ_WRITE,
			"Sun height contribution to Mie scattering effect." );

		MF_WATCH( "Client Settings/Sky Dome2/air turbidity offset",
			turbidityOffset_,
			Watcher::WT_READ_WRITE,
			"Base level of air turbidity, or amount of suspended particulate"
			" matter.  Proportionally affects Mie scattering effect." );

		MF_WATCH( "Client Settings/Sky Dome2/air turbidity factor",
			turbidityFactor_,
			Watcher::WT_READ_WRITE,
			"Multiplier on air turbidity, or how the Mie scattering effect "
			"responds to increases in the global fog amount." );

		MF_WATCH( "Client Settings/Sky Dome2/effective turbidity",
			effectiveTurbidity_,
			Watcher::WT_READ_WRITE,
			"Calculated air turbidity, (turbidity offset + fogLevel * "
			"turbidity factor)" );

		MF_WATCH( "Client Settings/Sky Dome2/fog factors",
			fogFactors_,
			Watcher::WT_READ_ONLY,
			"Calculated fog factors (colour, amount)" );

		s_SGDFogMultiplier = 0.2f;
		MF_WATCH( "Client Settings/Sky Dome2/fog multiplier",
			s_SGDFogMultiplier,
			Watcher::WT_READ_WRITE,
			"How quickly does the sky gradient dome become the fog colour, on foggy days" );

		watchingSkyGradient = true;
	}

	createUnmanagedObjects();
}


SkyGradientDome::~SkyGradientDome()
{
	pRayleighMap_ = NULL;
	parameters_.clear();
}


void SkyGradientDome::createUnmanagedObjects()
{
}


void SkyGradientDome::deleteUnmanagedObjects()
{
	//retrieve effect parameters handles again (needed?)
	parameters_.clear();
}


/**
 *	XML file looks like the following example:
 *
 *	\verbatim
 *	<texture>			maps/fx/mining_sky_gradient.tga	</texture>
 *	<mieAmount>			0.25							</mieAmount>
 *	<turbidityOffset>	0.5								</turbidityOffset>
 *	<turbidityFactor>	0.1								</turbidityFactor>
 *	<vertexHeightEffect>0.3								</vertexHeightEffect>
 *	<sunHeightEffect>	1.1								</sunHeightEffect>
 *	<power>				3.5								</power>
 *	<nearFog>			0.5								</nearFog>
 *  \endverbatim
 */
void SkyGradientDome::load( DataSectionPtr root )
{
	inited_ = false;
	std::string texName = root->readString("texture", "");
	if (texName == "")
	{
		ERROR_MSG( "SkyGradientDome::load - texture was not specified in xml file" );
		return;
	}

	mieEffect_ = root->readFloat( "mieAmount", 0.3f );
	turbidityOffset_ = root->readFloat( "turbidityOffset", 0.2f );
	turbidityFactor_ = root->readFloat( "turbidityFactor", 0.1f );
	vertexHeightEffect_ = root->readFloat( "vertexHeightEffect", 1.f );
	sunHeightEffect_ = root->readFloat( "sunHeightEffect", 1.f );
	power_ = root->readFloat( "power", 4.f );
	fogEmitter_.nearMultiplier_ = root->readFloat( "nearFog",
		fogEmitter_.nearMultiplier_ );

    if (loadTexture(texName))
    {
		inited_ = true;
	}
}

bool SkyGradientDome::loadTexture( const std::string & textureName )
{
    bool ok = false; // Assume that the load failed.
    // When getting the texture, we allow animations, but don't want a
    // new texture created even if the load failed.
    Moo::BaseTexturePtr newTexture = 
        Moo::TextureManager::instance()->get(textureName, true, false, true, "texture/environment");
    if (newTexture.hasObject())
    {	    
        ok = createAnimationsFromTexture(newTexture);
        if (ok)
        {
            pRayleighMap_ = newTexture;
        }
#ifdef EDITOR_ENABLED
        texName_ = textureName;
#endif
    }
    return ok;
}

#ifdef EDITOR_ENABLED

void SkyGradientDome::save( DataSectionPtr root )
{
    root->writeString("texture"           , texName_            );
    root->writeFloat( "mieAmount"         , mieEffect_          );
	root->writeFloat( "turbidityOffset"   , turbidityOffset_    );
	root->writeFloat( "turbidityFactor"   , turbidityFactor_    );
	root->writeFloat( "vertexHeightEffect", vertexHeightEffect_ );
	root->writeFloat( "sunHeightEffect"   , sunHeightEffect_    );
	root->writeFloat( "power"             , power_              );
}

#endif


/**
 *	This method updates the sky dome, which really
 *	involves just calculating the fog factors that we will draw with.
 *
 *	post :  fogFactors_ is calculated.
 *			fogEmitter_.colour is calculated.
 *
 *	@param time		24 hour time from the TimeOfDay class
 */
void
SkyGradientDome::update( float time )
{
	//magic number...
	//...adjusts the sky dome's fog colour by the MIE scattering effect.
	//without "extraTurb" the scene fogging is darker than the sky dome's runtime colour.	
	static float S_FUDGE = -127.f;
	if (S_FUDGE < 0.f)
	{
		//MF_WATCH( "Client Settings/Sky Dome2/FUDGE", S_FUDGE );		
		S_FUDGE = -S_FUDGE;
	}

	effectiveTurbidity_ = turbidityOffset_ + ( FogController::instance().multiplier() * turbidityFactor_ );
	float extraTurb = effectiveTurbidity_ * (mieEffect_ * S_FUDGE);

	Vector3 turb(	extraTurb+effectiveTurbidity_,
					extraTurb+effectiveTurbidity_,
					extraTurb+effectiveTurbidity_);
	Vector3 col(modulateColour_.x, modulateColour_.y, modulateColour_.z);
	Vector3 fogV3 = col * fogAnimation_.animate( time ) + turb;
	float fogAlpha = textureAlphas_.animate( time );

	//Darken fog at night-time	
	//assuming the star map is predominantly black.
	uint32 fogCol = Colour::getUint32( fogV3 * fogAlpha );		
	
	fogEmitter_.colour_ = fogCol;	
}

void SkyGradientDome::addFogEmitter()
{
	emitterID_ = FogController::instance().addEmitter( fogEmitter_ );
}


void SkyGradientDome::remFogEmitter()
{
	FogController::instance().delEmitter( emitterID_ );
}


/**
 * Draws the sky gradient dome.  This version uses the scattering shader.
 */
void SkyGradientDome::draw( TimeOfDay* timeOfDay )
{
	if ( !inited_ )
		return;

	if ( !skyDome_ )
		return;	

	//calculate runtime effect values				
	Matrix sunToWorld = timeOfDay->lighting().sunTransform;
	sunToWorld.postRotateX( DEG_TO_RAD( 2.f * timeOfDay->sunAngle() ) );
	sunToWorld.postRotateY( MATH_PI );
	sunToWorld.translation( sunToWorld.applyToUnitAxisVector(2) * Vector3( 60, 30, 10 ).length() );
	Vector4 sunPosition(sunToWorld.applyToOrigin(),1.f);
	sunPosition.normalise();	
	
	Moo::FogHelper::setFogTableMode( D3DFOG_LINEAR );

	//calculate how much we fog the sky dome. we use the total fog multiplier,
	//and subtract 1 because that is our contribution to the fog.
	//the shader will blend from its colour to the fog colour based on this.	
	float totalOtherFogging = FogController::instance().totalFogContributions() - 1.f;
	fogFactors_ = FogController::instance().v4colour();
	fogFactors_.z = FogController::instance().totalFogContributions();
	fogFactors_.w = 1.f -  min(totalOtherFogging * s_SGDFogMultiplier, 1.f);
	fogFactors_.w *= fogFactors_.w;	//drop off quickly
	fogFactors_.w *= fogFactors_.w;	//drop off very quickly

	//set manual constants on the effect	
	parameters_.setTexture( "RAYLEIGH_MAP", pRayleighMap_->pTexture() );
	parameters_.setVector( "SUN_POSITION", &sunPosition );
	parameters_.setFloat( "TURBIDITY", effectiveTurbidity_ );
	parameters_.setVector( "SUN_COLOUR", &timeOfDay->lighting().sunColour );
	parameters_.setFloat( "MIE_EFFECT_AMOUNT", mieEffect_ );
	parameters_.setFloat( "VERTEX_HEIGHT_EFFECT", vertexHeightEffect_ );
	parameters_.setFloat( "SUN_HEIGHT_EFFECT", sunHeightEffect_ );
	parameters_.setFloat( "POWER", power_ );
	parameters_.setFloat( "TIME_OF_DAY", timeOfDay->gameTime() / 24.f );
	parameters_.setFloat( "FOG_AMOUNT", fogFactors_.w );

	//Draw the sky dome
	if (material_->begin())
	{
		for ( uint32 i=0; i<material_->nPasses(); i++ )
		{
			material_->beginPass(i);
			skyDome_->justDrawPrimitives();
			material_->endPass();
		}
		material_->end();
	}		
}


bool SkyGradientDome::createAnimationsFromTexture( Moo::BaseTexturePtr pTex )
{
	fogAnimation_.clear();
	textureAlphas_.clear();

	if ( !pTex )
		return false;

	Moo::TextureExposer te( pTex );

	if ( te.format() != D3DFMT_A8R8G8B8 )
	{
		WARNING_MSG( "SkyGradientDome::createAnimationsFromTexture - sky gradient dome texture must have texture format A8R8G8B8\n" );
		return false;
	}

	uint32 lastRow = ( te.height() - 1 ) * te.pitch();
	const char* pBits = te.bits();

	//get the last row of bytes, and add keys
	int step = 1;
	if ( te.width() > 32 )
	{
		step = te.width() / 32;
	}

	uint32* pCols;
	pCols = (uint32*)(pBits + lastRow);

	for ( int i = 0; i < te.width(); i+=step )
	{
		uint32 col;
		col = pCols[i];

		float progress = (float)i / (float)te.width();
		float time = 24.f * progress;		
		fogAnimation_.addKey( time, Colour::getVector3Normalised(col) );
		float fogAlpha = (float)( col >> 24 ) / 255.f;
		textureAlphas_.addKey( time, fogAlpha );
	}

	fogAnimation_.loop( true, 24.f );

	return true;
}


std::ostream& operator<<(std::ostream& o, const SkyGradientDome& t)
{
	o << "SkyGradientDome\n";
	return o;
}

// sky_gradient_dome.cpp
