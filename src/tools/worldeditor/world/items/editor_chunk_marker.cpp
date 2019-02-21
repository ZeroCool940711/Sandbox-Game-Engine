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
#include "worldeditor/world/items/editor_chunk_marker.hpp"
#include "worldeditor/world/items/editor_chunk_substance.ipp"
#include "worldeditor/world/items/editor_chunk_item_tree_node.hpp"
#include "worldeditor/world/items/editor_chunk_binding.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/editor/item_editor.hpp"
#include "worldeditor/editor/item_properties.hpp"
#include "worldeditor/gui/pages/page_properties.hpp"
#include "worldeditor/resource.h"
#include "entitydef/constants.hpp"
#include "appmgr/options.hpp"
#include "chunk/chunk_marker_cluster.hpp"
#include "chunk/chunk_model.hpp"
#include "chunk/chunk_manager.hpp"
#include "worldeditor/world/editor_chunk.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "romp/geometrics.hpp"
#include "model/super_model.hpp"
#include "model/super_model_animation.hpp"
#if UMBRA_ENABLE
#include <umbraModel.hpp>
#include <umbraObject.hpp>
#include "chunk/chunk_umbra.hpp"
#endif

static AutoConfigString s_defaultModel("dummy/characterModel");


DECLARE_DEBUG_COMPONENT2( "EditorChunk", 0 )


class MarkerModels
{
public:
	~MarkerModels()
	{

	}

	static MarkerModels& instance()
	{
		static MarkerModels s_models;
		return s_models;
	}

	static std::string model( const std::string& markerName )
	{
		return instance()[ markerName ];
	}

	std::string operator [] ( const std::string& markerName )
	{
		StringHashMap<std::string>::iterator it = models_.find( markerName );
		if (it != models_.end())
			return it->second;
		return defaultModel_;
	}


private:
	MarkerModels()
	{
		defaultModel_ = s_defaultModel.value();		

		DataSectionPtr pMarkerSection = BWResource::openSection(
			EntityDef::Constants::markerCategoriesFile() );
		if (pMarkerSection)
		{
			DataSectionIterator it = pMarkerSection->begin();
			DataSectionIterator end = pMarkerSection->end();

			while(it != end)
			{
				DataSectionPtr pDS = *it;
				std::string markerName = pDS->readString( "editor/model" );
				if (markerName.length())
				{
					DEBUG_MSG( "MarkerModels::MarkerModels - Finding model for marker %s\n", pDS->sectionName().c_str() );
					if (BWResource::fileExists( markerName ))
					{
						models_[ pDS->sectionName() ] = markerName;
					}
					else
					{
						ERROR_MSG( "MarkerModels::MarkerModels - Unable to find model %s, using default\n", markerName.c_str() );
						models_[ pDS->sectionName() ] = defaultModel_;
					}
				}
				it++;
			}
		}
	}

	StringHashMap< std::string>	models_;
	std::string					defaultModel_;
};


// -----------------------------------------------------------------------------
// Section: EditorChunkMarker
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
EditorChunkMarker::EditorChunkMarker()
: mtype_( MType_Unknown )
, superModel_( NULL )
, loadEntityDescription_( true )
{
	edEntity_ = new EditorChunkEntity();
	edEntity_->setSurrogateMarker( NULL );
}


/**
 *	Destructor.
 */
EditorChunkMarker::~EditorChunkMarker()
{
	if (superModel_)
		delete superModel_;
	superModel_ = NULL;
	fv_.clear();
}


