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

#include "cstdmf/guard.hpp"

#include "chunk_manager.hpp"

#if UMBRA_ENABLE
#include <umbraCamera.hpp>
#include <umbraCell.hpp>
#include <umbraCommander.hpp>
#include "chunk_umbra.hpp"
#endif

#include <queue>

#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"

#include "chunk_overlapper.hpp"
#include "chunk_boundary.hpp"
#include "chunk_loader.hpp"
#include "chunk_space.hpp"
#include "chunk_exit_portal.hpp"
#include "chunk_vlo.hpp"
#include "chunk.hpp"
#include "chunk_terrain.hpp"

#include "romp/line_helper.hpp"

#include "math/mathdef.hpp"
#include "math/portal2d.hpp"
#include "moo/render_context.hpp"
#include "moo/visual_compound.hpp"
#include "speedtree/speedtree_renderer.hpp"
#include "terrain/terrain_settings.hpp"

#ifndef MF_SERVER
#include "romp/geometrics.hpp"
#endif

DECLARE_DEBUG_COMPONENT2( "Chunk", 0 )

// Force linking with chunktrees
extern int ChunkTree_token;
const int s_chunkTokenSet = ChunkTree_token;

PROFILER_DECLARE( ChunkManager_tick, "ChunkManager Tick" );
PROFILER_DECLARE( ChunkManager_draw, "ChunkManager Draw" );

PROFILER_DECLARE( UMBRA_tick, "UMBRA Tick" );
PROFILER_DECLARE( UMBRA_resolveVisibility, "UMBRA ResolveVisibility" );

PROFILER_DECLARE( SpeedTree_tick, "SpeedTree Tick" );
PROFILER_DECLARE( SpeedTree_beginFrame, "SpeedTree BeginFrame" );
PROFILER_DECLARE( SpeedTree_endFrame, "SpeedTree EndFrame" );
PROFILER_DECLARE( SpeedTree_drawTrees, "SpeedTree DrawTrees" );

// Class statics
std::string ChunkManager::s_specialConsoleString;

// Differentiate between debug and release, since in debug you get lots of
// stuttering if you load chunks at a faster rate than one per frame.
#ifdef _DEBUG

namespace 
{
	// The maximum number of chunks that can be scheduled for loading:
	const uint32 MAX_LOADING_CHUNKS	= 1;

	// The maximum number of chunks that the scan can find to schedule for
	// loading during one call:
	const uint32 MAX_WORTHY_CHUNKS	= 1;
}

#else

namespace 
{
	// The maximum number of chunks that can be scheduled for loading:
	const uint32 MAX_LOADING_CHUNKS	= 20;

	// The maximum number of chunks that the scan can find to schedule for
	// loading during one call:
	const uint32 MAX_WORTHY_CHUNKS	= 10;
}

#endif // _DEBUG


/**
 *	Constructor.
 */
ChunkManager::ChunkManager() :
	initted_( false ),
	workingInSyncMode_( 0 ),
	waitingForTerrainLoad_( 0 ),
	cameraTrans_( Matrix::identity ),
	pCameraSpace_( NULL ),
	cameraChunk_( NULL ),
	pFoundSeed_( NULL ),
	fringeHead_( NULL ),
	maxLoadPath_( 750.f ),
	minEjectPath_( 1000.f ),
	scanSkippedFor_( 60.f ),
	cameraAtLastScan_( 0.f, -1000000.f, 0.f ),
	noneLoadedAtLastScan_( false ),

#ifdef EDITOR_ENABLED
	maxEjectChunks_( 4 )
#else//EDITOR_ENABLED
	maxEjectChunks_( 1 )
#endif//EDITOR_ENABLED

#if UMBRA_ENABLE
	,
	umbraCamera_( NULL )
#endif
{
}


/**
 *	Destructor.
 */
ChunkManager::~ChunkManager()
{
	if (initted_) fini();
}

bool caching_enabled=true;

/**
 *	This method initialises the Chunk manager.
 */
bool ChunkManager::init()
{
	BW_GUARD;
	// People, when you're adding to this method, please try to keep
	// the same order of variable initialisation as they are defined
	// in the class. It makes it easier to see if anything is missing.

	//g_chunkSize += 64*1024;	// stack size guess

	// start the camera off at the origin
	cameraTrans_.setIdentity();
	pCameraSpace_ = NULL;
	cameraChunk_ = NULL;

#if !UMBRA_ENABLE
	MF_WATCH( "Chunks/Counters/Traversed Chunks", ChunkManager::s_chunksTraversed, 
		Watcher::WT_READ_ONLY, "Number of chunks traversed to draw the scene" );
	MF_WATCH( "Chunks/Counters/Visible Chunks", ChunkManager::s_chunksVisible, 
		Watcher::WT_READ_ONLY, "Number of chunks actually drawn after culling" );
	MF_WATCH( "Chunks/Counters/Reflected Chunks", ChunkManager::s_chunksReflected, 
		Watcher::WT_READ_ONLY, "Number of chunks drawn for the reflection passes" );
	MF_WATCH( "Chunks/Counters/Visible Items", ChunkManager::s_visibleCount, 
		Watcher::WT_READ_ONLY, "Number of chunk items drawn to compose the whole scene" );
	MF_WATCH( "Chunks/Counters/Draw passes", ChunkManager::s_drawPass, 
		Watcher::WT_READ_ONLY, "Number of draw passes (1-main scene + n-reflections)" );
	MF_WATCH( "Chunks/Visibility Bounding Boxes", ChunkManager::s_drawVisibilityBBoxes, 
		Watcher::WT_READ_WRITE, "Toggles chunks visibility bounding boxes" );
	MF_WATCH( "Chunks/Enable Visibility Culling", ChunkManager::s_enableChunkCulling, 
		Watcher::WT_READ_WRITE, "Toggles chunk culling based on their visibility bounding boxes" );
#endif // !UMBRA_ENABLE

	MF_WATCH( "Chunks/caching", caching_enabled, Watcher::WT_READ_WRITE, "Enable caching" );
	
#if UMBRA_ENABLE
	// Init umbra
	ChunkUmbra::init();

	// Create the default umbra camera and the umbra cell used for outdoors
	umbraCamera_ = Umbra::Camera::create();
#endif

	speedtree::SpeedTreeRenderer::init();

	// and we're done
	initted_ = true;
	
	Chunk::init();

	return true;
}


/**
 *	This method finalises the ChunkManager.
 */
bool ChunkManager::fini()
{
	BW_GUARD;
	if (!initted_) return false;

	//g_chunkSize -= 64*1024;	// stack size guess

	// get rid of loading chunks (presumably loaded by now)
	while (!loadingChunks_.empty())
	{
		loadingChunks_.back()->mapping()->decRef();
		loadingChunks_.pop_back();
	}
	
	if (uintptr(pFoundSeed_) > 1)
	{
		pFoundSeed_->mapping()->decRef();
		delete pFoundSeed_;
	}
	pFoundSeed_ = NULL;

	// take the camera away
	pCameraSpace_ = NULL;

	// clear any old spaces
	while (!spaces_.empty())
	{
		ChunkSpace * pCS = spaces_.begin()->second;
		pCS->clear();
		// if someone still has references to the space, then erase it anyway
		if (!spaces_.empty() && spaces_.begin()->second == pCS)
			spaces_.erase( spaces_.begin() );
	}

	speedtree::SpeedTreeRenderer::fini();

	Moo::VisualCompound::fini();

#if UMBRA_ENABLE
	// Clean up our umbra resources
	umbraCamera_->release();

	// Shut down umbra
	ChunkUmbra::fini();
#endif

	Chunk::fini();
	ChunkVLO::fini();

	ChunkBoundary::fini();	

	initted_ = false;

	return true;
}

/**
 *	Set the camera position (we don't use Moo's camera)
 *	If you're taking it from Moo this should be the
 *	cameraToWorld transform, i.e. invView
 */
