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
#include "worldeditor/undo_redo/linker_operations.hpp"
#include "worldeditor/world/editor_chunk_item_linker.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk_item_linker_manager.hpp"
#include "common/properties_helper.hpp"


using namespace std;


/**
 *  This is the LinkerUndoChangeLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param oldEcil		The linker object previously pointed at.
 *  @param newEcil		The linker object being added to the property.
 *  @param propIdx		The property index.
 */
LinkerUndoChangeLinkOperation::LinkerUndoChangeLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * oldEcil,
		const EditorChunkItemLinkable * newEcil,
		PropertyIndex propIdx) :
	UndoRedo::Operation(int(typeid(LinkerUndoChangeLinkOperation).name())),
	propIdx_(propIdx)
{
	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	oldUID_ = oldEcil->guid();
	oldCID_ = oldEcil->getOutsideChunkId();

	newUID_ = newEcil->guid();
	newCID_ = newEcil->getOutsideChunkId();
}


/**
 *  This restores the link information.
 */
/*virtual*/ void LinkerUndoChangeLinkOperation::undo()
{
	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * oldEcil =
		WorldManager::instance().linkerManager().forceLoad( oldUID_, oldCID_ );
	EditorChunkItemLinkable * newEcil =
		WorldManager::instance().linkerManager().forceLoad( newUID_, newCID_ );
	
	// The force load should be successful for all cases
	MF_ASSERT(	startEcil && oldEcil && newEcil &&
				"LinkerUndoChangeLinkOperation::undo : "
				"One or more linker objects failed to force load!" );

	UndoRedo::instance().add(new LinkerUndoChangeLinkOperation(startEcil, newEcil, oldEcil, propIdx_));

	WorldManager::instance().linkerManager().addLinkInternal
		(
			startEcil,
			oldEcil,
			propIdx_
		);
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoChangeLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUndoAddLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param endEcil		The target linker object.
 *  @param propIdx		The property index.
 */
LinkerUndoAddLinkOperation::LinkerUndoAddLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * endEcil,
		PropertyIndex propIdx) :
	UndoRedo::Operation(int(typeid(LinkerUndoAddLinkOperation).name())),
	propIdx_(propIdx)
{
	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	endUID_ = endEcil->guid();
	endCID_ = endEcil->getOutsideChunkId();
}


/**
 *  This deletes the link information.
 */
/*virtual*/ void LinkerUndoAddLinkOperation::undo()
{
	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * endEcil =
		WorldManager::instance().linkerManager().forceLoad( endUID_, endCID_ );
	
	// The force load should be successful for all cases
	MF_ASSERT(	startEcil && endEcil &&
				"LinkerUndoAddLinkOperation::undo : "
				"One or more linker objects failed to force load!" );

	DataSectionPtr data = startEcil->propHelper()->propGet(propIdx_.valueAt(0));
	
	UndoRedo::instance().add(new LinkerUndoDeleteLinkOperation(startEcil, endEcil, data, propIdx_));

	// Need to delete the link here
	WorldManager::instance().linkerManager().deleteLinkInternal( startEcil, endEcil, propIdx_ );
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoAddLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUndoDeleteLinkOperation constructor.
 *
 *  @param startEcil	The source linker object.
 *  @param endEcil		The target linker object.
 *  @param data			The data section containing the original property info.
 *  @param propIdx		The property index.
 */
LinkerUndoDeleteLinkOperation::LinkerUndoDeleteLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		const EditorChunkItemLinkable * endEcil,
		DataSectionPtr data,
		PropertyIndex propIdx) :
	UndoRedo::Operation(int(typeid(LinkerUndoDeleteLinkOperation).name())),
	data_(data), propIdx_(propIdx)
{
	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();

	endUID_ = endEcil->guid();
	endCID_ = endEcil->getOutsideChunkId();
}


/**
 *  This restores the link information.
 */
/*virtual*/ void LinkerUndoDeleteLinkOperation::undo()
{
	EditorChunkItemLinkable * startEcil =
		WorldManager::instance().linkerManager().forceLoad( startUID_, startCID_ );
	EditorChunkItemLinkable * endEcil =
		WorldManager::instance().linkerManager().forceLoad( endUID_, endCID_ );
	
	// The force load should be successful for all cases
	MF_ASSERT(	startEcil && endEcil &&
				"LinkerUndoDeleteLinkOperation::undo : "
				"One or more linker objects failed to force load!" );

	UndoRedo::instance().add(new LinkerUndoAddLinkOperation(startEcil, endEcil, propIdx_));

	// Need to create the property
	startEcil->propHelper()->propSet( propIdx_.valueAt(0), data_ );

	WorldManager::instance().linkerManager().updateLink( startEcil, endEcil );

	// Save the changes
	startEcil->chunkItem()->edSave( startEcil->chunkItem()->pOwnSect() );
	if ( startEcil->chunkItem()->chunk() != NULL )
        WorldManager::instance().changedChunk( startEcil->chunkItem()->chunk() );

	endEcil->chunkItem()->edSave( endEcil->chunkItem()->pOwnSect() );
	if ( endEcil->chunkItem()->chunk() != NULL )
        WorldManager::instance().changedChunk( endEcil->chunkItem()->chunk() );

}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUndoDeleteLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}


/**
 *  This is the LinkerUpdateLinkOperation constructor.
 *
 *  @param startEcil		The source linker object.
 *  @param targetUID		The target linker object's Unique Id.
 *  @param targetChunkID	The target linker object's Chunk Id.
 */
LinkerUpdateLinkOperation::LinkerUpdateLinkOperation(
		const EditorChunkItemLinkable * startEcil,
		std::string targetUID,
		std::string targetChunkID) :
	UndoRedo::Operation(int(typeid(LinkerUpdateLinkOperation).name()))
{
	startUID_ = startEcil->guid();
	startCID_ = startEcil->getOutsideChunkId();
	targetUID_ = UniqueID(targetUID);
	targetCID_ = targetChunkID;
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void LinkerUpdateLinkOperation::undo()
{
	EditorChunkItemLinkable* startEcil =
		WorldManager::instance().linkerManager().forceLoad(startUID_, startCID_);
	EditorChunkItemLinkable* targetEcil =
		WorldManager::instance().linkerManager().forceLoad(targetUID_, targetCID_);
	
	// The force load should be successful for all cases
	MF_ASSERT(	startEcil && targetEcil &&
				"LinkerUpdateLinkOperation::undo : "
				"One or more linker objects failed to force load!" );

	UndoRedo::instance().add(new LinkerUpdateLinkOperation(startEcil, targetUID_, targetCID_));

	WorldManager::instance().linkerManager().updateLink(startEcil, targetEcil);

	// Save the changes
	startEcil->chunkItem()->edSave( startEcil->chunkItem()->pOwnSect() );
	if ( startEcil->chunkItem()->chunk() != NULL )
        WorldManager::instance().changedChunk( startEcil->chunkItem()->chunk() );

	targetEcil->chunkItem()->edSave( targetEcil->chunkItem()->pOwnSect() );
	if ( targetEcil->chunkItem()->chunk() != NULL )
        WorldManager::instance().changedChunk( targetEcil->chunkItem()->chunk() );
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool LinkerUpdateLinkOperation::iseq(
	UndoRedo::Operation const &other) const
{
    return false;
}
