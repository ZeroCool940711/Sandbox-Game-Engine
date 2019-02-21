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
#include "terrain_quad_tree_cell.hpp"

#include "../terrain_collision_callback.hpp"
#include "terrain_height_map2.hpp"

#include "cstdmf/debug.hpp"
#include "physics2/worldtri.hpp"

using namespace Terrain;

namespace // anonymous
{

const float COLLISION_EPSILON = 0.0001f;

/*
 * This methods calculates an outcode for a Vector3's intersection
 * with a bounding box
 */
Outcode boundingBoxVector3Outcode( const BoundingBox& bb, const Vector3& v )
{
	Outcode oc = 0;
	if (v.x < bb.minBounds().x)
		oc |= OUTCODE_LEFT;
	if (v.x > bb.maxBounds().x)
		oc |= OUTCODE_RIGHT;
	if (v.y < bb.minBounds().y)
		oc |= OUTCODE_BOTTOM;
	if (v.y > bb.maxBounds().y)
		oc |= OUTCODE_TOP;
	if (v.z < bb.minBounds().z)
		oc |= OUTCODE_NEAR;
	if (v.z > bb.maxBounds().z)
		oc |= OUTCODE_FAR;
	return oc;
}

/*
 *	This method calculates the outcode and combined outcode for a
 *  WorldTriangle, it assumes that the outcode is initialised
 *  outside the method, this is so that we can call calcOutcodes
 *	for multiple triangles and get a merged result.
 */
void calcOutcodes( const BoundingBox& bb, const WorldTriangle& tri,
	  Outcode& oc, Outcode& combinedOC )
{
	Outcode tempOC = boundingBoxVector3Outcode( bb, tri.v0() );
	oc |= tempOC;
	combinedOC &= tempOC;

	tempOC = boundingBoxVector3Outcode( bb, tri.v1() );
	oc |= tempOC;
	combinedOC &= tempOC;

	tempOC = boundingBoxVector3Outcode( bb, tri.v2() );
	oc |= tempOC;
	combinedOC &= tempOC;
}
/*
 *  This method moves a world triangle so that it touches the bounding box
 *	with one of its vertices.
 *	@param tri start triangle
 *	@param dir the direction to move the triangle
 *	@param crossedBoundaries the boundaries to project
 *	@param bb the bounding box to project onto
 */
void projectTriangleOnBoundingBox( WorldTriangle& tri, const Vector3& dir,
		Outcode crossedBoundaries, const BoundingBox& bb )
{
	BW_GUARD;
	float dist = 0.f;

	// The algorithm used is to find the boundary the triangle is furthest
	// from in the direction of dist and project onto that boundary

	// Project onto left boundary
	if (crossedBoundaries & OUTCODE_LEFT)
	{
		float delta = bb.minBounds().x - tri.v0().x;
		delta = std::min( bb.minBounds().x - tri.v1().x, delta );
		delta = std::min( bb.minBounds().x - tri.v2().x, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.x, dist );
		}
	}

	// Project onto right boundary
	if (crossedBoundaries & OUTCODE_RIGHT)
	{
		float delta = bb.maxBounds().x - tri.v0().x;
		delta = std::max( bb.maxBounds().x - tri.v1().x, delta );
		delta = std::max( bb.maxBounds().x - tri.v2().x, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.x, dist );
		}
	}

	// Project onto bottom boundary
	if (crossedBoundaries & OUTCODE_BOTTOM)
	{
		float delta = bb.minBounds().y - tri.v0().y;
		delta = std::min( bb.minBounds().y - tri.v1().y, delta );
		delta = std::min( bb.minBounds().y - tri.v2().y, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.y, dist );
		}
	}

	// Project onto top boundary
	if (crossedBoundaries & OUTCODE_TOP)
	{
		float delta = bb.maxBounds().y - tri.v0().y;
		delta = std::max( bb.maxBounds().y - tri.v1().y, delta );
		delta = std::max( bb.maxBounds().y - tri.v2().y, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.y, dist );
		}
	}

	// Project onto near boundary
	if (crossedBoundaries & OUTCODE_NEAR)
	{
		float delta = bb.minBounds().z - tri.v0().z;
		delta = std::min( bb.minBounds().z - tri.v1().z, delta );
		delta = std::min( bb.minBounds().z - tri.v2().z, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.z, dist );
		}
	}

	// Project onto far boundary
	if (crossedBoundaries & OUTCODE_FAR)
	{
		float delta = bb.maxBounds().z - tri.v0().z;
		delta = std::max( bb.maxBounds().z - tri.v1().z, delta );
		delta = std::max( bb.maxBounds().z - tri.v2().z, delta );

		if (!almostZero(delta, COLLISION_EPSILON))
		{
			dist = std::max( delta / dir.z, dist );
		}
	}

	// Calculate the delta and project the triangle.
	Vector3 triangleDelta = dir * dist;

	tri = WorldTriangle(
		tri.v0() + triangleDelta,
		tri.v1() + triangleDelta,
		tri.v2() + triangleDelta,
		tri.flags()
		);
}