void EditorChunkMarker::draw()
{
	if (!edShouldDraw())
		return;

	if (WorldManager::instance().drawSelection())
	{
		WorldManager::instance().registerDrawSelectionItem( this );
	}

	// draw the marker
	Moo::rc().push();
	Moo::rc().preMultiply( this->edTransform() );
	superModel_->draw( &fv_ );
	Moo::rc().pop();

	if( WorldManager::instance().drawSelection() )
		return;

	// draw a line back to the cluster (if applicable)
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	if (getParent())
	{
		const Moo::Colour lineColour = 0xff0000ff;

		Vector3 from = chunk()->transform().applyPoint( edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);
		Vector3 to = getParent()->chunk()->transform().applyPoint( getParent()->edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.1f, 0.f);

		Geometrics::drawLine( from, to, lineColour, false );	// false = z-buffer the lines

		// draw a little arrow to indicate which is parent
		Vector3 direction = to - from;
		const float distance = direction.length();
		direction.normalise();

		Vector3 up(0.f, 1.f, 0.f);
		if (direction.dotProduct(up) > 0.9f)
			up = Vector3(1.f, 0.f, 0.f);

		const Vector3 rightAngleVector = direction.crossProduct(up);

		float arrowDisp = 0.2f;
		const float crossOver = 5.f;
		if (distance < crossOver)
			arrowDisp = 0.5f;

		float arrowDistanceAway = arrowDisp * distance;
		if (arrowDistanceAway > 5.f)
			arrowDistanceAway = 5.f;

		const float arrowWidth = 0.3f;
		const float arrowLength = 0.5f;
		Vector3 arrowHead = arrowDistanceAway * direction + from;
		Vector3 arrowBase1 = arrowHead - arrowLength * direction;
		Vector3 arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );

		arrowHead = -arrowDistanceAway * direction + to;
		arrowBase1 = arrowHead - arrowLength * direction;
		arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );
	}


	const EditorChunkEntity::BindingProperties & bProps = edEntity_->getBindingProps();
	for (EditorChunkEntity::BindingProperties::const_iterator bit = bProps.begin();
		bit != bProps.end();
		bit++)
	{
		const EditorChunkEntity::BindingProperty bProp = *bit;
		PyObject * action = const_cast<PyObject *>(bProp.first);
		const std::string argName = bProp.second;

		std::string toID = "";
		PyObject * val = PyObject_GetAttrString( action, const_cast<char *>(argName.c_str()) );
		if (val)
		{
			toID = PyString_AsString( val );
			Py_DECREF( val );
		}

		if (toID.empty())
			continue;

		const Moo::Colour lineColour = 0xffffff00;

		Vector3 from = chunk()->transform().applyPoint( edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.15f, 0.f);

		ChunkItemTreeNodePtr markerTo = nodeCache().find( toID );
		Vector3 to = markerTo->chunk()->transform().applyPoint( markerTo->edTransform().applyToOrigin() )
			+ Vector3(0.f, 0.15f, 0.f);

		Geometrics::drawLine( from, to, lineColour, false );	// false = z-buffer the lines

		// draw a little arrow to indicate which is parent
		Vector3 direction = to - from;
		const float distance = direction.length();
		direction.normalise();

		Vector3 up(0.f, 1.f, 0.f);
		if (direction.dotProduct(up) > 0.9f)
			up = Vector3(1.f, 0.f, 0.f);

		const Vector3 rightAngleVector = direction.crossProduct(up);

		float arrowDisp = 0.2f;
		const float crossOver = 5.f;
		if (distance < crossOver)
			arrowDisp = 0.5f;

		float arrowDistanceAway = arrowDisp * distance;
		if (arrowDistanceAway > 5.f)
			arrowDistanceAway = 5.f;

		const float arrowWidth = 0.3f;
		const float arrowLength = 0.5f;
		Vector3 arrowHead = arrowDistanceAway * direction + from;
		Vector3 arrowBase1 = arrowHead - arrowLength * direction;
		Vector3 arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );

		arrowHead = -arrowDistanceAway * direction + to;
		arrowBase1 = arrowHead - arrowLength * direction;
		arrowBase2 = arrowBase1 - rightAngleVector * arrowWidth;
		arrowBase1 += rightAngleVector * arrowWidth;

		Geometrics::drawLine( arrowHead, arrowBase1, lineColour, false );
		Geometrics::drawLine( arrowBase1, arrowBase2, lineColour, false );
		Geometrics::drawLine( arrowBase2, arrowHead, lineColour, false );
	}


	Moo::rc().pop();
}