void ChunkManager::camera( const Matrix & cameraTrans, ChunkSpacePtr pSpace, Chunk* pOverride )
{
	BW_GUARD;
	cameraTrans_ = cameraTrans;
	pCameraSpace_ = pSpace;
	if (pCameraSpace_)
	{
		cameraChunk_ = NULL;	// no camera chunk over focus (could be stale)


		if (pOverride)
			cameraChunk_ = pOverride;
		else
		{
			pCameraSpace_->focus( cameraTrans_.applyToOrigin() );

			cameraChunk_ =
				pCameraSpace_->findChunkFromPoint( this->cameraNearPoint() );
		}
		MF_ASSERT_DEV( !cameraChunk_ || cameraChunk_->online() );

		if (pCameraSpace_ && pCameraSpace_->terrainSettings())
		{
			pCameraSpace_->terrainSettings()->setActiveRenderer();
		}

#if UMBRA_ENABLE
		// Do umbra related work
		if (cameraChunk_)
		{
			checkCameraBoundaries();
		}

		Matrix cameraToCell = Matrix::identity;

		if (cameraChunk_)
		{
			// Set the cell of the chunk the camera is in
			umbraCamera_->setCell(cameraChunk_->getUmbraCell());

			// all cells are in world space now
			cameraToCell = Matrix::identity;
		}
		else
		{
			// If there is no camera chunk set the camera cell to NULL
			umbraCamera_->setCell( NULL );
		}
        
		// Multiply with the camera transform
		cameraToCell.preMultiply( cameraTrans );

		// Make sure the last column of the matrix uses good
		// values as umbra is quite picky about its matrices
		Matrix m = Matrix::identity;
		m[0] = cameraToCell[0];
		m[1] = cameraToCell[1];
		m[2] = cameraToCell[2];
		m[3] = cameraToCell[3];

		// Set the matrix
		umbraCamera_->setCameraToCellMatrix((Umbra::Matrix4x4&)m);

		// Set up the umbra camera and frustum
		Umbra::Frustum f;

		const Moo::Camera& camera = Moo::rc().camera();
		float hh = (float)tan(camera.fov() / 2.0) * camera.nearPlane();
		f.zNear  = camera.nearPlane();
		f.zFar   = camera.farPlane();
		f.right  = hh*camera.aspectRatio();
		f.left   = -f.right;
		f.top    = hh;
		f.bottom = -f.top;
		umbraCamera_->setFrustum(f);

		// Set up the type of culling, this is determined by the
		// umbra helper class
		uint32 properties = Umbra::Camera::VIEWFRUSTUM_CULLING;
		if (UmbraHelper::instance().occlusionCulling())
			properties |= Umbra::Camera::OCCLUSION_CULLING;
		else
			properties |= Umbra::Camera::DEPTH_PASS;

		umbraCamera_->setParameters(	(int)Moo::rc().screenWidth(),
									(int)Moo::rc().screenHeight(), 
									properties, 0.25, 0.25 );
#endif
	}
	else
	{
		cameraChunk_ = NULL;
	}
}

#if UMBRA_ENABLE
namespace
{
/*
 *	This helper function checks whether a 3d line intersects with
 *	portal, the line is assumed to me on the plane of the portal.
 */
bool lineOnPlaneIntersectsPortal( const ChunkBoundary::Portal* p, 
		const Vector3& start, const Vector3& end )
{
	BW_GUARD;
	// calculate the outcode of the two points
	uint32 oc1 = p->outcode( start );
	uint32 oc2 = p->outcode( end );

	// if either of the two points are completely inside
	// the portal, the line and portal are intersecting
	if (oc1 == 0 || oc2 == 0)
		return true;

	// if the two lines do not share an outcode the line and portal
	// may intersect
	if (!(oc1 & oc2))
	{
		// Calculate a line equation based on the two points
		Vector3 diff = start - end;
		Vector3 op1 = start - p->origin;
		Vector2 p2d( op1.dotProduct( p->uAxis ), op1.dotProduct( p->vAxis ) );
		Vector2 diff2d( -diff.dotProduct( p->vAxis ), diff.dotProduct( p->uAxis ) );
		
		float d = diff2d.dotProduct( p2d );

		bool allLess = true;
		bool allGreater = true;

		// Iterate over the points in the portal and update flags
		// depending on which side of the line the point is
		for (uint32 i = 0; i < p->points.size();i++)
		{
			if (d < diff2d.dotProduct( p->points[i] ))
			{
				allGreater = false;
			}
			else
			{
				allLess = false;
			}
		}

		// If we have points on both sides of the line, the portal
		// and line are intersecting.
		return allLess == false && allGreater == false;
	}
	return false;
}

};

/*
 * In boundary conditions we need to check if the centre of the 
 * near plane and the camera cross a portal boundary.
 * If it does we need to check if the near plane rect intersects
 * with the portal, if it does, we need to render from the
 * chunk the camera position is in, otherwise the chunk the
 * near plane position is in will be the start of rendering.
 */
void ChunkManager::checkCameraBoundaries()
{
	BW_GUARD;
	
	// Get the camera transform in chunk space
	Matrix localCamera = cameraChunk_->transformInverse();
	localCamera.preMultiply( cameraTrans_ );

	// Iterate over all chunk boundaries
	ChunkBoundaries& cb = cameraChunk_->bounds();
	for (uint32 i = 0; i < cb.size(); i++)
	{
		// Iterate over bound portals in chunk boundaries
		for (uint32 j = 0; j < cb[i]->boundPortals_.size(); j++)
		{
			// Grab the current portal
			ChunkBoundary::Portal* p = cb[i]->boundPortals_[j];
			
			// Make sure we have a chunk, if both the camera chunk 
			// and the portal chunk are outside we will ignore this
			// portal as outside chunks don't do portal clipping
			if (p->hasChunk() &&
				!(p->pChunk->isOutsideChunk() && 
					cameraChunk_->isOutsideChunk()))
			{
				// If the camera position is on the other side
				// of the portal, check if the near plane intersects
				// with the portal
				if (!p->plane.isInFrontOf( localCamera.applyToOrigin() ))
				{
					// Get the near plane rectangle
					const Moo::Camera& camera = Moo::rc().camera();
					float nearUp = (float)tan(camera.fov() / 2.0) * 
						camera.nearPlane();
					float nearRight = nearUp * camera.aspectRatio();
					float nearZ = camera.nearPlane();
					
					bool inFront[4];
					Vector3 points[4];
					points[0] = localCamera.applyPoint( 
						Vector3( -nearRight, nearUp, nearZ ));
					points[1] = localCamera.applyPoint( 
						Vector3( nearRight, nearUp, nearZ ));
					points[2] = localCamera.applyPoint( 
						Vector3( nearRight, -nearUp, nearZ ));
					points[3] = localCamera.applyPoint( 
						Vector3( -nearRight, -nearUp, nearZ ));

					// Intersect the nearplane rectangle with the
					// portal plane
					uint32 nInFront = 0;
					for (uint32 k = 0; k < 4; k++)
					{
						inFront[k] =
							p->plane.isInFrontOf( points[k] );
						nInFront = inFront[k] ? nInFront + 1 : nInFront;
					}

					// if all the points of the near plane are on one side
					// of the portal don't bother checking if it intersects
					if (nInFront != 4)
					{
						// Get the line that makes up the intersection
						// between the portal plane and the near plane
						Vector3 pointsOnPlane[2];
						uint32 pointIndex = 0;
						for (uint32 k = 0; k < 4; k++)
						{
							if (inFront[k] != inFront[(k + 1) % 4])
							{
								Vector3 dir = points[(k + 1) % 4] - points[k];
								dir.normalise();
								pointsOnPlane[pointIndex++] =
									p->plane.intersectRay( points[k], dir );
							}
						}
						// If the line intersects the portal we put the camera
						// on the other side of the portal
						if (lineOnPlaneIntersectsPortal( p, 
							pointsOnPlane[0], pointsOnPlane[1] ))
						{
							cameraChunk_ = p->pChunk;
							i = cb.size();
							break;
						}
					}
				}
			}
		}
	}
}
#endif

