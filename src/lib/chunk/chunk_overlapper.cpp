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
#include "chunk_overlapper.hpp"
#include "chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "resmgr/bwresource.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/guard.hpp"


DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )


// -----------------------------------------------------------------------------
// Section: ChunkOverlapper
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ChunkOverlapper::ChunkOverlapper() :
	pOverlapper_( NULL )
{
}


/**
 *	Destructor.
 */
ChunkOverlapper::~ChunkOverlapper()
{
}

/**
 *	Load this overlapper.
 */
bool ChunkOverlapper::load( DataSectionPtr pSection, Chunk * pChunk, std::string* errorString )
{
	BW_GUARD;
	// does not support reloading! (would not be hard to add)
	IF_NOT_MF_ASSERT_DEV( pOverlapper_ == NULL )
	{
		if( errorString )
			*errorString = "Overlapper chunk item is already loaded";

		return false;
	}

	pChunk->hasInternalChunks( true );

	overlapperID_ = pSection->asString();
	std::string err;
	if (!overlapperID_.empty())
	{
		DataSectionPtr pBBSect = pSection->openSection( "boundingBox" );
		if (!pBBSect)
		{
			pBBSect = BWResource::openSection( pChunk->mapping()->path() +
				overlapperID_ + ".chunk/boundingBox" );
		}
		if (pBBSect)
		{
			pOverlapper_ = new Chunk( overlapperID_, pChunk->mapping() );
#ifndef MF_SERVER
				ChunkManager::instance().loadChunkExplicitly( overlapperID_, pChunk->mapping(), true );
#endif//MF_SERVER
			return true;
		}
		else
		{
			err = "Cannot find bounding box for overlapper for " + overlapperID_;
		}
	}	
	else
	{
		err = "Overlapper chunk item is empty string";
	}

	if ( errorString )
	{
		*errorString = err;
	}
	else
	{
		ERROR_MSG( "%s\n", err.c_str() );
	}

	return false;
}

/**
 *	Toss this overlapper into the given chunk.
 */
void ChunkOverlapper::toss( Chunk * pChunk )
{
	BW_GUARD;
	if (pChunk_ != NULL)
		ChunkOverlappers::instance( *pChunk_ ).del( this );
	for (uint i = 0; i < alsoIn_.size(); i++)
		alsoIn_[i]->del( this, true );
	alsoIn_.clear();
	this->ChunkItem::toss( pChunk );
	if (pChunk_ != NULL && pOverlapper_ != NULL)
		ChunkOverlappers::instance( *pChunk_ ).add( this );
}

/**
 *	Bind this overlapper
 */
void ChunkOverlapper::bind( bool looseNotBind )
{
	BW_GUARD;
	if (!pOverlapper_->ratified())
		pOverlapper_ = pChunk_->space()->findOrAddChunk( pOverlapper_ );
	if( !looseNotBind && pOverlapper_->online() )
		pOverlapper_->bindPortals( true );
	}


/**
 *	A chunk overlappers collection is telling us that we are also in its chunk
 */
void ChunkOverlapper::alsoInAdd( ChunkOverlappers * ai )
{
	BW_GUARD;
	alsoIn_.push_back( ai );
}

/**
 *	A chunk overlappers collection is telling us that we are no longer wanted
 */
void ChunkOverlapper::alsoInDel( ChunkOverlappers * ai )
{
	BW_GUARD;
	for (uint i = 0; i < alsoIn_.size(); i++)
	{
		if (alsoIn_[i] == ai)
		{
			alsoIn_.erase( alsoIn_.begin() + i );
			break;
		}
	}
}

#undef IMPLEMENT_CHUNK_ITEM_ARGS
#define IMPLEMENT_CHUNK_ITEM_ARGS (pSection, pChunk, &errorString)
IMPLEMENT_CHUNK_ITEM( ChunkOverlapper, overlapper, 0 )



