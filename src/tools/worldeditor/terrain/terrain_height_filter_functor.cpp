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

#include "terrain_height_filter_functor.hpp"

#include "worldeditor/undo_redo/terrain_height_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"

#include "romp/flora.hpp"

#include "appmgr/options.hpp"

#include "space_height_map.hpp"


PY_TYPEOBJECT( TerrainHeightFilterFunctor )


PY_BEGIN_METHODS( TerrainHeightFilterFunctor )
PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( TerrainHeightFilterFunctor )
	PY_ATTRIBUTE( applyRate )
	PY_ATTRIBUTE( falloff )
	PY_ATTRIBUTE( strengthMod )
	PY_ATTRIBUTE( framerateMod )
	PY_ATTRIBUTE( index )
	PY_ATTRIBUTE( constant )
	PY_ATTRIBUTE( name )
PY_END_ATTRIBUTES()


PY_FACTORY( TerrainHeightFilterFunctor, Functor )


FUNCTOR_FACTORY( TerrainHeightFilterFunctor )


namespace
{
	// Globals to maintain a powf lookup table, which stores the return values of
	// powf( X, useFalloff_ ), where X is between 0.0 and 1.0.
	const int POW_LOOKUP_FACTOR = 1000;
	float s_powLookup[ POW_LOOKUP_FACTOR + 1 ];
	int s_lastUseFalloff = -1;
}


/**
 *	Constructor.
 */
/*explicit*/ TerrainHeightFilterFunctor::TerrainHeightFilterFunctor(
	PyTypePlus * pType ):
		TerrainFunctor( pType ),
		avgHeight_( 0.0f ),
		falloff_( 0.0f ),
		strength_( 0.0f ),
		applying_( false ),
		appliedFor_( 0.0f ),
		applyRate_( 0.0f ),
		useFalloff_( 0 ),
		useStrength_( false ),
		useDTime_( false ),
		filterIndex_( 0 ),
		spaceHeightMap_( new SpaceHeightMap() )
{
	if( MatrixFilter::instance().size() > 0 )
	{
		filter_ = MatrixFilter::instance().filter( 0 );
	}
	else
	{
		filter_.clear();
		// set a minimum, neutral, kernel
		for (int i = 0; i < 9; ++i)
		{
			filter_.kernel_.push_back( 0.0f );
		}
		filter_.kernelWidth_  = 3;
		filter_.kernelHeight_ = 3;
	}
}


/**
 *	Destructor.
 */
TerrainHeightFilterFunctor::~TerrainHeightFilterFunctor()
{
}


/** 
 *	This method updates the height pole height functor.
 *	if the left mouse button is down, the filter will be applied
 *
 *	@param dTime	The change in time since the last frame.
 *	@param tool		The tool we are using.
 */
