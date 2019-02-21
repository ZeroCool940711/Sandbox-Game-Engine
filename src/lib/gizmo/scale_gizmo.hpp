/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SCALE_GIZMO_HPP
#define SCALE_GIZMO_HPP


#include "gizmo_manager.hpp"
#include "solid_shape_mesh.hpp"
#include "input/input.hpp"
#include "coord_mode_provider.hpp"


class	ScaleFloatProxy;
class	ScaleMatrixProxy;


typedef SmartPointer<MatrixProxy>		MatrixProxyPtr;
typedef SmartPointer<ScaleFloatProxy>	ScaleFloatProxyPtr;
typedef SmartPointer<ScaleMatrixProxy>	ScaleMatrixProxyPtr;


class ScaleShapePart;

/**
 *	This class implements a gizmo that allows interactive
 *	editing of the scale part of a matrix.
 *
 *	The gizmo represents itself as a ring that by default
 *	appears only when the ALT key is held down, although this
 *	can be overridden by the creator of the object.
 */
class ScaleGizmo : public Gizmo, public Aligned
{
public:
	enum Type
	{
		SCALE_XYZ,			// allow scaling in every direction
		SCALE_UV			// only scale in uv (x-z) directions
	};

	ScaleGizmo
	( 
		MatrixProxyPtr		pMatrix, 
		int					enablerModifier		= MODIFIER_ALT, 
		float				scaleSpeedFactor	= 1.0f,
		Type				type				= SCALE_XYZ
	);
	~ScaleGizmo();

	bool draw( bool force );
	bool intersects( const Vector3& origin, const Vector3& direction, float& t );
	void click( const Vector3& origin, const Vector3& direction );
	void rollOver( const Vector3& origin, const Vector3& direction );
	Matrix getCoordModifier() const;

	Type type() const;

private:
	ScaleGizmo( const ScaleGizmo& );				// not permitted
	ScaleGizmo& operator=( const ScaleGizmo& );		// not permitted

protected:
	Matrix	objectTransform() const;	
	Matrix	gizmoTransform();
	Matrix  objectCoord() const;

	bool					active_;
	SolidShapeMesh			selectionMesh_;
	Moo::VisualPtr			drawMesh_;
	ScaleShapePart *		currentPart_;
	MatrixProxyPtr			pMatrix_;
	Moo::Colour				lightColour_;
	int						enablerModifier_;
	float					scaleSpeedFactor_;
	ScaleFloatProxyPtr		scaleXFloatProxy_;
	ScaleFloatProxyPtr		scaleYFloatProxy_;
	ScaleFloatProxyPtr		scaleZFloatProxy_;
	ScaleMatrixProxyPtr		scaleMatrixProxy_;
	Type					type_;
};

typedef SmartPointer<ScaleGizmo> ScaleGizmoPtr;

#endif // SCALE_GIZMO_HPP
