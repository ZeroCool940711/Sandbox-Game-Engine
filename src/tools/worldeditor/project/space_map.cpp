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
#include "worldeditor/project/space_map.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk.hpp"
#include "chunk/base_chunk_space.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "moo/base_texture.hpp"
#include "moo/texture_manager.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/texture_compressor.hpp"
#include "moo/effect_material.hpp"
#include "romp/custom_mesh.hpp"
#include "common/material_utility.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )


SpaceMap* SpaceMap::s_instance_ = NULL;


SpaceMap& SpaceMap::instance()
{
	if (s_instance_ == NULL)
		s_instance_ = new SpaceMap();
	return *s_instance_;
}


/*static*/ void SpaceMap::deleteInstance()
{ 
	delete s_instance_;
	s_instance_ = NULL;

	SpaceMapDebug::deleteInstance();
}


SpaceMap::SpaceMap():
	nTexturesPerFrame_(1),
	nPhotosPerFrame_(1),	
	map_( "spaceMap" ),
	material_( NULL ),
	cacheNeedsRetrieval_( true ),
	modifiedThumbnails_(20),
    deviceReset_(false),
	mark_(0)
{
	material_ = new Moo::EffectMaterial();
	material_->load( BWResource::openSection( "resources/materials/space_map.mfm" ));
	MaterialUtility::viewTechnique( material_, "spaceMap" );	
}


SpaceMap::~SpaceMap()
{
}


void SpaceMap::setTexture( uint8 textureStage )
{
	Moo::rc().setTexture( textureStage, map_.pTexture() );
}


DX::BaseTexture *SpaceMap::texture()
{
	return map_.pTexture();
}


void SpaceMap::spaceInformation( const SpaceInformation& info )
{
	if ( info != info_ )
	{
		info_ = info;

		this->timestampCache().spaceInformation( info_ );
		SpaceMapDebug::instance().spaceInformation( info_ );
		allThumbnails_.spaceInformation( info_ );
		missingThumbnails_.spaceInformation( info_ );
		modifiedThumbnails_.spaceInformation( info_ );

		this->load();
	}
}


bool SpaceMap::init( DataSectionPtr pSection )
{
	return true;
}


void SpaceMap::createRenderTarget()
{
	if ( !map_.pTexture() )
	{
		//TODO : make configurable and device dependent.
		int width = 2048;
		int height = 2048;
		map_.create( width, height );
        map_.clearOnRecreate(true, 0xffffffff);
	}
}


void SpaceMap::createTextures()
{
	this->createRenderTarget();
    if (!deviceReset_)
	{
		this->loadTemporaryCache();
        cacheNeedsRetrieval_ = false;
	}
}


void SpaceMap::releaseTextures()
{
	if ( map_.pTexture() != NULL )
	{
        if (!deviceReset_)
		    this->saveTemporaryCache();
		map_.release();
	}
}


void SpaceMap::createUnmanagedObjects()
{
	createTextures();
}


void SpaceMap::deleteUnmanagedObjects()
{
    deviceReset_ = Moo::rc().device()->TestCooperativeLevel() != D3D_OK;
	releaseTextures();
}