/*
 * This helper method clips a prism made up of a worldTriangle and an
 * extent to the bounding box provided.
 * It returns true if the prism intersects the bounding box, it also adjusts
 * the triangle and extent to the edges of the bounding box.
 */
bool clipPrism( WorldTriangle& triangle, Vector3& extent, const BoundingBox& bb )
{
	BW_GUARD;
	bool res = false;

	WorldTriangle tri = triangle;
	Vector3 dir = extent - tri.v0();
	WorldTriangle triEnd( tri.v0() + dir, tri.v1() + dir,
		tri.v2() + dir );

	Outcode ocTri1 = 0;
	Outcode combinedOCTri1 = OUTCODE_MASK;
	calcOutcodes( bb, tri, ocTri1, combinedOCTri1 );

	Outcode ocTri2 = 0;
	Outcode combinedOCTri2 = OUTCODE_MASK;
	calcOutcodes( bb, triEnd, ocTri2, combinedOCTri2 );

	Outcode combinedOC = combinedOCTri1 & combinedOCTri2;
	Outcode oc = ocTri1 | ocTri2;

	// If the combined outcode is non zero we don't need to do any more work
	// as we know that we are completely outside the bounding box
	if (!combinedOC)
	{
		if (combinedOCTri1 || combinedOCTri2)
		{
			// See which bb boundaries the whole source triangle is outside,
			// but not the destination
			Outcode sourceCrossing = combinedOCTri1 & ~combinedOCTri2;

			// If the source is outside and crossing a boundary
			// extend to the edge of the bb
			if (sourceCrossing)
			{
				projectTriangleOnBoundingBox( tri, dir, sourceCrossing, bb  );
				ocTri1 = 0;
				combinedOCTri1 = OUTCODE_MASK;
				calcOutcodes( bb, tri, ocTri1, combinedOCTri1 );
			}

			// If the destination is outside and crossing a boundary
			// shrink to the edge of the bb
			Outcode destCrossing = combinedOCTri2 & ~combinedOCTri1;

			if (destCrossing)
			{
				projectTriangleOnBoundingBox( triEnd, -dir, destCrossing, bb  );
				ocTri2 = 0;
				combinedOCTri2 = OUTCODE_MASK;
				calcOutcodes( bb, triEnd, ocTri2, combinedOCTri2 );
			}

			combinedOC = combinedOCTri1 & combinedOCTri2;
			oc = ocTri1 | ocTri2;

			if (!combinedOC)
			{
				triangle = tri;
				extent = triEnd.v0();
				res = true;
			}
		}
		else
		{
			// If the outcode is zero we know that we are completely inside the
			// bounding box and do not need to clip.
			res = true;
		}
	}

	return res;
}

	uint32 quadtreeSize = 0;
	bool firstTime = true;
}

/**
 *  This is the TerrainQuadTreeCell constructor.
 */
TerrainQuadTreeCell::TerrainQuadTreeCell(const std::string& allocator)
{
	BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
	allocator_ = allocator;

	// Track memory usage
	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::SYSTEM),
							(uint)( sizeof(*this) + allocator_.capacity() ))
#endif	
	quadtreeSize += sizeof( *this );

	if (firstTime)
	{
		MF_WATCH( "Render/Terrain/Terrain2/QuadtreeSize", quadtreeSize );
		firstTime = false;
	}
}

TerrainQuadTreeCell::TerrainQuadTreeCell( const TerrainQuadTreeCell& cell )
{
	BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
	allocator_ = cell.allocator_;

	// Track memory usage
	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::SYSTEM),
							(uint)( sizeof(*this) + allocator_.capacity() ))