/**
 *	Private method to get the camera point for the purposes of determining
 *	which chunk it is in.
 */
Vector3 ChunkManager::cameraNearPoint() const
{
	return cameraTrans_.applyToOrigin() +
		Moo::rc().camera().nearPlane() *
			cameraTrans_.applyToUnitAxisVector( Z_AXIS );
}


Vector3 ChunkManager::cameraAxis(int axis) const
{
	return cameraTrans_.applyToUnitAxisVector( axis );
}

/**
 *	Perform periodic duties, and call everyone else's tick
 */
void ChunkManager::tick( float dTime )
{
	BW_GUARD_PROFILER( ChunkManager_tick );

	ChunkManager::s_chunksTraversed = 0;
	ChunkManager::s_chunksVisible   = 0;
	ChunkManager::s_chunksReflected = 0;
	ChunkManager::s_visibleCount    = 0;
	ChunkManager::s_drawPass        = 0;

	// make sure we have been initialised
	if (!initted_) return;

	{
		SimpleMutexHolder smh( pendingChunksMutex_ );
		while( !pendingChunks_.empty() )
		{
			loadChunkExplicitly( pendingChunks_.begin()->first, pendingChunks_.begin()->second );
			pendingChunks_.erase( pendingChunks_.begin() );
		}
	}

	if( workingInSyncMode_ )
		return;

	// update far plane for load/eject purposes
	float farPlane = Moo::rc().camera().farPlane();

	// tick the speedtrees
	PROFILER_BEGIN( SpeedTree_tick );
	speedtree::SpeedTreeRenderer::tick( dTime );
	PROFILER_END();

	// first see if any waiting chunks are ready
	uint32 preLoadingChunksSize = loadingChunks_.size();
	bool anyChanges = this->checkLoadingChunks();

	// put the camera where it belongs
	if (cameraChunk_ != NULL && !cameraChunk_->online())
		cameraChunk_ = NULL;
	if (cameraChunk_ == NULL && pCameraSpace_)
	{
		cameraChunk_ =
			pCameraSpace_->findChunkFromPoint( this->cameraNearPoint() );
		MF_ASSERT_DEV( !cameraChunk_ || cameraChunk_->online() );
	}

	static DogWatch chunkScan( "ChunkScan" );
	chunkScan.start();
	// now fill the chunk graph outwards from there until
	//  we've covered the drawable distance and then some
	if (cameraChunk_ != NULL)
	{
		scanSkippedFor_ += dTime;
		bool noGo = false;
		// Don't bother scanning if we are in a stable state
		if ((cameraTrans_.applyToOrigin()-cameraAtLastScan_).
			lengthSquared() < 100.f)
		{
			// first check if we're loading anyway
			if (loadingChunks_.size() >= MAX_LOADING_CHUNKS) noGo = true;
			// now see if we couldn't find anything to load
			if (preLoadingChunksSize == 0 && noneLoadedAtLastScan_ &&
				scanSkippedFor_ < 1.f) noGo = true;
		}
		if (!noGo)
		{
			anyChanges |= this->scan();
			scanSkippedFor_ = 0.f;
		}

		// if we have an unwanted seed get rid of it
		if (uintptr(pFoundSeed_) > 1)
		{
			ChunkDirMapping * pMapping = pFoundSeed_->mapping();
			delete pFoundSeed_;	// it's just a stub so ok to delete it
			pMapping->decRef();
			pFoundSeed_ = NULL;
		}
	}
/*
    else if (pCameraSpace_ && !pCameraSpace_->chunks().empty())
	{
		// uh-oh, we can't find the chunk that the camera is in!
		// help! I'm blind! whatever we do, let's not panic!!!
		anyChanges |= this->blindpanic();
	}
*/
	else if (pCameraSpace_)
	{
		anyChanges |= this->autoBootstrapSeedChunk();
	}
	chunkScan.stop();

	// if there were any changes to chunks loaded then we'd better see if
	// the camera wants to go somewhere different - and (very importantly)
	// update any stale columns - we don't want to go a whole frame with
	// stale data hanging around in them (especially if chunks were ejected)
	if (pCameraSpace_ && anyChanges)
		this->camera( cameraTrans_, pCameraSpace_ );

	static DogWatch s_dwTickSpaces("Spaces");
	s_dwTickSpaces.start();
	// cool, now call everyone's tick as has been the custom of old
	for (ChunkSpaces::iterator it = spaces_.begin(); it != spaces_.end(); it++)
	{
		it->second->tick( dTime );
	}
	s_dwTickSpaces.stop();

	// Clear out the chunk caching..
	clearCache();

#if UMBRA_ENABLE
	// Do umbra tick
	PROFILER_BEGIN( UMBRA_tick );
	ChunkUmbra::tick();
	PROFILER_END();
#endif

	Chunk::nextVisibilityMark();
}

#undef DrawState	// silly Windows!
/**
 *	Helper struct for drawing
 */
struct DrawState
{
	DrawState( Chunk * pChunk, Portal2DRef pClipPortal ) :
		pChunk_( pChunk ), pClipPortal_( pClipPortal ) { }

	Chunk * pChunk_;
	Portal2DRef pClipPortal_;
};


typedef std::vector< std::pair< Chunk*,bool> > CacheVector;
CacheVector chunk_cache;

void ChunkManager::addToCache(Chunk* pChunk, bool fringeOnly)
{
	BW_GUARD;
	if (caching_enabled)
		chunk_cache.push_back( std::make_pair(pChunk,fringeOnly) );
}

void ChunkManager::clearCache()
{
	BW_GUARD;
	chunk_cache.clear();
}

#if UMBRA_ENABLE
static uint32 s_reflectionMark = 0;
/**
 *	This method traverses and draws the scene using umbra.
 */
void ChunkManager::umbraDraw()
{
	BW_GUARD_PROFILER( ChunkManager_draw );

	++ChunkManager::s_drawPass;

	// If there is no camera or no cell for the camera return
	if (umbraCamera_ == NULL || umbraCamera_->getCell() == NULL)
		return;

	// Get the current enviro minder
	EnviroMinder * envMinder = NULL;
	if (pCameraSpace_.getObject() != NULL)
	{
		envMinder = &ChunkManager::instance().cameraSpace()->enviro();
	}

	// Start speedtree rendering
	PROFILER_BEGIN( SpeedTree_beginFrame );
	speedtree::SpeedTreeRenderer::beginFrame( envMinder );
	PROFILER_END();

	ChunkExitPortal::seenExitPortals().clear();	

	if (cameraChunk_)
	{
		cameraChunk_->drawCaches();	
		/* Make sure the camera chunk is ticked
		 */	
		cameraChunk_->drawMark(Chunk::s_nextMark_);
	}
	
	s_reflectionMark = Chunk::nextMark();

	clearCache();

	static DogWatch s_umbraTime( "UMBRA" );
	s_umbraTime.start();
	PROFILER_BEGIN( UMBRA_resolveVisibility );
	umbraCamera_->resolveVisibility( ChunkUmbra::pCommander(), 1 );
	PROFILER_END();
	s_umbraTime.stop();

	// To ensure correct reflection:
	if (Moo::rc().reflectionScene() && caching_enabled)
	{
		for (CacheVector::iterator it = chunk_cache.begin(); it != chunk_cache.end(); it++)
		{
			Chunk* pChunk = it->first;
			if (!pChunk || pChunk->drawMark() == s_reflectionMark)
				continue;
			if (pChunk->online() && pChunk->drawSelf())
				pChunk->drawMark( s_reflectionMark );
		}
			}
	PROFILER_BEGIN( SpeedTree_endFrame );
	speedtree::SpeedTreeRenderer::endFrame();
	PROFILER_END();

	// move on the mark
	Chunk::nextMark();
}
/**
 *	This method traverses and draws the scene using umbra.
 */