void TerrainHeightFilterFunctor::update( float dTime, Tool & tool )
{
	bool lBtnDown = InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE   );
	bool mBtnDown = InputDevices::isKeyDown( KeyEvent::KEY_MIDDLEMOUSE );

	if (
		dragHandler().isDragging( MouseDragHandler::KEY_LEFTMOUSE ) 
		|| 
		dragHandler().isDragging( MouseDragHandler::KEY_MIDDLEMOUSE ) )
	{
		if (!applying_)
		{
			beginApplying();
			applying_ = true;
			
			if (s_lastUseFalloff != useFalloff_)
			{
				int exponent = useFalloff_;
				if (exponent == AVERAGE_FALLOFF)
				{
					// Normally we use "useFalloff_" as the exponent, but for
					// "Average" mode we use the "Curve" falloff, to use the
					// same nice falloff.
					exponent = CURVE_FALLOFF;
				}
				// The useFalloff_ member has changed, need to initialise the
				// powf lookup table.
				for(int i = 0; i < POW_LOOKUP_FACTOR + 1; ++i)
				{
					s_powLookup[i] =
						1.0f - powf( 1.0f - i / float( POW_LOOKUP_FACTOR ),
									(float) exponent );
				}
				s_lastUseFalloff = useFalloff_;
			}
		}

		// limit the applications to a fixed rate if desired
		if (applyRate_ > 0.0f)
		{
			appliedFor_ += dTime;

			if (appliedFor_ < 1.0f / applyRate_) 
			{
				return;
			}
			
			// but don't apply more than once per frame...
			appliedFor_ = fmodf( appliedFor_, 1.0f / applyRate_ );
		}

		//per update calculations
		falloff_ = 2.0f/tool.size();
		
		if (useFalloff_ == AVERAGE_FALLOFF)
		{
			if (!lBtnDown)
			{
				// Do nothing if using middle mouse button in average mode.
				return;
			}
			strength_ = filter_.constant_;
		}
		else
		{
			strength_ = lBtnDown ? filter_.constant_ : -filter_.constant_;
		}

		if (useStrength_) 
		{
			strength_ *= tool.strength()*0.001f;
		}
		if (useDTime_) 
		{
			strength_ *= dTime*30.0f;
		}

		if (useFalloff_ == AVERAGE_FALLOFF)
		{
			// Mode is "Average". Get average height into "avgHeight_"
			Vector3 pos = tool.locator()->transform().applyToOrigin();
			
			avgHeight_ = 0.0f;

			Chunk * pChunk = 
				ChunkManager::instance().cameraSpace()->findChunkFromPoint( pos );

			if (pChunk)
			{
				EditorChunkTerrain* pTerrain = static_cast<EditorChunkTerrain*>(
					ChunkTerrainCache::instance( *pChunk ).pTerrain());
				if (pTerrain)
				{
					Vector3 localPos = pChunk->transformInverse().applyPoint( pos );
					int avgCount = 0;
					float stepX = pTerrain->block().heightMap().spacingX();
					float stepZ = pTerrain->block().heightMap().spacingZ();
					float toolHalfSize = std::min( tool.size() / 2.0f, 50.0f ); // max 100x100 pixels averaged for performance.

					for (float z = -toolHalfSize; z < toolHalfSize; z += stepZ)
					{
						for (float x = -toolHalfSize; x < toolHalfSize; x += stepX)
						{
							avgHeight_ += pTerrain->block().heightAt( localPos.x + x, localPos.z + z );
							avgCount++;
						}
					}
					if (avgCount > 0)
					{
						avgHeight_ /= float( avgCount );
					}
				}
			}
		}

		TerrainFunctor::apply( tool );
	}
	else
	{
		if (applying_)
		{
			// turn off functor
			appliedFor_ = applyRate_ > 0 ? 1.0f / applyRate_ : 0.0f;
			applying_ = false;

			endApplying();

			// and put in an undo barrier
			UndoRedo::instance().barrier( "Terrain Height Change", true );

			Flora::floraReset();
		}
	}
}


/**
 *	This method responds to the comma and period keys,
 *	which changes the filter that is to be applied.
 *
 *	@param	keyEvent	The key event to process.
 *	@param	t			The tool we are using.
 */
bool TerrainHeightFilterFunctor::handleKeyEvent( const KeyEvent & keyEvent, Tool & t )
{
	TerrainFunctor::handleKeyEvent( keyEvent, t);

	if (!filter_.included_)
	{
		return false;
	}
	if (keyEvent.isKeyDown())
	{
		int dir = 1;
		switch (keyEvent.key())
		{
		case KeyEvent::KEY_COMMA:
			dir = -1;
			// fallthru intentional
		case KeyEvent::KEY_PERIOD:
			this->filterIndex( filterIndex_ + dir );

			while( !filter_.included_ )
			{
				this->filterIndex( filterIndex_ + dir );
			}

			Options::setOptionInt( "terrain/filter/index", filterIndex_ );

			char sbuf[256];
			bw_snprintf( sbuf, sizeof(sbuf), "Selected filter %d: %s",
				filterIndex_, filter_.name_ );

			WorldManager::instance().addCommentaryMsg( sbuf, 0 );

			return true;
			break;
		}
	}

	return false;
}


/**
 *	This returns whether the tool is being applied.
 *
 *	@return				True if the tool is being applied, false otherwise.
 */
bool TerrainHeightFilterFunctor::applying() const 
{ 
	return applying_; 
}


/**
 *	This method is called to get the resolution of the height poles.
 *
 *	@param chunkTerrain	The terrain whose resolution is being requested.
 *	@param format		This is filled out with the format of the terrain.
 */