void EditorChunkMarker::toss( Chunk * pChunk )
{
	EditorChunkSubstance<ChunkMarker>::toss( pChunk );

	// tell it about the new chunk without calling its toss method
	edEntity_->chunk( pChunk_ );
	// tell it about the new data section too
	edEntity_->setSurrogateMarker( pOwnSect_ );

	MF_ASSERT(id_ != UniqueID::zero())
}


bool EditorChunkMarker::load( DataSectionPtr pSection, std::string* errorString )
{
	if (!this->EditorChunkSubstance<ChunkMarker>::load( pSection ))
	{
		if ( errorString )
		{
			*errorString = "Marker load failed";
		}
		else
		{
			ERROR_MSG( "EditorChunkMarker::load - failed\n" );
		}
		return false;
	}

	std::vector< std::string > models;
	models.push_back( "helpers/props/standin.model" );
	superModel_ = new SuperModel( models );

	if ( Moo::g_renderThread )
	{
		// If loading from the main thread, load straight away
		edMainThreadLoad();
	}

	return true;
}

void EditorChunkMarker::edMainThreadLoad()
{
	if (loadEntityDescription())
		findCommonProperties();
}


bool EditorChunkMarker::loadEntityDescription()
{
	if (!loadEntityDescription_)
		return false;

	loadEntityDescription_ = false;

	// we are here either on initial load of if the category has changed
	// and the item has been auto - reselected

	// see if it is a marker or entity look-a-like from the data section
	std::string eType = "";
	if (pOwnSect()->openSection( "type", false ))
	{
		// this is really an entity
		eType = pOwnSect()->readString( "type" );
		if (eType.empty())
			eType = category_;
		else
			category_ = eType;

		mtype_ = MType_Entity;
	}
	else
	{
		DataSectionPtr sect = BWResource::openSection(EntityDef::Constants::markerCategoriesFile());
		if (sect)
		{
			DataSectionPtr categoryLevel = sect->openSection( getCategory() + "/level" );
			if (categoryLevel)
			{
				DataSectionPtr markerXMLs = BWResource::openSection( EntityDef::Constants::markerEntitiesPath() );

				if (markerXMLs && categoryLevel->countChildren() > 0)
				{
					DataSectionPtr pFirst = categoryLevel->openChild( 0 );
					DataSectionPtr mEntity = markerXMLs->openSection( pFirst->sectionName() + ".xml" );
					// TODO: Error handling here!!
					MF_ASSERT(mEntity);
					eType = mEntity->readString( "entity/type", "" );
				}
				else
				{
					// TODO: Error handling here!!
					MF_ASSERT( 0 );
				}
			}
		}
		else if (sect->openSection( getCategory() + "/Properties" ))
		{
			eType = getCategory();
		}

		if (!eType.empty())
		{
			mtype_ = MType_Marker;
		}
		else
		{
			mtype_ = MType_MarkerWithoutEntity;
		}
	}

	if (!eType.empty() && !edEntity_->edLoad( eType, pOwnSect() ))
	{
		ERROR_MSG( "EditorChunkMarker::load - failed to entity description\n" );
	}

	syncReprModel( true );
	return true;
}


/**
 *	Save any property changes to this data section
 */
bool EditorChunkMarker::edSave( DataSectionPtr pSection )
{
	if (!edCommonSave( pSection ))
		return false;

	MF_ASSERT(mtype_ != MType_Unknown);

	ChunkItemTreeNode::save( pSection );

	pSection->writeMatrix34( "transform", transform_ );

	pSection->delChild( "category" );
	pSection->delChild( "type" );
	if (mtype_ == MType_Entity)
        pSection->writeString( "type", category_ );
	else
        pSection->writeString( "category", category_ );

	return edEntity_->edSave( pSection );
}


