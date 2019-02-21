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
#include "worldeditor/undo_redo/terrain_texture_layer_undo.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "romp/flora.hpp"
#include "moo/png.hpp"
#include "worldeditor/gui/pages/page_terrain_texture.hpp"


DECLARE_DEBUG_COMPONENT2("WorldEditor", 2)


//----------------------------------------------------------------------------
// Section: class TerrainTextureLayerUndo
//----------------------------------------------------------------------------

/**
 *  This is the TerrainTextureLayerState constructor.
 *
 *  @param block            The terrain block whose blends should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainTextureLayerState::TerrainTextureLayerState(
	Terrain::EditorBaseTerrainBlockPtr block,
	ChunkPtr chunk )
	: block_( block )
	, chunk_( chunk )	
{
    if (block_ != NULL)
    {
        for (size_t i = 0; i < block_->numberTextureLayers(); ++i)
        {
            Terrain::TerrainTextureLayer &ttl = block_->textureLayer(i);
             
            LayerInfo layerInfo;
            layerInfo.textureName_ = ttl.textureName();
			if ( ttl.hasUVProjections() )
			{
				layerInfo.uProjection_ = ttl.uProjection();
				layerInfo.vProjection_ = ttl.vProjection();
			}
            Terrain::TerrainTextureLayerHolder holder(&ttl, true);
			layerInfo.compBlends_ = 
				Moo::compressImage
				(
					ttl.image(), 
					"BinaryBlock/TerrainTextureLayerUndo"
				);
            textureInfo_.push_back(layerInfo);
        }
    }
}


/**
 *	Restores the saved state to the block/chunk
 */
void TerrainTextureLayerState::restore()
{
    // Now apply our stored change:
    if (block_ != NULL)
    {
        // First remove all the existing layers:
        size_t numLayers = block_->numberTextureLayers();
        for (size_t i = 0; i < numLayers; ++i)
            block_->removeTextureLayer(0);

        // Now add back in the old layers:
        for (size_t i = 0; i < textureInfo_.size(); ++i)
        {
            LayerInfo &info = textureInfo_[i];
            int32 newLayer = block_->insertTextureLayer();
            Terrain::TerrainTextureLayer &ttl = block_->textureLayer(newLayer);
            ttl.textureName(textureInfo_[i].textureName_);
            if (ttl.hasUVProjections())
            {
                ttl.uProjection(info.uProjection_);
                ttl.vProjection(info.vProjection_);
            }
			Terrain::TerrainTextureLayer::ImageType img;
			Moo::decompressImage(info.compBlends_, img);
            Terrain::TerrainTextureLayerHolder holder(&ttl, true);
            ttl.image().blit(img);
        }

        // Let the block know that the layers have all changed:
        block_->rebuildCombinedLayers();

	    // reset the flora
	    Flora::floraReset();

        // ...and let everyone know it's changed:
        WorldManager::instance().chunkTexturesPainted(chunk_, false); // let WorldEditor do the LOD textures
        WorldManager::instance().changedTerrainBlock(chunk_);
    }
}


/**
 *	operator == that tests if two TerrainTextureLayerState are the same
 */
bool TerrainTextureLayerState::operator==( const TerrainTextureLayerState &oth ) const
{
	if (block_ != oth.block_ || textureInfo_.size() != oth.textureInfo_.size())
        return false;
    for (size_t i = 0; i < textureInfo_.size(); ++i)
    {
        if 
        (
            textureInfo_[i].textureName_ != oth.textureInfo_[i].textureName_
            ||
            textureInfo_[i].uProjection_ != oth.textureInfo_[i].uProjection_
            ||
            textureInfo_[i].vProjection_ != oth.textureInfo_[i].vProjection_
            ||
            textureInfo_[i].compBlends_.getObject() != oth.textureInfo_[i].compBlends_.getObject()
        )
        {
            return false;
        }
    }
	return true;
}


//----------------------------------------------------------------------------
// Section: class TerrainTextureLayerUndo
//----------------------------------------------------------------------------

/**
 *  This is the TerrainTextureLayerUndo constructor.
 *
 *  @param block            The terrain block whose blends should be 
 *                          done/undone.
 *  @param chunk            The chunk that the block belongs too.
 */
TerrainTextureLayerUndo::TerrainTextureLayerUndo
(
    Terrain::EditorBaseTerrainBlockPtr  block,     
    ChunkPtr                        chunk
):
    UndoRedo::Operation(int(typeid(TerrainTextureLayerUndo).name())),
	layerState_( new TerrainTextureLayerState( block, chunk ) )
{
    addChunk(chunk);
}


/**
 *  This is the TerrainTextureLayerUndo constructor.
 *
 *  @param layerState      TerrainTextureLayerState object containing the state
 *                         that will be used on undo.
 */
TerrainTextureLayerUndo::TerrainTextureLayerUndo
(
    TerrainTextureLayerStatePtr  layerState
):
    UndoRedo::Operation(int(typeid(TerrainTextureLayerUndo).name())),
	layerState_( layerState )
{
	MF_ASSERT( layerState_ != NULL );
    addChunk( layerState_->chunk() );
}


/**
 *  This restores the terrain's texture's blends.
 */
/*virtual*/ void TerrainTextureLayerUndo::undo()
{
    // First add the current state of this block to the undo/redo list:
    UndoRedo::instance().add
    (
        new TerrainTextureLayerUndo( layerState_->block(), layerState_->chunk() )
    );

	layerState_->restore();
	PageTerrainTexture::instance()->updatePythonMask();
}


/**
 *  This tests to see if the undo operations are the same.
 */
/*virtual*/ bool TerrainTextureLayerUndo::iseq(const UndoRedo::Operation &oth) const
{
	const TerrainTextureLayerUndo& othTTL = static_cast<const TerrainTextureLayerUndo&>( oth );

	return *layerState_ == *othTTL.layerState_;
}
