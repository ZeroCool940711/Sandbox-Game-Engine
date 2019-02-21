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

#include "lens_effect.hpp"
#include "lens_effect_manager.hpp"
#include "cstdmf/debug.hpp"
#include "moo/render_context.hpp"
#include "math/colour.hpp"
#include "resmgr/auto_config.hpp"

DECLARE_DEBUG_COMPONENT2( "Romp", 2 )

#ifndef CODE_INLINE
#include "lens_effect.ipp"
#endif

/**
 *	Constructor
 */
FlareData::FlareData() :
	colour_( 0xffffffff ),
	material_( "" ),
	clipDepth_( 1.f ),
	width_( 0.1f ),
	height_( 0.1f ),
	age_( OLDEST_LENS_EFFECT )
{
}


/**
 *	Destructor
 */
FlareData::~FlareData()
{
}


/**
 *	This method loads the flare information from the provided datasection.
 *
 *	@param pSection A pointer to the datasection containing flare data.
 */
void FlareData::load( DataSectionPtr pSection )
{
	this->material( pSection->readString( "type", "" ) );
	this->size( pSection->readFloat( "size", 1.f ) );
	this->width( pSection->readFloat( "width", this->width() ) );
	this->height( pSection->readFloat( "height", this->height() ) );
	this->clipDepth( pSection->readFloat( "depth", 1.f ) );	
	this->colour( Colour::getUint32( 
		pSection->readVector4( "rgba", 
			Vector4( 255.f, 255.f, 255.f, 255.f ) ) ) ); 

	//preload our texture
	LensEffectManager::instance().preload( this->material() );

	//load any secondary flares
	DataSectionPtr pSecondarySection = pSection->openSection( "secondaries" );
	if (pSecondarySection)
	{
		std::vector< DataSectionPtr > secondaries;
		pSecondarySection->openSections( "Flare", secondaries );

		std::vector< DataSectionPtr >::iterator it = secondaries.begin();
		std::vector< DataSectionPtr >::iterator end = secondaries.end();

		while (it != end)
		{
			DataSectionPtr & pFlareSection = *it++;

			FlareData flare;
			flare.load( pFlareSection );
			secondaries_.push_back( flare );
		}
	}	
}


/**
 *	This method draws a square mesh, using a given material.
 *	Assumes rendering states have been setup correctly.
 *
 *	@param clipPos		The position in clip space of the flare
 *	@param alphaStrength	The intensity of the alpha channel.
 *	@param scale		The scale to apply to the flare's opacity (alpha)
 *	@param lensColour	The Colour to apply to the flare.
 */
void FlareData::draw( const Vector4 & clipPos, float alphaStrength, 
						float scale, uint32 lensColour ) const
{
	Moo::EffectMaterialPtr material = 
		LensEffectManager::instance().getMaterial( this->material() );
	if (!material)
	{
		return;
	}

	Vector4 projPos = clipPos;
	projPos.z = 0;
	projPos.w = 1;

	projPos.x = projPos.x * this->clipDepth();
	projPos.y = projPos.y * this->clipDepth();

	float halfScreenWidth = Moo::rc().screenWidth() / 2.f;
	float halfScreenHeight = Moo::rc().screenHeight() / 2.f;

	//Create a spin-to-face poly
	float xExtent = Moo::rc().projection().m[ 0 ][ 0 ];
	float yExtent = Moo::rc().projection().m[ 1 ][ 1 ];

	// Tint the flare's colour with the provided colour
	Vector4 globalC = Colour::getVector4( lensColour );
	Vector4 flareC = Colour::getVector4( this->colour() );
	float r = (globalC.x * flareC.x) / 255;
	float g = (globalC.y * flareC.y) / 255;
	float b = (globalC.z * flareC.z) / 255;
	float a = (globalC.w * flareC.w) / 255;
	uint32 tintedC = Colour::getUint32( Vector4( r, g, b, a ) );

	// Find the alpha value
	int alpha = (tintedC & 0xff000000) >> 24;
	alpha = (int)(alphaStrength * (float)alpha * scale);
	int colour = (alpha << 24) | (tintedC & 0x00ffffff);

	Moo::VertexTLUV vertex[ 4 ];

	vertex[ 0 ].colour_ = colour;
	vertex[ 1 ].colour_ = colour;
	vertex[ 2 ].colour_ = colour;
	vertex[ 3 ].colour_ = colour;

	float effectXExtent = xExtent * this->width();
	float effectYExtent = yExtent * this->height();

	vertex[ 0 ].pos_.x = projPos.x - effectXExtent;
	vertex[ 0 ].pos_.y = projPos.y - effectYExtent;
	vertex[ 0 ].pos_.z = projPos.z;
	vertex[ 0 ].pos_.w = projPos.w;
	vertex[ 0 ].uv_.set( 0.f, 1.f );

	vertex[ 1 ].pos_.x = projPos.x - effectXExtent;
	vertex[ 1 ].pos_.y = projPos.y + effectYExtent;
	vertex[ 1 ].pos_.z = projPos.z;
	vertex[ 1 ].pos_.w = projPos.w;
	vertex[ 1 ].uv_.set( 0.f, 0.f );

	vertex[ 2 ].pos_.x = projPos.x + effectXExtent;
	vertex[ 2 ].pos_.y = projPos.y - effectYExtent;
	vertex[ 2 ].pos_.z = projPos.z;
	vertex[ 2 ].pos_.w = projPos.w;
	vertex[ 2 ].uv_.set( 1.f, 1.f );

	vertex[ 3 ].pos_.x = projPos.x + effectXExtent;
	vertex[ 3 ].pos_.y = projPos.y + effectYExtent;
	vertex[ 3 ].pos_.z = projPos.z;
	vertex[ 3 ].pos_.w = projPos.w;
	vertex[ 3 ].uv_.set( 1.f, 0.f );

	for (int i = 0; i < 4; ++i)
	{
		vertex[i].pos_.x *= halfScreenWidth;
		vertex[i].pos_.y *= -halfScreenHeight;
		vertex[i].pos_.x += halfScreenWidth;
		vertex[i].pos_.y += halfScreenHeight;
	}

	Moo::rc().setFVF( Moo::VertexTLUV::fvf() );
	material->begin();
	for (uint32 i = 0; i < material->nPasses(); ++i)
	{
		material->beginPass(i);
		Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, vertex, 
									sizeof(vertex[0]) );
		material->endPass();
	}
	material->end();
}