/**
 *	Get the current transform
 */
const Matrix & EditorChunkMarker::edTransform()
{
	return transform_;
}


/**
 *	Change our transform, temporarily or permanently
 */
bool EditorChunkMarker::edTransform( const Matrix & m, bool transient )
{
	// it's permanent, so find out where we belong now
	Chunk * pOldChunk = pChunk_;
	Chunk * pNewChunk = this->edDropChunk( m.applyToOrigin() );
	if (pNewChunk == NULL) return false;

	if (transient)
	{
		// if this is only a temporary change, keep it in the same chunk
		transform_ = m;
	}
	else
	{
		// make sure the chunks aren't readonly
		if (!EditorChunkCache::instance( *pOldChunk ).edIsWriteable() 
			|| !EditorChunkCache::instance( *pNewChunk ).edIsWriteable())
			return false;

		// ok, accept the transform change then
		transform_.multiply( m, pOldChunk->transform() );
		transform_.postMultiply( pNewChunk->transformInverse() );

		// note that both affected chunks have seen changes
		WorldManager::instance().changedChunk( pOldChunk );
		WorldManager::instance().changedChunk( pNewChunk );

		// and move ourselves into the right chunk. we have to do this
		// even if it's the same chunk so the col scene gets recreated
		pOldChunk->delStaticItem( this );
		pNewChunk->addStaticItem( this );
	}

	// tell all our bindings that we've moved
	for (BindingList::iterator it = bindings_.begin();
		it != bindings_.end();
		it++)
	{
		EditorChunkBinding* binding = (EditorChunkBinding*)&*(*it);
		binding->calculateTransform(transient);
	}

	for (BindingList::iterator it = bindingsToMe_.begin();
		it != bindingsToMe_.end();
		it++)
	{
		EditorChunkBinding* binding = (EditorChunkBinding*)&*(*it);
		binding->calculateTransform(transient);
	}
	this->syncInit();
	return true;
}

/**
 *	Add the properties of this item to the given editor
 */
bool EditorChunkMarker::edEdit( class ChunkItemEditor & editor )
{
	if (!allChildrenLoaded())
		return false;

	MatrixProxy * pMP = new ChunkItemMatrix( this );
	editor.addProperty( new ChunkItemPositionProperty(
		L("WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK/POSITION"), pMP, this ) );
	editor.addProperty( new GenRotationProperty(
		L("WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK/ROTATION"), pMP ) );

	std::vector<std::string> categories;
	
	DataSectionPtr sect = BWResource::openSection(EntityDef::Constants::markerCategoriesFile());
	if (sect)
	{	
		for (int i = 0; i < sect->countChildren(); i++)
			categories.push_back(sect->openChild(i)->sectionName());
	}

	// add all the entities as categories too
	DataSectionPtr edEntitySect = BWResource::openSection(EntityDef::Constants::entitiesEditorPath());
	if (edEntitySect)
	{
		for (int i = 0; i < edEntitySect->countChildren(); i++)
		{
			std::string entityName = edEntitySect->openChild(i)->sectionName();
			size_t index = entityName.find_first_of( "." );
			if ( index != std::string::npos &&
				entityName.substr( index ) == ".py" )
			{
				categories.push_back( entityName.substr(0, index) );
			}
		}
	}

	editor.addProperty( new ListTextProperty(
		L("WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK/CATEGORY"),
		new AccessorDataProxy< EditorChunkMarker, StringProxy >(
			this, "category", 
			&EditorChunkMarker::getCategory, 
			&EditorChunkMarker::setCategory ), categories ) );


	// a marker is a representation of an entity, find the first underlying entity
	// if changed, reload its properties (may have changed)
	if (loadEntityDescription())
		findCommonProperties();

	// call edEdit for the properties
	if (!edEntity_->edEditProperties( editor, pMP ))
	{
		ERROR_MSG( "EditorChunkMarker::edEdit - failed to load entity properties\n" );
		return false;
	}

	return true;
}