void ChunkManager::umbraRepeat()
{
	BW_GUARD;
	// If there is no camera or no cell for the camera return
	if (umbraCamera_ == NULL || umbraCamera_->getCell() == NULL)
			return;

	// Get the current enviro minder
	EnviroMinder * envMinder = NULL;
	if (pCameraSpace_.getObject() != NULL)
	{
		envMinder = &ChunkManager::instance().cameraSpace()->enviro();
		}

	// Start speedtree rendering
	speedtree::SpeedTreeRenderer::beginFrame( envMinder );

	ChunkExitPortal::seenExitPortals().clear();	

	if (cameraChunk_)
		cameraChunk_->drawCaches();

	ChunkUmbra::repeat();

	speedtree::SpeedTreeRenderer::endFrame();
}
#endif


/**
 *	Draw the chunky scene, from the point of view of the
 *	camera set in the last call to our 'camera' method.
 */
void ChunkManager::draw()
{
	BW_GUARD_PROFILER( ChunkManager_draw );

	++ChunkManager::s_drawPass;

	EnviroMinder * envMinder = NULL;
	if (pCameraSpace_.getObject() != NULL)
	{
		envMinder = &ChunkManager::instance().cameraSpace()->enviro();
	}

	PROFILER_BEGIN( SpeedTree_beginFrame );
	speedtree::SpeedTreeRenderer::beginFrame( envMinder );
	PROFILER_END();

	ChunkExitPortal::seenExitPortals().clear();	

	if (cameraChunk_ != NULL && !cameraChunk_->online())
		cameraChunk_ = NULL;
	if (cameraChunk_ == NULL)
	{
		// err, we're having some technical problems here...
		return;
	}

	ChunkBoundary::Portal::updateFrustumBB();

	//s_specialConsoleString_ += "Current chunk: " + cameraChunk_->identifier() + "\n";

	if (Moo::rc().reflectionScene() && caching_enabled)
	{
		CacheVector::iterator it = chunk_cache.begin();
		CacheVector::iterator itEnd = chunk_cache.end();
		while ( it != itEnd )
		{
			if (!it->first->online())
				it = chunk_cache.erase(it);
			else
			{
				(*it).first->drawSelf( (*it).second );
				++it;
			}
		}
		speedtree::SpeedTreeRenderer::endFrame();
		Chunk::nextMark();
		return;
	}
	else
		clearCache();


	// ok, draw them chunks then!
	ChunkManager::drawTreeBranch( cameraChunk_, " + camera" );
	//cameraChunk_->draw( NULL );
	cameraChunk_->drawBeg();
	uint32 nextMark = cameraChunk_->drawMark();

	static VectorNoDestructor<DrawState>	stack;
	MF_ASSERT_DEV( stack.empty() );	// can't clear 'coz dtors should be called

	stack.push_back( DrawState( cameraChunk_, Portal2DRef() ) );

	while (!stack.empty())
	{
		DrawState cur = stack.back();
		stack.back().pClipPortal_ = Portal2DRef();	// clear since no dtor
		stack.pop_back();
		Chunk * pChunk = cur.pChunk_;

		// go through all bound portals
		for (Chunk::piterator it = pChunk->pbegin(); it != pChunk->pend(); it++)
		{
			switch (ulong( it->pChunk ))
			{
			case ChunkBoundary::Portal::NOTHING:
				// do nothing (why is a portal defined?)
				break;
			case ChunkBoundary::Portal::HEAVEN:
				// heavens are always drawn ... unless we are an inside chunk
				if (!pChunk->isOutsideChunk())
				{
					ChunkSpace::Column * pCol =
						pChunk->space()->column( it->centre, false );
					if (pCol == NULL) break;
					Chunk * pOutChunk = pCol->pOutsideChunk();
					if (pOutChunk == NULL || pOutChunk->traverseMark() == nextMark)
						break;

					it->pChunk = pOutChunk;	// very dodgy!
					Portal2DRef pWindow = it->traverse(
						pChunk->transform(),
						pChunk->transformInverse(),
						cur.pClipPortal_ );
					it->pChunk = (Chunk*)ChunkBoundary::Portal::HEAVEN;
					if (pWindow.valid())
						stack.push_back( DrawState( pOutChunk, pWindow ) );
				}
				break;
			case ChunkBoundary::Portal::EARTH:
				// connection to earth could be used to draw terrain ... but
				// now terrain is added as a chunk item so this is unnecessary
				break;
			default:
				// ok, see if we can draw it then
				if (!it->pChunk->online()) break;	// should be online if bound				

				if (it->pChunk->traverseMark() == nextMark) break;

				// we can - draw away
				Portal2DRef pWindow = it->traverse(
					pChunk->transform(),
					pChunk->transformInverse(),
					cur.pClipPortal_ );
				if (pWindow.valid())
				{					
					//here, we could set up scissors rectangles or clip planes to
					//confine drawing to just the region seen through the portal.
					stack.push_back( DrawState( it->pChunk, pWindow ) );
				}
				break;
			}
		}

		// now do final drawing
		// (earlier than before, yes - but not too bad)
		pChunk->drawEnd();
		pChunk->traverseMark( nextMark );
		ChunkManager::drawTreeReturn();
	}

	//g_specialConsoleString += ChunkManager::drawTree();


	// draw any fringe chunks too
	Chunk * pFringe = fringeHead_;
	fringeHead_ = (Chunk*)1;
	while (pFringe != NULL)
	{
		// draw it, but only the appropriate lent items
		pFringe->drawSelf( true );

		// and take it out of the list
		Chunk * pNext = pFringe->fringeNext();
		pFringe->fringePrev( NULL );
		pFringe->fringeNext( NULL );

		//g_specialConsoleString += "Fringe chunk: " + pFringe->identifier() + "\n";

		pFringe = pNext;
	}
	MF_ASSERT_DEV( fringeHead_ == (Chunk*)1 );
	fringeHead_ = NULL;

#if ENABLE_DRAW_PORTALS
	// draw exit portals
	/*ExitPortalTraversals::iterator it = exitPortalTraversals().begin();
	ExitPortalTraversals::iterator end = exitPortalTraversals().end();
	while (it != end)
	{
		Matrix& transform = it->second->transform();
		ChunkBoundary::Portal* portal = it->first;

		Moo::rc().push();
		Moo::rc().world( transform );
		Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
		Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

		Vector3 pts[4];
		for (uint32 i=0; i<4; i++)
			portal->objectSpacePoint(i,pts[i]);

		Geometrics::drawLinesInWorld(pts,4,Moo::Colour( 1, 0.5, 0, 0 ));
		Moo::rc().pop();
		it++;
	}*/
#endif

	// move on the mark
	Chunk::nextMark();
	
	PROFILER_BEGIN( SpeedTree_endFrame );
	speedtree::SpeedTreeRenderer::endFrame();
	PROFILER_END();
}


/**
 *	Add this chunk which is on the fringe of the traversal
 *	 (and should be drawn due to items that may be visible
 *   lent into other chunks which are in the traversal proper)
 */
void ChunkManager::addFringe( Chunk * pChunk )
{
	BW_GUARD;
	// don't allow fringe chunks to be added if we're drawing the fringe
	if (fringeHead_ == (Chunk*)1) return;

	// the furthest chunks will tend to be added first
	// (because they're added after the traversal recursion call)
	// so it is good to put newly added chunks on the head of the list
	pChunk->fringePrev( (Chunk*)1 );
	pChunk->fringeNext( fringeHead_ );
	if (fringeHead_ != NULL) fringeHead_->fringePrev( pChunk );
	fringeHead_ = pChunk;
}