// -----------------------------------------------------------------------------
// Section: ChunkOverlappers
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
ChunkOverlappers::ChunkOverlappers( Chunk & chunk ) :
	chunk_( chunk ),
	bound_( false ),
	halfBound_( false ),
	complete_( true )
{
}

/**
 *	Destructor
 */
ChunkOverlappers::~ChunkOverlappers()
{
	BW_GUARD;
	// let all our foreign items know that they are no longer in us
	for (uint i = 0; i < foreign_.size(); i++)
		foreign_[i]->alsoInDel( this );
}


/**
 *	When we are bound, we tell all our overlappers to bind themselves.
 *	It doesn't hurt to do this again when we've already been bound.
 */
void ChunkOverlappers::bind( bool looseNotBind )
{
	BW_GUARD;
	// see if we are complete
	// we only have to get our neighbours to re-check themselves if we
	// are already bound, i.e. if it's only one of our portals getting
	// bound to another chunk that loaded, not when we load ourselves -
	// this is because when we load ourselves all the chunks we are bound
	// to will also get bind called and they will do it for us
	this->checkIfComplete( bound_ );

	// we don't actually want to do anything if we get bound again -
	// if another portal just connected to one of ours, then the
	// ChunkOverlappers object in that chunk will take care of getting
	// the bits of our collection that it wants itself, and it will then
	// share everything it has with us, as we would do if we were binding
	if (bound_) return;

	// get all our overlappers to ratify their chunks
	for (uint i = 0; i < overlappers_.size(); i++)
		overlappers_[i]->bind( looseNotBind );

	// pull in all the overlappers of our neighbours if we also want them -
	// this will bring us up to date with the currently bound world
	ChunkBoundaries & bounds = chunk_.bounds();	// done above
	for (ChunkBoundaries::iterator bit = bounds.begin();
		bit != bounds.end();
		bit++)
	{
		ChunkBoundary::Portals::iterator pit;
		for (pit = (*bit)->boundPortals_.begin();
			pit != (*bit)->boundPortals_.end();
			pit++)
		{
			ChunkBoundary::Portal & p = **pit;
			if (!p.hasChunk()) continue;
			Chunk * pChunk = p.pChunk;
			if (!pChunk->online() || !pChunk->isOutsideChunk()) continue;

			if (ChunkOverlappers::instance.exists( *pChunk ))
			{
				this->copyFrom( ChunkOverlappers::instance( *pChunk ) );
			}
		}
	}

	// we're half way there
	halfBound_ = true;

	// share our overlappers around (including any we just got)
	this->share();

	// if we don't have any overlappers by this stage then we're either
	// never going to get any, or another chunk is going to access us when it
	// wants to give us some, so we may as well delete ourselves right here
	if (overlappers_.empty())
	{
		//ChunkOverlappers::instance.clear( chunk_ );
		// actually, can't do this, because it depends on getting further
		// 'bind' calls for checkIfComplete to work... :(
		bound_ = true;
	}
	else
	{
		// we are now properly bound
		bound_ = true;
	}
}

/**
 *	This method shares out all our overlappers to any other chunks around
 *	us that might want them.
 */
void ChunkOverlappers::share()
{
	BW_GUARD;
	// see if anyone of our online neighbours want any of our overlappers
	// this is very similar to the sharing around of lights
	// (we will get their overlappers by them calling share)
	ChunkBoundaries & bounds = chunk_.bounds();
	for (ChunkBoundaries::iterator bit = bounds.begin();
		bit != bounds.end();
		bit++)
	{
		ChunkBoundary::Portals::iterator pit;
		for (pit = (*bit)->boundPortals_.begin();
			pit != (*bit)->boundPortals_.end();
			pit++)
		{
			ChunkBoundary::Portal & p = **pit;
			if (!p.hasChunk()) continue;
			Chunk * pChunk = p.pChunk;
			if (!pChunk->online() || !pChunk->isOutsideChunk()) continue;

			ChunkOverlappers & oth = ChunkOverlappers::instance( *pChunk );
			oth.copyFrom( *this );
		}
	}
}

