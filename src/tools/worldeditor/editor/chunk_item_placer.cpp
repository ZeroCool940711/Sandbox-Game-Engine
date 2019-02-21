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
#include "worldeditor/editor/chunk_item_placer.hpp"
#include "worldeditor/editor/snaps.hpp"
#include "worldeditor/editor/autosnap.hpp"
#include "worldeditor/editor/item_view.hpp"
#include "worldeditor/editor/chunk_placer.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/vlo_manager.hpp"
#include "worldeditor/world/editor_chunk.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "worldeditor/world/items/editor_chunk_station.hpp"
#include "worldeditor/world/items/editor_chunk_vlo.hpp"
#include "worldeditor/misc/placement_presets.hpp"
#include "appmgr/options.hpp"
#include "gizmo/tool_manager.hpp"
#include "gizmo/current_general_properties.hpp"
#include "gizmo/general_properties.hpp"
#include "gizmo/item_functor.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk.hpp"
#include "pyscript/py_data_section.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/string_provider.hpp"
#include "appmgr/commentary.hpp"
#include "appmgr/options.hpp"
#include "cstdmf/unique_id.hpp"


DECLARE_DEBUG_COMPONENT2( "Editor", 0 )

namespace {
	///The amount to offset items that are created or cloned relative to the camera position
	float ITEM_CAMERA_OFFSET = 2.0f;
}

std::set<CloneNotifier*>* CloneNotifier::notifiers_ = NULL;

/*~ function WorldEditor.autoSnap
 *	@components{ worldeditor }
 *
 *	This function snaps two chunks together, by finding their closest matching portals.
 *
 *	@param chunk1	A ChunkItemRevealer object of the chunk that is currently being edited.
 *	@param chunk2	A ChunkItemRevealer object to the chunk that needs to be snapped to
 *					the chunk that is currently being edited.
 */
static PyObject * py_autoSnap( PyObject * args )
{
	// get args
	PyObject *pPyRev1, *pPyRev2;
	if (!PyArg_ParseTuple( args, "OO", &pPyRev1, &pPyRev2 ) ||
		!ChunkItemRevealer::Check( pPyRev1 ) ||
		!ChunkItemRevealer::Check( pPyRev2 ))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.autoSnap() "
			"expects two ChunkItemRevealers" );
		return NULL;
	}
	
	ChunkItemRevealer* chunkRevealer = static_cast<ChunkItemRevealer*>( pPyRev1 );
	ChunkItemRevealer* snapToRevealer = static_cast<ChunkItemRevealer*>( pPyRev2 );
//static PyObject* autoSnap( ChunkItemRevealer* chunkRevealer, ChunkItemRevealer* snapToRevealer )
//{

	// make sure there's only one
	std::vector<Chunk*> snapChunks = extractChunks( chunkRevealer );
	std::vector<Chunk*> snapToChunks = extractChunks( snapToRevealer );
	if (snapChunks.size() != 1 || snapToChunks.size() != 1)
	{
		PyErr_Format( PyExc_ValueError, "WorldEditor.autoSnap() "
			"Must snap exactly one item to another item" );
		return NULL;
	}

	if (CurrentPositionProperties::properties().empty())
	{
		return Py_BuildValue( "i", 0 );
	}

	Chunk* snapToChunk = snapToChunks.front();

	Chunk* chunk = snapChunks.front();

	Matrix autoSnapTransform = findAutoSnapTransform( chunk, snapToChunk );

	if (autoSnapTransform != Matrix::identity)
	{
		// Apply the new transform to the chunk
		Matrix m = chunk->transform();

		UndoRedo::instance().add(
			new ChunkMatrixOperation( chunk, m ) );

		EditorChunkCache::instance( *chunk ).edTransformClone( autoSnapTransform );

		// set a meaningful barrier name
		UndoRedo::instance().barrier( L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/AUTO_SNAP"), false );

		return Py_BuildValue("i", 1);
	}
	return Py_BuildValue("i", 0);
}
PY_MODULE_FUNCTION( autoSnap, WorldEditor )

/*~ function WorldEditor.rotateSnap
 *	@components{ worldeditor }
 *
 *	This function rotate snap a chunk
 *
 *	@param chunks	A ChunkItemRevealer object of the chunk that is currently being rotate-snapped
 */