void SpaceMap::update( float dTime, bool fullUpdate /*= false*/ )
{
	static enum
	{
		PHOTOGRAPH_DIRTY_THUMBNAIL,
		SWAPIN_UPDATED_THUMBNAIL,
		SWAPIN_MODIFIED_THUMBNAIL,
		INSPECT_TILES,
		PHOTOGRAPH_MISSING_THUMBNAIL
	}
	currentTask = PHOTOGRAPH_DIRTY_THUMBNAIL;

	// Rendering is done here, so we need to have a valid device:
	if (Moo::rc().device()->TestCooperativeLevel() != D3D_OK)
	{
		return;
	}

    // If the device was reset then the best we can do is to redraw everything.
    if (deviceReset_)
    {
        recreateAfterReset();
        deviceReset_ = false;
    }
    // If the window was resized then load the cached image (this is not 
    // possible on a device reset).
	else if (cacheNeedsRetrieval_)
    {
		this->loadTemporaryCache();
        cacheNeedsRetrieval_ = false;
    }

	WorldManager::instance().markChunks();
	ChunkManager::instance().tick( dTime );

	WorldManager::instance().checkMemoryLoad();

	// Now if for whatever reason the cacheNeedsRetrieval_ is still true,
	// make sure we don't do anything this frame!  It could overwrite useful
	// data.
	if (cacheNeedsRetrieval_)
	{
		return;
	}

	// Any dirty chunks should be photographed first.  they should be in memory
	if (fullUpdate || currentTask == PHOTOGRAPH_DIRTY_THUMBNAIL)
	{
		currentTask = SWAPIN_UPDATED_THUMBNAIL;
		photographChunks( nPhotosPerFrame_, dirtyThumbnails_ );
		if (!fullUpdate)
		{
			return;
		}
	}
	// Any recently photographed thumbnails get added onto the map.
	if (fullUpdate || currentTask == SWAPIN_UPDATED_THUMBNAIL)
	{
		currentTask = SWAPIN_MODIFIED_THUMBNAIL;
		swapInTextures( nTexturesPerFrame_, updatedThumbnails_, NULL, 0xffffffff );
		if (!fullUpdate)
		{
			return;
		}
	}

	// Then look on disk to see if any thumbnails have been saved to disk
	// but not written into the space map as recently. (should not happen)
	if (fullUpdate || currentTask == SWAPIN_MODIFIED_THUMBNAIL)
	{
		currentTask = INSPECT_TILES;
		swapInTextures( nTexturesPerFrame_, modifiedThumbnails_, NULL, 0xffffffff );
		if (!fullUpdate)
		{
			return;
		}
	}

	if (fullUpdate || currentTask == INSPECT_TILES)
	{
		currentTask = PHOTOGRAPH_MISSING_THUMBNAIL;
		// Inspect tiles finds chunks that have missing thumbnails (should not happen)
		if (!inspectTiles( 1, allThumbnails_ ))
		{
			allThumbnails_.reset();
		}
		if (!fullUpdate)
		{
			return;
		}
	}

	currentTask = PHOTOGRAPH_DIRTY_THUMBNAIL;
	// Try to create an missing thumbnails.
	photographChunks( nPhotosPerFrame_, missingThumbnails_ );
}


/**
 *	This method looks at all the tiles provided by the ChunkWalker,
 *	and checks for thumbnail data section existence.
 *
 *	Any missing maps go into the missingThumbnails cache.
 */
bool SpaceMap::inspectTiles( uint32 n, IChunkWalker& chunkWalker )
{
	int16 gridX, gridZ;
	std::string mapName;
	std::string pathName, chunkName;
	uint32 num = n;

	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	pathName= dirMap->path();

	while ( num > 0 )
	{
		if (!chunkWalker.nextTile(chunkName, gridX, gridZ))
			break;

		if ( !::thumbnailExists( pathName, chunkName ) )
		{
            if (!missingThumbnails_.added( gridX, gridZ ))
			    missingThumbnails_.add( gridX, gridZ );
		}
		num--;
	}

	return (num<n);
}


/**
 *	This method swaps n textures into the large bitmap, given
 *	a chunk walker.
 */
