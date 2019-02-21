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

#ifndef SWITCHED_MODEL_HPP
#define SWITCHED_MODEL_HPP

#include <strstream>

#include "model.hpp"
#include "model_animation.hpp"
#include "switched_model_animation.hpp"


/**
 *	Helper class for models whose animations can only switch their bulk
 */
template < class BULK >
class SwitchedModel : public Model
{
public:
	typedef BULK (*BulkLoader)( Model & m, const std::string & resourceID );

	SwitchedModel( const std::string & resourceID,
		DataSectionPtr pFile );

	virtual bool valid() const	{ return (bulk_) && (bulk_->isOK()); }

	virtual void dress();

	void setFrame( BULK frame ) { frameDress_ = frame; }

	float				blend( int blendCookie )
		{ return (blendCookie == blendCookie_) ? blendRatio_ : 0.f; }
	void				blend( int blendCookie, float blendRatio )
		{ blendCookie_ = blendCookie; blendRatio_ = blendRatio; }

protected:
	bool wireSwitch(
		DataSectionPtr pFile,
		BulkLoader loader,
		const std::string & mainLabel,
		const std::string & frameLabel );

	BULK					bulk_;
	BULK					frameDress_;
	BULK					frameDraw_;
	// if we want nodeless models to do the animation inheritance thing
	// then these 'frame' things will have to be a static array (elt per part)
	// or the anim play will have to take the model it's operating on or ... ?
	// (atm this is inconsistent because animations that we inherit from
	// a parent switched model do not effect this model unless we respec them)



	float	blendRatio_;
	int		blendCookie_;
};





/**
 *	Constructor
 */
template <class BULK> SwitchedModel<BULK>::SwitchedModel(
		const std::string & resourceID, DataSectionPtr pFile ) :
	Model( resourceID, pFile ),
	bulk_( NULL ),
	frameDress_( NULL ),
	frameDraw_( NULL ),
	blendRatio_( 0.f ),
	blendCookie_( Model::getUnusedBlendCookie() )
{
}


/**
 *	Initialisation function (called during construction)
 */
template <class BULK> bool SwitchedModel<BULK>::wireSwitch(
	DataSectionPtr pFile,
	BulkLoader loader,
	const std::string & mainLabel,
	const std::string & frameLabel )
{
	// load our bulk
	std::string bulkName = pFile->readString( mainLabel );
	bulk_ = (*loader)( *this, bulkName );

	// make sure something is there
	if (!bulk_)
	{
		ERROR_MSG( "SwitchedModel::SwitchedModel: "
			"Could not load resource '%s' as main bulk of model %s\n",
			bulkName.c_str(), resourceID_.c_str() );
		return false;
	}

	// create and add the default animation
	SwitchedModelAnimation<BULK> * pDefAnim = new SwitchedModelAnimation<BULK>( *this, "", 1.f );
	pDefAnim->addFrame( bulk_ );
	pDefaultAnimation_ = pDefAnim;

	// load all the other animations
	std::vector<DataSectionPtr>	vAnimSects;
	pFile->openSections( "animation", vAnimSects );
	for (uint i = 0; i < vAnimSects.size(); i++)
	{
		DataSectionPtr pAnimSect = vAnimSects[i];

		// read the common data
		std::string animName = pAnimSect->readString( "name" );
		float frameRate = pAnimSect->readFloat( "frameRate", 30.f );

		if (animName.empty())
		{
			ERROR_MSG( "Animation section %d in model %s has no name\n",
				i, resourceID_.c_str() );
			continue;
		}

		// now read the frames out
		std::vector<std::string> frameNames;
		if (!frameLabel.empty())
		{
			pAnimSect->readStrings( frameLabel, frameNames );
		}
		else
		{
			int frameCount = pAnimSect->readInt( "frameCount" );
			char ssbuf[32];
			for (int j = 0; j < frameCount; j++ )
			{
				std::strstream ss( ssbuf, sizeof(ssbuf) );
				ss << j << std::ends;
				frameNames.push_back(
					bulkName + "." + animName + "." + ss.str() );
			}
		}
		if (frameNames.empty())
		{
			ERROR_MSG( "Animation %s in model %s has no frames\n",
				animName.c_str(), resourceID_.c_str() );
			continue;
		}

		// ok, start with an empty animation
		SwitchedModelAnimation<BULK> * pAnim = new SwitchedModelAnimation<BULK>( *this, animName, frameRate );

		// fill it in
		for (uint j = 0; j < frameNames.size(); j++)
		{
			BULK pBulk = (*loader)( *this, frameNames[j] );

			if (!pBulk)
			{
				ERROR_MSG( "SwitchedModel::SwitchedModel: "
					"Frame %d of animation %s in model %s is "
					"missing its resource '%s'\n", j, animName.c_str(),
					resourceID_.c_str(), frameNames[j].c_str() );
			}
			else
			{
				pAnim->addFrame( pBulk );
			}
		}

		if (pAnim->numFrames() == 0)
		{
			delete pAnim;
			continue;
		}

		// and tell the world about it
		int existingIndex = this->getAnimation( animName );
		if (existingIndex == -1)
		{
			animationsIndex_.insert(
				std::make_pair( animName, animations_.size() ) );
			animations_.push_back( pAnim );
		}
		else
		{
			animations_[ existingIndex ] = pAnim;
		}
	}

	return true;
}


/**
 *	This method dresses this switched model
 */
template <class BULK> void SwitchedModel<BULK>::dress()
{
	frameDraw_ = frameDress_;
	frameDress_ = NULL;

	if (!frameDraw_) frameDraw_ = bulk_;
}




#endif // SWITCHED_MODEL_HPP