#endif
	quadtreeSize += sizeof( *this );
	*this = cell;
}


/**
 *  This is the TerrainQuadTreeCell destructor.
 */
TerrainQuadTreeCell::~TerrainQuadTreeCell()
{
	BW_GUARD;	
#if ENABLE_RESOURCE_COUNTERS
	// Track memory usage
	RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool(allocator_, (uint)ResourceCounters::SYSTEM),
							(uint)( sizeof(*this) + allocator_.capacity() ))
	quadtreeSize -= sizeof( *this );
#endif
}


/**
 *  This function creates a sub quad-tree in the given area.
 *
 *  @param map          The underlying TerrainHeightMap2.
 *  @param xOffset      The x-offset into the TerrainHeightMap2 in terms of
 *                      poles.
 *  @param zOffset      The y-offset into the TerrainHeightMap2 in terms of
 *                      poles.
 *  @param xSize        The x-size into the TerrainHeightMap2 in terms of
 *                      poles.
 *  @param zSize        The z-size into the TerrainHeightMap2 in terms of
 *                      poles.
 *  @param xMin         The x-offset into the TerrainHeightMap2 in terms of
 *                      meters.
 *  @param zMin         The y-offset into the TerrainHeightMap2 in terms of
 *                      meters.
 *  @param xMax         The x-size into the TerrainHeightMap2 in terms of
 *                      meters.
 *  @param zMax         The z-size into the TerrainHeightMap2 in terms of
 *                      meters.
 */
void TerrainQuadTreeCell::init( const TerrainHeightMap2* map,
	uint32 xOffset, uint32 zOffset, uint32 xSize, uint32 zSize,
	float xMin, float zMin, float xMax, float zMax, float minCellSize )
{
	BW_GUARD;
	const Vector3 BOUNDING_BOX_EPSILON( COLLISION_EPSILON, 0.1f, COLLISION_EPSILON );

	// If we are not on the lowest level of the quadtree, create the
	// children's bounding boxes
	if (xSize > minCellSize && zSize > minCellSize)
	{
#if ENABLE_RESOURCE_COUNTERS
		TerrainQuadTreeCell childPrototype(allocator_);
		children_.resize(4, childPrototype);
#else
		children_.resize(4);
#endif
		float halfXDiff = (xMax - xMin) * 0.5f;
		float halfZDiff = (zMax - zMin) * 0.5f;

		uint32 halfXSize = xSize / 2;
		uint32 halfZSize = zSize / 2;

		children_[0].init( map,
			xOffset, zOffset,
			halfXSize, halfZSize,
			xMin, zMin,
			xMax - halfXDiff, zMax - halfZDiff, minCellSize);
		children_[1].init( map,
			xOffset + halfXSize, zOffset,
			halfXSize, halfZSize,
			xMin + halfXDiff, zMin,
			xMax, zMax - halfZDiff, minCellSize);
		children_[2].init( map,
			xOffset, zOffset + halfZSize,
			halfXSize, halfZSize,
			xMin, zMin + halfZDiff ,
			xMax - halfXDiff, zMax, minCellSize);
		children_[3].init( map,
			xOffset + halfXSize, zOffset + halfZSize,
			halfXSize, halfZSize,
			xMin + halfXDiff, zMin + halfZDiff ,
			xMax, zMax, minCellSize);

		boundingBox_ = children_[0].boundingBox_;
		boundingBox_.addBounds( children_[1].boundingBox_ );
		boundingBox_.addBounds( children_[2].boundingBox_ );
		boundingBox_.addBounds( children_[3].boundingBox_ );
	}
	else
	{
		float y = map->heightAt((int)xOffset, (int)zOffset);

		boundingBox_.setBounds(
            Vector3( xMin, y, zMin ) - BOUNDING_BOX_EPSILON,
            Vector3( xMax, y , zMax ) + BOUNDING_BOX_EPSILON );
		for (uint32 z = zOffset; z < (zOffset + zSize + 1); z++)
		{
			for (uint32 x = xOffset; x < (xOffset + xSize + 1); x++)
			{
				boundingBox_.addYBounds( map->heightAt((int)x, (int)z) );
			}
		}
	}
}