static PyObject * py_rotateSnap( PyObject * args )
{
	// get args
	PyObject *pPyRev1, *pPyRev2 = NULL;
	float dz;
	if (!PyArg_ParseTuple( args, "Of|O", &pPyRev1, &dz, &pPyRev2 ) ||
		!ChunkItemRevealer::Check( pPyRev1 ) ||
		(pPyRev2&&!ChunkItemRevealer::Check( pPyRev2 )))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.rotateSnap() "
			"expects one or two ChunkItemRevealer(s) and a float" );
		return NULL;
	}

	ChunkItemRevealer* rotateRevealer = static_cast<ChunkItemRevealer*>( pPyRev1 );
	ChunkItemRevealer* referenceRevealer = static_cast<ChunkItemRevealer*>( pPyRev2 );

	SnappedChunkSetSet snapChunks( extractChunks( rotateRevealer ) );
	std::vector<Chunk*> referenceChunks;
	if( referenceRevealer )
		referenceChunks = extractChunks( referenceRevealer );

	if (CurrentPositionProperties::properties().empty())
	{
		PyErr_Format( PyExc_ValueError, "WorldEditor.rotateSnap() "
			"No current editor" );
		return NULL;
	}

	Chunk* referenceChunk = NULL;
	
	if( referenceRevealer && !referenceChunks.empty() )
		referenceChunk = referenceChunks.front();

	int done = 0;
	for( unsigned int i = 0; i < snapChunks.size(); ++i )
	{
		SnappedChunkSet chunks = snapChunks.item( i );
		Matrix rotateSnapTransform = findRotateSnapTransform( chunks, dz > 0.f, referenceChunk );

		if (rotateSnapTransform != Matrix::identity)
		{
			for( unsigned int j = 0; j < chunks.chunkSize(); ++j )
			{
				// Apply the new transform to the chunk
				Chunk* snapChunk = chunks.chunk( j );
				Matrix m = snapChunk->transform();

				UndoRedo::instance().add(
					new ChunkMatrixOperation( snapChunk, m ) );

				m.postMultiply( rotateSnapTransform );

				EditorChunkCache::instance( *snapChunk ).edTransformClone( m );

				// set a meaningful barrier name
				UndoRedo::instance().barrier( L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/AUTO_SNAP"), false );
			}
			done = 1;
		}
	}
	return Py_BuildValue("i", done);
}
PY_MODULE_FUNCTION( rotateSnap, WorldEditor )

// This is causing problems, ergo it's commented out

//PY_AUTO_MODULE_FUNCTION( RETOWN, autoSnap,
//	NZARG( ChunkItemRevealer*, NZARG( ChunkItemRevealer*, END ) ), WorldEditor )


// -----------------------------------------------------------------------------
// Section: ChunkItemExistenceOperation
// -----------------------------------------------------------------------------

void ChunkItemExistenceOperation::undo()
{
	// Invalid op.
	if (!pItem_)
		return;

	// If we're removing the item, check that it's ok with that
	if (!pOldChunk_ && !pItem_->edCanDelete())
		return;

	std::vector<ChunkItemPtr> selection = WorldManager::instance().selectedItems();

	// Since the current chunk will change, save it to use it later as the old
	// or previous chunk for the item.
	Chunk* undoOldChunk = pItem_->chunk();

	// Add or delete it the item.
	if (pOldChunk_ != NULL)
	{
		pOldChunk_->addStaticItem( pItem_ );
		pItem_->edPostCreate();

		selection.push_back( pItem_ );
	}
	else
	{
		pItem_->edPreDelete();
		if (pItem_->chunk()) //a vlo reference could have been tossed out...checking
			pItem_->chunk()->delStaticItem( pItem_ );

		std::vector<ChunkItemPtr>::iterator i =
			std::find( selection.begin(), selection.end(), pItem_ );

		if (i != selection.end())
			selection.erase( i );
	}

	// Add the current state of this item to the undo/redo list. It's done at
	// the end to be consistent with the rest of the 
	UndoRedo::instance().add(
		new ChunkItemExistenceOperation( pItem_, undoOldChunk ) );

}

// -----------------------------------------------------------------------------
// Section: LinkerExistenceOperation
// -----------------------------------------------------------------------------

void LinkerExistenceOperation::undo()
{
	// Invalid op.
	if (!pItem_)
		return;

	// If we're removing the item, check that it's ok with that
	if (!pOldChunk_ && !pItem_->edCanDelete())
		return;

	// first add the current state of this item to the undo/redo list
	UndoRedo::instance().add(
		new LinkerExistenceOperation( pItem_, pItem_->chunk() ) );

	// now add or delete it
	if (pOldChunk_ != NULL)
	{
		pOldChunk_->addStaticItem( pItem_ );
		pItem_->edPostCreate();
	}
	else
	{
		pItem_->edPreDelete();
		if (pItem_->chunk()) //a vlo reference could have been tossed out...checking
			pItem_->chunk()->delStaticItem( pItem_ );
	}
}

// -----------------------------------------------------------------------------
// Section: ChunkItemPlacer
// -----------------------------------------------------------------------------