void TerrainHeightFilterFunctor::getBlockFormat(
    const EditorChunkTerrain	& chunkTerrain,
    TerrainUtils::TerrainFormat & format ) const
{
	const Terrain::TerrainHeightMap & heightMap = 
		chunkTerrain.block().heightMap();
	format.polesWidth	= heightMap.polesWidth();
	format.polesHeight	= heightMap.polesHeight();
	format.blockWidth	= heightMap.blocksWidth();
	format.blockHeight	= heightMap.blocksHeight();
	format.poleSpacingX = heightMap.spacingX();
	format.poleSpacingY = heightMap.spacingZ();
	format.visOffsX     = heightMap.xVisibleOffset();
	format.visOffsY     = heightMap.zVisibleOffset();
}


/**
 *	This is called whenever a new terrain is touched by the tool, it can be 
 *	used to save the undo/redo buffer for example.
 *
 *  @param chunkTerrain	The new terrain touched.
 */
void TerrainHeightFilterFunctor::onFirstApply( 
	EditorChunkTerrain & chunkTerrain )
{
	WorldManager::instance().lockChunkForEditing( chunkTerrain.chunk(), true );
	UndoRedo::instance().add( 
		new TerrainHeightMapUndo( &chunkTerrain.block(), 
			chunkTerrain.chunk() ) );
}


/**
 *	Determines if the filter is too big (i.e. it exceeds the invisible vertices
 *  at the border of the terrain) so it needs to consider neighbouring blocks.
 *
 *  @param ect		The new terrain touched.
 *  @param format	Terrain format info.
 *  @return			True if the filter is too big and needs neightbours, false
 *					otherwise.
 */
bool TerrainHeightFilterFunctor::considerNeighbours(
	EditorChunkTerrain &				chunkTerrain, 
	const TerrainUtils::TerrainFormat &	format )
{
	if (filter_.noise_)
	{
		return false;
	}


	int filterHalfWidth  = (filter_.kernelWidth_  - 1)/2;
	int filterHalfHeight = (filter_.kernelHeight_ - 1)/2;
	return
		filterHalfWidth > int(format.visOffsX) 
		||
		filterHalfHeight > int(format.visOffsY);
}


/**
 *	This is called for all relevant chunks before any call to applyToSubBlock
 *  occurs. If the filter is too big (i.e. it exceeds the invisible vertices
 *  at the border of the terrain), it will initialise the spaceHeightMap and
 *  add the block's terrain image to it for later processing.
 *
 *  @param chunkTerrain	The new terrain touched.
 */
void TerrainHeightFilterFunctor::onBeginApply(
	EditorChunkTerrain & chunkTerrain )
{
	TerrainUtils::TerrainFormat format;
	this->getBlockFormat( chunkTerrain, format );

	// Locking here, so any other locks are nested and fast.
	Terrain::TerrainHeightMap &heightMap = chunkTerrain.block().heightMap();
	heightMap.lock( false );
	lockMap_.push_back( &chunkTerrain );

	if (filter_.noise_)
	{
		if (singleHeightMapCache_.isEmpty())
		{
			int noiseW = rand() * 8 / RAND_MAX + 8;
			int noiseH = rand() * 8 / RAND_MAX + 8;
			Terrain::TerrainHeightMap::ImageType noise( noiseW, noiseH );
			for (size_t y = 0; y < noise.height(); ++y)
			{
				Terrain::TerrainHeightMap::PixelType * pp   = noise.getRow( y );
				Terrain::TerrainHeightMap::PixelType * pend = pp + noise.width();
				while (pp != pend)
				{
					*pp++ = float(rand()) / RAND_MAX - 0.5f;
				}
			}
			int xratio = std::max( filter_.noiseSizeX_, 1 );
			int yratio = std::max( filter_.noiseSizeY_, 1 );
			float constant = (format.poleSpacingX + format.poleSpacingY) / 2.0f;
			singleHeightMapCache_.resize( noise.width() * xratio, noise.height() * yratio );
			for (size_t y = 0; y < singleHeightMapCache_.height(); ++y)
			{
				for (size_t x = 0; x < singleHeightMapCache_.width(); ++x)
				{
					singleHeightMapCache_.set(
						x, y, 
						constant * noise.getBicubic( float(x) / xratio, float(y) / yratio ) );
				}
			}
		}
	}
	else
	{
		if (considerNeighbours( chunkTerrain, format ))
		{
			if (!spaceHeightMap_->prepared())
			{
				spaceHeightMap_->prepare( format );
			}

			Vector3 blockPos( 
				chunkTerrain.chunk()->transform().applyToOrigin() );
			spaceHeightMap_->add( blockPos[0], blockPos[2], 
				heightMap.image() );
		}
	}
}