/**
 *	Remove this chunk from the fringe of the traversal
 *
 *	@see addFringe
 */
void ChunkManager::delFringe( Chunk * pChunk )
{
	BW_GUARD;
	// don't allow fringe chunks to be removed if we're drawing the fringe
	if (fringeHead_ == (Chunk*)1) return;

	// fix up the list
	Chunk * pPrev = pChunk->fringePrev();
	Chunk * pNext = pChunk->fringeNext();
	if (pPrev != (Chunk*)1) pPrev->fringeNext( pNext );
	if (pNext != NULL)		pNext->fringePrev( pPrev );

	// fix up our head pointer
	if (pPrev == (Chunk*)1) fringeHead_ = pNext;

	// and clear the chunk's pointers
	pChunk->fringePrev( NULL );
	pChunk->fringeNext( NULL );
}


/**
 *	Explicitly load a given chunk (needed for bootstrapping,
 *	teleportation, and other unexplained phenomena)
 */
void ChunkManager::loadChunkExplicitly(
	const std::string & identifier, ChunkDirMapping * pMapping, bool isOverlapper /*= false*/ )
{
	BW_GUARD;
	if( workingInSyncMode_ )
	{
		loadChunkNow( identifier, pMapping );
	}
	else
	{
		if( MainThreadTracker::isCurrentThreadMain() && !isOverlapper )
		{
			Chunk* chunk = findChunkByName( identifier, pMapping );

			// make sure it's not already loaded or being loaded
			if (chunk != NULL && !chunk->loaded() && std::find( loadingChunks_.begin(),
				loadingChunks_.end(), chunk) == loadingChunks_.end())
			{
				// ok, schedule it for loading ahead of time then
				this->loadChunk( chunk, false );
			}
		}
		else
		{
			SimpleMutexHolder smh( pendingChunksMutex_ );
			pendingChunks_.insert( std::make_pair( identifier, pMapping ) );
		}
	}
}

Chunk* ChunkManager::findChunkByGrid( int16 x, int16 z, ChunkDirMapping * pMapping )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pMapping )
	{
		return NULL;
	}

	ChunkSpacePtr pSpace = pMapping->pSpace();
	std::pair< int, int > coord( x, z );

	if ( !pSpace->gridChunks().count( coord ) )
		return NULL;


	GridChunkMap & gridChunks = pSpace->gridChunks();
	const std::vector< Chunk* > & mappings = gridChunks[coord];
	Chunk * chunk = NULL;

	for (uint i = 0; i < mappings.size(); i++)
	{
		if (mappings[i]->mapping() == pMapping)
		{
			chunk = mappings[i];
			break;
		}
	}

	return chunk;
}

Chunk* ChunkManager::findChunkByName( const std::string & identifier, 
    ChunkDirMapping * pMapping, bool createIfNotFound /*= true*/ )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( pMapping )
	{
		return NULL;
	}

	ChunkSpacePtr pSpace = pMapping->pSpace();
	ChunkMap::const_iterator found = pSpace->chunks().find( identifier );
	Chunk * chunk = NULL;

	if (found != pSpace->chunks().end())
	{
		for (uint i = 0; i < found->second.size(); i++)
		{
			if (found->second[i]->mapping() == pMapping)
			{
				chunk = found->second[i];
				break;
			}
		}
	}

	if (chunk == NULL && createIfNotFound)
	{
		// make the chunk
		chunk = new Chunk( identifier, pMapping );

		// add it to its space's map of chunks
		pSpace->addChunk( chunk );
	}
	return chunk;
}

void ChunkManager::loadChunkNow( Chunk* chunk )
{
	BW_GUARD;
	if (!chunk)
	{
		CRITICAL_MSG( "Trying to load NULL chunk at 0x%08X\n", uint32(this) );
	}

	if (chunk->loaded())
	{
		ERROR_MSG( "Trying to load loaded chunk %s at 0x%08X\n",
			chunk->identifier().c_str(), uint32(this) );
        return;
	}

	loadingChunks_.push_back( chunk );

	chunk->loading( true );

	// loading chunks retain refs to their mapping
	chunk->mapping()->incRef();

	ChunkLoader::loadNow( chunk );
}

void ChunkManager::loadChunkNow( const std::string & identifier, ChunkDirMapping * pMapping )
{
	BW_GUARD;
	Chunk* chunk = findChunkByName( identifier, pMapping );

	// make sure it's not already loaded or being loaded
	if (chunk != NULL && !chunk->loaded() && std::find( loadingChunks_.begin(),
		loadingChunks_.end(), chunk) == loadingChunks_.end())
	{
		// ok, schedule it for loading ahead of time then
		this->loadChunkNow( chunk );
	}
}

/**
 *	See if any chunks waiting to be loaded have been
 */
bool ChunkManager::checkLoadingChunks()
{
	BW_GUARD;
	bool anyChanges = false;

	for (uint i=0; i<loadingChunks_.size(); i++)
	{
		Chunk * pChunk = loadingChunks_[i];
		if (pChunk->loaded() )
		{
			// ok, we have a new chunk, so stop
			// polling it to see if it's done
			loadingChunks_.erase( loadingChunks_.begin() + (i--) );
			pChunk->loading( false );

			// make sure its mapping has not been condemned
			ChunkDirMapping * pMapping = pChunk->mapping();
			if (pMapping->condemned())
			{
				pChunk->space()->ejectLoadedChunkBeforeBinding( pChunk );
				delete pChunk;
				pMapping->decRef();
				continue;
			}
			pMapping->decRef();

			// otherwise bind it in
			if( !pChunk->isOutsideChunk() && pCameraSpace_ )
				pCameraSpace_->focus( cameraTrans_.applyToOrigin() );
			pChunk->bind( true );

			if( pChunk->isOutsideChunk() && pCameraSpace_ && Chunk::overlapperFinder() )
			{
				std::vector<Chunk*> shells = Chunk::overlapperFinder()( pChunk );
				if( !shells.empty() )
				{
					pCameraSpace_->focus( cameraTrans_.applyToOrigin() );
					for( std::vector<Chunk*>::iterator iter = shells.begin(); iter != shells.end();
						++iter )
					{
						(*iter)->bindPortals( true );
					}
				}
			}

			if (waitingForTerrainLoad_)
			{
				ChunkTerrain* terrain = ChunkTerrainCache::instance( *pChunk ).pTerrain();

				while (terrain && terrain->doingBackgroundTask())
				{
					BgTaskManager::instance().tick();
				}
			}

			anyChanges = true;
		}
	}

	return anyChanges;
}

void ChunkManager::switchToSyncMode( bool sync )
{
	BW_GUARD;
	if( !sync )
	{
		MF_ASSERT_DEV( workingInSyncMode_ > 0 );
		--workingInSyncMode_;
		return;
	}
	else if( sync && workingInSyncMode_ > 0 )
	{
		++workingInSyncMode_;
		return;
	}
	++workingInSyncMode_;
	while( busy() )
	{
		checkLoadingChunks();
		Sleep( 50 );
	}
}

void ChunkManager::switchToSyncTerrainLoad( bool sync )
{
	BW_GUARD;
	if( !sync )
	{
		MF_ASSERT_DEV( waitingForTerrainLoad_ > 0 );
		--waitingForTerrainLoad_;
	}
	else
	{
		++waitingForTerrainLoad_;
	}
}

/// Bias in metres when determining whether or not a camera is inside an
/// overlapper in an outside chunk, which should cause that chunk to be
/// loaded before any other.
static const float CAMERA_INSIDE_OVERLAPPER_BIAS = 10.f;