/**
 * The creation and deletion of chunk items is handled with a pair of functions
 * in the WorldEditor module, rather than with a functor. It didn't really make
 * sense to have a functor that immediately popped itself. The controlling
 * script could jump through some hoops to create and delete items, or we
 * could just let them do what they want.
 *
 *
 */
class ChunkItemPlacer
{
	typedef ChunkItemPlacer This;

public:
	PY_MODULE_STATIC_METHOD_DECLARE( py_createChunkItem )
	PY_MODULE_STATIC_METHOD_DECLARE( py_deleteChunkItems )
	PY_MODULE_STATIC_METHOD_DECLARE( py_cloneChunkItems )

private:
	static void removeFromSelected( ChunkItemPtr pItem );
};


PY_MODULE_STATIC_METHOD( ChunkItemPlacer, createChunkItem, WorldEditor )
PY_MODULE_STATIC_METHOD( ChunkItemPlacer, deleteChunkItems, WorldEditor )
PY_MODULE_STATIC_METHOD( ChunkItemPlacer, cloneChunkItems, WorldEditor )


/**
 *	Calculate world space average position of the given items
 */
static Vector3 calculateAverageOrigin( const ChunkItemRevealer::ChunkItems& items )
{
	if (items.empty())
	{
		return Vector3( 0, 0, 0 );
	}
	
	Vector3 averagePosition = Vector3::zero();
	for (ChunkItemRevealer::ChunkItems::const_iterator i = items.begin();
			i != items.end();
			++i )
	{
		Chunk* chunk = (*i)->chunk();

		if (!chunk || !chunk->online())
			continue;

		averagePosition += chunk->transform().applyPoint(
			(*i)->edTransform().applyToOrigin() );
	}
	averagePosition /= (float) items.size();

	return averagePosition;
}

/*~ function WorldEditor.cloneChunkItems
 *	@components{ worldeditor }
 *
 *	This function makes a copy of a group of chunk items. It places the newly
 *	created chunk items at the position of the supplied ToolLocator object.
 *
 *	@param revealear A ChunkItemRevealer object of the selected items to clone.
 *	@param locator	 A ToolLocator object which has the position of where the copies 
 *					 should be placed in the space.
 *
 *	@return			 A ChunkItemGroup object of the cloned items.
 *
 *	Code Example:
 *	@{
 *	group = WorldEditor.cloneChunkItems( self.selection, bd.itemTool.locator.subLocator )
 *	# where bd is a BigBandDirector object
 *	@}
 */
