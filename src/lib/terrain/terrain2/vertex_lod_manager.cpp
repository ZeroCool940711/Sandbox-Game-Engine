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
#include "vertex_lod_manager.hpp"

#include "../terrain_settings.hpp"
#include "cstdmf/bgtask_manager.hpp"
#include "aliased_height_map.hpp"
#include "terrain_block2.hpp"
#include "terrain_height_map2.hpp"

using namespace Terrain;

namespace
{
	uint32 numVertexLodsQueued = 0;
}

/**
* Construct an empty TerrainLodManager with space for a given number of LODs.
* @param owner		Reference to owning block.
* @param numLods	How many LODs to create.
*/
VertexLodManager::VertexLodManager( TerrainBlock2& owner, uint32 numLods )
	: owner_( owner )
{
	BW_GUARD;
	object_ = new VertexLodArray( numLods );

	static bool watchesAdded = false;
	if ( !watchesAdded )
	{
		watchesAdded = true;
		MF_WATCH( "Render/Terrain/Terrain2/numVertexLodsQueued", 
			numVertexLodsQueued, Watcher::WT_READ_ONLY,
			"Number of vertex lods waiting to be generated." );
	}
}

VertexLodManager::~VertexLodManager()
{
	object_ = NULL;
}

/**
 * Perform lod manager specific streaming.
 */
void VertexLodManager::stream( ResourceStreamType streamType )
{
	BW_GUARD;
	// Is the requested working set the same as what we have? 
	if ( !requestedWorkingSet_.IsWithin( currentWorkingSet_ ) )
	{
		// Working set is different - we need to issue a load if there isn't 
		// already one in place.
		if ( getState() != RS_Loading )
		{
			numVertexLodsQueued++;

			// copy requested set for load request
			loadingWorkingSet_ = requestedWorkingSet_; 

			// Work out if we're overlapping our requested set.
			bool overlap = 
				currentWorkingSet_.IsOverlapping( requestedWorkingSet_ );

			// Remove unwanted from set, but if we're not overlapping (that is
			// we moved so fast we need entirely different lods) its better to
			// hang onto one lod for rendering.
			evictNotInSet( requestedWorkingSet_, !overlap );

			// Remember how we loaded this
			streamType_ = streamType;

			if ( streamType == RST_Asyncronous )
			{
				// increment reference to owner, so it doesn't get removed
				// during asynchronous load.
				owner_.incRef();

				// create a background task for load request
				startAsyncTask();
			}
			else
			{
				// perform load request immediately
				load();
			}
		}
	}
}

/**
 * Override Resource<> load method, using the loadingWorkingSet_ as set by 
 * stream() above.
 */
bool VertexLodManager::load()
{
	BW_GUARD;
	bool status = true;

	for ( uint32 i = loadingWorkingSet_.start_; 
			i < loadingWorkingSet_.end_ + 1 ; i++ )
	{
		status &= generate( i );
	}

	// If we succeeded, update the new working set.
	if ( status )
	{
		// Set the new current working set
		currentWorkingSet_.start_ = loadingWorkingSet_.start_;
		currentWorkingSet_.end_	= loadingWorkingSet_.end_;
	}
	else
	{
		WARNING_MSG( "VertexLodManager::load() Failed to create vertex lod. \n" );
		currentWorkingSet_.start_	= 0;
		currentWorkingSet_.end_		= 0;
	}

	// Finish off

	if ( streamType_ == RST_Asyncronous)
	{
		// remove reference to owner, which can be deleted now
		owner_.decRef();
	}
	
	numVertexLodsQueued--;

	return status;
}

/* This method returns a specific LOD. If that LOD is null, then it will
* only try to return another one if doSubstitution is true. Even then,
* this function may still return a null lod if the lod cache is empty.
*
* @parm level the LOD required ( from 0 to getNumLods()-1 ).
* @param doSubsitution Try to find an alternative lod.
* @return the requested LOD (if not NULL), or a substitute, or NULL.
*/
VertexLodEntryPtr VertexLodManager::getLod( uint32	level, 
										    bool	doSubstitution /*= false */ )
{
	BW_GUARD;
	// Get target LOD
	VertexLodEntryPtr targetLOD = (*object_)[ level ];

	// If necessary substitute with the most detailed LOD available.
	if ( !targetLOD.exists() && doSubstitution )
	{
		const uint32 numLods = getNumLods();

		for ( uint32 i = 0; i < numLods; i++ )			
		{
			VertexLodEntryPtr substituteLOD = (*object_)[ i ];
			if ( substituteLOD.exists() )
			{
				targetLOD = substituteLOD;
				break;
			}
		}
	}

	return targetLOD;
}

/**
 * Evict any LODs that aren't in working set.
 */
void VertexLodManager::evictNotInSet( const WorkingSet & workingSet,
									  bool preserveOne )
{
	BW_GUARD;
	const uint32 numLods = getNumLods();

	// Evict lods we don't need
	for ( uint32 i = 0; i < numLods; i++ )
	{
		// if its not in the working set, evict it.
		if ( i < workingSet.start_ || i > workingSet.end_ )
		{
			// if theres a lod not in set
			if ( (*object_)[ i ] != NULL )
			{
				// If we need to keep one, skip and turn off flag then
				// continue.
				if ( preserveOne )
				{
 					preserveOne = false;
 					continue;
				}

				(*object_)[ i ] = NULL;
			}
		}
	}
}

/**
 * Generate the vertex lod for given level.
 */
bool VertexLodManager::generate( uint32 level )
{
	BW_GUARD;
	// don't load if already there
	if ( (*object_)[ level ].exists() )
	{
		return true;
	}

	// get best available height map from owner.
	TerrainHeightMap2Ptr pHeightMap = owner_.getHighestLodHeightMap();
	MF_ASSERT( pHeightMap.exists() );

	uint32 hmLod = pHeightMap->lodLevel();

	// We can't make a vertex grid larger than source height map
	if ( hmLod > level )
	{
		WARNING_MSG("VertexLodManager::generate() can't generate, "
			"source height map lod is too small - got %d, need %d.\n",
			hmLod, level );
		return false;
	}

	// work out relative lod level of this height map to the vertex grid.
	uint32 relLodLevel = level - hmLod;

	// The order this occurs is important - we can't assign an uninitialised
	// LodEntry to cache, as it may be used in other thread.
	VertexLodEntryPtr newLod = new VertexLodEntry();
	MF_ASSERT( newLod.exists() );

	AliasedHeightMap baseMap( relLodLevel, pHeightMap );
	AliasedHeightMap previousMap( relLodLevel + 1, pHeightMap );

	// Get desired size of vertex grid.
	uint32 gridSize = getLodSize( level, getNumLods() );

	if ( newLod->init( &baseMap, &previousMap, gridSize ) )
	{
		// Now that it is fully initialized, add it to list ( where main thread
		// may pick it up ).
		(*object_)[ level ] = newLod;
		return true;
	}

	return false;
}