/**
 *  This function does a collision detection with this level of the
 *  quad-subtree.
 *
 *  @param source       The start of the line-segment for collision.
 *  @param extent       The end of the line-segment for collision.
 *  @param pOwner       The owner TerrainHeightMap2.
 *  @param pCallback    The collision callback.
 *
 *  @returns            True if a collision is found, false otherwise.
 */
bool TerrainQuadTreeCell::collide( const Vector3& source, const Vector3& extent,
	const TerrainHeightMap2* pOwner, TerrainCollisionCallback* pCallback ) const
{
	BW_GUARD;
	Vector3 s = source;
	Vector3 e = extent;

	// Clip the line to the bounding box of the quadtree cell
	if (boundingBox_.clip( s, e ))
	{
		// Check if we have any children
		if (children_.size())
		{
			Vector3 diff( 1.f / boundingBox_.width(), 0, 1.f / boundingBox_.depth() );

			Vector3 sNormalised = (s - boundingBox_.minBounds()) * diff;
			Vector3 eNormalised = (e - boundingBox_.minBounds()) * diff;

			uint32 startQuadrant = (sNormalised.x > 0.5f ? 1 : 0) + (sNormalised.z > 0.5f ? 2 : 0);
			uint32 endQuadrant = (eNormalised.x > 0.5f ? 1 : 0) + (eNormalised.z > 0.5f ? 2 : 0);

			// Check the quadrants that the ray passes through to see if we hit something.
			if (startQuadrant == endQuadrant)
			{
				return children_[startQuadrant].collide( source, extent, pOwner, pCallback );
			}
			else if ((startQuadrant & endQuadrant) == 0)
			{
				if (children_[startQuadrant].collide( source, extent, pOwner, pCallback ) ||
					children_[startQuadrant ^ 1].collide( source, extent, pOwner, pCallback ) ||
					children_[endQuadrant ^ 1].collide( source, extent, pOwner, pCallback ) ||
					children_[endQuadrant].collide( source, extent, pOwner, pCallback ))
				{
					return true;
				}
			}
			else
			{
				if (children_[startQuadrant].collide( source, extent, pOwner, pCallback ) ||
					children_[endQuadrant].collide( source, extent, pOwner, pCallback ))
				{
					return true;
				}
			}
		}
		else
		{
			return pOwner->hmCollide( source, s, e, pCallback );
		}
	}

	return false;
}

bool TerrainQuadTreeCell::collide(
	const WorldTriangle& source, const Vector3& extent,
	const TerrainHeightMap2* pOwner, TerrainCollisionCallback* pCallback ) const
{
	BW_GUARD;
	WorldTriangle wt = source;
	Vector3 wtExt = extent;

	// Clip the line to the bounding box of the quadtree cell
	if (clipPrism(wt, wtExt, boundingBox_))
	{
		// Check if we have any children
		if (children_.size())
		{
			if (children_[0].collide( source, extent, pOwner, pCallback ) ||
				children_[1].collide( source, extent, pOwner, pCallback ) ||
				children_[2].collide( source, extent, pOwner, pCallback ) ||
				children_[3].collide( source, extent, pOwner, pCallback ))
			{
				return true;
			}
		}
		else
		{

			Vector3 delta = wtExt - wt.v0();

			// Create boundingbox encompassing our prism.
			BoundingBox bbCombined;
			bbCombined.addBounds(wt.v0());
			bbCombined.addBounds(wt.v1());
			bbCombined.addBounds(wt.v2());
			bbCombined.addBounds(wt.v0() + delta);
			bbCombined.addBounds(wt.v1() + delta);
			bbCombined.addBounds(wt.v2() + delta);

			// We want the union of the cell bounding box and the
			// prism bounding box on the x/z axis

			float xStart = std::max(
				boundingBox_.minBounds().x,
				bbCombined.minBounds().x );
			float xEnd = std::min(
				boundingBox_.maxBounds().x,
				bbCombined.maxBounds().x );
			float zStart = std::max(
				boundingBox_.minBounds().z,
				bbCombined.minBounds().z );
			float zEnd = std::min(
				boundingBox_.maxBounds().z,
				bbCombined.maxBounds().z );


			return pOwner->hmCollide( source, wtExt, xStart, zStart, xEnd, zEnd, pCallback );
		}
	}

	return false;
}