/**
 *	This method makes a height pole get its height filtered
 *
 *	@param	chunkTerrain The terrain that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainHeightFilterFunctor::applyToSubBlock
(
	EditorChunkTerrain &		chunkTerrain,
	const Vector3 &				toolOffset,
	const Vector3 &				/*chunkOffset*/,
	const TerrainUtils::TerrainFormat & format,
	int32						minx,
	int32						minz,
	int32						maxx,
	int32						maxz
)
{
	Terrain::TerrainHeightMap&				heightMap	= chunkTerrain.block().heightMap();
	Terrain::TerrainHeightMap::ImageType &	image		= heightMap.image();
	uint32								xVisOffs		= heightMap.xVisibleOffset();
	uint32								zVisOffs		= heightMap.zVisibleOffset();

	int filterHalfWidth  = (filter_.kernelWidth_  - 1) / 2;
	int filterHalfHeight = (filter_.kernelHeight_ - 1) / 2;
	bool useNeighbours = considerNeighbours( chunkTerrain, format );

	int polesWidth = heightMap.polesWidth();

	bool needsClear = false;

	float noiseCoordX = 0;
	float beginNoiseX = 0;
	float noiseCoordY = 0;

	if (!filter_.noise_)
	{
		needsClear = true;
		if (useNeighbours)
		{
			// Filter size too big, might need pixels from neighbouring blocks,
			// so use the virtual heightmap to get the relevant heights.
			// Optimisation: assuming the transform is only translation, so we can
			// apply it here and just increment the coords in the X and Z axes.
			Vector3 minCoords(
				chunkTerrain.chunk()->transform().applyPoint(
					Vector3(
						(minx - filterHalfWidth) * format.poleSpacingX,
						0.0f,
						(minz - filterHalfHeight) * format.poleSpacingY) ) );
			Vector3 maxCoords(
				chunkTerrain.chunk()->transform().applyPoint(
					Vector3(
						(maxx + filterHalfWidth) * format.poleSpacingX,
						0.0f,
						(maxz + filterHalfHeight) * format.poleSpacingY) ) );
			bool imgOk =
				spaceHeightMap_->getRect(
					minCoords[0], minCoords[2],
					maxCoords[0], maxCoords[2],
					singleHeightMapCache_ );
			if (!imgOk)
			{
				// this should never happen
				return;
			}
		}
		else
		{
			// Everything fits in this heightmap, so no need to get pixels from
			// neighbours.
			singleHeightMapCache_ = image;	
		}
	}
	else
	{
		// filter_.noise_ == true.
		// calculate global, zero-based noise coords to avoid seams
		Vector3 spaceCoords(
			chunkTerrain.chunk()->transform().applyPoint(
				Vector3(
					(minx + xVisOffs) * format.poleSpacingX,
					0.0f,
					(minz + zVisOffs) * format.poleSpacingY) ) );
		beginNoiseX =
			spaceCoords[0] -
			ChunkManager::instance().cameraSpace()->minGridX() * GRID_RESOLUTION;
		noiseCoordY =
			spaceCoords[2] -
			ChunkManager::instance().cameraSpace()->minGridY() * GRID_RESOLUTION;
	}
	
	bool powLookupOutOfRange = false;

	for (int32 zIdx = minz; zIdx <= maxz; ++zIdx)
	{
		if (zIdx + zVisOffs >= 0 && zIdx + zVisOffs < image.height())
		{
			float dz = zIdx*format.poleSpacingY - toolOffset.z;
			if (filter_.noise_)
			{
				noiseCoordX = beginNoiseX;
			}

			Terrain::TerrainHeightMap::PixelType * pix =
								image.getRow( zIdx + zVisOffs ) + minx + xVisOffs;

			for (int32 xIdx = minx; xIdx <= maxx; ++xIdx)
			{
				if (xIdx + xVisOffs >= 0 && xIdx + xVisOffs < image.width())
				{
					float dx = xIdx*format.poleSpacingX - toolOffset.x;			
					float falloffFactor = (1.0f - (std::sqrt(dx*dx + dz*dz) * falloff_));
					if (falloffFactor > 0.0f)
					{
						float newPix = 0.0f;

						int powIdx = int( falloffFactor * float( POW_LOOKUP_FACTOR ) );
						if (powIdx < 0 || powIdx > POW_LOOKUP_FACTOR)
						{
							// This should never happen, but better to clamp it
							// than having the user lose all his data in a
							// crash or an assert.
							powLookupOutOfRange = true;
							powIdx = std::max( 0, powIdx );
							powIdx = std::min( powIdx, POW_LOOKUP_FACTOR );
						}
						// fade out the filter effect as desired
						falloffFactor = useFalloff_ ?
							s_powLookup[ powIdx ] : 1.0f;

						if (useFalloff_ == AVERAGE_FALLOFF)
						{
							newPix = (*pix) * (1.0f - strength_) + avgHeight_ * strength_;
							if (fabs( (*pix) - avgHeight_ ) < fabs( (*pix) - newPix ))
							{
								newPix = avgHeight_;
							}
						}
						else
						{
							newPix = strength_ * filter_.strengthRatio_;
							// strength_ is derived from filter_.constant_

							if (!filter_.noise_)
							{
								std::vector<float>::iterator kernelValue = filter_.kernel_.begin();
								int xOffset;
								int zOffset;
								if (useNeighbours)
								{
									xOffset = xIdx + filterHalfWidth - minx;
									zOffset = zIdx + filterHalfWidth - minz;
								}
								else
								{
									xOffset = xIdx + xVisOffs;
									zOffset = zIdx + zVisOffs;
								}

								int mapWidth = singleHeightMapCache_.width();
								int mapHeight = singleHeightMapCache_.height();

								int filterMinX = -filterHalfWidth + xOffset;
								int filterMinZ = -filterHalfHeight + zOffset;
								int filterMaxX = filterHalfWidth + xOffset;
								int filterMaxZ = filterHalfHeight + zOffset;

								int startPadX = filterMinX < 0 ? -filterMinX : 0;
								int startPadZ = filterMinZ < 0 ? -filterMinZ : 0;
								int endPadX = filterMaxX >= mapWidth ? filterMaxX - mapWidth + 1 : 0;
								int endPadZ = filterMaxZ >= mapHeight ? filterMaxZ - mapHeight + 1 : 0;

								Terrain::TerrainHeightMap::PixelType * oldPix;

								kernelValue += startPadX + startPadZ * filter_.kernelWidth_;

								for (int z = filterMinZ + startPadZ; z <= filterMaxZ - endPadZ; z++)
								{
									oldPix =
										singleHeightMapCache_.getRow( z ) + filterMinX + startPadX;

									for (int x = filterMinX + startPadX; x <= filterMaxX - endPadX; x++)
									{
										newPix += (*oldPix++) * (*kernelValue++);
									}
									kernelValue += startPadX + endPadX;
								}

								newPix /= filter_.kernelSum_;
							}
							else
							{
								// calculate mirror tiling
								float mx =
									fmod( noiseCoordX, singleHeightMapCache_.width() * 2.0f );
								if ( mx >= singleHeightMapCache_.width() )
								{
									mx = singleHeightMapCache_.width() * 2.0f - mx - 1.0f;
								}
								float my =
									fmod( noiseCoordY, singleHeightMapCache_.height() * 2.0f );
								if (my >= singleHeightMapCache_.height())
								{
									my = singleHeightMapCache_.height() * 2.0f - my - 1.0f;
								}

								// actual noise painting
								newPix += image.get( xIdx + xVisOffs, zIdx + zVisOffs );
								newPix += filter_.strengthRatio_ * 
									singleHeightMapCache_.getBicubic( mx, my );
							}
						}
						(*pix) = 
							(*pix)
							* 
							(1.0f - falloffFactor) + newPix * falloffFactor;
					}
				}
				++pix;
				if (filter_.noise_)
				{
					noiseCoordX += format.poleSpacingX;
				}
			}
		}
		if (filter_.noise_)
		{
			noiseCoordY += format.poleSpacingY;
		}
	}

	if (needsClear)
	{
		singleHeightMapCache_.clear();
	}

	if (powLookupOutOfRange)
	{
		std::string chunkId = "<unknown>";
		if (chunkTerrain.chunk())
		{
			chunkId = chunkTerrain.chunk()->identifier();
		}

		ERROR_MSG( "Calculation of the index into the powf lookup table "
					"failed at least once for chunk %s.\n",
					 chunkId.c_str() );
	}

}


