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
#include "chunk_vlo_obstacle.hpp"

#include <set>
#include "chunk_space.hpp"
#include "chunk_obstacle.hpp"

#ifdef MF_SERVER
#include "server_super_model.hpp"
#else  // MF_SERVER (is client)
#include "model/super_model.hpp"
#endif // MF_SERVER

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

// ----------------------------------------------------------------------------
// Section: ChunkVLOObstacle
// ----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkVLOObstacle::ChunkVLOObstacle( Chunk & chunk ) :
	ChunkModelObstacle( chunk )
{
}


/**
 *	Destructor
 */
ChunkVLOObstacle::~ChunkVLOObstacle()
{
}

/**
 *	This private method adds one obstacle to its chunk space columns.
 *  Overrides the base class implementation to add an obstacle only to the
 *  chunk specifed.
 *
 *	@return number of columns added to (always 1 now)
 */
int ChunkVLOObstacle::addToSpace( ChunkObstacle & cso )
{
	int foco = 0;
	
	// add to this chunk's column
	ChunkSpace::Column * pColumn = this->pChunk_->space()->column( this->pChunk_->boundingBox().centre() );
	if (!pColumn)
	{
		ERROR_MSG( "ChunkVLOObstacle::addToSpace: "
				"Object is not inside the space -\n" );
		return 0;
	}

	// and add it
	pColumn->addObstacle( cso );

	return 1;
}


/**
 *	This method deletes the obstacle from its chunk space column
 */
void ChunkVLOObstacle::delFromSpace( ChunkObstacle & cso )
{
	// add to this chunk's column
	ChunkSpace::Column * pColumn = this->pChunk_->space()->column( this->pChunk_->boundingBox().centre() );

	// and flag them all as stale
	if (pColumn)
		pColumn->stale();
}


/// Static instance accessor initialiser
ChunkCache::Instance<ChunkVLOObstacle> ChunkVLOObstacle::instance;
