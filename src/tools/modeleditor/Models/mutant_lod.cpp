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

#include "model/super_model.hpp"

#include "resmgr/xml_section.hpp"

#include "undo_redo.hpp"

#include "romp/geometrics.hpp"

#include "utilities.hpp"

#include "me_consts.hpp"

#include "mutant.hpp"

DECLARE_DEBUG_COMPONENT2( "Mutant_LOD", 0 )

float Mutant::lodExtent( const std::string& modelFile )
{
	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return LOD_HIDDEN;

	return models_[modelFile]->readFloat( "extent", LOD_HIDDEN );
}

void Mutant::lodExtent( const std::string& modelFile, float extent )
{
	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, models_[modelFile], models_[modelFile] ));

	if (extent != LOD_HIDDEN)
	{
		models_[modelFile]->writeFloat( "extent", extent );
	}
	else
	{
		models_[modelFile]->delChild( "extent" );
	}
}

void Mutant::lodParents( std::string modelName, std::vector< std::string >& parents )
{
	DataSectionPtr model = BWResource::openSection( modelName, false );
	while ( model )
	{
		parents.push_back( modelName );
		modelName = model->readString( "parent", "" ) + ".model";
		model = BWResource::openSection( modelName, false );
	}
}

bool Mutant::hasParent( const std::string& modelName )
{
	return (models_.find(modelName) != models_.end());
}

bool Mutant::isHidden( const std::string& modelFile )
{
	bool hidden = false;
	float extent = 0.f;

	for (size_t i=0; i<=lodList_.size(); i++)
	{
		if (((lodList_[i].second != LOD_HIDDEN) && (lodList_[i].second <= extent)) ||
			(extent == LOD_HIDDEN))
		{
			hidden = true;
		}
		else
		{
			hidden = false;
			extent = lodList_[i].second;
		}
		if (lodList_[i].first.second == modelFile )
			break;
	}

	return hidden;
}

std::string Mutant::lodParent( const std::string& modelFile )
{
	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return "";

	return models_[modelFile]->readString( "parent", "" );
}

void Mutant::lodParent( const std::string& modelFile, const std::string& parent )
{
	//First make sure the model exists
	if (models_.find(modelFile) == models_.end())
		return;

	UndoRedo::instance().add( new UndoRedoOp( 0, models_[modelFile], models_[modelFile] ));

	if (parent != "")
	{
		models_[modelFile]->writeString( "parent", parent );
	}
	else
	{
		models_[modelFile]->delChild( "parent" );
		models_[modelFile]->delChild( "extent" );
	}
}

/*
 * This method commits a LOD list that has been edited by using the lod bar
 */
void Mutant::lodList( LODList* newList )
{
	//Update all the extents
	for (unsigned i=0; i<newList->size(); i++)
	{
		lodExtent((*newList)[i].first.second, (*newList)[i].second );
	}

	reloadAllLists();

}

/*
 * This method sets the virtual LOD distance
 */
void Mutant::virtualDist( float dist )
{
	virtualDist_ = dist;
}