/**
 *	This is called for all relevant chunks after all calls to applyToSubBlock
 *  occur. If the filter is too big (i.e. it exceeds the invisible vertices
 *  at the border of the terrain), it will clear the spaceHeightMap so it
 *  frees all resources gathered during onBeginApply and applyToSubBlock.
 *
 *  @param chunkTerrain	The new terrain touched.
 */
void TerrainHeightFilterFunctor::onEndApply( 
	EditorChunkTerrain & chunkTerrain )
{
	// Make sure all resources are freed
	if (spaceHeightMap_->prepared())
	{
		spaceHeightMap_->clear();
	}

	singleHeightMapCache_.clear();
}


/**
 *	This method is called when all height poles have been adjusted.
 *	we recalculate the bounding box of the terrain chunks that have
 *	been changed.
 *
 *	@param	t	The tool we are using.
 */
void TerrainHeightFilterFunctor::onApplied( Tool & t )
{

	//Temporarily increase the tools area of influence so that
	//height poles at the edges of chunks are updated correctly.
	t.findRelevantChunks( 4.0f );

	ChunkPtrVector::iterator it  = t.relevantChunks().begin();
	ChunkPtrVector::iterator end = t.relevantChunks().end();

	while (it != end)
	{
		Chunk * pChunk = *it++;

		EditorChunkTerrain * pChunkTerrain = 
			static_cast< EditorChunkTerrain * >(
				ChunkTerrainCache::instance( *pChunk ).pTerrain());

		if (pChunkTerrain != NULL)
		{
			pChunkTerrain->onEditHeights();
			if (newTouchedChunk( pChunkTerrain ))
			{
				this->onFirstApply( *pChunkTerrain );
			}
		}
	}

	t.findRelevantChunks();

	// Unlock all the chunks we have locked.
	for (std::vector< EditorChunkTerrain * >::iterator i = lockMap_.begin();
		i != lockMap_.end(); ++i)
	{
		(*i)->block().heightMap().unlock();
	}
	lockMap_.clear();
}


