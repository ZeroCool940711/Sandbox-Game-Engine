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
#include "mutant.hpp"


#include "resmgr/string_provider.hpp"
#include "resmgr/xml_section.hpp"
#include "romp/geometrics.hpp"
#include "model/super_model.hpp"
#include "model/super_model_animation.hpp"
#include "undo_redo.hpp"
#include "utilities.hpp"
#include "moo/animation_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Mutant_Animation", 0 )

AnimationInfo::AnimationInfo():
	frameRates( NULL ),
	channels( NULL )
{}

AnimationInfo::AnimationInfo(
	DataSectionPtr cData,
	DataSectionPtr cModel,
	SuperModelAnimationPtr cAnimation,
	std::map< std::string, float >& cBoneWeights,
	DataSectionPtr cFrameRates,
	ChannelsInfoPtr cChannels,
	float animTime
):
	data (cData),
	model (cModel),
	animation (cAnimation),
	boneWeights (cBoneWeights),
	frameRates (cFrameRates),
	channels (cChannels)
{
	if (channels == NULL)
	{
		channels = new ChannelsInfo;
		backupChannels();
	}
	if (animTime != -1.f)
	{
		animation->time = animTime;
	}
}

AnimationInfo::~AnimationInfo()
{
} 

Moo::AnimationPtr AnimationInfo::getAnim()
{
	std::string animPath = data->readString("nodes","");

	if (animPath == "") return NULL;

	animPath = animPath + ".animation";

	Moo::AnimationPtr anim = Moo::AnimationManager::instance().find( animPath );

	return anim;
}

Moo::AnimationPtr AnimationInfo::backupChannels()
{
	Moo::AnimationPtr anim = getAnim();

	if (!anim) return NULL;
	
	channels->list.clear();
	channels->list.reserve( anim->nChannelBinders() );

	for (uint i = 0; i < anim->nChannelBinders(); i++)
	{
		channels->list.push_back( anim->channelBinder( i ).channel()->duplicate() );
	}

	return anim;
}

Moo::AnimationPtr AnimationInfo::restoreChannels()
{
	Moo::AnimationPtr anim = getAnim();

	if (!anim) return NULL;
	
	MF_ASSERT ( anim->nChannelBinders() == channels->list.size() );
	
	for (uint i = 0; i < anim->nChannelBinders(); i++)
	{
		Moo::AnimationChannelPtr clone = channels->list[i]->duplicate();

		MF_ASSERT( clone );

		anim->channelBinder( i ).channel( clone );
	}

	return anim;
}

void AnimationInfo::uncompressAnim( Moo::AnimationPtr anim, std::vector< Moo::AnimationChannelPtr >& oldChannels )
{
	if (!anim) return;
	
	MF_ASSERT ( anim->nChannelBinders() == channels->list.size() );
	
	for (uint i = 0; i < anim->nChannelBinders(); i++)
	{
		oldChannels.push_back( anim->channelBinder( i ).channel());
		
		Moo::AnimationChannelPtr clone = channels->list[i]->duplicate();

		MF_ASSERT( clone );

		anim->channelBinder( i ).channel( clone );
	}
}

void AnimationInfo::restoreAnim( Moo::AnimationPtr anim, std::vector< Moo::AnimationChannelPtr >& oldChannels )
{
	if (!anim) return;
	
	MF_ASSERT ( anim->nChannelBinders() == channels->list.size() );
	
	for (uint i = 0; i < anim->nChannelBinders(); i++)
	{
		anim->channelBinder( i ).channel( oldChannels[i] );
	}
}

bool Mutant::canAnim( const std::string& modelPath )
{
	if (models_.find( modelPath ) == models_.end()) return false;

	DataSectionPtr nodefull = models_[ modelPath ]->openSection("nodefullVisual");

	return !!nodefull;
}