/// Bias in metres when determining whether a chunk should remain 'wired'
/// in memory. Wired chunks are not removable even if they would otherwise
/// be, due to being near to the camera. This should probably be set a least
/// a little higher than CAMERA_INSIDE_OVERLAPPER_BIAS so it does not thrash.
static const float CAMERA_NEARBY_WIRED_BIAS = 20.f;

// Helper function to find an overlapper in a chunk
static Chunk * findOverlapper( Chunk * pInChunk, const Vector3 & pos,
	uint32 mark )
{
	BW_GUARD;	
#ifndef EDITOR_ENABLED
	if (ChunkOverlappers::instance.exists( *pInChunk ))
	{
		const ChunkOverlappers::Overlappers & overlappers =
			ChunkOverlappers::instance( *pInChunk ).overlappers();
		for (uint i = 0; i < overlappers.size(); i++)
		{
			ChunkOverlapper * pOverlapper = &*overlappers[i];
			if (pOverlapper->pOverlapper()->online()) continue;		  // online
			if (pOverlapper->pOverlapper()->traverseMark() == mark) continue; // loading
			if (pOverlapper->bb().intersects( pos,
				CAMERA_INSIDE_OVERLAPPER_BIAS ))
			{
				Chunk * pChunk = pOverlapper->pOverlapper();
				pChunk->traverseMark( mark );
				pChunk->pathSum( 0.f );
				return pChunk;
			}
		}
	}
#endif
	return NULL;
}

struct GridComp
{
	bool operator()( std::pair<int, int> grid1, std::pair<int, int> grid2 )
	{
		return grid1.first * grid1.first + grid1.second * grid1.second <
			grid2.first * grid2.first + grid2.second * grid2.second;
	}
};
/**
 *	Scan over the graph from the camera's chunk and see if
 *	there's anything else we want to load.
 *
 *	Returns true if any changes affecting focus grids were made.
 */
PROFILER_DECLARE( ChunkManager_scan, "ChunkManager Scan" );
PROFILER_DECLARE( ChunkManager_scan2, "ChunkManager Scan2" );

bool ChunkManager::scan()
{
	BW_GUARD_PROFILER( ChunkManager_scan );

	static int savedMaxLoadGrid = 0;
	static std::vector<std::pair<int, int> > sortedGridBounds;

	ChunkDirMapping* mapping = cameraChunk_->mapping();

	if( savedMaxLoadGrid < ChunkSpace::pointToGrid( maxLoadPath_ ) )
	{
		savedMaxLoadGrid = ChunkSpace::pointToGrid( maxLoadPath_ );

		sortedGridBounds.resize( 0 );
		sortedGridBounds.reserve( ( savedMaxLoadGrid * 2 + 1 ) * ( savedMaxLoadGrid * 2 + 1 ) );

		for( int i = -savedMaxLoadGrid; i < savedMaxLoadGrid + 1; ++i )
		{
			for( int j = -savedMaxLoadGrid; j < savedMaxLoadGrid + 1; ++j )
			{
				sortedGridBounds.push_back( std::make_pair( i, j ) );
			}
		}

		std::sort( sortedGridBounds.begin(), sortedGridBounds.end(), GridComp() );
	}

	cameraAtLastScan_ = cameraTrans_.applyToOrigin();
	noneLoadedAtLastScan_ = true;	
	float closestUnloadedChunk = 1000000.f;

	uint32	mark = Chunk::nextMark();

	Chunk *		pMostWorthy[MAX_WORTHY_CHUNKS] = { NULL };
	size_t		pMostWorthSize = 0;

	PROFILER_BEGIN( ChunkManager_scan2 );

	int cameraGridX = ChunkSpace::pointToGrid( cameraAtLastScan_.x );
	int cameraGridY = ChunkSpace::pointToGrid( cameraAtLastScan_.z );

	for( std::vector<std::pair<int, int> >::iterator iter = sortedGridBounds.begin();
		iter != sortedGridBounds.end(); ++iter )
	{
		if( ChunkSpace::gridToPoint( iter->first ) * ChunkSpace::gridToPoint( iter->first ) +
			ChunkSpace::gridToPoint( iter->second ) * ChunkSpace::gridToPoint( iter->second ) >
			maxLoadPath_ * maxLoadPath_ )
			break;

		int x = cameraGridX + iter->first;
		int z = cameraGridY + iter->second;

		if ( x < mapping->minLGridX() || x > mapping->maxLGridX() ||
			 z < mapping->minLGridY() || z > mapping->maxLGridY() )
			continue;

		Chunk* chunk = findChunkByGrid( x, z, mapping );

		if ( !chunk )
		{
			std::string chunkName = mapping->outsideChunkIdentifier( x, z, false );

			// make the chunk
			chunk = new Chunk( chunkName, mapping );

			// add it to its space's map of chunks
			mapping->pSpace()->addChunk( chunk );
		}



		if( chunk && !chunk->loading() && !chunk->loaded() )
		{
			pMostWorthy[pMostWorthSize++] = chunk;
			if (pMostWorthSize >= MAX_WORTHY_CHUNKS)
				break;
		}
	}

	PROFILER_END();

	// Get the position of the grid square the camera is in
	Vector3 cameraGridPos( 
		ChunkSpace::gridToPoint( cameraGridX ),
		0,
		ChunkSpace::gridToPoint( cameraGridY ) );

	std::vector< Chunk * > furthestChunks;
	furthestChunks.reserve( maxEjectChunks_ );

	for (ChunkMap::reverse_iterator it = pCameraSpace_->chunks().rbegin();
		it != pCameraSpace_->chunks().rend() && furthestChunks.size() < maxEjectChunks_;
		++it)
	{
		std::vector<Chunk*>::reverse_iterator i;
		for (i = it->second.rbegin();
			i != it->second.rend() && furthestChunks.size() < maxEjectChunks_; ++i)
		{
			if (!(*i)->online()) continue;
			if (!(*i)->removable()) continue;
			Vector3 origin = (*i)->transform().applyToOrigin();
			if( ( origin.x - cameraGridPos.x ) * ( origin.x - cameraGridPos.x ) + 
				( origin.z - cameraGridPos.z ) * ( origin.z - cameraGridPos.z ) <=
				minEjectPath_ * minEjectPath_ )
				continue;
			if (!(*i)->isOutsideChunk())
			{
				Vector3 pos = (*i)->transform().applyToOrigin();
				std::string identifier = (*i)->mapping()->outsideChunkIdentifier( pos );
				Chunk* chunk = findChunkByName( identifier, (*i)->mapping(), false );
				if (chunk != NULL && chunk->loaded())
					continue;
			}
			else
			{
				bool allShellsLoaded = true;
				const ChunkOverlappers::Overlappers & overlappers =
					ChunkOverlappers::instance( **i ).overlappers();
				for (uint i = 0; i < overlappers.size(); i++)
				{
					ChunkOverlapper * pOverlapper = &*overlappers[i];
					if (pOverlapper->pOverlapper()->loading())
					{
						allShellsLoaded = false;
						break;
					}
				}
				if( !allShellsLoaded )
					continue;
			}

			if ((*i)->boundingBox().intersects(
				cameraGridPos, CAMERA_NEARBY_WIRED_BIAS )) continue;

			furthestChunks.push_back( *i );
		}
		if (i != it->second.rend()) break;
	}

	// ok, we found some to load! finally! yay!
	for 
	(
		size_t pMostWorthIdx = 0;
		pMostWorthIdx < pMostWorthSize;
		++pMostWorthIdx
	)
		{
			// only load it if we're not already loading too many
			// (we still do the scan in case others expect it)
			if (loadingChunks_.size() < MAX_LOADING_CHUNKS)
			{
				Chunk *chunk = pMostWorthy[pMostWorthIdx];
				this->loadChunk( chunk, chunk->pathSum() == 0 );
				noneLoadedAtLastScan_ = false;

				Vector3 origin = chunk->transform().applyToOrigin();
				float dist = ( ( origin.x - cameraAtLastScan_.x ) * ( origin.x - cameraAtLastScan_.x ) + 
							   ( origin.z - cameraAtLastScan_.z ) * ( origin.z - cameraAtLastScan_.z ) );

				closestUnloadedChunk = std::min( closestUnloadedChunk, sqrtf(dist) );
			}
			else
			{
				break;
		}
	}

	// try to find a space
	if ( pCameraSpace_ )
	{
		pCameraSpace_->closestUnloadedChunk( closestUnloadedChunk );
	}

	bool anyChanges = false;

	// also get rid of the furthest one if it is far enough away
	if (!furthestChunks.empty())
	{
		noneLoadedAtLastScan_ = false;

		for (std::vector< Chunk * >::iterator it = furthestChunks.begin();
			it != furthestChunks.end(); ++it)
		{
			IF_NOT_MF_ASSERT_DEV( (*it)->removable() )
			{
				continue;
			}

			// cut it free from its bindings
			(*it)->loose( false );

			// and clean it out
			(*it)->eject();
		}

		// it is now unloaded but still ratified by its chunk space
		anyChanges = true;
	}

	return anyChanges;
}