bool SpaceMap::swapInTextures
( 
    uint32              n, 
    IChunkWalker        &chunkWalker, 
    CacheChunkWalker    *removeCache,
	uint32				hintColour
)
{
	// Now if for whatever reason the cacheNeedsRetrieval_ is true,
	// make sure we don't actually write anything into the space map,
	// It could overwrite useful data.
	if (cacheNeedsRetrieval_)
		return false;

	std::string mapName;
	std::string pathName, chunkName;
	int16 gridX, gridZ;

	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	pathName= dirMap->path();

	uint32 num = n;

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	bool didOne = false;
	if ( map_.pTexture() && map_.push() )
	{
		material_->begin();
		for ( uint32 i=0; i<material_->nPasses(); i++ )
		{
			material_->beginPass(i);
			
			while ( num > 0 )
			{
				if (!chunkWalker.nextTile( chunkName, gridX, gridZ ))
					break;				

				bool triedLoad = false;
				Moo::BaseTexturePtr pTexture = this->thumbnail(chunkName, &triedLoad);

				if ( pTexture )
				{
					Moo::rc().setTexture(0,pTexture->pTexture());
					this->drawGridSquare( gridX, gridZ );
					SpaceMapDebug::instance().onDraw(gridX, gridZ, hintColour);						
					++mark_;
					didOne = true;
				}
				else
				{
					Moo::rc().setTexture(0,NULL);
					if (triedLoad)
						ERROR_MSG( "SpaceMap::swapInTextures(...) - Could not load bmp %s, even though the resource exists\n", chunkName.c_str() );
					else
						ERROR_MSG( "SpaceMap::swapInTextures(...) - Could not load cdata file %s%s.cdata\n", pathName.c_str(), chunkName.c_str() );
				}
				if (removeCache != NULL)
					removeCache->erase( gridX, gridZ );				
					
				--num;
			}
			material_->endPass();
		}
		material_->end();

		map_.pop();
	}

    Moo::rc().endScene();

	return didOne;
}


/**
 *	This method takes photographs of all chunks in our dirtyThumbnails_ list,
 *	possibly shows a progress bar for the entire operation,
 *	and draws them onto the space map (done by photographChunks).
 */
void SpaceMap::regenerateAllDirty(bool showProgress)
{
	ProgressTask       *genHMTask = NULL;
    
    try
    { 
        size_t numOperationsDone = 0; // Number of steps calculated so far.
        size_t numOperations     = 0; // Number of steps in total.

        // Create a progress indicator if requested:
        if (showProgress)
        {
            float numOperations = (float)dirtyThumbnails_.size();

            genHMTask =
                new ProgressTask
                (
                    WorldManager::instance().progressBar(), 
                    L("WORLDEDITOR/WORLDEDITOR/PROJECT/SPACE_MAP/UPDATE_PROJECT_VIEW"), 
                    (float)(numOperations)
                );
		}

        // Photograph dirty thumbnails.  The photograph chunks method
		// also swaps them into the space map.
        while (dirtyThumbnails_.size() != 0)
        {
            photographChunks( 1, dirtyThumbnails_ );            
            if (genHMTask != NULL)
            {
                genHMTask->step();
                ++numOperationsDone;
            }
        }

        // Update the progress indicator to look as though everything was done:
        if (genHMTask != NULL)
        {
            while (numOperationsDone < numOperations)
            {
                genHMTask->step();
                ++numOperationsDone;
            }
        }

        // Cleanup:
        delete genHMTask; genHMTask = NULL;

        // Save the result:
        save();
    }
    catch (...)
    {
        delete genHMTask; genHMTask = NULL;
        throw;
    }
}


/**
 *	This returns a token that is incremented each time the space map changes
 *	in some way.
 *
 *  @returns		An integer that can be used to determine whether the
 *					space map has changed.
 */
uint32 SpaceMap::mark() const
{
	return mark_;
}


bool SpaceMap::swapInTextures( Chunk* chunk )
{
	if (cacheNeedsRetrieval_)
		return false;

	if( !chunk->isOutsideChunk() )
		return false;

	bool success = false;

	std::string mapName;
	std::string pathName, chunkName;
	int16 gridX, gridZ;

	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	pathName= dirMap->path();

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	if ( map_.pTexture() && map_.push() )
	{
		material_->begin();
		for ( uint32 i=0; i<material_->nPasses(); i++ )
		{
			material_->beginPass(i);

			if( !WorldManager::instance().chunkDirMapping()->gridFromChunkName( chunk->identifier(), gridX, gridZ ) )
				break;
			chunkID( chunkName, gridX, gridZ );

			bool triedLoad = false;
			Moo::BaseTexturePtr pTexture = thumbnail(chunk->identifier(), &triedLoad);
			
			if ( pTexture != NULL )
			{
				Moo::rc().setTexture(0,pTexture->pTexture());
				this->drawGridSquare( gridX, gridZ );
				SpaceMapDebug::instance().onDraw(gridX, gridZ, 0x000000ff);				
				++mark_;
				success = true;				
			}
			else
			{
				Moo::rc().setTexture(0,NULL);
				if (triedLoad)
					ERROR_MSG( "SpaceMap::swapInTextures(chunk) - Could not load bmp %s, even though the resource exists\n", mapName.c_str() );
				else
					ERROR_MSG( "SpaceMap::swapInTextures(chunk) - Could not load cdata file %s%s.cdata\n", pathName.c_str(), chunkName.c_str() );
			}

			material_->endPass();
		}
		material_->end();

		map_.pop();
	}

	Moo::rc().endScene();	

	return success;	
}