bool Mutant::hasAnims( const std::string& modelPath )
{
	if (models_.find( modelPath ) == models_.end()) return false;

	DataSectionPtr anim = models_[ modelPath ]->openSection("animation");

	return !!anim;
}

StringPair Mutant::createAnim( const StringPair& animID, const std::string& animPath )
{
	StringPair new_anim = animID;
	
	//Lets get a unique name
	char buf[256];
	int index = 1;
	while (animations_.find(new_anim) != animations_.end())
	{
		index++;
		bw_snprintf(buf, sizeof(buf), "%s %d", animID.first.c_str(), index);
		new_anim.first = std::string(buf);
	}

	UndoRedo::instance().add( new UndoRedoOp( 0, models_[animID.second], models_[animID.second] ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/ADDING_ANIM"), false );

	DataSectionPtr newAnim = models_[animID.second]->newSection( "animation" );
	newAnim->writeString( "name", new_anim.first );
	newAnim->writeFloat( "frameRate", 30.f );
	newAnim->writeString( "nodes", animPath );

	visibilityBoxDirty_ = true;

	reloadAllLists();

	return new_anim;
}

void Mutant::changeAnim( const StringPair& animID, const std::string& animPath )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;
	
	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model, false, animID ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_ANIM"), false );

	animations_[animID].data->writeString( "nodes", animPath );

	//Make sure these are cleared so they are recalculated on reload
	animations_[animID].animation->time = 0.f;
	animations_[animID].channels = NULL;

	visibilityBoxDirty_ = true;

	reloadAllLists();
}

void Mutant::removeAnim( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].model, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/DELETING_ANIM"), false );

	animations_[animID].model->delChild( animations_[animID].data );

	visibilityBoxDirty_ = true;

	reloadAllLists();
}

void Mutant::cleanAnim( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	//Make sure these are cleared so they are recalculated on reload
	animations_[animID].animation->time = 0.f;
	animations_[animID].channels = NULL;
}

Moo::AnimationPtr Mutant::getMooAnim( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return NULL;

	std::string animName = animFile( animID );

	if (animName == "")
		return NULL;
		
	Moo::AnimationPtr anim = Moo::AnimationManager::instance().find( animName );

    return anim;
}

Moo::AnimationPtr Mutant::restoreChannels( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return NULL;

	return animations_[animID].restoreChannels();
}

Moo::AnimationPtr Mutant::backupChannels( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return NULL;

	return animations_[animID].backupChannels();
}
	
void Mutant::setAnim( size_t pageID, const StringPair& animID )
{
	animMode_ = true;
	currAnims_[pageID] = animID;
	recreateFashions();
}

void Mutant::stopAnim( size_t pageID )
{
	AnimIt animIt = currAnims_.find( pageID );
	if (animIt != currAnims_.end())
	{
		currAnims_.erase( animIt );
		recreateFashions();
	}
}

bool Mutant::animName( const StringPair& animID, const std::string& animName )
{
	StringPair new_anim = animID;
	new_anim.first = animName;
	
	if (animID.first == animName)
		return true;

	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return false;

	//If the new animation name is already used exit gracefully
	if (animations_.find(new_anim) != animations_.end())
		return false;
		
	//Rename the animation std::map element
	animations_[new_anim] = animations_[animID];
    animations_.erase(animID);

	//Now we must adjust the tree list
	for (unsigned i=0; i<animList_.size(); i++)
	{
		if (animList_[i].first.second == animID.second)
		{
			for (unsigned j=0; j<animList_[i].second.size(); j++)
			{
				if (animList_[i].second[j] == animID.first)
				{
					animList_[i].second[j] = animName;
				}
			}
		}
	}

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[new_anim].data, animations_[new_anim].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_ANIM_NAME"), false );
	
	animations_[new_anim].data->writeString( "name", animName );

	//Make sure that we update the current animation ID if needed
	std::map< size_t, StringPair >::iterator it = currAnims_.begin();
	std::map< size_t, StringPair >::iterator end = currAnims_.end();
	for(; it != end; ++it)
	{
		if (it->second == animID)
		{
			setAnim( it->first, new_anim );
		}
	}

	return true;
}

