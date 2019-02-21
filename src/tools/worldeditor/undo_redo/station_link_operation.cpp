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
#include "worldeditor/undo_redo/station_link_operation.hpp"
#include "worldeditor/world/items/editor_chunk_link.hpp"


/**
 *  StationLinkOperation constructor.
 *
 *  @param a            Node A.
 *  @param b            Node B.
 *  @param lt           The link type between A and B to save.
 */
StationLinkOperation::StationLinkOperation
( 
    EditorChunkStationNodePtr           a,
    EditorChunkStationNodePtr           b,
    ChunkLink::Direction                dir 
): 
    UndoRedo::Operation(int(typeid(StationLinkOperation).name())),
    dir_(dir)
{
    idA      = a->id();
    idGraphA = a->graph()->name();
    idB      = b->id();
    idGraphB = b->graph()->name();
	addChunk( a->chunk() );
	addChunk( b->chunk() );
}


/**
 *  Undo a link-change operation between node A and node B.
 */
/*virtual*/ void StationLinkOperation::undo()
{
    EditorChunkStationNodePtr a_ = getNodeA();
    EditorChunkStationNodePtr b_ = getNodeB();

    if (a_ == NULL || b_ == NULL)
        return;

	// First add the current state of the link to the undo/redo list
	UndoRedo::instance().add
    ( 
        new StationLinkOperation
        (
		    a_, b_, a_->getLinkDirection(&*b_)
        ) 
    );

	// Set to the recorded link type
	EditorChunkLink *newLink = 
        (EditorChunkLink *)a_->createLink( dir_, &*b_ ).getObject();
    // newLink can be NULL if the link direction was DIR_NONE
    if (newLink != NULL)
        newLink->makeDirty();
    a_->makeDirty();
    b_->makeDirty();
}


/**
 *  This tests whether this operation is the same as oth.
 *
 *  @param oth              The other undo operaiton.
 *  @returns                True if the operations are the same.
 */
/*virtual*/ bool StationLinkOperation::iseq
( 
    const UndoRedo::Operation & oth 
) const
{
	const StationLinkOperation& o = 
        static_cast<const StationLinkOperation&>( oth );

	return 
        o.idA == idA 
        && 
        o.idB == idB 
        && 
        o.idGraphA == idGraphA
        &&
        o.idGraphB == idGraphB;
}


/**
 *  Get the node A.  We saved the graph and node id's.
 *
 *  @returns                Node A.
 */
EditorChunkStationNodePtr StationLinkOperation::getNodeA() const
{
    StationGraph *g = StationGraph::getGraph(idGraphA);
    if (g == NULL)
        return NULL;
    ChunkStationNode *aNode = g->getNode(idA);
    if (aNode == NULL)
        return NULL;
    return (EditorChunkStationNode *)aNode;
}


/**
 *  Get the node B.  We saved the graph and node id's.
 *
 *  @returns                Node B.
 */
EditorChunkStationNodePtr StationLinkOperation::getNodeB() const
{
    StationGraph *g = StationGraph::getGraph(idGraphB);
    if (g == NULL)
        return NULL;
    ChunkStationNode *bNode = g->getNode(idB);
    if (bNode == NULL)
        return NULL;
    return (EditorChunkStationNode *)bNode;
}
