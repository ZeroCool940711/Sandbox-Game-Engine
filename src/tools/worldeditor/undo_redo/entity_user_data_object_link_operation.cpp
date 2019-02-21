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
#include "worldeditor/undo_redo/entity_user_data_object_link_operation.hpp"
#include "worldeditor/world/world_manager.hpp"


/**
 *  This is the EntityUserDataObjectLinkOperation constructor.
 *
 *  @param entity           The entity whose link information may need to be
 *                          restored.
 */
EntityUserDataObjectLinkOperation::EntityUserDataObjectLinkOperation
(
    EditorChunkEntityPtr        entity,
	const	std::string&		linkName
)
: 
UndoRedo::Operation(int(typeid(EntityUserDataObjectLinkOperation).name())),
entity_(entity), linkName_( linkName )
{
	PropertyIndex propIdx =
		entity_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		ERROR_MSG("Failed to create Undo, could not find property %s of the entity used to construct the link", linkName_);
		return;
	}
	entityLink_ = entity_->propHelper()->propGetString(propIdx);
	addChunk(entity_->chunk());
}


/**
 *  This restores the link information for the entity.
 */
/*virtual*/ void EntityUserDataObjectLinkOperation::undo()
{
    UndoRedo::instance().add(new EntityUserDataObjectLinkOperation(entity_, linkName_));
	PropertyIndex propIdx =
		entity_->propHelper()->propGetIdx(linkName_);
	if ( propIdx.empty() )
	{
		ERROR_MSG("Failed to execute Undo, could not find property %s of the entity used to construct the link", linkName_);
		return;
	}
	entity_->propHelper()->propSetString(propIdx, entityLink_);
}


/**
 *  This compares this operation with another.
 *
 *  @param other        The operation to compare.
 *  @returns            false.
 */
/*virtual*/ bool 
EntityUserDataObjectLinkOperation::iseq(UndoRedo::Operation const &other) const
{
    return false;
}
