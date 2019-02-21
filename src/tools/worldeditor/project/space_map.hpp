/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SPACE_MAP_HPP
#define SPACE_MAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/project/space_information.hpp"
#include "worldeditor/project/space_map_timestamp_cache.hpp"
#include "worldeditor/project/grid_coord.hpp"
#include "worldeditor/project/chunk_walker.hpp"
#include "moo/render_target.hpp"
#include "moo/device_callback.hpp"
#include "resmgr/datasection.hpp"
#include "math/colour.hpp"
#include <set>


/**
 *	This class manages the rendering of an automatically
 *	generated map of the whole space.
 *
 *	It uses small tiles that are comitted to CVS by various
 *	WorldEditor users.
 *
 *	When WorldEditor does a full save, the SpaceMap recreates
 *	any bitmap tiles not up to date by photographing the
 *	appropriate chunks from above.
 */
class SpaceMap : public Moo::DeviceCallback
{
public:
	static SpaceMap& instance();
	static void deleteInstance();
	~SpaceMap();

	void spaceInformation( const SpaceInformation& info );
	void setTexture( uint8 textureStage );
	DX::BaseTexture *texture();
	bool init( DataSectionPtr pSection );
	void update( float dTime, bool fullUpdate = false );

	//this method forces a re-photograph of the whole space.
	void invalidateAllChunks();

	//load cached, saved map off disk.
	void load();

	//save the map to disk.  This also makes sure all changedThumbnails
	//are drawn on the space map.
	void save();

	//notification that something has changed in memory, that
	//means the current thumbnail for the chunk is now invalid.
	void dirtyThumbnail( Chunk* pChunk );

	//notification that a photo was taken of the chunk, so the
	//chunk needs to be swapped into the space map at some stage.
	void chunkThumbnailUpdated( Chunk* pChunk );	

    void regenerateAllDirty(bool showProgress);

	uint32 mark() const;
	void drawDebugOverlay();

	SpaceMapTimestampCache & timestampCache();

private:
	SpaceMap();
	void createTextures();
	void createRenderTarget();
	void releaseTextures();
	void createUnmanagedObjects();
	void deleteUnmanagedObjects();

	void drawGridSquare( int16 gridX, int16 gridZ );
	void drawTile( float x, float y, float dx, float dy );

	bool inspectTiles( uint32 n, IChunkWalker& chunkWalker );	
	bool swapInTextures( Chunk* chunk );
	bool swapInTextures( uint32 n, IChunkWalker& chunkWalker, CacheChunkWalker *removeCache, uint32	hintColour);
	bool photographChunks( uint32 n, IChunkWalker& chunkWalker );

	bool cacheName( std::string& ret );

	//temporary cache ( used only from create/delete unmanaged )
	void saveTemporaryCache();
	void loadTemporaryCache();

    void recreateAfterReset();

	Moo::BaseTexturePtr thumbnail(const std::string& chunkName, bool *triedLoad = NULL) const;	

	Moo::EffectMaterialPtr		material_;
	SpaceInformation			info_;
	int	                        nTexturesPerFrame_;
	int	                        nPhotosPerFrame_;
	Moo::RenderTarget           map_;

	// allThumbnails_ looks through all chunks to see if any thumbs are missing
	LinearChunkWalker           allThumbnails_;

	// missingThumbnails_ lists all chunks that have no thumbnail on disk.
	// this list is populated by results of the allThumbnails_ traversal.
	CacheChunkWalker            missingThumbnails_;

	// modifiedThumbnails_ returns all thumbnails on disk that are newer
	// than the one in the current space map.
	ModifiedFileChunkWalker     modifiedThumbnails_;

	// dirtyThumbnails_ lists all chunks have been changed by WorldEditor, whose
	// thumbnails need to be generated.  It contains the session's current
	// changes.
	CacheChunkWalker            dirtyThumbnails_;

	// updatedThumbnails_ lists all chunks have been photographed, and
	// are ready to put into the space map.
	CacheChunkWalker            updatedThumbnails_;

	std::vector<uint16>         modifiedTileNumbers_;
	bool                        cacheNeedsRetrieval_;
	std::set<Chunk *>           changedThumbnailChunks_;
    bool                        deviceReset_;
	uint32						mark_;
	SpaceMapTimestampCache		timestampCache_;

	static SpaceMap*			s_instance_;
};


#endif // SPACE_MAP_HPP