void EditorChunkMarker::findCommonProperties()
{
	if (mtype_ != MType_Marker)
	{
		edEntity_->clearEditProps();
		return;
	}

    // then check for other entities and remove properties that are not common

	DataSectionPtr category;
	
	// find the associated marker entity types
	std::vector< std::string > markerEntityTypes;
	DataSectionPtr sect = BWResource::openSection(EntityDef::Constants::markerCategoriesFile());
	if (sect)
	{
		category = sect->openSection( getCategory() );
		for (DataSectionIterator it = category->begin(); it != category->end(); it++)
		{
			for (DataSectionIterator itLevel = category->begin(); 
				itLevel != category->end(); itLevel++)
			{
				DataSectionPtr pLevelDS = *itLevel;
				if (pLevelDS->sectionName() != "level")
					continue;

				for (DataSectionIterator itNames = pLevelDS->begin(); 
					itNames != pLevelDS->end(); itNames++)
				{
					DataSectionPtr pNameDS = *itNames;
					markerEntityTypes.push_back( pNameDS->sectionName() );
				}
			}
		}
	}

	// find the associated entities
	std::vector< std::string > entityTypes;
	std::vector< std::string > overriddenPropNames;
	DataSectionPtr markerXMLs = BWResource::openSection(EntityDef::Constants::markerEntitiesPath());
	for (std::vector< std::string >::iterator it = markerEntityTypes.begin();
		it != markerEntityTypes.end(); it++)
	{
		DataSectionPtr mEntity = markerXMLs->openSection( *it + ".xml" );
		std::string eType = mEntity->readString( "entity/type", "" );
		if (eType.empty())
		{
			WARNING_MSG( "EditorChunkMarker::edEdit - section %s has not "
						"specified an entity/type\n", mEntity->sectionName().c_str() );
			continue;
		}

		// do not create duplicates
		if (std::find( entityTypes.begin(), entityTypes.end(), eType ) != entityTypes.end())
			continue;

		entityTypes.push_back( eType );

		DataSectionPtr mProps = mEntity->openSection( "entity/properties" );
		if (mProps)
		{
			for (DataSectionIterator it = mProps->begin(); it != mProps->end(); it++)
			{
				DataSectionPtr pDS = *it;
				overriddenPropNames.push_back( pDS->sectionName() );
			}
		}
	}

	// add the marker category as an entity type if it has properties of its own
	if (category->openSection( "Properties" ))
	{
		entityTypes.push_back( getCategory() );
	}


	// find the common properties
	// pick the first entity type, add all its properties,
	// then remove properties that don't exist in the other entities
	std::list< std::string > propNames;
	std::list< std::string > propNames2;
	for (std::vector< std::string >::iterator it = entityTypes.begin();
		it != entityTypes.end(); it++)
	{
		const EntityDescription * desc = EditorEntityType::instance().get( *it );
		MF_ASSERT( desc );
		unsigned int totalProps = desc->propertyCount();
		for (unsigned int i = 0; i < totalProps; i++)
		{
			DataDescription* prop = desc->property( i );
			if (!prop->editable())
				continue;

			if (it == entityTypes.begin())
			{
				// add to our prop list
				propNames.push_back( prop->name() );
			}
			else
			{
				// add to our test prop list
				propNames2.push_back( prop->name() );
			}
		}

		if (it != entityTypes.begin())
		{
			// check that all of propNames is a subset of propNames2
			for (std::list<std::string>::iterator itName = propNames.begin();
				itName != propNames.end(); )
			{
				if ( std::find( propNames2.begin(), propNames2.end(), *itName)
						== propNames2.end() )
				{
					// not found, have to remove
					itName = propNames.erase( itName );
				}
				else
				{
					++itName;
				}
			}

			propNames2.clear();
		}
	}

	// now remove the properties that have already been defined
	for (std::vector< std::string >::iterator itName = overriddenPropNames.begin();
		itName != overriddenPropNames.end(); ++itName )
	{
		std::list< std::string >::iterator itPN =
			std::find( propNames.begin(), propNames.end(), *itName );

		if (itPN != propNames.end())
		{
			// overriden property found, remove so it's not editable
			propNames.erase( itPN );
		}
	}

	// now make all the properties not in propNames not editable
	edEntity_->setEditProps( propNames );
}