PyObject * ChunkItemPlacer::py_cloneChunkItems( PyObject * args )
{
	CloneNotifier::Guard guard;

	// get args
	PyObject * pPyRev;
	PyObject * pPyLoc;

	if (!PyArg_ParseTuple( args, "OO", &pPyRev, &pPyLoc ) ||
		!ChunkItemRevealer::Check( pPyRev ) ||
		!ToolLocator::Check( pPyLoc ) )
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.cloneChunkItem() "
			"expects a ChunkItemRevealer and a ToolLocator" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );
	ToolLocator* locator = static_cast<ToolLocator*>( pPyLoc );

	// make sure there's only one
	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );

	std::vector<ChunkItemPtr> newItems;
    std::vector<EditorChunkStationNodePtr> copiedNodes;
    std::vector<EditorChunkStationNodePtr> newNodes;

	// Used to store the mappings between original linker GUIDs and their
	// cloned copies
	EditorChunkItemLinkableManager::GuidToGuidMap linkerCloneGuidMapping;

	// now get all the positions
	Vector3 centrePos = calculateAverageOrigin( items );
	Vector3 locPos = locator->transform().applyToOrigin();
	Vector3 newPos = locPos - centrePos;
	SnapProvider::instance()->snapPositionDelta( newPos );

	Matrix offset;
	offset.setTranslate( newPos );

	// calculate all the transformation matrices first
	std::vector<Matrix> itemMatrices(items.size());
	ChunkItemPtr item;
	Chunk* pChunk;

	ChunkItemRevealer::ChunkItems::iterator i = items.begin();
	std::vector<Matrix>::iterator j = itemMatrices.begin();	

	std::vector< EditorChunkTerrain * > modifiedTerrains;
	for (; i != items.end(); ++i, ++j)
	{
		item = *i;
		pChunk = item->chunk();

		Matrix m;
		m.multiply( item->edTransform(), pChunk->transform() );
		m.postMultiply( offset );
		BoundingBox bb;
		if (!locator->positionValid())
		{
			/* Don't place when in terrain or object snap mode, unless the item to be cloned
			   is a shell model.
			   We can't use the method WorldManager::freeSnapsEnabled() because that
			   returns false if a chunk is selected
			 */
			if ( Options::getOptionInt( "snaps/itemSnapMode", 0 ) != 0)
			{
				PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.cloneChunkItem() "
					"invalid locking mode for camera relative placing, must be Free" );
				return NULL;
			}	
			item->edBounds(bb);
			Vector3 dim( bb.maxBounds() - bb.minBounds() );
			Vector3 dir = locator->direction();
			dir *= ITEM_CAMERA_OFFSET;
			Vector3 trans( dir.x * dim.x, dir.y * dim.y , dir.z * dim.z );				
			m.postTranslateBy(trans);
		}

		if ( WorldManager::instance().terrainSnapsEnabled() )
		{
			Vector3 pos( m.applyToOrigin() );
			//snap to terrain only
			pos = Snap::toGround( pos );
			m.translation( pos );
		}
		else if ( WorldManager::instance().obstacleSnapsEnabled() && items.size() == 1 )
		{
			Vector3 normalOfSnap = SnapProvider::instance()->snapNormal( m.applyToOrigin() );
			Vector3 yAxis( 0, 1, 0 );
			yAxis = m.applyVector( yAxis );

			Vector3 binormal = yAxis.crossProduct( normalOfSnap );

			normalOfSnap.normalise();
			yAxis.normalise();
			binormal.normalise();

			float angle = acosf( Math::clamp(-1.0f, yAxis.dotProduct( normalOfSnap ), +1.0f) );

			Quaternion q( binormal.x * sinf( angle / 2.f ),
				binormal.y * sinf( angle / 2.f ),
				binormal.z * sinf( angle / 2.f ),
				cosf( angle / 2.f ) );

			q.normalise();

			Matrix rotation;
			rotation.setRotate( q );

			Vector3 pos( m.applyToOrigin() );

			m.translation( Vector3( 0.f, 0.f, 0.f ) );
			m.postMultiply( rotation );

			m.translation( pos );
		}
		*j = m;
	}

	for (i = items.begin(), j = itemMatrices.begin(); i != items.end(); ++i, ++j)
	{
		item = *i;
		pChunk = item->chunk();

		Matrix m = *j;

		if (item->isShellModel())
		{
			// Clone the chunk
			ChunkItemPtr pItem = ChunkPlacer::cloneChunk( pChunk, m, linkerCloneGuidMapping );
			if (!pItem)
				continue;

			// select the appropriate shell model
			newItems.push_back( pItem );
		}
		else
		{
			BoundingBox lbb( Vector3(0.f,0.f,0.f), Vector3(1.f,1.f,1.f) );
			item->edBounds( lbb );
			// Clone the chunk item
			DataSectionPtr sect = item->pOwnSect();

			if (!sect)
			{
				PyErr_Format( PyExc_ValueError, "WorldEditor.cloneChunkItem() "
					"Item does not expose a DataSection" );
				UndoRedo::instance().barrier( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_ITEMS" ), false );
				UndoRedo::instance().undo();
				return NULL;
			}

			bool isTerrain = sect->sectionName() == "terrain";
			
			Chunk* pNewChunk = isTerrain ? EditorChunk::findOutsideChunk( m.applyPoint( lbb.centre() ) ) :
				pChunk->space()->findChunkFromPoint( m.applyPoint( lbb.centre() ) );

			if( pNewChunk == NULL || !EditorChunkCache::instance( *pNewChunk).edIsWriteable() )
			{
				WorldManager::instance().addCommentaryMsg(
					L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/TARGET_NOT_WRITABLE", item->edDescription() ), Commentary::WARNING );
				continue;
			}

			m.postMultiply( pNewChunk->transformInverse() );

			// Copy the DataSection
			DataSectionPtr chunkSection = EditorChunkCache::instance( *pNewChunk ).pChunkSection();
			DataSectionPtr newSection;

			// if the item is terrain, remove old one
			if (isTerrain)
			{
				newSection = chunkSection->findChild( sect->sectionName() );

				EditorChunkTerrain* pEct = static_cast<EditorChunkTerrain*>(
					ChunkTerrainCache::instance( *pChunk ).pTerrain());

				DataSectionPtr terrainSection = EditorChunkCache::instance( *pNewChunk ).pCDataSection()->openSection( 
					pEct->block().dataSectionName() );

				pEct->block().rebuildLodTexture( pChunk->transform() );
				pEct->block().save( terrainSection );

				pEct = static_cast<EditorChunkTerrain*>(
					ChunkTerrainCache::instance( *pNewChunk ).pTerrain());
				pNewChunk->delStaticItem( pEct );

				UndoRedo::instance().add(
					new ChunkItemExistenceOperation( pEct, pNewChunk ) );
			}
			else
			{
				 newSection = chunkSection->newSection( sect->sectionName() );

				item->edCloneSection( pNewChunk, m, newSection );
			}

			WorldManager::instance().linkerManager().updateLinkerGuid( newSection, linkerCloneGuidMapping );
			// create the new item
			ChunkItemFactory::Result result = pNewChunk->loadItem( newSection );
			if ( result && result.onePerChunk() )
			{
				WARNING_MSG( result.errorString().c_str() );
				UndoRedo::instance().add(
							new ChunkItemExistenceOperation( result.item(), pNewChunk ) );

				std::vector<ChunkItemPtr>::iterator oldItem =
					std::find( newItems.begin(), newItems.end(), result.item() );
				if ( oldItem != newItems.end() )
					newItems.erase( oldItem );

				result = pNewChunk->loadItem( newSection );
			}
			if (!result)
			{
				PyErr_SetString( PyExc_ValueError, "WorldEditor.cloneChunkItem() "
					"error creating item from given section" );
				UndoRedo::instance().barrier( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_ITEMS" ), false );
				UndoRedo::instance().undo();
				return NULL;
			}

			// get the new item out of the chunk
			ChunkItemPtr pItem = result.item();
			if (!pItem)
			{
				PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.cloneChunkItem() "
					"Couldn't create Chunk Item" );
				UndoRedo::instance().barrier( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_ITEMS" ), false );
				UndoRedo::instance().undo();
				return NULL;
			}

			newItems.push_back( pItem );

			// call the main thread load 
			pItem->edChunkBind(); 

			WorldManager::instance().changedChunk( pNewChunk );

			// If it was terrain then fix terrain edges and regenerate the lod texture
			if ( isTerrain )
			{
				EditorChunkTerrain* pEct = 
					static_cast<EditorChunkTerrain*>(ChunkTerrainCache::instance( *pNewChunk ).pTerrain());
				modifiedTerrains.push_back( pEct );
				pEct->block().rebuildLodTexture(pEct->chunk()->transform());
			}

			// ok, everyone's happy then. so add an undo which deletes it
			UndoRedo::instance().add(
				new ChunkItemExistenceOperation( pItem, NULL ) );

			// Tell pitem we just cloned it from item
			pItem->edPostClone( &*item );

            // Add to the list of cloned nodes, if this is a node, we do some
            // special processing below:
            if (item->isEditorChunkStationNode())
            {
                EditorChunkStationNode *copiedNode = 
                    static_cast<EditorChunkStationNode *>(item.getObject());
                copiedNodes.push_back(copiedNode);
                EditorChunkStationNode *newNode =
                    static_cast<EditorChunkStationNode *>(pItem.getObject());
                newNodes.push_back(newNode);

			}
		}
	}

	// fix terrain edges
	if (modifiedTerrains.size() != 0)
	{
		for (std::vector< EditorChunkTerrain * >::iterator i = modifiedTerrains.begin(); i != modifiedTerrains.end(); ++i)
		{
			(*i)->onEditHeights();
			// also change left & bottom chunks' heightmap
			EditorChunkTerrain* pEct = (*i)->neighbour( EditorChunkTerrain::WEST );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::SOUTH );
			if (pEct)
			{
				pEct->onEditHeights();
			}
			pEct = (*i)->neighbour( EditorChunkTerrain::SOUTH_WEST );
			if (pEct)
			{
				pEct->onEditHeights();
			}

		}

	}

    if (copiedNodes.size() != 0)
    {
        EditorChunkStationNode::linkClonedNodes(copiedNodes, newNodes);
    }

	// If linker objects have been cloned, inform the linker manager so that it
	// can update its representation of the scene
	if (linkerCloneGuidMapping.size())
	{
		WorldManager::instance().linkerManager().updateMappedLinkers( linkerCloneGuidMapping );
	}

    // Cloning is always done as part of a clone & move tool, don't set a barrier here	
	// set a meaningful barrier name
	if( !newItems.empty() )
	{
		WorldManager::instance().setSelection( newItems );
		if (items.size() == 1)
			UndoRedo::instance().barrier( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_ITEMS",
				newItems.front()->edDescription() ), false );
		else
			UndoRedo::instance().barrier( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_ITEMS" ), false );

		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CLONED_SELECTION" ) );
		// and that's it then. return a group containing the new items
	}

	return new ChunkItemGroup( newItems );
}

/*~ function WorldEditor.createChunkItem
 *	@components{ worldeditor }
 *
 *	This method is used to create a chunk item(s). It places the newly created
 *	chunk item at the position of the supplied ToolLocator object. It also accepts an 
 *	optional parameter which specifies whether the new item should be moved to 
 *	the ToolLocator's local coordinates or world coordinates.
 *	
 *	@param dataSection  The DataSection of the item to be created.
 *	@param locator		The ToolLocator object which is used to see where
 *						the item should be placed in the space.
 *	@param useLocalPos	An optional int value which specifies whether the item should
 *						be placed localy to the ToolLocator or in world position (default).
 *						useLocalPos = 2 for local transform, useLocalPos = 1 for world transform.
 *
 *	@return	A ChunkItemGroup object of the newly created item(s).
 */
PyObject * ChunkItemPlacer::py_createChunkItem( PyObject * args )
{
	// get snaps
	Vector3 snaps = WorldManager::instance().movementSnaps();

	// get the arguments
	PyObject * pPyDS, * pPyLoc;
	int useLocPos = 1;
	if (!PyArg_ParseTuple( args, "OO|i", &pPyDS, &pPyLoc, &useLocPos) ||
		!PyDataSection::Check(pPyDS) ||
		!ToolLocator::Check(pPyLoc))
	{
		// we need the locator to find out what chunk to create it
		// in, even if it hasn't got a transform
		PyErr_SetString( PyExc_TypeError, "WorldEditor.createChunkItem() "
			"expects a PyDataSection, a ToolLocator, and an optional bool" );
		return NULL;
	}

	// find out what chunk it goes in
	ToolLocator * pLoc = static_cast<ToolLocator*>( pPyLoc );
	Vector3 wpos = pLoc->transform().applyToOrigin();
	if ( WorldManager::instance().snapsEnabled() )
	{
		Snap::vector3( wpos, snaps );
	}
	if ( WorldManager::instance().terrainSnapsEnabled() )
	{
		wpos = Snap::toGround( wpos );
	}
	Chunk * pChunk = ChunkManager::instance().cameraSpace()->
		findChunkFromPoint( wpos );
	if (!pChunk)
	{
		PyErr_SetString( PyExc_ValueError,
			formatString( "WorldEditor.createChunkItem() "
			"cannot find chunk at point (%0,%1,%2)",
			wpos.x, wpos.y, wpos.z ) );
		return NULL;
	}

	if (!EditorChunkCache::instance( *pChunk ).edIsWriteable())
	{
		WorldManager::instance().addCommentaryMsg(
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CANNOT_ADD_ASSET_ON_NON_LOCKED_CHUNK") );
		Py_Return;
	}
	
	// ok, simply create the chunk item from the data section then
	DataSectionPtr pSection = static_cast<PyDataSection*>( pPyDS )->pSection();
	ChunkItemFactory::Result result = pChunk->loadItem( pSection );
	if ( result && result.onePerChunk() )
	{
		ERROR_MSG( result.errorString().c_str() );
		UndoRedo::instance().add(
					new ChunkItemExistenceOperation( result.item(), pChunk ) );

		removeFromSelected( result.item() );

		result = pChunk->loadItem( pSection );
	}
	if (!result)
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.createChunkItem() "
			"error creating item from given section" );
		WorldManager::instance().addError( NULL, NULL,
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CANNOT_ADD_ASSET") );
		return NULL;
	}

	ChunkItemPtr pItem = result.item();
	if (!pItem)
	{
		PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.createChunkItem() "
			"Couldn't create Chunk Item" );
		WorldManager::instance().addError( NULL, NULL,
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CANNOT_ADD_ASSET") );
		return NULL;
	}

	// call the main thread load
	pItem->edChunkBind();

	WorldManager::instance().changedChunk( pChunk );

	BoundingBox bb;
	pItem->edBounds( bb );
	/* Note for the case of entities, like the beast
	dim seems to be incorrect as it does not show the size
	of the loaded model, only the red box substitute.
	*/
	Vector3 dim( bb.maxBounds()-bb.minBounds() );
	Matrix pos = pLoc->transform();
	if ( !pLoc->positionValid() )
	{
		/* Don't place when in terrain or object snap mode*/
		if (Options::getOptionInt( "snaps/itemSnapMode", 0 ) != 0){
			PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.createChunkItem() "
				"invalid locking mode for camera relative placing, must be Free" );
			return NULL;
		}
		Vector3 dir = pLoc->direction();
		dir *= ITEM_CAMERA_OFFSET;
		Vector3 trans( dir.x * dim.x, dir.y * dim.y , dir.z * dim.z);				
		pos.postTranslateBy ( trans );		
	}

	// now move it to the matrix of the locator, if desired
	if (useLocPos)
	{
		Matrix localPose;
		localPose.multiply( pos, pChunk->transformInverse() );
		Matrix newPose;

		if (useLocPos == 2)
		{
			newPose = localPose;
		}
		else
		{
			newPose = pItem->edTransform();
			newPose.translation( localPose.applyToOrigin() );
		}

		if ( WorldManager::instance().snapsEnabled() )
		{
			Snap::vector3( *(Vector3*)&newPose._41, snaps );
		}

		if ( WorldManager::instance().terrainSnapsEnabled() )
		{
			Vector3 worldPos(
				pChunk->transform().applyPoint(
					newPose.applyToOrigin() ) );
			
			Snap::toGround( worldPos );
			newPose.translation( pChunk->transformInverse().applyPoint( worldPos ) );
		}

		// check for random rotation placement
		if ( ( pSection->sectionName() == "model" || pSection->sectionName() == "speedtree" ) &&
			!PlacementPresets::instance()->defaultPresetCurrent() )
		{
			// get the random rotation placement values from the current preset
			float minRotX = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::X_AXIS, PlacementPresets::MIN );
			float maxRotX = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::X_AXIS, PlacementPresets::MAX );
			float minRotY = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::Y_AXIS, PlacementPresets::MIN );
			float maxRotY = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::Y_AXIS, PlacementPresets::MAX );
			float minRotZ = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::Z_AXIS, PlacementPresets::MIN );
			float maxRotZ = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::ROTATION, PlacementPresets::Z_AXIS, PlacementPresets::MAX );

			// rotation
			// yaw/pitch/roll rotation: X = yaw, Pitch = Y, Roll = Z, should change it )
			Matrix work = Matrix::identity;
			work.setRotate(
				DEG_TO_RAD( ( rand() % int( maxRotX - minRotX + 1 ) ) + minRotX ),
				DEG_TO_RAD( ( rand() % int( maxRotY - minRotY + 1 ) ) + minRotY ),
				DEG_TO_RAD( ( rand() % int( maxRotZ - minRotZ + 1 ) ) + minRotZ ) );
			newPose.preMultiply( work );