/**
 *	This method photographs the next n appropriate chunks and saves
 *	their thumbnails.
 *
 *	For each chunk photographed, the tile is swapped in immediately.
 */
bool SpaceMap::photographChunks( uint32 n, IChunkWalker& chunkWalker )
{
	CacheChunkWalker ccw;

	std::string pathName, chunkName, mapName;
	int16 gridX, gridZ;

	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	pathName= dirMap->path();

	ChunkSpacePtr pSpace = ChunkManager::instance().cameraSpace();

	uint32 num = n;
	while ( num > 0 )
	{
		if (!chunkWalker.nextTile( chunkName, gridX, gridZ ))
			break;

		Chunk* pChunk = pSpace->findChunk( chunkName, "" );
		if ( pChunk && pChunk->online() )
		{
			WorldManager::instance().discardChunkForThumbnail( pChunk );

			bool ok = EditorChunkCache::instance(*pChunk).calculateThumbnail();
			if (ok)
				ccw.add( gridX, gridZ );
			++mark_;
		}
		else
		{
			WorldManager::instance().loadChunkForThumbnail( chunkName );
			++num;
		}

		num--;
	}

	if ( num<n )
	{
		swapInTextures( (n-num), ccw, NULL, 0xffffffff );
		return true;
	}
	return false;
}


/**
 *	This method calcualtes the screen-space area
 *	for the given grid square, and calls drawTile with those
 *	values.
 */
void SpaceMap::drawGridSquare( int16 gridX, int16 gridZ )
{		
	if (cacheNeedsRetrieval_)
		return;

	uint16 biasedX,biasedZ;
	::biasGrid(info_.localToWorld_,gridX,gridZ,biasedX,biasedZ);	

	float dx = Moo::rc().screenWidth() / float(info_.gridWidth_);
	float dy = Moo::rc().screenHeight() / float(info_.gridHeight_);

	float x = (float(biasedX) / float(info_.gridWidth_)) * (Moo::rc().screenWidth());
	float y = (float(biasedZ) / float(info_.gridHeight_))* (Moo::rc().screenHeight());

	drawTile( x, y, dx, dy );

	this->timestampCache().touch( gridX, gridZ );
}


/**
 *	Draws a single tile ( i.e quad ) in screen space.
 *
 *	Pass in non-texel aligned screen space coordinates.
 */
void SpaceMap::drawTile( float x, float y, float dx, float dy )
{
	static CustomMesh< Moo::VertexTUV > s_gridSquare( D3DPT_TRIANGLEFAN );

	Vector2 uvStart( 0.f, 0.f );
	Vector2 uvEnd( 1.f, 1.f );

	if ( dy < 0.f )
	{
		//handle case of flipped textures.
		uvEnd.y = 0.f;
		uvStart.y = 1.f;
		y += dy;
		dy = -dy;
	}

	//screen/texel alignment	
	x -= 0.5f;
	y -= 0.5f;	

	s_gridSquare.clear();
	Moo::VertexTUV v;
	v.pos_.set(x,y,1.f,1.f);
	v.uv_.set(uvStart.x,uvEnd.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x+dx,y,1.f,1.f);
	v.uv_.set(uvEnd.x,uvEnd.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x+dx,y+dy,1.f,1.f);
	v.uv_.set(uvEnd.x,uvStart.y);
	s_gridSquare.push_back(v);

	v.pos_.set(x,y+dy,1.f,1.f);
	v.uv_.set(uvStart.x,uvStart.y);
	s_gridSquare.push_back(v);

	s_gridSquare.drawEffect();	
}