/**
 *	Ah, we seem to have misplaced the camera. So what we do
 *	is load the chunk closest to the camera, and we keep doing
 *	this until the camera can find itself. Since we always
 *	load one chunk here (unless it's already loading), we're
 *	guaranteed to eventually find the camera even if it's
 *	hidden deep underground in a warren of chunks. And for
 *	most cases we should find it pretty quickly.
 *
 *	@note: This method shouldn't need to be called in the normal
 *	course of things - it is for emergencies only.
 *
 *	@note: 'Closest' above means as the crow flies, not visibility,
 *	because we can't calculate visibility if we haven't loaded
 *	those chunks yet!
 */
bool ChunkManager::blindpanic()
{
	BW_GUARD;
	uint32	mark = Chunk::nextMark();

	// if something's loading then make sure we exclude them
	for (ChunkVector::iterator it = loadingChunks_.begin();
		it != loadingChunks_.end();
		it++)
	{
		(*it)->traverseMark( mark );
		(*it)->pathSum( -1 );
	}

	const Vector3 & cameraPoint = cameraTrans_.applyToOrigin();

	// ok, now find the closest one
	Chunk * pFurthest = NULL;
	Chunk * pClosest = NULL;

	for (ChunkMap::iterator it = pCameraSpace_->chunks().begin();
		it != pCameraSpace_->chunks().end();
		it++)
	{
		for (uint i = 0; i < it->second.size(); i++)
		{
			Chunk * pChunk = it->second[i];
			if (!pChunk->online()) continue;
			if (pChunk->traverseMark() == mark) continue;

			// record some sort of distance to it
			pChunk->traverseMark( mark );
			pChunk->pathSum( (pChunk->centre() - cameraPoint).lengthSquared() );

			// see if it's the furthest loaded chunk
			if ((pFurthest == NULL || pChunk->pathSum() > pFurthest->pathSum())
				&& pChunk->removable() && !pChunk->boundingBox().intersects(
					cameraPoint, CAMERA_NEARBY_WIRED_BIAS ))
			{
				pFurthest = pChunk;
			}

			// see if it's connected to the closest unloaded chunk
			float	canDist = 0.f;
			Chunk * pCandidate = pChunk->findClosestUnloadedChunkTo(
				cameraPoint, &canDist );

			if (pCandidate != NULL && pCandidate->traverseMark() != mark &&
				(pClosest == NULL || canDist < pClosest->pathSum()))
			{
				MF_ASSERT_DEV( !pCandidate->online() );
				pClosest = pCandidate;
				pClosest->pathSum( canDist );
			}
		}
	}

	bool anyChanges = false;

	// get rid of the furthest one if it is far enough away
	if (pFurthest != NULL && pFurthest->pathSum() > minEjectPath_*minEjectPath_)
	{
		MF_ASSERT_DEV( pFurthest->removable() );

		if( pFurthest->removable() )
		{
			noneLoadedAtLastScan_ = false;

			// cut it free from its bindings
			pFurthest->loose( false );

			// and clean it out
			pFurthest->eject();

			// it is now unloaded but still ratified by its chunk space
			anyChanges = true;
		}
	}

	// if we still can't find anything it's because none of the
	// loaded chunks reference _ANY_ unloaded chunks.
	// unlikely, but it happened the very first time this
	// algorithm executed :)
	// [actually, happens when still loading say first chunk]
	if (pClosest == NULL)
	{
		return anyChanges;
	}

	// ok, load that one then!
	if (loadingChunks_.size() < 3)
	{
		// as long as there aren't too many already loading
		this->loadChunk( pClosest, true );
	}

	return anyChanges;
}


/**
 *	This method automatically bootstraps a seed chunk into the space
 *	containing the camera. It does this by assuming a naming format
 *	for chunks and selecting one to load accordingly.
 */
bool ChunkManager::autoBootstrapSeedChunk()
{
	BW_GUARD;
	// first see if we are already finding a seed
	if (pFoundSeed_ == NULL)
	{
		TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
			"seed chunk NULL so starting search\n" );
		// no, so get the loader to do it in its own thread
		(uint32&)pFoundSeed_ = 1;
		ChunkLoader::findSeed( &*pCameraSpace_, this->cameraNearPoint(),
			pFoundSeed_ );
	}
	// see if the operation has completed
	else if (uint32(pFoundSeed_) != 1)
	{
		TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
			"seed chunk determined: %s\n", pFoundSeed_->identifier().c_str() );
		Chunk * pChunk = pFoundSeed_;
		pFoundSeed_ = NULL;

		// make sure its mapping has not been condemned
		ChunkDirMapping * pMapping = pChunk->mapping();
		if (pMapping->condemned())
		{
			delete pChunk;	// it's just a stub so ok to delete it
			pMapping->decRef();
		}
		else
		{
			pMapping->decRef();

			// see if it was for our space
			if (pChunk->space() == pCameraSpace_)
			{
				pChunk = pCameraSpace_->findOrAddChunk( pChunk );
				if (!pChunk->loading() && !pChunk->loaded())
				{
					this->loadChunk( pChunk, true );
					TRACE_MSG( "ChunkManager::autoBootstrapSeedChunk: "
						"seed chunk submitted for loading\n" );
				}
				
				pCameraSpace_->closestUnloadedChunk( 0.f );

/*
				MF_ASSERT_DEV( !pChunk->ratified() );
				pCameraSpace_->addChunk( pChunk );
				MF_ASSERT_DEV( !pChunk->loaded() );
*/
			}
			else
			{
				// otherwise just delete it and wait to be called again
				delete pChunk;
			}
		}

		// note that if the operation completed but did not find any chunk
		// then pFoundSeed_ will be set to NULL, which is exactly what we want
	}
	// otherwise just wait for the find seed operation to complete

	/*
	// see what the space has to suggest
	Chunk * pChunk = pCameraSpace_->guessChunk( this->cameraNearPoint() );

	// if it came up with something then use that
	if (pChunk != NULL)
	{
		MF_ASSERT_DEV( !pChunk->ratified() );
		pCameraSpace_->addChunk( pChunk );
		MF_ASSERT_DEV( !pChunk->loaded() );
		this->loadChunk( pChunk, true );
	}

	// otherwise do nothing ... there's probably nothing mapped into the space
	*/
	

	return false;
}



/**
 *	Load the given unloaded chunk
 */