/**
 *	Constructor
 */
LensEffect::LensEffect() :
	id_( 0 ),
	position_( 0, 0, 0 ),
	maxDistance_( 150.f ),
	fadeSpeed_( 1.f ),
	visibility_( 1.f ),
	added_( LensEffectManager::s_drawCounter_ - 16 ), // stay away from counter
	colour_( 0xffffffff ),
	clampToFarPlane_( false )
{
}


/**
 *	Destructor
 */
LensEffect::~LensEffect()
{
}


/**
 *	This method loads the lens effect's properties.	
 */
bool LensEffect::load( DataSectionPtr pSection )
{
	MF_ASSERT(pSection);

	occlusionLevels_.clear();

	this->maxDistance_ = pSection->readFloat( "maxDistance", 150.f ); 
	this->area( pSection->readFloat( "area", 1.f ) );
	this->fadeSpeed( pSection->readFloat( "fadeSpeed", 1.f ) );

	std::vector< DataSectionPtr > pFlareSections;
	pSection->openSections( "Flare", pFlareSections );

	std::vector< DataSectionPtr >::iterator it = pFlareSections.begin();
	std::vector< DataSectionPtr >::iterator end = pFlareSections.end();

	while (it != end)
	{
		DataSectionPtr & pFlare = *it++;

		FlareData flare;
		flare.load( pFlare );

		float occlusionLevel = pFlare->readFloat( "occlusionLevel", 1.f );
		
		if (occlusionLevels_.find( occlusionLevel ) == occlusionLevels_.end())
		{
			occlusionLevels_[ occlusionLevel ] = flare;			
		}
		else
		{
			ERROR_MSG( "Cannot load %s as it's occlusion level conflicts "
						"with another flare", flare.material().c_str() );
		}
	}

	return true;
}


/**
 *	This method saves the lens effect's properties.
 */
bool LensEffect::save( DataSectionPtr pSection )
{
	//TODO : implement
	return false;
}


/**
 *	This method gets the age of the visible flare. If no flare is visible then
 *	it returns the age of the youngest flare.
 */
float LensEffect::age() const
{
	// +2 since lens effects are killed after OLDEST_LENS_EFFECT + 1.f
	float age = OLDEST_LENS_EFFECT + 2.f;

	// Reverse iterate because we want to go in descending order
	OcclusionLevels::const_reverse_iterator it = occlusionLevels_.rbegin();
	OcclusionLevels::const_reverse_iterator end = occlusionLevels_.rend();

	while (it != end)
	{
		if (it->first <= visibility_)
		{
			// If a flare is visible then it will eventually get to age 0 
			// anyway. Since we only compare with OLDEST_LENS_EFFECT then 
			// returning a constant value is ok.
			return 0.f;
		}

		// This makes sure that we get the youngest age if nothing is visible.
		age = std::min( age, it->second.age() );
		++it;
	}

	return age;
}

/**
 *	This method increases the age of the visible lens flare by the given age.
 *	It also ages the flares that are not currently visible by a rate dependent on their
 *	occlusion level and the current visibility.
 *
 *	@param ageBy How much to age the lens effect by.
 */