/*				// per-axis rotation
			if ( maxRotY - minRotY > 0 )
			{
				work.setRotateY( DEG_TO_RAD(
					( rand() % int( maxRotY - minRotY ) ) + minRotY  ) );
				result.preMultiply( work );
			}
			if ( maxRotX - minRotX > 0 )
			{
				work.setRotateX( DEG_TO_RAD(
					( rand() % int( maxRotX - minRotX ) ) + minRotX  ) );
				result.preMultiply( work );
			}
			if ( maxRotZ - minRotZ > 0 )
			{
				work.setRotateZ( DEG_TO_RAD(
					( rand() % int( maxRotZ - minRotZ ) ) + minRotZ ) );
				result.preMultiply( work );
			} */
		}

		// check for random scale placement
		if ( ( pSection->sectionName() == "model" || pSection->sectionName() == "speedtree" ) )
		{
			// get the random scale placement values from the current preset
			float minScaX = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::X_AXIS, PlacementPresets::MIN );
			float maxScaX = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::X_AXIS, PlacementPresets::MAX );
			float minScaY = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::Y_AXIS, PlacementPresets::MIN );
			float maxScaY = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::Y_AXIS, PlacementPresets::MAX );
			float minScaZ = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::Z_AXIS, PlacementPresets::MIN );
			float maxScaZ = PlacementPresets::instance()->getCurrentPresetData(
				PlacementPresets::SCALE, PlacementPresets::Z_AXIS, PlacementPresets::MAX );

			// scale
			float largestEdge = max( dim.x, dim.z );
			if ( largestEdge < 0.01f )
				largestEdge = 0.01f;
			float largestScale = 99.99f / largestEdge;
			if( largestEdge > 99.99 )
			{
				WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/WILL_BE_SCALED",
					pSection->readString( "resource" ) ), Commentary::CRITICAL );
			}
			float scaleX = randInRange( min( largestScale, minScaX ), min( largestScale, maxScaX ) );
			float scaleY = randInRange( min( largestScale, minScaY ), min( largestScale, maxScaY ) );
			float scaleZ = randInRange( min( largestScale, minScaZ ), min( largestScale, maxScaZ ) );

			if ( PlacementPresets::instance()->isCurrentDataUniform( PlacementPresets::SCALE ) )
			{
				scaleY = scaleX;
				scaleZ = scaleX;
			}

			Matrix work = Matrix::identity;
			work.setScale( scaleX, scaleY, scaleZ );
			newPose.preMultiply( work );
		}

		if (!pItem->edTransform( newPose, false ))
		{
			// couldn't move it there, so throw it away then
			pItem->chunk()->delStaticItem( pItem );

			PyErr_SetString( PyExc_ValueError, "WorldEditor.createChunkItem() "
				"could not move item to desired transform" );
			return NULL;
		}
	}

	// tell the item it was just created
	pItem->edPostCreate();


	float minEdge = min( min( dim.x, dim.y ), dim.z );
	if( minEdge < 0.001 )
	{
		PyErr_SetString( PyExc_EnvironmentError, "WorldEditor.createChunkItem() "
			"the item is too small in at least one axis, please check its bounding box" );
		WorldManager::instance().addError( NULL, NULL,
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/ITEM_TOO_SMALL" ) );

		if (pItem->isShellModel())
			ChunkPlacer::deleteChunk( pChunk );
		else
		{
			// see if it wants to be deleted
			if (pItem->edCanDelete())
			{
				// tell it it's going to be deleted
				pItem->edPreDelete();

				// delete it now
				pChunk->delStaticItem( pItem );
				WorldManager::instance().changedChunk( pChunk );
			}
		}
		return NULL;
	}
	// ok, everyone's happy then. so add an undo which deletes it
	UndoRedo::instance().add(
		new ChunkItemExistenceOperation( pItem, NULL ) );

	std::vector<ChunkItemPtr> items;
	items.push_back( pItem );
	WorldManager::instance().setSelection( items );

	// set a meaningful barrier name
	UndoRedo::instance().barrier(
		L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/CREATE_ASSET", pItem->edDescription() ), false );

	// and that's it then. return a group that contains it
	ChunkItemGroup * pRes = new ChunkItemGroup();
	pRes->add( pItem );

	return pRes;
}


/*~	function WorldEditor.deleteChunkItems
 *
 *	This function deletes chunk items referenced by the supplied ChunkItemRevealer object.
 *	
 *	@param revealer The ChunkItemRevealer object of the items that need
 *					to be deleted.
 */
PyObject * ChunkItemPlacer::py_deleteChunkItems( PyObject * args )
{
	// get args
	PyObject * pPyRev;
	if (!PyArg_ParseTuple( args, "O", &pPyRev ) ||
		!ChunkItemRevealer::Check( pPyRev ))
	{
		PyErr_SetString( PyExc_ValueError, "WorldEditor.deleteChunkItems() "
			"expects a ChunkItemRevealer" );
		return NULL;
	}

	ChunkItemRevealer* pRevealer = static_cast<ChunkItemRevealer*>( pPyRev );

	ChunkItemRevealer::ChunkItems items;
	pRevealer->reveal( items );

	WorldManager::instance().setSelection( std::vector<ChunkItemPtr>() );

	ChunkItemRevealer::ChunkItems::iterator i = items.begin();
	bool chunkDeleted = false;
	for (; i != items.end(); ++i)
	{
		ChunkItemPtr pItem = *i;
		Chunk* pChunk = pItem->chunk();


		// make sure it hasn't already been 'deleted'
		if (pChunk == NULL)
		{
			PyErr_Format( PyExc_ValueError, "WorldEditor.deleteChunkItems() "
				"Item has already been deleted!" );
			return NULL;
		}

		if (pItem->isShellModel())
		{
			// Delete the item's chunk
			if (!ChunkPlacer::deleteChunk( pChunk ))
				continue;
			chunkDeleted = true;
		}
		else
		{
			// Delete the item

			// see if it wants to be deleted
			if (!pItem->edCanDelete())
				continue;

			// tell it it's going to be deleted
			pItem->edPreDelete();

			// delete it now
			pChunk->delStaticItem( pItem );
			WorldManager::instance().changedChunk( pChunk );

			// set up an undo which creates it
			UndoRedo::instance().add(
				new ChunkItemExistenceOperation( pItem, pChunk ) );
		}
	}

	if (chunkDeleted)
	{
		VLOManager::instance()->updateReferences( NULL );
	}

	// set a meaningful barrier name
	if (items.size() == 1)
		UndoRedo::instance().barrier(
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/DELETE_BARRIER", items[0]->edDescription() ), false );
	else
		UndoRedo::instance().barrier(
			L("WORLDEDITOR/WORLDEDITOR/CHUNK/CHUNK_ITEM_PLACER/DELETE_ITEMS_BARRIER"), false );

	// and that's it
	Py_Return;
}

/*static*/ void ChunkItemPlacer::removeFromSelected( ChunkItemPtr pItem )
{
	//remove it from the selection
	std::vector<ChunkItemPtr> selection = WorldManager::instance().selectedItems();

	std::vector<ChunkItemPtr>::iterator i =
		std::find( selection.begin(), selection.end(), pItem );

	if (i != selection.end())
		selection.erase( i );

	WorldManager::instance().setSelection( selection, false );
}
