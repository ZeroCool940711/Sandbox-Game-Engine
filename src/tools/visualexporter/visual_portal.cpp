/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include "visual_portal.hpp"
#include "expsets.hpp"

#include "max.h"
#include "cstdmf/debug.hpp"
#include "convex_hull.hpp"
#include <algorithm>

DECLARE_DEBUG_COMPONENT2( "Exporter", 0 )

// -----------------------------------------------------------------------------
// Section: VisualPortal
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualPortal::VisualPortal()
{
}


/**
 *	Destructor.
 */
VisualPortal::~VisualPortal()
{
}


/**
 *	Add point
 */
void VisualPortal::addPoint( const Vector3 & pt )
{
	if (ExportSettings::instance().useLegacyOrientation())
	{
		pts_.push_back( Vector3( -pt.x, pt.z, -pt.y ) );
	}
	else
	{
		pts_.push_back( Vector3( pt.x, pt.z, pt.y ) );
	}
}


/**
 *	Add an already swizzled point ( do not swap pt.z, pt.y )
 */
void VisualPortal::addSwizzledPoint( const Vector3 & pt )
{
	pts_.push_back( pt );
}


/**
 *	Set name
 */
void VisualPortal::name( const std::string & name )
{
	name_ = name;
}



const char * propStrs[] = { "heaven", "earth", "invasive", "exit" };
const char * dataStrs[] = { "heaven", "earth", "invasive", "invasive" };


/**
 *	This method converts the given property name into the appropriate
 *	data name, or returns NULL if the property name is not a portal name.
 */
const char* VisualPortal::propDataFromPropName( const char* propName )
{
	for (int i = 0; i < sizeof(propStrs)/sizeof(propStrs[0]); i++)
	{
		if (!::_stricmp(propName,propStrs[i]))
			return dataStrs[i];
	}

	return NULL;
}


/**
 *	This method sets the portal name from the user properties
 *	in the given node.
 */
void VisualPortal::nameFromProps( INode* node )
{
	for (int i = 0; i < sizeof(propStrs)/sizeof(propStrs[0]); i++)
	{
		BOOL isProp = false;
		node->GetUserPropBool( propStrs[i], isProp );
		if (isProp)
		{
			const char * data = propDataFromPropName(propStrs[i]);
			if (data) this->name(data);
		}
	}
}


/**
 *	Save to the given data section.
 *
 *	The section must be a boundary section, with
 *	a plane normal and d-value already written out.
 */
void VisualPortal::save( DataSectionPtr pInSect )
{
	MF_ASSERT( pts_.size() >= 3 )
	MF_ASSERT( &*pInSect->findChild( "normal" ) != NULL );
	MF_ASSERT( &*pInSect->findChild( "d" ) != NULL );

	//read in the boundary section, to create the plane basis
	//and generate the uaxis, and plane-local points.
	Vector3 normal = pInSect->readVector3( "normal" );
	float d = pInSect->readFloat( "d" );
	PlaneEq peq( normal, d );

	//create the basis matrix
	Vector3 uAxis = pts_[1] - pts_[0];
	uAxis.normalise();
	Vector3 vAxis = normal.crossProduct(uAxis);

	Matrix basis;
	basis[0] = uAxis;
	basis[1] = vAxis;
	basis[2] = normal;
	basis.translation( normal * d );
	Matrix invBasis; invBasis.invert( basis );

	DataSectionPtr pPortal = pInSect->newSection( "portal" );
	if ( name_ != "" )
		pPortal->writeString( "chunk", name_ );
	pPortal->writeVector3( "uAxis", uAxis );
	for (uint32 i = 0; i < pts_.size(); i++)
	{
		pPortal->newSection( "point" )->setVector3(
			invBasis.applyPoint(pts_[i]) );
	}
}


// this is a quick implementation, not optimised
static bool cullByPlane( std::vector<Vector3>& convex, const PlaneEq& plane )
{
	bool culled = false;
	std::vector<Vector3> newConvex;
	for( std::vector<Vector3>::iterator iter = convex.begin();
		convex.size() > 2 && iter != convex.end(); ++iter )
	{
		std::vector<Vector3>::iterator iter2 = ( iter + 1 != convex.end() ) ? iter + 1 : convex.begin();
		if( !plane.isInFrontOfExact( *iter ) )
		{
			newConvex.push_back( *iter );
			if( plane.isInFrontOfExact( *iter2 ) )
			{
				newConvex.push_back( plane.intersectRay( *iter, *iter2 - *iter ) );
				culled = true;
			}
		}
		else
		{
			if( !plane.isInFrontOfExact( *iter2 ) )
			{
				newConvex.push_back( plane.intersectRay( *iter, *iter2 - *iter ) );
				culled = true;
			}
		}
	}
	convex = newConvex;
	if( convex.size() < 3 )
		convex.clear();

	return culled;
}


bool VisualPortal::cull( const BoundingBox& cbb, bool& culled )
{
	//adjust bounding box to account for floating point imprecision.
	BoundingBox bb(cbb);	
	float epsilon = 0.0001f;
	bb.addBounds( bb.minBounds() + Vector3(-epsilon,-epsilon,-epsilon) );
	bb.addBounds( bb.maxBounds() + Vector3(epsilon, epsilon, epsilon) );

	culled = false;
	culled |= cullByPlane( pts_, PlaneEq( Vector3( bb.minBounds().x, 0.f, 0.f ), Vector3( -1.f, 0.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, bb.minBounds().y, 0.f ), Vector3( 0.f, -1.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, 0.f, bb.minBounds().z ), Vector3( 0.f, 0.f, -1.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( bb.maxBounds().x, 0.f, 0.f ), Vector3( 1.f, 0.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, bb.maxBounds().y, 0.f ), Vector3( 0.f, 1.f, 0.f ) ) );
	culled |= cullByPlane( pts_, PlaneEq( Vector3( 0.f, 0.f, bb.maxBounds().z ), Vector3( 0.f, 0.f, 1.f ) ) );
	return !pts_.empty();
}

bool VisualPortal::cullHull( const std::vector<PlaneEq>& boundaries, bool& culled )
{
	culled = false;
	float epsilon = 0.0001f;

	// Iterate through the boundaries
	std::vector<PlaneEq>::const_iterator cit;
	for(cit = boundaries.begin();
		cit != boundaries.end();
		++cit)
	{
		// Adjust plane to account for floating point inaccuracies
		PlaneEq testPlane(-cit->normal(), -cit->d() + epsilon);

		culled |= cullByPlane( pts_, testPlane );
	}

	return !pts_.empty();
}

void VisualPortal::reverse()
{
	std::reverse( pts_.begin(), pts_.end() );
}

void VisualPortal::planeEquation( PlaneEq& result )
{
	result.init( pts_[0], pts_[1], pts_[2], PlaneEq::SHOULD_NORMALISE );
}


void VisualPortal::createConvexHull()
{
	ConvexHull::GrahamScan gs(pts_);	
}
