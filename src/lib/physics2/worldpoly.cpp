/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 *	@file
 */

#include "pch.hpp"

#include "worldpoly.hpp"

#include "cstdmf/debug.hpp"
#include "math/vector3.hpp"

#ifndef CODE_INLINE
#include "worldpoly.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: WorldPolygon
// -----------------------------------------------------------------------------

/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It returns whether or
 *	not two points are too close.
 */
inline static bool tooClose( const Vector3 & p1, const Vector3 & p2 )
{
	const float MIN_DIST = 0.001f; // A millimetre
	const float MIN_DIST2 = MIN_DIST * MIN_DIST;

	return (p1 - p2).lengthSquared() <= MIN_DIST2;
}


/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It adds a point to the
 *	polygon if it is not too close to the previous point.
 */
inline static int addToPoly( WorldPolygon & poly, const Vector3 & point )
{
	if (poly.empty() || !tooClose( poly.back(), point ))
	{
		poly.push_back( point );

		return 0;
	}

	return 1;
}


/**
 *	@internal
 *	This is a simple helper function used by splitPoly. It removes some
 *	unnecessary points.
 */
inline static void compressPoly( WorldPolygon & poly )
{
	if (poly.size() < 3)
	{
		poly.clear();
		return;
	}

	if (tooClose( poly.front(), poly.back() ))
	{
		poly.pop_back();
	}

	if (poly.size() < 3)
	{
		poly.clear();
	}
}


/**
 *	This method splits this polygon into the two polygons based on the input
 *	plane.
 *
 *	@param planeEq		The partitioning plane.
 *	@param frontPoly	The polygon in front of the partitioning plane is put in
 *						here.
 *	@param backPoly		The polygon behind the partitioning plane is put in
 *						here.
 */
void WorldPolygon::split( const PlaneEq & planeEq,
	WorldPolygon & frontPoly,
	WorldPolygon & backPoly ) const
{
	frontPoly.clear();
	backPoly.clear();

	if (this->empty())
		return;

	IF_NOT_MF_ASSERT_DEV( this->size() >= 3 )
	{
		return;
	}

	const Vector3 * pPrevPoint = &this->back();
	float prevDist = planeEq.distanceTo( *pPrevPoint );
	bool wasInFront = (prevDist > 0.f);
	int compressions = 0;

	WorldPolygon::const_iterator iter = this->begin();

	while (iter != this->end())
	{
		float currDist = planeEq.distanceTo( *iter );
		bool isInFront = (currDist > 0.f);

		if (isInFront != wasInFront)
		{
			float ratio = fabsf(prevDist / (currDist - prevDist));
			Vector3 cutPoint =
				(*pPrevPoint) + ratio * (*iter - *pPrevPoint);

			compressions += addToPoly( frontPoly, cutPoint );
			compressions += addToPoly( backPoly, cutPoint );

			MF_ASSERT( fabs( planeEq.distanceTo( cutPoint ) ) < 0.01f );
		}

		if (isInFront)
		{
			compressions += addToPoly( frontPoly, *iter );
		}
		else
		{
			compressions += addToPoly( backPoly, *iter );
		}

		wasInFront = isInFront;
		pPrevPoint = &(*iter);
		prevDist = currDist;

		iter++;
	}

//	MF_ASSERT( this->size() + 4 == frontPoly.size() + backPoly.size() + compressions );

	compressPoly( frontPoly );
	compressPoly( backPoly );
}


/**
 *	This method chops this polygon by the input plane. Only the part of the
 *	polygon in the half-space in front of the plane equation is kept.
 */
void WorldPolygon::chop( const PlaneEq & planeEq )
{
	// TODO: This is probably pretty inefficient.
	WorldPolygon newPoly;
	WorldPolygon dummyPoly;

	this->split( planeEq, newPoly, dummyPoly );

	*this = newPoly;
}

// worldtri.cpp
