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
#include "worldeditor/undo_redo/terrain_hole_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "romp/flora.hpp"


DECLARE_DEBUG_COMPONENT2("WorldEditor", 2)


/**
 *  This is the TerrainHoleMapUndo constructor.
 *
 *  @param block            The terrain block whose holes should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainHoleMapUndo::TerrainHoleMapUndo
(
    Terrain::EditorBaseTerrainBlockPtr  block, 
    ChunkPtr							chunk
):
    UndoRedo::Operation(int(typeid(TerrainHoleMapUndo).name())),
    block_(block),
    chunk_(chunk)
{
    addChunk(chunk_);
    if (block_ != NULL)
    {
        Terrain::TerrainHoleMap &thm = block_->holeMap();
        Terrain::TerrainHoleMapHolder holder(&thm, true);  
        holes_ = thm.image();
    }
}


/**
 *  This restores the holes.
 */
/*virtual*/ void TerrainHoleMapUndo::undo()
{
    // First add the current state of this block to the undo/redo list:
    UndoRedo::instance().add(new TerrainHoleMapUndo(block_, chunk_));
    // Now apply our stored change:
    if (!holes_.isEmpty() && block_ != NULL)
    {
        Terrain::TerrainHoleMap &thm = block_->holeMap();
        Terrain::TerrainHoleMapHolder holder(&thm, false);
        Terrain::TerrainHoleMap::ImageType &image = thm.image();
        image.blit(holes_);    
    }
	// reset the flora
	Flora::floraReset();

    // ...and let everyone know it's changed:
    WorldManager::instance().changedTerrainBlock( chunk_ );
}


/**
 *  This tests to see if the undo operations are the same.
 */
/*virtual*/ bool TerrainHoleMapUndo::iseq(const UndoRedo::Operation &oth) const
{
	const TerrainHoleMapUndo& othTHM = static_cast<const TerrainHoleMapUndo&>(oth);
    return 
		block_ == othTHM.block_ &&
        holes_ == othTHM.holes_;
}