/**
 *	See if we want to copy any of the given chunk overlappers' collection
 */
void ChunkOverlappers::copyFrom( ChunkOverlappers & oth )
{
	BW_GUARD;
	bool anyNew = false;

	// see if there are any that we want
	for (uint i = 0; i < oth.overlappers_.size(); i++)
	{
		ChunkOverlapperPtr pOverlapper = oth.overlappers_[i];

		if( !pOverlapper->pOverlapper()->online() )
			continue;

		if (!chunk_.boundingBox().intersects( pOverlapper->bb() ))
			continue;

		//Overlappers::iterator found = std::find(
		//	overlappers_.begin(), overlappers_.end(), pOverlapper );
		//if (found != overlappers_.end()) continue;
		Overlappers::iterator oit;
		for (oit = overlappers_.begin(); oit != overlappers_.end(); ++oit)
		{
			// check the item itself and the chunk it points to
			// (needn't check identifier as all chunks ratified by here)
			if (*oit == pOverlapper) break;
			if ((*oit)->pOverlapper() == pOverlapper->pOverlapper()) break;
		}
		if (oit != overlappers_.end()) continue;

		// ok this is a new one
		overlappers_.push_back( pOverlapper );
		foreign_.push_back( pOverlapper );
		pOverlapper->alsoInAdd( this );
		anyNew = true;

		DEBUG_MSG( "ChunkOverlappers::copyFrom: %s also intersects with %s, "
			"which hails from %s, tipoff by %s\n",
			chunk_.identifier().c_str(),
			pOverlapper->pOverlapper()->identifier().c_str(),
			pOverlapper->chunk()->identifier().c_str(),
			oth.chunk_.identifier().c_str() );
	}

	// if we saw any new ones then tell all our neighbours,
	// as they might also be interested in them
	if (anyNew)
	{
		// if we're not yet bound then we're going to share with
		// everyone after we've finished copying from them all
		if (!bound_)
		{
			// this is just for error checking and my sanity - if we're
			// halfbound then we should never be seeing anything new - we should
			// always have got everything we wanted from our neighbouring chunks
			// already, so when they call share after getting a new chunk from
			// us, which leads to a copyFrom call on us, we shouldn't find
			// anything that we didn't find before
			MF_ASSERT_DEV( !halfBound_ );
			return;
		}

		// ok, it's all good, share away then
		this->share();
	}
}


/**
 *	This method determines whether or not this object is complete,
 *	i.e. does it have all the overlappers it is going to get
 */
void ChunkOverlappers::checkIfComplete( bool checkNeighbours )
{
	BW_GUARD;
	complete_ = true;
	ChunkBoundaries & bounds = chunk_.bounds();
	for (ChunkBoundaries::iterator bit = bounds.begin();
		bit != bounds.end();
		bit++)
	{
		// see if we have any unbound portals ourselves
		ChunkBoundary::Portals::iterator pit;
		for (pit = (*bit)->unboundPortals_.begin();
			pit != (*bit)->unboundPortals_.end();
			pit++)
		{
			ChunkBoundary::Portal & p = **pit;
			if (!p.hasChunk()) continue;
			complete_ = false;
			break;
		}

		// see if any of our bound portals connect to chunks with
		// unbound portals whose bounding boxes intersect ours
		for (pit = (*bit)->boundPortals_.begin();
			pit != (*bit)->boundPortals_.end();
			pit++)
		{
			ChunkBoundary::Portal & p = **pit;
			if (!p.hasChunk()) continue;
			Chunk * pChunk = p.pChunk;
			if (!pChunk->online() || !pChunk->isOutsideChunk()) continue;

			if (checkNeighbours && ChunkOverlappers::instance.exists( *pChunk ))
			{
				ChunkOverlappers & oth = ChunkOverlappers::instance( *pChunk );
				oth.checkIfComplete( false );
				// can't just re-check if not complete since this may be called
				// if chunk unbound and it is now incomplete
			}

			ChunkBoundaries & bounds2 = pChunk->bounds();
			for (ChunkBoundaries::iterator bit2 = bounds2.begin();
				bit2 != bounds2.end() && complete_;
				bit2++)
			{
				ChunkBoundary::Portals::iterator pit2;
				for (pit2 = (*bit2)->unboundPortals_.begin();
					pit2 != (*bit2)->unboundPortals_.end();
					pit2++)
				{
					ChunkBoundary::Portal & p2 = **pit2;
					if (!p2.hasChunk()) continue;

					Vector3 wpt;
					ChunkBoundary::V2Vector & pts = p2.points;
					for (uint i = 0; i < pts.size(); i++)
					{
						pChunk->transform().applyPoint( wpt, Vector3( p2.origin+
							p2.uAxis * pts[i][0] + p2.vAxis * pts[i][1] ) );
						if (chunk_.boundingBox().intersects( wpt, 1.f ))
						{	// just check each pt of portal with our bb
							complete_ = false;
							break;
						}
					}
				}
			}
		}
	}
}