bool SpaceMap::cacheName( std::string& ret )
{
	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	if ( dirMap )
	{
		ret = dirMap->path() + "space";
		return true;
	}
	return false;
}


void SpaceMap::load()
{
	CWaitCursor wait;

	std::string name;
	if ( !cacheName( name ) )
		return;

	// Ensure that the render target is ok.
	this->createRenderTarget();

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	// First load the cache texture.
	if ( map_.pTexture() && map_.push() )
	{
		material_->begin();
		for ( uint32 i=0; i<material_->nPasses(); i++ )
		{
			material_->beginPass(i);
			Moo::rc().setTexture(0,NULL);

			std::string mapName = name + ".thumbnail.dds";

			if (BWResource::openSection(mapName,false))
			{
				Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
						mapName, true, false, true, "texture/project" );

				if ( pTexture )
				{
					Moo::rc().setTexture(0,pTexture->pTexture());
				}
				++mark_;
			}

			this->drawTile( 0.f, Moo::rc().screenHeight(), Moo::rc().screenWidth(), -Moo::rc().screenHeight() );

			material_->endPass();
		}
		material_->end();
		map_.pop();
	}

	Moo::rc().endScene();

	cacheNeedsRetrieval_ = false;

	//load the thumbnail modification date cache.
	this->timestampCache().load();

	//load where the disk traversal was up to last time,
	//so we can restart from there.
	modifiedThumbnails_.load();
}


void SpaceMap::save()
{
	if (cacheNeedsRetrieval_)
		return;	

	if ( !map_.pTexture() )
	{
		return;
	}

	std::string name;
	if (!cacheName( name ))
		return;

	CWaitCursor wait;
	
	std::string mapName = name + ".thumbnail.dds";

	//save the thumbnail modification date cache.
	this->timestampCache().save();

	//save where the disk traversal was up to, so we can restart from there later on.
	modifiedThumbnails_.save();

	//Then save out the texture
	TextureCompressor tc( static_cast<DX::Texture*>(map_.pTexture()), D3DFMT_DXT1, 1 );
	tc.save( mapName );
}


/**
 *	Load temporary cached map off disk, and apply to render target.
 */
void SpaceMap::loadTemporaryCache()
{
	CWaitCursor wait;

	std::string name;
	if ( !cacheName( name ) )
		return;

	Moo::rc().beginScene();
	if (Moo::rc().mixedVertexProcessing())
		Moo::rc().device()->SetSoftwareVertexProcessing( TRUE );

	//First load the cache texture
	if ( map_.pTexture() && map_.push() )
	{
		material_->begin();
		for ( uint32 i=0; i<material_->nPasses(); i++ )
		{
			material_->beginPass(i);
			Moo::rc().setTexture(0,NULL);

			std::string mapName = name + ".temp_thumbnail.dds";
			
			//try retrieving our temporary cache
			if (BWResource::openSection(mapName,false))
			{
				Moo::BaseTexturePtr pTexture = Moo::TextureManager::instance()->get(
						mapName, true, false, true, "texture/project" );

				if ( pTexture )
				{
					Moo::rc().setTexture(0,pTexture->pTexture());
				}

				::DeleteFile( BWResource::resolveFilename( mapName ).c_str() );

				++mark_;
			}

			this->drawTile( 0.f, Moo::rc().screenHeight(), Moo::rc().screenWidth(), -Moo::rc().screenHeight() );
			material_->endPass();
		}
		material_->end();
		map_.pop();
	}

	Moo::rc().endScene();

	//And load the temporary timestamps too
	this->timestampCache().loadTemporaryCopy();

	cacheNeedsRetrieval_ = false;
}


/**
 *	Save cached large space map temporarily to disk, because
 *	we are about to recreate the underlying texture.
 */
void SpaceMap::saveTemporaryCache()
{
	if ( cacheNeedsRetrieval_ )
		return;	

	CWaitCursor wait;

	std::string name;
	if (!cacheName( name ))
		return;
	std::string tempMapName = name + ".temp_thumbnail.dds";

	//Then save out the texture
	TextureCompressor tc( static_cast<DX::Texture*>(map_.pTexture()), D3DFMT_DXT1, 1 );
	tc.save( tempMapName );

	//And save out the timestamps too
	this->timestampCache().saveTemporaryCopy();
}