/**
 * Return false if any of the markers are not yet loaded
 */
bool EditorChunkMarker::edCanDelete()
{
	if ((getParent() && getParent()->allChildrenLoaded()) || (!getParent() && (parentID_ == UniqueID::zero())))
		return true;

	return false;
}

/**
 * Tell the markers they are no longer part of a cluster
 */
void EditorChunkMarker::edPreDelete()
{
	// tell the parent they not longer have a child
	if (isNodeConnected())
	{
		UndoRedo::instance().add( new ChunkItemTreeNodeOperation( this, true ) );
		removeThisNode();
	}
	EditorChunkItem::edPreDelete();
}

/**
 *	Copy the category, nothing else
 */
void EditorChunkMarker::edPostClone( EditorChunkItem* srcItem )
{
	// erxtract from any existing links, assign new ID
	removeThisNode();
	setNewNode();

	edSave( pOwnSect() );
	if ( chunk() != NULL )
		WorldManager::instance().changedChunk( chunk() );
	this->syncInit();
}


/**
 *	Return a modelptr that is the representation of this chunk item
 */
ModelPtr EditorChunkMarker::reprModel() const
{
	MF_ASSERT(superModel_->topModel(0));
	return superModel_->topModel(0);
}

void EditorChunkMarker::syncReprModel(bool createNewModelOnly)
{
	// sync model with the entity model if req
	std::string newModelID = "";

	if ( mtype_ == MType_Entity  &&  edEntity_->getReprModel() )
		newModelID = edEntity_->getReprModel()->resourceID();
	else
		newModelID = MarkerModels::model( category_ );

	MF_ASSERT( !newModelID.empty() );
	if (/*createNewModelOnly ||*/
		(reprModel()->resourceID() != newModelID) )
	{
		// save chunk pointer
		Chunk* c = chunk();

		// make a new supermodel
		std::vector< std::string > models;
		models.push_back( newModelID );
		bool needToss = false;
		if (superModel_)
		{
			if ( c )
			{
				// Hack: get it out of the chunk, so it's removed from the
				// collision scene BEFORE the model is deleted and the BB gets
				// corrupted in ChunkObstacle.
				// We really need to fix the issue of ChunkObstacle using a
				// reference to a bounding box instead of an updateable pointer
				// or a nicer way to keep the BB in sync with the obstacle.
				toss( NULL );
				needToss = true;
			}
			delete superModel_;
		}
		superModel_ = new SuperModel( models );
		MF_ASSERT( superModel_->nModels() > 0 );
		SuperModelAnimationPtr anim = superModel_->getAnimation( "default" );
		fv_.clear();
		fv_.push_back( anim );

		if ( needToss )
		{
			// Hack: The collision scene was deleted, so create it again by
			// putting ourself in the chunk again.
			toss( c );
		}
		this->syncInit();
		if (createNewModelOnly)
			return;

		// Update the collision scene

		// don't let the ref count go to 0 in the following commands
		ChunkItemPtr ourself = this;

		c->delStaticItem( this );
		c->addStaticItem( this );
	}
}


/// Write the factory statics stuff
#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, &errorString)
IMPLEMENT_CHUNK_ITEM( EditorChunkMarker, marker, 1 )