std::string Mutant::animName( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return "";

	return animations_[animID].data->readString( "name", "" );
}

void Mutant::firstFrame( const StringPair& animID, int frame )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	//Make sure there was actually a change
	if (frame == firstFrame(animID))
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_FIRST_FRAME"), false );

	if (frame != -1)
		animations_[animID].data->writeInt( "firstFrame", frame );
	else
		animations_[animID].data->delChild( "firstFrame" );

	visibilityBoxDirty_ = true;

	reloadAllLists();
}

int Mutant::firstFrame( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return -1;

	return animations_[animID].data->readInt( "firstFrame", -1 );
}

void Mutant::lastFrame( const StringPair& animID, int frame )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	//Make sure there was actually a change
	if (frame == lastFrame(animID))
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_LAST_FRAME"), false );

	if (frame != -1)
		animations_[animID].data->writeInt( "lastFrame", frame );
	else
		animations_[animID].data->delChild( "lastFrame" );

	visibilityBoxDirty_ = true;

	reloadAllLists();
}

int Mutant::lastFrame( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return -1;

	return animations_[animID].data->readInt( "lastFrame", -1 );
}

void Mutant::localFrameRate( const StringPair& animID, float rate, bool final )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	//Make sure there has actually been a change
	if (rate == lastFrameRate_) return;

	if (final)
	{
		animations_[animID].frameRates->writeFloat( "curr", lastFrameRate_ );
		UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].frameRates, NULL ));
		UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_FRAME_RATE"), false );
		lastFrameRate_ = rate;
	}

	animations_[animID].frameRates->writeFloat( "curr", rate );
}

float Mutant::localFrameRate( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
	{
		return 30.f;
	}

	return animations_[animID].frameRates->readFloat( "curr", 30.f );
}

void Mutant::saveFrameRate( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].frameRates, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/SAVING_FRAME_RATE"), false );

	float rate = animations_[animID].frameRates->readFloat( "curr", 30.f );
	animations_[animID].frameRates->writeFloat( "init", rate );
	animations_[animID].data->writeFloat( "frameRate", rate );

	reloadAllLists();
}

float Mutant::frameRate( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return 30;

	return animations_[animID].data->readFloat( "frameRate", 30.0 );
}

void Mutant::frameNum( const StringPair& animID, int num )
{
	clearAnim_ = false;
	// Since we arent recreating fashions we need to make sure the action queue is empty
	if ( actionQueue_.fv().size() > 1 )
		actionQueue_.flush();

	if ((animID.first != "") && (animID.second != "") && (animations_.find(animID) != animations_.end()))
	{
		SuperModelAnimationPtr animation = animations_[animID].animation; 

		if (animation)
		{
			float time = num / frameRate(animID);
			time = Math::clamp( 0.f, time, animation->pSource(*superModel_)->duration_ );
			animation->time = time;
		}
	}
}

int Mutant::frameNum( const StringPair& animID )
{
	if ((animID.first != "") && (animID.second != "") && (animations_.find(animID) != animations_.end()))
	{
		SuperModelAnimationPtr animation = animations_[animID].animation; 

		if (animation)
		{
			float framef = frameRate(animID) * animation->time;
			int framei = (int)(framef);
			if ( almostEqual( framef, (float) framei ) )
				return framei;
			else
				return (int)(ceilf(framef));
		}
	}
	return 0;
}

int Mutant::numFrames( const StringPair& animID )
{
	if ((animID.first != "") && (animID.second != "") && (animations_.find(animID) != animations_.end()))
	{
		SuperModelAnimationPtr animation = animations_[animID].animation; 

		if ((animation) && (animation->pSource(*superModel_)))
		{
			return (int)(ceilf(frameRate(animID) * animation->pSource(*superModel_)->duration_));
		}
	}
	return 100;
}