PROFILER_DECLARE( ChunkManager_loadChunk, "ChunkManager LoadChunk" );
void ChunkManager::loadChunk( Chunk * pChunk, bool highPriority )
{
	BW_GUARD_PROFILER( ChunkManager_loadChunk );

	if( workingInSyncMode_ )
	{
		loadChunkNow( pChunk );
		return;
	}
	if (!pChunk)
	{
		CRITICAL_MSG( "Trying to load NULL chunk at 0x%08X\n", uint32(this) );
	}
	/*
	if (pChunk->spaceID() != 0)
	{
		CRITICAL_MSG( "Trying to load corrupt chunk at 0x%08X\n", uint32(this) );
	}
	*/
	if (pChunk->loaded())
	{
		ERROR_MSG( "Trying to load loaded chunk %s at 0x%08X\n",
			pChunk->identifier().c_str(), uint32(this) );
        return;
	}
	//MF_ASSERT_DEV( pChunk && !pChunk->loaded() );

	loadingChunks_.push_back( pChunk );
	pChunk->loading( true );

	// loading chunks retain refs to their mapping
	pChunk->mapping()->incRef();
	
	ChunkLoader::load( pChunk, highPriority ? BgTaskManager::HIGH : BgTaskManager::DEFAULT );
}


/**
 *	This method returns the space with the given id, creating it if necessary.
 */
ChunkSpacePtr ChunkManager::space( ChunkSpaceID id, bool createIfMissing )
{
	BW_GUARD;
	// Spaces are only every created or deleted (through last ref thrown away)
	// from the main thread, so we don't need to worry about concurrency here.
	// The loading thread can increment references when it creates a stub chunk,
	// but only from an existing held reference.
	ChunkSpaces::iterator it = spaces_.find( id );
	if (it != spaces_.end()) return it->second;

	if (createIfMissing)
	{
		if ( id != NULL_CHUNK_SPACE )
		{
			return new ChunkSpace( id );
		}
		else
		{			
			ERROR_MSG("Somebody tried to create space with NULL Space ID %d\n",
				NULL_CHUNK_SPACE );
		}
	}
	return NULL;
};

/**
 *	This method returns the space that the camera is currently in.
 *	Note that it can return NULL
 *
 *	@return The chunk the camera is in, or NULL if there is no current space.
 */
ChunkSpacePtr ChunkManager::cameraSpace() const
{
	return pCameraSpace_;
}

/**
 *	This method clears out all spaces
 *
 *	@param	keepClientOnlySpaces	If true this function will leave spaces
 *									created locally intact.
 */
void ChunkManager::clearAllSpaces( bool keepClientOnlySpaces )
{
	BW_GUARD;
	if (spaces_.empty())
		return;	// not for optimisation - fixes weird crash

	{
		SimpleMutexHolder smh( pendingChunksMutex_ );
		pendingChunks_.clear();
	}

	// get all ids first; spaces are likely to refer to each other,
	// and stuff up our iterators if we iterate differently
	int numSpaces = spaces_.size();
	int i = 0;
	ChunkSpaceID * spaceIDs = new ChunkSpaceID[ numSpaces ];
	for (ChunkSpaces::iterator it = spaces_.begin(); it != spaces_.end(); it++)
	{
		spaceIDs[i++] = it->first;
	}

	for (i = 0; i < numSpaces; i++)
	{
		ChunkSpacePtr pSpace = this->space( spaceIDs[i], false );
		if (pSpace && (pSpace->id() < (1<<30) || !keepClientOnlySpaces))
			pSpace->clear();
	}

	delete [] spaceIDs;
}


/**
 *	This static method returns the singleton ChunkManager instance.
 */
ChunkManager & ChunkManager::instance()
{
	static	ChunkManager	chunky;
	return chunky;
}


static int g_treeLevel = 0;
static std::string g_str;

void ChunkManager::drawTreeBranch( Chunk * pChunk, const std::string & why )
{
	if (!g_treeLevel) g_str = "";

	//for (int i=0; i < g_treeLevel; i++) g_str += "\t";
	//g_str += pChunk->identifier() + why + "\n";
	//when enabling above also enable s_specialConsoleString_ concat above
	g_treeLevel++;
}

void ChunkManager::drawTreeReturn()
{
	g_treeLevel--;
}

const std::string & ChunkManager::drawTree()
{
	return g_str;
}

/**
 * Set suitable values for maxLoadPath and minEjectPath based on a given far
 * plane
 */
void ChunkManager::autoSetPathConstraints( float farPlane )
{
	BW_GUARD;
	const float diagonalGridResolution = 
		sqrtf(GRID_RESOLUTION*GRID_RESOLUTION +
			GRID_RESOLUTION*GRID_RESOLUTION);

	//need to load up one more chunk than the hypotenuse of the far plane,
	//so walking forward doesn't reveal unloaded chunks.
	maxLoadPath_ = 
		sqrtf( farPlane*farPlane*2 ) + diagonalGridResolution;

	// Eject path epsilon as we are using quantized values
	// the eject path distance between two chunks is modified
	// a bit so that we don't get conflicts as a result of 
	// floating point inaccuracies
	const float EJECT_PATH_EPSILON = 1.01f;

	//minEjectPath is one more chunk, for antihysteresis
	//TODO : allow configurable antihysteresis radius.
	minEjectPath_ = 
		maxLoadPath_ + diagonalGridResolution * EJECT_PATH_EPSILON;
}

float ChunkManager::closestUnloadedChunk( ChunkSpacePtr pSpace ) const
{
	return pSpace->closestUnloadedChunk();
}

int ChunkManager::s_chunksTraversed = 0;
int ChunkManager::s_chunksVisible   = 0;
int ChunkManager::s_chunksReflected = 0;
int ChunkManager::s_visibleCount    = 0;
int ChunkManager::s_drawPass        = 0;
bool ChunkManager::s_drawVisibilityBBoxes = false;
bool ChunkManager::s_enableChunkCulling   = true;


/**
 *	This method is called by a chunk space to add itself to our list
 */
void ChunkManager::addSpace( ChunkSpace * pSpace )
{
	BW_GUARD;
	IF_NOT_MF_ASSERT_DEV( spaces_.find( pSpace->id() ) == spaces_.end() )
	{
		return;
	}

	spaces_.insert( std::make_pair( pSpace->id(), pSpace ) );

	if (pCameraSpace_ == NULL)
	{
		this->camera( Matrix::identity, pSpace );
	}
}


/**
 *	This method is called by a chunk space to remove itself from our list
 */
void ChunkManager::delSpace( ChunkSpace * pSpace )
{
	BW_GUARD;
	if (!initted_) return;

	// As we are operating on the pending chunks, grab the mutex
	pendingChunksMutex_.grab();

	// if one mapping belongs to 2 spaces, this code will not work
	// but if this will ever happen, it will break everything
	const ChunkSpace::ChunkDirMappings& mappings = pSpace->getMappings();
	bool changed = true;
	while( changed )
	{
		changed = false;
		for( std::set<std::pair<std::string, ChunkDirMapping*> >::iterator iter = pendingChunks_.begin();
			iter != pendingChunks_.end() && !changed; ++iter )
		{
			for( ChunkSpace::ChunkDirMappings::const_iterator mpiter = mappings.begin();
				mpiter != mappings.end(); ++mpiter )
			{
				if( iter->second == mpiter->second )
				{
					pendingChunks_.erase( iter );
					changed = true;
					break;
				}
			}
		}
	}

	// We are done with the pending chunks, give mutex
	pendingChunksMutex_.give();

	ChunkSpaces::iterator found = spaces_.find( pSpace->id() );
	MF_ASSERT_DEV( found != spaces_.end() );
	MF_ASSERT_DEV( pSpace->refCount() == 0 );
	if( found != spaces_.end() )
		spaces_.erase( found );
}

/**
 *  This constructor adds one to the synced mode reference count.
 */
ScopedSyncMode::ScopedSyncMode()
{
    ChunkManager::instance().switchToSyncMode(true);
}


/**
 *  This destructor decreases the synced mode reference count by one.
 */
ScopedSyncMode::~ScopedSyncMode()
{
    ChunkManager::instance().switchToSyncMode(false);
}


// chunk_manager.cpp