void LensEffect::age( float ageBy )
{
	OcclusionLevels::reverse_iterator it = occlusionLevels_.rbegin();
	OcclusionLevels::reverse_iterator end = occlusionLevels_.rend();
	
	bool occluderFound = false;

	while (it != end)
	{
		// Factor into account delta from occlusion level to current visibility
		// for a more natural fadeSpeed.
		float rate = fadeSpeed_;
		if ((visibility_ > 0.f) && (fabs( visibility_ - it->first ) > 0.f))
		{
			rate *= visibility_ / fabs( visibility_ - it->first );
		}
		
		float newAge;
		if (!occluderFound && (it->first <= visibility_))
		{
			// If ageBy is positive then we want to age the visible flare 
			// (for whatever reason)
			if (ageBy > 0.f)
			{
				newAge = it->second.age() + ageBy;
			}
			else if (rate >= 1.f )
			{
				// rate can be 0 so be careful.
				newAge = it->second.age() - OLDEST_LENS_EFFECT/rate;
			}
			else
			{
				newAge = 0.f;
			}

			// Make sure visible flare not older then OLDEST_LENS_EFFECT
			newAge = Math::clamp( 0.f, newAge, OLDEST_LENS_EFFECT );
			occluderFound = true;
		}
		else
		{
			if (rate != 0.f)
			{
				newAge = it->second.age() + OLDEST_LENS_EFFECT/rate;
				// Give it a 1s grace period
				if (newAge > OLDEST_LENS_EFFECT + 1.f)
				{
					newAge = OLDEST_LENS_EFFECT + 1.f;
				}
			}
			else
			{
				newAge = OLDEST_LENS_EFFECT + 1.f;
			}


		}

		it->second.age( newAge );
		++it;
	}
}


/**
 *	This method sets all the flares for this LensEffect to have the same size.
 */
void LensEffect::size( float size )
{
	OcclusionLevels::iterator it = occlusionLevels_.begin();
	OcclusionLevels::iterator end = occlusionLevels_.end();

	while (it != end)
	{
		it->second.size( size );
		++it;
	}
}


/**
 *	This method updates the lens effect.
 *
 *	@param dTime	The change in time from the last frame
 *	@param visibility The calculated visible percentage of the lens effect.
 */
void LensEffect::tick( float dTime, float visibility )
{
	visibility_ = visibility;

	// tell our visible flare to decrease its age.
	this->age( -1.f );
}


/**
 *	This method draws the lens effect.
 */
void LensEffect::draw()
{
	OcclusionLevels::iterator it = occlusionLevels_.begin();
	OcclusionLevels::iterator end = occlusionLevels_.end();

	while (it != end)
	{
		FlareData & flare = it->second;

		if (flare.age() <= OLDEST_LENS_EFFECT)
		{
			float alphaStrength = 1.f - (flare.age() / OLDEST_LENS_EFFECT);

			//calculate distance and projected postiion of flare
			Vector4 projPos( this->position().x, this->position().y, 
							 this->position().z, 1.f );
			Moo::rc().viewProjection().applyPoint( projPos, projPos );

			// If its infront of the screen
			if (projPos.w > 0.f)
			{
				float oow = 1.f / projPos.w;
				projPos.x *= oow;
				projPos.y *= oow;
				projPos.z *= oow;
				projPos.w = oow;

				//calculate distance of flare
				float dist = Vector3( this->position() - 
								Moo::rc().invView().applyToOrigin() ).length();
				
				float scale;
				if (this->maxDistance() != 0.f)
				{
					float t = dist / this->maxDistance();
					t = Math::clamp( 0.f, t, 1.f );
					scale = 1.f - (t*t);
				}
				else
				{
					scale = 0.f;
				}

				if (scale > 0.f)
				{
					flare.draw( projPos, alphaStrength, scale, colour_ );

					//draw secondaries
					FlareData::Flares::const_iterator it = flare.secondaries().begin();
					FlareData::Flares::const_iterator end = flare.secondaries().end();

					while ( it != end )
					{
						FlareData fd = *it++;

						fd.draw( projPos, alphaStrength, scale, colour_ );
					}
				}
			}
		}

		++it;
	}
}


/**
 *	This method looks at a file and determines whether it is a lens effect file
 */
/*static*/ bool LensEffect::isLensEffect( const std::string & file )
{
	DataSectionPtr pSection = BWResource::openSection( file );
	if (pSection)
	{
		DataSectionPtr pFlare = pSection->findChild("Flare");
		if (pFlare)
		{
			if (pFlare->findChild("type"))
			{
				return true;
			}
		}
	}

	return false;
}


/**
 *	Assignment operator
 */
LensEffect & LensEffect::operator=( const LensEffect & other )
{
	id_ = other.id_;
	position_ = other.position_;
	maxDistance_ = other.maxDistance_;
	area_ = other.area_;
	fadeSpeed_ = other.fadeSpeed_;
	visibility_ = other.visibility_;
	colour_ = other.colour_;
	occlusionLevels_ = other.occlusionLevels_;
	added_ = other.added_;
	clampToFarPlane_ = other.clampToFarPlane_;
	return *this;
}

// lens_effect.cpp