void Mutant::animFile( const StringPair& animID, const std::string& animFile )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_ANIM_FILE"), false );

	animations_[animID].data->writeString( "nodes", animFile );

	visibilityBoxDirty_ = true;

	reloadAllLists();
}

std::string Mutant::animFile( const StringPair& animID )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
		return "";

	if (animations_[animID].data == NULL)
		return "";

	std::string animName = animations_[animID].data->readString( "nodes", "" );
	if (animName == "") return "";
	return animName + ".animation";
}

/**
 *	This method sets the given bone's alpha blend weighting.
 *
 *	@param animID The animation->model string pair.
 *	@param boneName The name of the bone whose alpha blend weighting to set.
 */
void Mutant::animBoneWeight( const StringPair& animID, const std::string& boneName, float val )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
	{
		return;
	}
	
	DataSectionPtr alpha = animations_[animID].data->openSection( "alpha" );

	// Need to sanitise the boneName
	std::string boneNameSanitised = SanitiseHelper::substringReplace( boneName );

	//Make sure the value has actually changed...
	if (alpha)
	{
		float oldVal = alpha->readFloat( boneNameSanitised, -1.f );
		if (val == oldVal) 
		{
			return;
		}
	}
	else if (val == 1.f) 
	{
		return;
	}

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/CHANGING_NODE_WEIGHT"), false );

	//Update the data section
	if (!alpha)	// If the datasection doesn't already exist...
	{
		alpha = animations_[animID].data->newSection( "alpha" ); //Create it
	}
	alpha->writeFloat( boneNameSanitised, val ); //Write the new value

	//Update the weighting in our map
	animations_[animID].boneWeights[boneName] = val;

	reloadModel();
}


/**
 *	This method returns the given bone's alpha blend weighting.
 *
 *	@param animID The animation->model string pair.
 *	@param boneName The name of the bone whose alpha blend weighting to return.
 *
 *	@return The alpha blend weighting of the bone.
 */
float Mutant::animBoneWeight( const StringPair& animID, const std::string& boneName )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
	{
		return -1.f;
	}

	//First make sure the bone exists
	if (animations_[animID].boneWeights.find(boneName) == animations_[animID].boneWeights.end())
	{
		return -1.f;
	}

	return animations_[animID].boneWeights[boneName];
}


/**
 *	This method removes the alpha blend weightingon the given bone from the 
 *	given animation.
 *
 *	@param animID The animation->model string pair.
 *	@param boneName The name of the bone whose alpha blend weighting to remove.
 */
void Mutant::removeAnimNode( const StringPair& animID, const std::string& boneName )
{
	//First make sure the animation exists
	if (animations_.find(animID) == animations_.end())
	{
		return;
	}

	// Need to sanitise the boneName
	std::string boneNameSanitised = SanitiseHelper::substringReplace( boneName );

	UndoRedo::instance().add( new UndoRedoOp( 0, animations_[animID].data, animations_[animID].model ));
	UndoRedo::instance().barrier( L("MODELEDITOR/MODELS/MUTANT_ANIMATIONS/REMOVING_BLEND"), false );

	//Remove the data section
	DataSectionPtr alpha = animations_[animID].data->openSection( "alpha" );
	if (alpha)
	{
		alpha->delChild( boneNameSanitised );
	}

	//Remove the weighting from our map
	animations_[animID].boneWeights.erase( boneName );

	//Remove the section if we just killed the last
	if (animations_[animID].boneWeights.size() == 0)
	{
		animations_[animID].data->delChild( "alpha" );
	}

	reloadModel();
}


void Mutant::playAnim( bool play )
{
	animMode_ = true; 
	clearAnim_ = false; 
	playing_ = play; 
	if ( actionQueue_.fv().size() > 1 )
	{
		actionQueue_.flush();
		recreateFashions();
	}
	if (play) 
		recreateFashions();
}