void SpaceMap::dirtyThumbnail( Chunk* pChunk )
{
	// Add to list of dirty thumbnails, so we eventually photograph the 
    // chunk.
    // Be careful not to re-add an existing chunk.
    if (!dirtyThumbnails_.added( pChunk ))
		dirtyThumbnails_.add( pChunk );
}


void SpaceMap::chunkThumbnailUpdated( Chunk* pChunk )
{
	//TODO : don't necessarily need to swap it in right now.
	//we can just put it on the updatedThumbnails list instead,
	//and when the space map is next visible, or saved, we call
	//swapInTextures for each of these updated ones.

	//HOWEVER doing it right now gives the space map a greater
	//chance of being up-to-date when the user first looks at it,
	//and in fact while the chunk is loaded we may as well use it
	//now.

	// Remove from list of dirty thumbnails, so we don't re-photograph it.
	dirtyThumbnails_.erase( pChunk );

	if (!swapInTextures( pChunk ))
	{
		if (!updatedThumbnails_.added( pChunk ))
			updatedThumbnails_.add( pChunk );
	}
}


void SpaceMap::invalidateAllChunks()
{
	CWaitCursor wait;
	allThumbnails_.reset();

	int16 gridX, gridZ;
	std::string mapName;
	std::string pathName, chunkName;	

	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	pathName= dirMap->path();

	while ( 1 )
	{
		if (!allThumbnails_.nextTile(chunkName, gridX, gridZ))
			break;
		
		DataSectionPtr pSection = BWResource::openSection( pathName + chunkName + ".cdata", false );
		if ( pSection )
		{
			pSection->delChild( "thumbnail.dds" );
			pSection->save();
		}		
	}	
	
	allThumbnails_.reset();
}


void SpaceMap::recreateAfterReset()
{
    allThumbnails_.reset();	

	this->timestampCache().spaceInformation( info_ );
	SpaceMapDebug::instance().spaceInformation( info_ );
	allThumbnails_.spaceInformation( info_ );
	missingThumbnails_.spaceInformation( info_ );
	modifiedThumbnails_.spaceInformation( info_ );

	this->load();
}


/**
 *	This gets either the cached, in-memory thumbnail or the thumbnail off disk.
 *	If neither of these exists then it returns NULL.
 *
 *  @param chunk		The chunk to get the thumbnail for.
 *  @param triedLoad	If this is not null then it will be set to true if
 *						the thumbnail should have existed, but didn't.
 *	@return				The thumbnail for the chunk.
 */
Moo::BaseTexturePtr SpaceMap::thumbnail(const std::string& chunkName, bool *triedLoad) const
{	
	Moo::BaseTexturePtr result;

	if (triedLoad != NULL) *triedLoad = false;

	// If the chunk is in memory get the in-memory thumbnail:
	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	Chunk *chunk = 
		ChunkManager::instance().findChunkByName(chunkName, dirMap, false);
	if (chunk)
	{
		result = EditorChunkCache::instance(*chunk).thumbnail();
	}
	
	// If not yet in memory then load the thumbnail from disk:
	if (result == NULL)
	{			
		std::string pathName= dirMap->path();
		if (::thumbnailExists(pathName, chunkName))
		{
			std::string mapName = 
				::thumbnailFilename(pathName, chunkName);
			result = 
				Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/project");
			if (triedLoad != NULL) *triedLoad = true;
			if (!result)
			{
				//There was a bug that put the thumbnail.dds files as a child
				//of the thumbnail.dds section,
				//i.e. *.cdata/thumbnail.dds/thumbnail.dds instead of 
				//*.cdata/thumbnail.dds.
				//Here we try to load these as well.
				mapName = mapName + "/thumbnail.dds";
				result = Moo::TextureManager::instance()->get(mapName, true, false, true, "texture/project");
			}
		}		
	}

	return result;
}


/**
 *  This method just returns the SpaceMapTimestampCache instance.
 */
SpaceMapTimestampCache & SpaceMap::timestampCache()
{
	return timestampCache_;
}
