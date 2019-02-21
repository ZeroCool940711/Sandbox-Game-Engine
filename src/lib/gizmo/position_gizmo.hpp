/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef POSITION_GIZMO_HPP
#define POSITION_GIZMO_HPP

#include "gizmo_manager.hpp"
#include "solid_shape_mesh.hpp"
#include "input/input.hpp"
#include "general_properties.hpp"
#include "math/planeeq.hpp"
#include "model/super_model.hpp"
#include "snap_provider.hpp"
#include "tool_manager.hpp"
#include "coord_mode_provider.hpp"

class PositionShapePart : public ShapePart
{
public:
	PositionShapePart( const Moo::Colour& col, int axis ) 
	:	colour_( col ),
		isFree_( false ),
		isPlane_( true )
	{
		Vector3 normal( 0, 0, 0 );
		normal[axis] = 1.f;
		planeEq_ = PlaneEq( normal, 0 );
	}
	PositionShapePart( const Moo::Colour& col, const Vector3& direction ) 
	:	colour_( col ),
		isPlane_( false ),
		isFree_( false ),
		direction_( direction )
	{
	}
	PositionShapePart( const Moo::Colour& col ) 
	:	colour_( col ),
		isFree_( true )
	{
	}

	const Moo::Colour& colour() const { return colour_; }
	const PlaneEq & plane() const{ return planeEq_; }
	const Vector3& direction() const { return direction_; };

	bool isFree() const {return isFree_;}
	bool isPlane() const {return isPlane_;};
private:
	bool		isFree_;
	bool		isPlane_;
	Moo::Colour	colour_;
	PlaneEq		planeEq_;
	Vector3		direction_;
};


/**
 *	This class implements a gizmo that allows interactive
 *	editing of the translation part of a matrix.
 *
 *	The gizmo itself has 6 interactable areas, namely the
 *	X,Y,Z primary axes, as well as the XY, XZ and YZ planes.
 */
class PositionGizmo : public Gizmo, public Aligned
{
public:
	PositionGizmo( int disablerModifiers = MODIFIER_SHIFT | MODIFIER_CTRL | MODIFIER_ALT,
		MatrixProxyPtr matrix = NULL,
		MatrixProxyPtr visualOffsetMatrix = NULL,
		float radius = 0.2f, 
		bool isPlanar = false );
	~PositionGizmo();

	bool draw( bool force );
	bool intersects( const Vector3& origin, const Vector3& direction, float& t );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );

	void setVisualOffsetMatrixProxy( MatrixProxyPtr matrix );
protected:
	virtual void rebuildMesh( bool force );
	Matrix	objectTransform() const;	
	bool snapToObstableEnabled() const;
	bool snapToTerrainEnabled() const;

	bool					active_;
	ToolPtr					lastTool_;
	float					radius_;
	SnapProvider::SnapMode	snapMode_;
	unsigned int			objectNum_;
	SolidShapeMesh			selectionMesh_;
	Moo::VisualPtr			drawMesh_[3];
	Moo::VisualPtr			pDrawMesh_;
	PositionShapePart*		currentPart_;
	Moo::Colour				lightColour_;
	int						disablerModifiers_;
	bool					isPlanar_;

	MatrixProxyPtr			matrixProxy_;	// the matrix to modify (otherwise uses the current properties)
	MatrixProxyPtr			visualOffsetMatrix_;	// to visually appear somewhere else ;)

	PositionGizmo( const PositionGizmo& );
	PositionGizmo& operator=( const PositionGizmo& );
	SuperModel*	pModel_;
};

#endif // POSITION_GIZMO_HPP