bool EditorChunkMarker::setCategory(const std::string & newCategory)
{
	// this is not a good test of mtype, but enough for saving
	// it will be updated on the next edEdit
	DataSectionPtr sect = BWResource::openSection(EntityDef::Constants::markerCategoriesFile());
	if ((sect) && (sect->openSection( newCategory, false )))
		mtype_ = MType_Marker;
	else
		mtype_ = MType_Entity;

	category_ = newCategory;

	// tell the entity our properties no longer count
	edEntity_->clearPropertySection();

	// remember to reload on the next call to edEdit
	loadEntityDescription_ = true;

	// reset the property pane
	PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
	if (pModule != NULL)
	{
		PyObject* pScriptObject = PyObject_GetAttr( pModule, Py_BuildValue( "s", "bd" ) );

		if (pScriptObject != NULL)
		{
			Script::call(
				PyObject_GetAttrString( pScriptObject, "resetSelUpdate" ),
				PyTuple_New(0),
				"EditorChunkMarker::setCategory");
			Py_DECREF( pScriptObject );
		}
		Py_DECREF( pModule );
	}

	syncReprModel();

	return true;
}

void EditorChunkMarker::setParent(ChunkItemTreeNodePtr parent)
{
	if (!edCanDelete())
		return;

	// Save the changes
	ChunkItemTreeNodePtr oldParent = getParent();
	ChunkItemTreeNode::setParent(parent);
	WorldManager::instance().changedChunk( chunk() );

	if (oldParent)
	{
		oldParent->edSave( oldParent->pOwnSect() );
		WorldManager::instance().changedChunk( oldParent->chunk() );
	}

	if (parent)
	{
		parent->edSave( parent->pOwnSect() );
		WorldManager::instance().changedChunk( parent->chunk() );
	}

	edSave( pOwnSect() );
}


void EditorChunkMarker::syncInit()
{	
	#if UMBRA_ENABLE
		// Grab the visibility bounding box
	if (this->superModel_)
	{
		BoundingBox bb = BoundingBox::s_insideOut_;
		this->superModel_->visibilityBox(bb);
		pUmbraModel_ = NULL;
		if (!pUmbraObject_.hasObject())
		{
			pUmbraModel_ = UmbraModelProxy::getObbModel( bb.minBounds(), bb.maxBounds() );
			pUmbraObject_ = UmbraObjectProxy::get( pUmbraModel_ );
		}
		
		// Set the user pointer up to point at this chunk item
		pUmbraObject_->object()->setUserPointer( (void*)this );

		// Set up object transforms
		Matrix m = this->edEntity_->chunk()->transform();
		m.preMultiply( transform_ );
		pUmbraObject_->object()->setObjectToCellMatrix( (Umbra::Matrix4x4&)m );
		pUmbraObject_->object()->setCell( pChunk_->getUmbraCell() );		
	} 
	else if (this->edEntity_)
	{
		this->edEntity_->syncInit();		
	}
	#endif
}