/**
 *	This is called when the tool has finished being applied.
 *
 *	@param chunkTerrain An EditorChunkTerrain that the tool was applied to.
 */
void TerrainHeightFilterFunctor::onLastApply( 
	EditorChunkTerrain & chunkTerrain )
{
	chunkTerrain.block().rebuildNormalMap( Terrain::NMQ_NICE );
	chunkTerrain.toss( chunkTerrain.chunk() );
	WorldManager::instance().lockChunkForEditing( chunkTerrain.chunk(), false );
	WorldManager::instance().markTerrainShadowsDirty( chunkTerrain.chunk() );
}


/**
 *	This gets an attribute for python.
 *
 *	@param attr		The attribute to get.
 *	@return			The attribute.
 */
PyObject * TerrainHeightFilterFunctor::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return TerrainFunctor::pyGetAttribute( attr );
}


/**
 *	This set an attribute for python.
 *
 *	@param attr		The attribute to set.
 *	@param value	The new value of the attribute.
 */
int TerrainHeightFilterFunctor::pySetAttribute( 
	const char *	attr, 
	PyObject *		value )
{
	PY_SETATTR_STD();

	return TerrainFunctor::pySetAttribute( attr, value );
}

/**
 *	This is the static python factory method.
 *
 *  @param pArgs	The arguments for creation.
 *	@return			A new TerrainHeightFilterFunctor.
 */
PyObject * TerrainHeightFilterFunctor::pyNew( PyObject * /*pArgs*/ )
{
	return new TerrainHeightFilterFunctor();
}


/**
 *	Set the filter to the given preset index.
 *
 *	@param idx		The preset index to use.
 */
void TerrainHeightFilterFunctor::filterIndex( int idx )
{
	int numFils = MatrixFilter::instance().size();
	if (idx >= 0 && idx < numFils)
	{
		filterIndex_ = idx;
		filter_ = MatrixFilter::instance().filter( filterIndex_ );
	}
}