/**
 *	There should be a ChunkOverlappers in every outside chunk at first bind time
 *	or else the sharing system will not work if a chunk without any overlappers
 *	but which should have some binds after one which has them.
 *
 *	So we create an instance for each outside chunk (but can delete it later...)
 */
void ChunkOverlappers::touch( Chunk & chunk )
{
	BW_GUARD;
	if (chunk.isOutsideChunk())
		ChunkOverlappers::instance( chunk );
}



/**
 *	Add the given overlapper to our collection
 */
void ChunkOverlappers::add( ChunkOverlapperPtr pOverlapper, bool foreign )
{
#ifdef EDITOR_ENABLED
	// if we already have an overlapper for this chunk then don't
	// accept another one... doesn't actually harm anything, so only
	// complain if we're running as the editor
	for (Overlappers::iterator it = overlappers_.begin();
		it != overlappers_.end();
		++it)
	{
		if ((*it)->pOverlapper()->identifier() ==
			pOverlapper->pOverlapper()->identifier())
		{
			ERROR_MSG( "ChunkOverlappers::add(%s): "
				"Tried to add two overlappers for the same chunk %s\n",
				chunk_.identifier().c_str(),
				pOverlapper->pOverlapper()->identifier().c_str() );
			return;
		}
	}
#endif

	overlappers_.push_back( pOverlapper );

	if (foreign)
		foreign_.push_back( pOverlapper );
}

/**
 *	Del the given overlapper from our collection
 */
void ChunkOverlappers::del( ChunkOverlapperPtr pOverlapper, bool foreign )
{
	BW_GUARD;
	Overlappers::iterator found = std::find(
		overlappers_.begin(), overlappers_.end(), pOverlapper );
	if (found != overlappers_.end())
		overlappers_.erase( found );

	if (foreign)
	{
		Overlappers::iterator found = std::find(
			foreign_.begin(), foreign_.end(), pOverlapper );
		if (found != foreign_.end())
			foreign_.erase( found );
	}
}

/// static initialiser
ChunkCache::Instance<ChunkOverlappers> ChunkOverlappers::instance;

static std::vector<Chunk*> OverlapperFinder( Chunk* chunk )
{
	BW_GUARD;
	std::vector<Chunk*> result;
	ChunkOverlappers::Overlappers overlappers = ChunkOverlappers::instance( *chunk ).overlappers();
	for( ChunkOverlappers::Overlappers::iterator iter = overlappers.begin();
		iter != overlappers.end(); ++iter )
	{
		Chunk* chunk = (*iter)->pOverlapper();
		if( chunk && chunk->online() )
			result.push_back( chunk );
	}
	return result;
}

static struct OverlapperFinderRegister
{
	OverlapperFinderRegister()
	{
		BW_GUARD;
		if( !Chunk::overlapperFinder() )
			Chunk::overlapperFinder( OverlapperFinder );
	}
}
OverlapperFinderRegister;

// chunk_overlapper.cpp