/*~ function WorldEditor.adjustMarkerCluster
 *	@components{ worldeditor }
 *
 *	This function adjusts the cluster, joins or removes the cluster association between 
 *	what is currently selected and the what the mouse is current over.
 *
 *	@param selected		A ChunkItemRevealer object which references what is currently selected.
 *	@param mouseOver	A ChunkItemRevealer object which references what the mouse is
 *						currently over.
*/
static PyObject* py_adjustMarkerCluster( PyObject* args )
{
	// parse arguments
	PyObject* pPyRev1 = NULL;
	PyObject* pPyRev2 = NULL;
	if (!PyArg_ParseTuple( args, "OO", &pPyRev1, &pPyRev2 ) ||
		!ChunkItemRevealer::Check( pPyRev1 ) ||
		!ChunkItemRevealer::Check( pPyRev2 ))
	{
		PyErr_SetString( PyExc_TypeError, "adjustMarkerCluster() "
			"expects two Revealer arguments" );
		return NULL;
	}

	ChunkItemRevealer::ChunkItems items1;
	ChunkItemRevealer::ChunkItems items2;

	(static_cast<ChunkItemRevealer*>( pPyRev1 ))->reveal( items1 );
	(static_cast<ChunkItemRevealer*>( pPyRev2 ))->reveal( items2 );

	// test the item set 1
	if (items1.size() == 0)
	{
		// nothing to do
		Py_Return;
	}

	int otherFound = 0;
	std::vector<ChunkItemTreeNodePtr> selectedList;	// what the user has selected
	ChunkItemTreeNodePtr highlighted = NULL;		// what the mouse is over

	for (std::vector<ChunkItemPtr>::iterator it = items1.begin();
		it != items1.end();
		it++)
	{
		if ( ((*it)->edDescription() == "marker") ||
			((*it)->edDescription() == "marker cluster") )
		{
			selectedList.push_back((ChunkItemTreeNode*)(&*(*it)));
		}
		else
		{
			otherFound++;
		}
	}

	if (otherFound != 0)
		return Py_BuildValue( "s", "Items must be of the same type to link." );


	// test the item set 2
	if (items2.size() != 1)
	{
		// nothing to do
		Py_Return;
	}

	if (items2.front()->edDescription() == L("WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK/ED_DESCRIPTION") )
	{
		// binding is a property, not an attribute
		// use middle button to set who sends a message to who
		ChunkItemTreeNode * itemTo = static_cast<ChunkItemTreeNode *>(&*(items2.front()));
		std::string selectedId = itemTo->id();
		PageProperties::instance().adviseSelectedId( selectedId );

		Py_Return;


		return Py_BuildValue( "s", 
				"Markers can not be a parent node. Binging coming soon." );
	}
	else if (items2.front()->edDescription() == L("WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_MARK_CLUSTER/ED_DESCRIPTION") )
	{
		highlighted = static_cast<ChunkMarkerCluster*>(&*(items2.front()));
	}
	else
	{
		return Py_BuildValue( "s", 
					"The 2ed arg must be a marker or marker cluster" );
		return NULL;
	}

	// at this point:
	// highlighted is the parent, selectedList will become or cease to become children

	// if parent not fully loaded, don't do anything (can't then check for circular links)
	if (!highlighted->allChildrenLoaded())
		return Py_BuildValue( "s", 
					"The parent has children yet to load, cannot link." );


	// change link
	bool allLinkedToSameParent = true;
	ChunkItemTreeNodePtr commonParent = (*(selectedList.begin()))->getParent();
	for (std::vector<ChunkItemTreeNodePtr>::iterator it = selectedList.begin();
		it != selectedList.end();
		it++)
	{
		if ((*it)->getParent() != commonParent)
		{
			allLinkedToSameParent = false;
			break;
		}
	}

	for (std::vector<ChunkItemTreeNodePtr>::iterator it = selectedList.begin();
		it != selectedList.end();
		it++)
	{
		// remove any opposing link
		if (highlighted->getParent() == (*it))
		{
			// Add an undo op
			UndoRedo::instance().add( new ChunkItemTreeNodeOperation( highlighted, true ) );
			highlighted->setParent(NULL);
		}

		// remove any link to other objects
		if ((*it)->getParent())
		{
			// Add an undo op
			UndoRedo::instance().add( new ChunkItemTreeNodeOperation( (*it), true ) );
			(*it)->setParent(NULL);
		}

		if ( !allLinkedToSameParent || 
			(allLinkedToSameParent && (!commonParent || (commonParent != highlighted))) )
		{
			// add the desired link
			(*it)->setParent(highlighted);
			// Add an undo op
			UndoRedo::instance().add( new ChunkItemTreeNodeOperation( (*it), false ) );
		}
		// else, leave unlinked
	}

	Py_Return;
}
PY_MODULE_FUNCTION( adjustMarkerCluster, WorldEditor )
