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

#include "script_math.hpp"

#include "script.hpp"
#include "stl_to_py.hpp"

#include "math/matrix.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "math/vector4.hpp"

#ifndef MF_SERVER
#include "math/blend_transform.hpp"
#endif

#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )



// -----------------------------------------------------------------------------
// Section: MatrixProvider
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
MatrixProvider::MatrixProvider( PyTypePlus * pType ) :
	PyObjectPlus( pType )
{
}


/*~ class Math.MatrixProvider
 *	@components{ all }
 *
 *	A matrix provider is the base class for all matrices used as transforms.
 *	In many cases when the attribute of a particular object has a
 *	MatrixProvider assigned to it, it stores a reference to the original
 *	MatrixProvider, rather than creating a copy.
 *
 *	For example, in the CursorCamera, its target could be an Entity's world
 *	transformation matrix.  As the entity moves around and updates its matrix,
 *	the Camera, which stores a reference to this matrix, automatically follows
 *	along.
 *
 *	In addition, sometimes the original MatrixProvider doesn't actually store
 *	the matrix but calculates it on the fly.  For example, the DiffDirProvider
 *	provides the matrix to move from a source position to a destination
 *	position.
 *
 *	The base class has no attributes or methods.
 *
 *	The easiest way to examine the actual elements of the matrix is to create a
 *	Matrix based on the MatrixProvider.  This can be done with the following
 *	code:
 *	@{
 *	matrix = Math.Matrix( matrixProvider )
 *	@}
 *	matrix can then be interrogated using all the Matrix methods and
 *	functions.  Updating matrix	will not update matrixProvider, however.
 */
PY_TYPEOBJECT( MatrixProvider )

PY_BEGIN_METHODS( MatrixProvider )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixProvider )
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( MatrixProvider )

typedef SmartPointer<MatrixProvider> MatrixProviderPtr;

// -----------------------------------------------------------------------------
// Section: PyMatrix
// -----------------------------------------------------------------------------


/*~ class Math.Matrix
 *	@components{ all }
 *
 *	The Matrix class is a 4x4 matrix.  It inherits from MatrixProvider.
 *
 *	A Matrix can be created using the Math.Matrix() function.
 */
PY_TYPEOBJECT_WITH_CALL( PyMatrix )

PY_BEGIN_METHODS( PyMatrix )

	/*~ function Matrix.set
	 *	@components{ all }
 	 *
	 *	This method sets the matrix to be the same as the supplied matrix.  It
	 *	has the same effect as assignment.
	 *
	 *	For example:
	 *	@{
	 *	m1.set( m2 ) # sets matrix m1 equal to matrix m2.
	 *	@}
	 *
	 *
	 *	@param	m	the matrix to set this matrix equal to.
	 */
	PY_METHOD( set )

	/*~ function Matrix.setZero
	 *	@components{ all }
	 *
	 *	This method sets the matrix to the zero matrix (with all elements equal
	 *	to zero).
	 *
	 *	For example:
	 *	@{
	 *	# sets the matrix mat to be the zero matrix.
	 *	mat.setZero()
	 *	@}
	 *
	 */
	PY_METHOD( setZero )

	/*~ function Matrix.setIdentity
	 *	@components{ all }
	 *
	 *	This method sets the matrix to the identity matrix (with 1s down the
	 *	main diagonal, and zeros elsewhere).
	 *
	 *	The important property of the identity matrix is that if it is
	 *	multiplied by another matrix, the product is that other matrix.
	 *
	 *	For example:
	 *	@{
	 *	>>> matI.setIdentity() # sets the matrix mat to be the identity matrix.
	 *	>>> matNew = matOther.preMultiply( matI )
	 *	>>>	matNew == matOther
	 *	1
	 *	@}
	 *	The example shows setting a matrix to the identity matrix and then
	 *	multiplying it by another matrix, which results in the same matrix
	 *	as before.
	 *
	 */
	PY_METHOD( setIdentity )

	/*~ function Matrix.setScale
	 *	@components{ all }
	 *
	 *	This method sets the matrix to be a scale matrix, with the specified
	 *	Vector3 scaling along the x, y and z axes.
	 *
	 *	For example:
	 *	@{
	 *	# set the matrix mat to scale 2 times along the x-axis, 3 times along
	 *	# the y-axis and 4 times along the z-axis.
	 *	mat.setScale( (2, 3, 4) )
	 *	@}
	 *
	 *	@param	scales	A Vector3, with the each component being the scale
	 *					along the corresponding axis.
	 */
	PY_METHOD( setScale )

	/*~ function Matrix.setTranslate
	 *	@components{ all }
	 *
	 *	This method sets the matrix to be a translation matrix, with the
	 *	specified Vector3 being the translations along the x, y and z axes.
	 *
	 *	For example:
	 *	@{
	 *	# set the matrix to translate by 4 along the x-axis, 5 along the y-axis
	 *	# and 6 along the z-axis.
	 *	mat.setTranslate( (4, 5, 6) )
	 *	@}
	 *
	 *	@param	translations	A Vector3, with each component being the translation
	 *							along the corresponding axis.
	 */
	PY_METHOD( setTranslate )

	/*~ function Matrix.setRotateX
	 *	@components{ all }
	 *
	 *	This method sets the matrix to rotate around the x-axis by the
	 *	specified amount.  Zero rotation is specified along the z-axis, with
	 *	positive rotation being clockwise.
	 *
	 *	The translation and rotations around the y and z axes are cleared.
	 *
	 *	For example:
	 *	@{
	 *	# sets the matrix to rotate by 1.0 radians clockwise about the x-axis.
	 *	mat.setRotateX( 1.0 )
	 *	@}
	 *
	 *
	 *	@param	rot		The size of the rotation.
	 */
	PY_METHOD( setRotateX )

	/*~ function Matrix.setRotateY
	 *	@components{ all }
	 *
	 *	This method sets the matrix to rotate around the y-axis by the
	 *	specified amount.  Zero rotation is specified along the z-axis, with
	 *	positive rotation being anti-clockwise.
	 *
	 *	The translation and rotations around the x and z axes are cleared.
	 *
	 *	For example:
	 *	@{
	 *	# sets the matrix to rotate by 1.0 radians anti-clockwise about the y-axis.
	 *	mat.setRotateY( 1.0 )
	 *	@}
	 *
	 *	@param	rot		The size of the rotation.
	 */
	PY_METHOD( setRotateY )

	/*~ function Matrix.setRotateZ
	 *	@components{ all }
	 *
	 *	This method sets the matrix to rotate around the z-axis by the
	 *	specified amount.  Zero rotation is specified along the x-axis, with
	 *	positive rotation being clockwise.
	 *
	 *	The translation and rotations around the x and y axes are cleared.
	 *
	 *	For example:
	 *	@{
	 *	# sets the matrix to rotate by 1.0 radians clockwise about the z-axis.
	 *	mat.setRotateZ( 1.0 )
	 *	@}
	 *
	 *	@param	rot		The size of the rotation.
	 */
	PY_METHOD( setRotateZ )

	/*~ function Matrix.setRotateYPR
	 *	@components{ all }
	 *
	 *	This method sets the rotation about all three axes with one function
	 *	call.  It takes a Vector3, the components of which specify the yaw
	 *	(rotation about y, with 0 along the z-axis, increasing anti-clockwise),
	 *	pitch ( rotation about x, with 0 along the z-axis, increasing
	 *	clockwise), and roll (rotation about z, with 0 along the x-axis,
	 *	increasing clockwise).
	 *
	 *	The translation is cleared.
	 *
	 *	For example:
	 *	@{
	 *	mat.setRotateYPR( (1.0, 2.0, 3.0) )
	 *	@}
	 *	sets the matrix to have yaw 1.0, pitch of 2.0 and roll of 3.0,
	 *	applied in that order.
	 *
	 *	@param	ypr	a Vector3 specifying the yaw, pitch and roll.
	 */
	PY_METHOD( setRotateYPR )

	/*~ function Matrix.preMultiply
	 *	@components{ all }
	 *
	 *	This method pre multiplies this matrix by the specified matrix.
	 *	Pre-multiplication means that the specified matrix is on the left of the
	 *	multiplication operator.  In transformation terms, the specified matrix
	 *	is applied BEFORE this matrix.
	 *
	 *	For example:
	 *	@{
	 *	>>> tr = Math.Matrix()
	 *	>>> tr.setTranslate( (1,0,0) )
	 *	>>>	ro = Math.Matrix()
	 *	>>> ro.setRotateY( 3.14159 / 2 )
	 *	>>> out = Math.Matrix()
	 *	>>>	out.set(ro)
	 *	>>> out.preMultiply(tr)
	 *	>>>	out.translation
	 *	(0.000, 1.000, 0.000)
	 *	>>>	out.yaw
	 *	-1.5707
	 *	@}
	 *	This example shows that pre-multiplying the rotation by the translation
	 *	applies the translation first, and then the rotation.  This results in
	 *	the transformed position being rotated, resulting in it being in a
	 *	different position.  Compare this to the results in the example on
	 *	post-multiplication.
	 *
	 *	@param	pre	a Matrix which is the matrix to pre-multiply by.
	 *
	 */
	PY_METHOD( preMultiply )

	/*~ function Matrix.postMultiply
	 *	@components{ all }
	 *
	 *	This method post multiplies this matrix by the specified matrix.
	 *	Post-multiplication means that the specified matrix is on the right of the
	 *	multiplication operator.  In transformation terms, the specified matrix
	 *	is applied AFTER this matrix.
	 *
	 *	For example:
	 *	@{
	 *	>>> tr = Math.Matrix()
	 *	>>> tr.setTranslate( (1,0,0) )
	 *	>>> ro = Math.Matrix()
	 *	>>> ro.setRotateY( 3.14159 / 2 )
	 *	>>> out = Math.Matrix()
	 *	>>> out.set(ro)
	 *	>>> out.postMultiply(tr)
	 *	>>> out.translation
	 *	(1.000, 0.000, 0.000)
	 *	>>> out.yaw
	 *	-1.5707
	 *	@}
	 *	This example shows that post-multiplying the rotation by the translation
	 *	applies the translation second, after rotation.  This results in
	 *	the initial frame being rotated, and then translated, resulting in
	 *	an unaltered translation.  Compare this to the result from
	 *	pre-multiplication.
	 *
	 *	@param	pre	A Matrix which is the matrix to post-multiply by.
	 *
	 */
	PY_METHOD( postMultiply )

	/*~ function Matrix.invert
	 *	@components{ all }
	 *
	 *	This method inverts the matrix. Inverting a matrix is only defined if
	 *	the determinant is non-zero.  If this method is called on a matrix with
	 *	zero determinant, then the matrix is set to the identity matrix.
	 *
	 *	In terms of transformations, inverting a matrix "undoes" its transform.
	 *
	 *	For example:
	 *	@{
	 *	>>> m = Math.Matrix()
	 *	>>> m.setTranslate( (1.0, 2.0, 3.0 ) )
	 *	>>> m.invert()
	 *	>>> m.tranlation
	 *	(-1.000, -2.000, -3.000)
	 *	>>> m1 = Math.Matrix()
	 *	>>> m1.setRotateY( 1.0 )
	 *	>>> m1.invert()
	 *	>>> m1.yaw
	 *	-1.0000
	 *	@}
	 *	The example shows how inverting the matrix "undoes" the translation for
	 *	m, and the rotation for m1.  The same holds for composite transformations.
	 */
	PY_METHOD( invert )

	/*~ function Matrix.lookAt
	 *	@components{ all }
	 *
	 *	This method sets the matrix up to transform by a specified position,
	 *	looking in a given direction with the z-axis and with the specified
	 *	vertical.
	 *
	 *	For example:
	 *	@{
	 *	modelMat = Math.Matrix()
	 *	modelMat.setTranslate( (0,0,0) )
	 *	cameraMat = Math.Matrix()
	 *	pos = Math.Vector3( 0, 0, -10 )
	 *	cameraMat.lookAt( pos, modelMat.applyToOrigin() - pos, (0,1,0) )
	 *	@}
	 *	This example sets up the camera matrix to be positioned at (0,0,-10)
	 *	and to be looking at the model which is placed at the origin. The
	 *	cameras up direction will be the same as the worlds.
	 *
	 *	@param	pos		The position to set the matrix translation to.
	 *	@param	direction	The direction to orient the "look at" matrix.
	 *	@param	up		The vertical axis for the camera.
	 */
	PY_METHOD( lookAt )

	/*~ function Matrix.setElement
	 *	@components{ all }
	 *
	 *	This method sets the specified element of the matrix to the specified
	 *	value.  The element is specified by the first two arguments, which are
	 *	the row and the column respectively.
	 *
	 *	For example:
	 *	@{
	 *	>>> m = Math.Matrix()
	 *	>>> m.setElement( 3, 0, 1.0 )
	 *	>>> m.setElement( 3, 1, 2.0 )
	 *	>>> m.setElement( 3, 2, 4.0 )
	 *	>>> m.translation
	 *	(1.0, 2.0, 4.0)
	 *	@}
	 *	The example sets the translation components of the matrix, element by
	 *	element.
	 *
	 *	@param	row		An integer between 0 and 3.  This is the row that is
	 *					being set.
	 *	@param	col		An integer between 0 and 3.  This is the column that is
	 *					being set.
	 *	@param	val		A float. The value to set the element to.
	 */
	PY_METHOD( setElement )

	/*~ function Matrix.get
	 *	@components{ all }
	 *
	 *	This method gets the value of a specified element of the matrix.
	 *
	 *	For example:
	 *	@{
	 *	>>> m = Math.Matrix()
	 *	>>>	m.setTranslate( (1.0, 2.0, 3.0) )
	 *	>>> m.get( 3, 0 )
	 *	1.0
	 *	>>>	m.get( 3, 1 )
	 *	2.0
	 *	>>>	m.get( 3, 2 )
	 *	3.0
	 *	@}
	 *	The example sets a translation matrix, and then obtains the components
	 *	one by one.
	 *
	 *	@param	row		An integer between 0 and 3.  This is the row that is
	 *					being returned.
	 *	@param	col		An integer between 0 and 3.  This is the column that is
	 *					being returned.
	 *
	 *	@return			A float.  This is the value of the specified element.
	 */
	PY_METHOD( get )

	/*~ function Matrix.applyPoint
	 *	@components{ all }
	 *
	 *	This function applies the matrix to the specified point.  The point
	 *	should be a Vector3, but will be treated as if it were a Vector4 with the
	 *	fourth component being 1.0.
	 *
	 *	For example:
	 *	@{
	 *	>>>	m = Math.Matrix()
	 *	>>>	m.setTranlate( (2,4,8) )
	 *	>>> v = Math.Vector3( 1,2,3 )
	 *	>>> m.applyPoint( v )
	 *	(3.000, 6.000, 11.000)
	 *	@}
	 *	The example applies a translation matrix to the point (1,2,3)
	 *
	 *	@param	point	A Vector3 which is the point to be transformed.
	 *
	 *	@return			A Vector3 which is the transformed point.
	 */
	PY_METHOD( applyPoint )

	/*~ function Matrix.applyV4Point
	 *	@components{ all }
	 *
	 *	This function applies the matrix to the specified point.  In this case
	 *	a point is Vector4, with the 4th component being w.
	 *
	 *	For example:
	 *	@{
	 *	>>>	m = Math.Matrix()
	 *	>>>	m.setTranlate( (2,4,8) )
	 *	>>> v = Math.Vector4( 1,2,3,1 )
	 *	>>> m.applyPoint( v )
	 *	(3.000, 6.000, 11.000, 1.000)
	 *	@}
	 *	The example applies a translation matrix to the point (1,2,3,1)
	 *
	 *	@param	point	A Vector4 which is the point to be transformed.
	 *
	 *	@return			A Vector4 which is the transformed point.
	 */
	PY_METHOD( applyV4Point )

	/*~ function Matrix.applyVector
	 *	@components{ all }
	 *
	 *	This function applies the matrix to the specified vector.  The vector
	 *	is a Vector3, and the last row of the matrix is ignored in the
	 *	calculations.
	 *
	 *	For example:
	 *	@{
	 *	>>>	m = Math.Matrix()
	 *	>>>	m.setTranlate( (2,4,8) )
	 *	>>> v = Math.Vector3( 1,2,3 )
	 *	>>> m.applyVector( v )
	 *	(1.000, 2.000, 3.000)
	 *	@}
	 *	The example applies a translation matrix to the Vector3 (1,2,3)
	 *
	 *	@param	vec	A Vector3 which is the Vector3 to be transformed.
	 *
	 *	@return		A Vector3 which is the transformed Vector3.
	 */
	PY_METHOD( applyVector )

	/*~ function Matrix.applyToAxis
	 *	@components{ all }
	 *
	 *	This function applies the matrix to the specified axis.  The axis is
	 *	specified using an integer between 0 and 2, with 0 corresponding to the
	 *	x-axis, 1 to the y-axis and 2 to the z-axis.
	 *	Numbers above 2 have undefined results.
	 *
	 *	For example:
	 *	@{
	 *	>>>	m = Math.Matrix()
	 *	>>> m.setRotateY( 3.1415 / 2 )
	 *	>>>	m.applyToAxis( 2 )
	 *	(1.00, 0.00, 0.00)
	 *	@}
	 *	This example rotates the z-axis through 90 degrees around the y-axis
	 *	leaving it lined up with the old x-axis.
	 *
	 *	@param	axis	an Ingeger between 0 and 2, specifying the axis to transform
	 *
	 *	@return			a Vector3, which is the transformed axis.
	 */
	PY_METHOD( applyToAxis )

	/*~ function Matrix.applyToOrigin
	 *	@components{ all }
	 *
	 *	This function applies the matrix to the origin.  This is exactly the same
	 *	as calling the applyPoint function on the point (0,0,0), or the
	 *	applyV4Point function on (0,0,0,1).  It is also the same as the
	 *	translation attribute.
	 *
	 *	For example:
	 *	@{
	 *	>>>	m = Math.Matrix()
	 *	>>> m.setTranlation( (1,2,3) )
	 *	>>>	m.applyToOrigin()
	 *	(1.00, 2.00, 3.00)
	 *	@}
	 *	This example applies a translation matrix to the origin.
	 *
	 *	@return			a Vector3 which is the transformed origin.
	 */
	PY_METHOD( applyToOrigin )

	/*~ function Matrix.orthogonalProjection
	 *	@components{ all }
	 *
	 *	This function sets the matrix to a left-handed orthogonal projection
	 *	matrix.  It can be used for a projection matrix on a camera to give
	 *	a non-perspective (orthogonal) view on the world.
	 *
	 *	@param width	a Float, the width of the viewport.
	 *	@param height	a Float, the height of the viewport.
	 *	@param near	a Float, the distance to the near clipping plane.
	 *	@param far	a Float, the distance to the far clipping plane.
	 */
	PY_METHOD( orthogonalProjection )

	/*~ function Matrix.perspectiveProjection
	 *	@components{ all }
	 *
	 *	This function sets the matrix to a left-handed perspective projection
	 *	matrix. This can be used as the projection matrix on a camera to give
	 *	a perspective view on the world.
	 *
	 *	@param	fov		A Float, the field of view.
	 *	@param	aspect	A Float, the aspect ratio (ratio of the width to the height).
	 *	@param near	A Float, the distance to the near clipping plane.
	 *	@param far	A Float, the distance to the far clipping plane.
	 */
	PY_METHOD( perspectiveProjection )

	PY_METHOD( __getstate__ )
	PY_METHOD( __setstate__ )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyMatrix )

	/*~ attribute Matrix.determinant
	 *	@components{ all }
	 *
	 *	This attribute returns the determinant of the matrix.
	 *
	 *	The determinant of a matrix has many mathematical uses.  In the context
	 *	of a 3d engine the most likely use is to compare it to zero.  If the
	 *	determinant of a matrix is zero, then the matrix cannot be inverted.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( determinant )

	/*~ attribute Matrix.isMirrored
	 *	@components{ all }
	 *
	 *	This attribute is non-zero(true) if the matrix is mirrored, and zero
	 *	(false) if the matrix is non mirrored.  This is derived from the
	 *	elements of the matrix.
	 *
	 *	If a matrix, when applied to a coordinate system, changes it from a
	 *	left hand system to a right hand system, then the matrix is described
	 *	as mirrored.
	 *
	 *	@type	Read-Only Integer (as boolean)
	 */
	PY_ATTRIBUTE( isMirrored )

	/*~ attribute Matrix.translation
	 *	@components{ all }
	 *
	 *	This attribute is the translation component of the matrix.  Unlike the
	 *	pitch, yaw and roll attributes, it is both read and write.
	 *
	 *	@type	Vector3
	 */
	PY_ATTRIBUTE( translation )

	/*~ attribute Matrix.yaw
	 *	@components{ all }
	 *
	 *	This attribute is the rotation specified by the matrix around the
	 *	y-axis.  Zero yaw is along the z-axis, with increase in yaw rotating
	 *	anti-clockwise when looking along the y-axis.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( yaw )

	/*~ attribute Matrix.pitch
	 *	@components{ all }
	 *
	 *	This attribute is the rotation specified by the matrix around the
	 *	x-axis.  Zero pitch is along the z-axis with increase in pitch
	 *	rotating clockwise when looking along the x-axis.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( pitch )

	/*~ attribute Matrix.roll
	 *	@components{ all }
	 *
	 *	This attribute is the rotation specified by the matrix around the
	 *	z-axis.  Zero pitch is along the x-axis with increase in roll
	 *	rotating clockwise when looking along the z-axis.
	 *
	 *	@type	Read-Only Float
	 */
	PY_ATTRIBUTE( roll )

PY_END_ATTRIBUTES()

/*~ function Math.Matrix
 *	@components{ all }
 *
 *	This function creates a new Matrix.  It can take either none or one
 *	arguments.  If it takes none, it creates a zero matrix.  If it takes
 *	one argument, then that must be a MatrixProvider, at which point this matrix
 *	is set to the matrix contained in the MatrixProvider at the time of the
 *	function call.
 *
 *	@param	matProv	an optional MatrixProvider, which this matrix is set to.
 *
 *	@return	a new Matrix
 */
PY_FACTORY_NAMED( PyMatrix, "Matrix", Math )

/**
 *	Get python attribute
 */
PyObject * PyMatrix::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return MatrixProvider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int PyMatrix::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return MatrixProvider::pySetAttribute( attr, value );
}

/**
 *	Pickling getstate method
 */
PyObject * PyMatrix::py___getstate__( PyObject * args )
{
	std::string state( (char*)static_cast<Matrix*>(this), sizeof(Matrix) );
	return Script::getData( state );
}

/**
 *	Picking setstate method
 */
PyObject * PyMatrix::py___setstate__( PyObject * args )
{
	if (PyTuple_Size( args ) != 1)
	{
		PyErr_SetString( PyExc_TypeError,
			"Matrix.__setstate__ was given wrong args" );
		return NULL;
	}
	PyObject * pyStr = PyTuple_GET_ITEM( args, 0 );
	if (!PyString_Check( pyStr ) || PyString_Size( pyStr ) != sizeof(Matrix))
	{
		PyErr_SetString( PyExc_ValueError,
			"Matrix.__setstate__ was given bad state" );
		return NULL;
	}
	this->set( *(Matrix*)( PyString_AsString( pyStr ) ) );
	Py_Return;
}

/**
 *	Python factory method
 */
PyObject * PyMatrix::pyNew( PyObject * args )
{
	PyObject * pympp = NULL;
	if (!PyArg_ParseTuple( args, "|O", &pympp ) ||
		(pympp != NULL && !MatrixProvider::Check( pympp )))
	{
		PyErr_SetString( PyExc_TypeError, "Math.Matrix(): "
			"Expected an optional MatrixProvider" );
		return NULL;
	}

	PyMatrix * pm = new PyMatrix();
	if (pympp != NULL) pm->set( static_cast<MatrixProvider*>( pympp ) );
	return pm;
}


// -----------------------------------------------------------------------------
// Section: MatrixProduct
// -----------------------------------------------------------------------------

/**
 *	This utility class calculates the expression 'a x b' from its argument
 *	matrices. Either or both can be None (both => identity).
 *
 *	It is useful also simply as a proxy or script pointer to a matrix,
 *	as well as a volatile expression.
 */
class MatrixProduct : public MatrixProvider
{
	Py_Header( MatrixProduct, MatrixProvider )

public:
	MatrixProduct( PyTypePlus * pType = &s_type_ ) :
		MatrixProvider( &s_type_ ) { }

	virtual void matrix( Matrix & m ) const;

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( a_, a )
	PY_RW_ATTRIBUTE_DECLARE( b_, b )

	static MatrixProduct * New()	{ return new MatrixProduct(); }
	PY_AUTO_FACTORY_DECLARE( MatrixProduct, END )

private:
	MatrixProviderPtr	a_;
	MatrixProviderPtr	b_;
};


/*~ class Math.MatrixProduct
 *	@components{ all }
 *  A subclass of MatrixProvider. This provides the result of the expression
 *  'a x b', where a and b are it's MatrixProvider arguments.
 *  An instance of this class can be created via the factory method
 *  Math.MatrixProduct.
 *
 *  It is not possible to directly access many of the internal attributes of a
 *  MatrixProduct. To view this data it is necessary to construct a
 *  Matrix which is a copy of this, & read the values from it. For
 *  example:
 *  @{
 *  # import Math module
 *  import Math
 *
 *  # create a MatrixProduct which provides the product of matrix1 and matrix2
 *  mprod = Math.MatrixProduct()
 *  mprod.a = matrix1
 *  mprod.b = matrix2
 *
 *  # create a MatrixProvider from the MatrixProduct
 *  m = Math.Matrix( mprod )
 *
 *  # print the yaw
 *  print m.yaw
 *  @}
 *  Note that a Matrix constructed as a copy of a MatrixProduct will
 *  only resemble the MatrixProduct at the time that it is created. It will
 *  not update to reflect changes.
 *
 *  Where a and/or b are not assigned, they are treated as if they were the
 *  equal to the identity matrix.
 */
PY_TYPEOBJECT( MatrixProduct )

PY_BEGIN_METHODS( MatrixProduct )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixProduct )

	/*~ attribute MatrixProduct.a
	 *	@components{ all }
	 *  This is the 'a' in the expression
	 *  'a x b', the results of this being what is provided by the MatrixProduct.
	 *  @type Read-Write MatrixProvider
	 */
	PY_ATTRIBUTE( a )

	/*~ attribute MatrixProduct.b
	 *	@components{ all }
	 *  This is the 'b' in the expression
	 *  'a x b', the results of this being what is provided by the MatrixProduct.
	 *  @type Read-Write MatrixProvider
	 */
	PY_ATTRIBUTE( b )

PY_END_ATTRIBUTES()

/*~ function Math MatrixProduct
 *	@components{ all }
 *  Creates a new MatrixProduct object. This provides the result of the
 *  expression 'a x b', where a and b are it's MatrixProvider arguments.
 */
PY_FACTORY( MatrixProduct, Math )


/**
 *	MatrixProvider matrix method
 */
void MatrixProduct::matrix( Matrix & m ) const
{
	if (a_)
	{
		a_->matrix( m );
		if (b_)
		{
			Matrix bm;
			b_->matrix( bm );
			m.postMultiply( bm );
		}
	}
	else
	{
		if (b_)
			b_->matrix( m );
		else
			m = Matrix::identity;
	}

}


/**
 *	Get python attribute
 */
PyObject * MatrixProduct::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return MatrixProvider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int MatrixProduct::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return MatrixProvider::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: MatrixAnimation
// -----------------------------------------------------------------------------

/**
 *	This utility class calculates the expression 'a x b' from its argument
 *	matrices. Either or both can be None (both => identity).
 *
 *	It is useful also simply as a proxy or script pointer to a matrix,
 *	as well as a volatile expression.
 */
#if !defined( MF_SERVER )
class MatrixAnimation : public MatrixProvider
{
	Py_Header( MatrixAnimation, MatrixProvider )

public:
	MatrixAnimation( PyTypePlus * pType = &s_type_ );

	virtual void matrix( Matrix & m ) const;
	virtual void tick( float dTime );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( time_, time )
	PY_RW_ATTRIBUTE_DECLARE( keyframesHolder_, keyframes )
	PY_RW_ATTRIBUTE_DECLARE( loop_, loop )

	static MatrixAnimation * New()	{ return new MatrixAnimation(); }
	PY_AUTO_FACTORY_DECLARE( MatrixProduct, END )

	typedef std::pair<float,MatrixProviderPtr> Keyframe;
	typedef std::vector< Keyframe> Keyframes;

private:
	mutable float		time_;
	Keyframes	keyframes_;
	PySTLSequenceHolder<Keyframes>	keyframesHolder_;
	bool		loop_;
};

// keyframe converters
namespace Script
{
	int setData( PyObject * pObject, MatrixAnimation::Keyframe & rpVal,
		const char * varName = "" )
	{
		MatrixAnimation::Keyframe t;
		bool good = false;
		if (PyTuple_Check( pObject ) && PyTuple_Size( pObject ) == 2)
		{
			int ret = 0;
			ret |= Script::setData( PyTuple_GET_ITEM( pObject, 0 ), t.first );
			ret |= Script::setData( PyTuple_GET_ITEM( pObject, 1 ), t.second );
			good = (ret == 0) && t.second;
		}
		if (!good)
		{
			PyErr_Format( PyExc_TypeError, "%s must be set to "
				"a tuple of a float and a MatrixProvider", varName );
			return -1;
		}

		rpVal = t;
		return 0;
	}

	PyObject * getData( const MatrixAnimation::Keyframe & pVal )
	{
		PyObject * pTuple = PyTuple_New( 2 );
		PyTuple_SET_ITEM( pTuple, 0, Script::getData( pVal.first ) );
		PyTuple_SET_ITEM( pTuple, 1, Script::getData( pVal.second ) );
		return pTuple;
	}
};


/*~ class Math.MatrixAnimation
 *	@components{ client, tools }
 *
 *	This is a subclass of MatrixProvider. This animates between the values
 *	provided by a number of MatrixProvider objects.
 *
 *	Keyframe values are specified as a list of tuples containing a time and a
 *	MatrixProvider. These are assumed to be in ascending order, so if a
 *	keyframe has a time which is before that of the previous keyframe, the
 *	Matrix which is provided will insantly snap to the new value, rather than
 *	transitioning smoothly. Also, the MatrixAnimation object will have a value
 *	equal to that of the first keyframe from the start of each cycle until the
 *	first keyframe has been hit. Similarly, a MatrixAnimation object's value
 *	will be equal to that of the last keyframe, between the time of the last
 *  keyframe, and the end of the cycle.
 *
 *	A new MatrixAnimation is created using Matrix.MatrixAnimation function.
 */
PY_TYPEOBJECT( MatrixAnimation )

PY_BEGIN_METHODS( MatrixAnimation )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixAnimation )

	/*~ attribute MatrixAnimation.time
	 *	@components{ client, tools }
	 *  The time that has passed since the animation
	 *  entered it's current cycle, in seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( time )

	/*~ attribute MatrixAnimation.keyframes
	 *	@components{ client, tools }
	 *  Each entry in this list contains a time to which the keyframe
	 *  applies, in seconds, and a MatrixProvider which provides the
	 *  keyframe value.
	 *  @type Read-Write list of tuples containing a float and a Vector4Provider
	 */
	PY_ATTRIBUTE( keyframes )

	/*~	attribute MatrixAnimation.loop
	 *	@components{ client, tools }
	 *	Animation sequence will loop if this is set to true.  Defaults to false.
	 *	@type boolean
	 */
	PY_ATTRIBUTE( loop )

PY_END_ATTRIBUTES()

/*~	function Math.MatrixAnimation
 *	@components{ client, tools }
 *  Creates a new instance of MatrixAnimation. This is a subclass of
 *  MatrixProvider which animates between the values provided by
 *  a number of MatrixProvider objects.
 *  @return The new MatrixAnimation
 */
PY_FACTORY( MatrixAnimation, Math )


#ifdef _WIN32
#pragma warning (disable : 4355 )	// this used in base member initialiser list
#endif

MatrixAnimation::MatrixAnimation( PyTypePlus * pType ) :
	MatrixProvider( &s_type_ ),
	keyframesHolder_( keyframes_, this, true ),
	time_( 0 ),
	loop_( false )
{

}

/**
 *	MatrixProvider matrix method
 */
void MatrixAnimation::matrix( Matrix & m ) const
{
	if (!keyframes_.size())
		// No animation for you!
		return;

	// do loop if loop is enabled
	if (loop_)
	{
		float lastTime = keyframes_.back().first;
		if (time_ > lastTime || time_ < 0)
		{
			time_ = fmodf( time_, lastTime );
			if (time_ < 0)
				time_ += lastTime;
		}
	}

	// do linear search through keyframes to find the two time_ falls between.
	Keyframes::const_iterator it = keyframes_.begin();
	while( it != keyframes_.end() && it->first <= time_ )
		++it;

	Keyframes::const_iterator lit;
	if  (it != keyframes_.begin())
	{
		lit = it;
		--lit;
	}
	else
	{
		lit = it;
		it = keyframes_.end();
	}

	if (it != keyframes_.end())
	{
		float t = (time_ - lit->first) / ( it->first - lit->first );
		Matrix m1;
		Matrix m2;
		lit->second->matrix( m1 );
		it->second->matrix( m2 );

		BlendTransform bt( m1 );
		bt.blend( t, BlendTransform( m2 ) );
		bt.output(m);
	}
	else
	{
		lit->second->matrix( m );
	}

}

/**
 *	MatrixProvider tick method
 */
void MatrixAnimation::tick( float dTime )
{
	time_ += dTime;
}

/**
 *	Get python attribute
 */
PyObject * MatrixAnimation::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return MatrixProvider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int MatrixAnimation::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return MatrixProvider::pySetAttribute( attr, value );
}


#endif  // MF_SERVER

// -----------------------------------------------------------------------------
// Section: PyVector<V>
// -----------------------------------------------------------------------------

template <class V>
bool PyVector<V>::safeSetVector( const V & v )
{
	if (isReadOnly_)
	{
		PyErr_SetString( PyExc_TypeError, "Vector is read-only" );
		return false;
	}

	if (!this->setVector( v ))
	{
		if (!PyErr_Occurred())
			PyErr_SetString( PyExc_ValueError, "Vector set to invalid value" );
		return false;
	}

	return true;
}


template <class V>
PyObject * PyVector<V>::_tp_new( PyTypeObject * pType,
		PyObject * args, PyObject * kwargs )
{
	MF_ASSERT( pType == &PyVector<V>::s_type_ );

	PyVector<V> * pObject = new PyVectorCopy<V>();

	if (PyTuple_Size( args ) > 0)
	{
		PyObject * pResult =  pObject->py_set( args );

		if (pResult)
		{
			Py_DECREF( pResult );
		}
		else
		{
			return NULL;
		}
	}

	return pObject;
}


template <class V> const int PyVector<V>::NUMELTS = sizeof(V)/sizeof(float);

/// Static function to create a new PyVector
template <class V>
PyObject * PyVector<V>::pyNew( PyObject * args )
{
	/*
#ifndef _MSC_VER
	// GCC complains if we try to assign this when the structure is
	// initialised. So as a workaround, we do it here instead.
	PyVector<V>::s_type_.ownType.tp_as_number = &PyVector<V>::s_as_number_;
	PyVector<V>::s_type_.ownType.tp_as_sequence = &PyVector<V>::s_as_sequence_;
#endif
*/
	PyVector<V> * v = new PyVectorCopy<V>();

	if (PyTuple_Size( args ) != 0)
	{
		PyObject * ret = v->py_set( args );

		if (ret == NULL)
		{
			Py_DECREF( v );
			return NULL;
		}
		else
		{
			Py_DECREF( ret );
		}
	}
	/*
	else
	{
		// Should set to 0
	}
	*/

	return v;
}


/**
 *	This allows scripts to get various properties of a PyVector
 */
template <class V>
PyObject * PyVector<V>::pyGetAttribute( const char * attr )
{
	const char	* memNames = "xyzw";

	if (attr[0] != '\0' && attr[1] == '\0')
	{
		const char * pPos = strchr( memNames, attr[0] );
		if (pPos && (pPos - attr < NUMELTS))
		{
			int index = pPos - memNames;
			return Script::getData( this->getVector()[index] );
		}
	}

	// This is a very dodgy hack to get rid of a compiler warning. It just so
	// happens that the macro needs typename at the start of the first line if
	// used by a template. Could make a special macro.
	typename PY_GETATTR_STD();

	return PyObjectPlus::pyGetAttribute( attr );
}


/**
 *	This allows scripts to set various properties of a PyVector
 */
template <class V>
int PyVector<V>::pySetAttribute( const char * attr, PyObject * value )
{
	// This is a very dodgy hack to get rid of a compiler warning. It just so
	// happens that the macro needs typename at the start of the first line if
	// used by a template. Could make a special macro.
	typename PY_SETATTR_STD();

	return PyObjectPlus::pySetAttribute( attr, value );
}


/// Return it as a string
template <class V>
PyObject * PyVector<V>::pyStr()
{
	char	str[128];

	V v = this->getVector();

	strcpy( str, "(" );
	for (int i=0; i < NUMELTS; i++)
	{
		if (i>0)
			strcat( str, ", " );
		bw_snprintf( str+strlen(str), sizeof(str)-strlen(str), "%f", v[i] );
	}
	strcat( str, ")" );

	return PyString_FromString( str );
}


/*
// TODO: We could just do the following and get all the functions out of the
// header files.
template <class V>
PyObject * PyVector_py_add( PyObject * a, PyObject * b )
{
	typedef PyVector<V> Vec;
	MF_ASSERT( Vec::Check( a ) );
	MF_ASSERT( Vec<V>::Check( b ) );

	return new PyVector<V>(
		static_cast< Vec * >( a )->getVector() +
			static_cast< Vec * >( b )->getVector() );
}
*/


/// Add
template <class V>
PyObject * PyVector<V>::_py_add( PyObject * a, PyObject * b )
{
	V aVec;
	V bVec;

	if (Script::setData( a, aVec ) == 0 &&
			Script::setData( b, bVec ) == 0)
	{
		return Script::getData( aVec + bVec );
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


/// Subtract
template <class V>
PyObject * PyVector<V>::_py_subtract( PyObject * a, PyObject * b )
{
	V aVec;
	V bVec;

	if (Script::setData( a, aVec ) == 0 &&
			Script::setData( b, bVec ) == 0)
	{
		return Script::getData( aVec - bVec );
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


/// Multiply
template <class V>
PyObject * PyVector<V>::_py_divide( PyObject * a, PyObject * b )
{
	float f;

	if (PyVector<V>::Check( a ) && Script::setData( b, f ) == 0)
	{
		return Script::getData( ((PyVector<V> *)a)->getVector() / f );
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


/// Multiply
template <class V>
PyObject * PyVector<V>::_py_multiply( PyObject * a, PyObject * b )
{
	float f;

	if (PyVector<V>::Check( a ))
	{
		if (Script::setData( b, f ) == 0)
		{
			return Script::getData( f * ((PyVector<V> *)a)->getVector() );
		}
	}
	else if (PyVector<V>::Check( b ))
	{
		if (Script::setData( a, f ) == 0)
		{
			return Script::getData( f * ((PyVector<V> *)b)->getVector() );
		}
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}

/// Multiply for Vector3
template <>
PyObject * PyVector<Vector3>::_py_multiply( PyObject * a,  PyObject * b )
{
	float f;

	if (PyVector<Vector3>::Check( a ))
	{
		if (PyVector<Vector3>::Check( b ))
		{
			return Script::getData(
				((PyVector<Vector3>*)a)->getVector().crossProduct(
					((PyVector<Vector3>*)b)->getVector() ) );
		}
		else if (Script::setData( b, f ) == 0)
		{
			return Script::getData( f * ((PyVector<Vector3> *)a)->getVector() );
		}
	}
	else if (PyVector<Vector3>::Check( b ) &&
		(Script::setData( a, f ) == 0))
	{
		return Script::getData( f * ((PyVector<Vector3> *)b)->getVector() );
	}

	PyErr_Clear();

	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}



/// Negative
template <class V>
PyObject * PyVector<V>::py_negative()
{
	return Script::getData( this->getVector() * -1.f );
}


/// Positive
template <class V>
PyObject * PyVector<V>::py_positive()
{
	return Script::getData( this->getVector() );
}


/// Non Zero
template <class V>
int PyVector<V>::py_nonzero()
{
	return this->getVector().lengthSquared() > 0.f;
}




/// In Place Add
template <class V>
PyObject * PyVector<V>::py_inplace_add( PyObject * b )
{
	V bVec;

	if (Script::setData( b, bVec ) == 0)
	{
		if (this->safeSetVector( this->getVector() + bVec ))
		{
			Py_INCREF( this );
			return this;
		}
		else
		{
			return NULL;
		}
	}

	PyErr_Clear();

	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


/// In Place Subtract
template <class V>
PyObject * PyVector<V>::py_inplace_subtract( PyObject * b )
{
	V bVec;

	if (Script::setData( b, bVec ) == 0)
	{
		if (this->safeSetVector( this->getVector() - bVec ))
		{
			Py_INCREF( this );
			return this;
		}
		else
		{
			return NULL;
		}
	}

	PyErr_Clear();

	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


/// In Place Multiply
template <class V>
PyObject * PyVector<V>::py_inplace_multiply( PyObject * b )
{
	float value;
	if (Script::setData( b, value ) == 0)
	{
		if (this->safeSetVector( this->getVector() * value ) )
		{
			Py_INCREF( this );
			return this;
		}
		else
		{
			return NULL;
		}
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}

/// In Place Multiply for Vector3
template <>
PyObject * PyVector<Vector3>::py_inplace_multiply( PyObject * b )
{
	if (PyVector<Vector3>::Check( b ))
	{
		if (!this->safeSetVector(
				this->getVector().crossProduct(
					((PyVector<Vector3>*)b)->getVector() ) ))
		{
			return NULL;
		}
	}
	else
	{
		float value;
		if (Script::setData( b, value ) == 0)
		{
			if (!this->safeSetVector( this->getVector() * value ) )
			{
				return NULL;
			}
		}
		else
		{
			PyErr_Clear();

			Py_INCREF( Py_NotImplemented );
			return Py_NotImplemented;
		}
	}

	Py_INCREF( this );
	return this;
}


/// In Place Divide
template <class V>
PyObject * PyVector<V>::py_inplace_divide( PyObject * b )
{
	float value;
	if (Script::setData( b, value ) == 0)
	{
		if (this->safeSetVector( this->getVector() / value ) )
		{
			Py_INCREF( this );
			return this;
		}
		else
		{
			return NULL;
		}
	}

	PyErr_Clear();
	Py_INCREF( Py_NotImplemented );
	return Py_NotImplemented;
}


// -----------------------------------------------------------------------------
// Section: as_sequence methods
// -----------------------------------------------------------------------------

template <class V>
int PyVector<V>::py_sq_length()
{
	return NUMELTS;
}

template <class V>
PyObject * PyVector<V>::py_sq_item( int index )
{
	if (index < 0 || NUMELTS <= index )
	{
		PyErr_SetString( PyExc_IndexError, "Vector index out of range" );
		return NULL;
	}

	return Script::getData( this->getVector()[ index ] );
}


template <class V>
PyObject * PyVector<V>::py_sq_slice( int startIndex, int endIndex )
{
	if (startIndex < 0)
		startIndex = 0;

	if (endIndex > NUMELTS)
		endIndex = NUMELTS;

	if (endIndex < startIndex)
		endIndex = startIndex;

	PyObject * pResult = NULL;

	int length = endIndex - startIndex;

	if (length == NUMELTS)
	{
		pResult = this;
		Py_INCREF( pResult );
	}
	else
	switch (length)
	{
		case 0:
			pResult = PyTuple_New( 0 );
			break;

		case 1:
			pResult = PyTuple_New( 1 );
			PyTuple_SET_ITEM( pResult, 0,
					PyFloat_FromDouble( this->getVector()[startIndex] ) );
			break;

		case 2:
		{
			Vector2 v;
			for (int i = startIndex; i < endIndex; i++)
			{
				v[i - startIndex] = this->getVector()[i];
			}
			pResult = Script::getData( v );
			break;
		}

		case 3:
		{
			Vector3 v;
			for (int i = startIndex; i < endIndex; i++)
			{
				v[i - startIndex] = this->getVector()[i];
			}
			pResult = Script::getData( v );
			break;
		}

		default:
			// This should not happen.
			PyErr_Format( PyExc_IndexError,
					"Bad slice indexes [%d, %d] for Vector%d",
					startIndex, endIndex, NUMELTS );
			break;
	}

	return pResult;
}


template <class V>
int PyVector<V>::py_sq_ass_item( int index, PyObject * value )
{
	if (index < 0 || NUMELTS <= index )
	{
		PyErr_SetString( PyExc_IndexError,
				"Vector assignment index out of range" );
		return -1;
	}

	V v = this->getVector();
	int result = Script::setData( value, v[index], "Vector element" );

	if (result == 0)
	{
		if (!this->safeSetVector( v ))
			return -1;
	}

	return result;
}

// -----------------------------------------------------------------------------
// Section: as_sequence methods
// -----------------------------------------------------------------------------

/// Set it
template <class V>
PyObject * PyVector<V>::py_set( PyObject * args )
{
	bool good = false;
	V v;

	if (PyTuple_Size(args) == 1)
	{
		PyObject * pItem = PyTuple_GetItem( args, 0 );

		if (PyTuple_Check( pItem ))
		{
			args = pItem;
			Py_INCREF( args );
		}
		else if (PyList_Check( pItem ))
		{
			args = PyList_AsTuple( pItem );
		}
		else if (PyVector<V>::Check( pItem ))
		{
			v = ((PyVector<V>*)pItem)->getVector();
			good = true;
		}
		else
		{
			float	f;
			if (Script::setData( pItem, f ) == 0)
			{
				for (int i=0; i < NUMELTS; i++)
				{
					v[i] = f;
				}
				good = true;
			}
		}
	}
	else
	{
		Py_INCREF( args );
	}

	// ok, try it as a tuple of NUMELTS then
	if (!good)
	{
		good = (Script::setData( args, v ) == 0);
		Py_DECREF( args );
	}

	if (!good)
	{
		PyErr_Format( PyExc_TypeError, "Vector.set must be set to "
				"a sequence of %d floats, or one float", NUMELTS );
		// TODO: Remove single-float repeating option
		return NULL;
	}

	if (!this->safeSetVector( v ))
	{
		return NULL;
	}

	Py_Return;
}


/// Scalar multiplication
template <class V>
PyObject * PyVector<V>::py_scale( PyObject * args )
{
	if (PyTuple_Size(args) == 1)
	{
		PyObject * pItem = PyTuple_GetItem( args, 0 );

		float	f;
		if (Script::setData( pItem, f ) == 0)
		{
			return Script::getData( this->getVector() * f );
		}
	}

	PyErr_SetString( PyExc_TypeError,
			"Vector.scale expects a float argument" );
	return NULL;
}


/// Dot product
template <class V>
PyObject * PyVector<V>::py_dot( PyObject * args )
{
	PyObject * po = PyVector<V>::pyNew( args );
	if (po == NULL)
	{
		return NULL;
	}

	float result =
		this->getVector().dotProduct( ((PyVector<V>*)po)->getVector() );

	Py_DECREF( po );
	return Script::getData( result );
}




/// Normalise it
template <class V>
PyObject * PyVector<V>::py_normalise( PyObject * args )
{
	if (PyTuple_Size(args) != 0)
	{
		PyErr_SetString( PyExc_TypeError,
			"Vector.normalise takes no arguments"
			" (nor does it brook any dissent :)" );
		return NULL;
	}

	V v = this->getVector();
	v.normalise();
	if (!this->safeSetVector( v ))
		return NULL;

	Py_Return;
}



/// Return it as a tuple
template <class V>
PyObject * PyVector<V>::py_tuple( PyObject * args )
{
	if (PyTuple_Size(args) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "Vector.tuple takes no arguments" );
		return NULL;
	}

	PyObject * pTuple = PyTuple_New( NUMELTS );

	V v = this->getVector();

	for (int i = 0; i < NUMELTS; i++)
	{
		PyTuple_SetItem( pTuple, i, PyFloat_FromDouble( v[i] ) );
	}

	return pTuple;
}


/// Return it as a list
template <class V>
PyObject * PyVector<V>::py_list( PyObject * args )
{
	if (PyTuple_Size(args) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "Vector.list takes no arguments" );
		return NULL;
	}

	V v = this->getVector();

	PyObject * pList = PyList_New( NUMELTS );
	for (int i=0; i < NUMELTS; i++)
	{
		PyList_SetItem( pList, i, Script::getData( v[i] ) );
	}

	return pList;
}


/// Return the 2D cross product
template <class V>
PyObject * PyVector<V>::py_cross2D( PyObject * /*args*/ )
{
	PyErr_SetString( PyExc_TypeError, "No cross2D for this type of vector" );
	return NULL;
}


/// Return the 2D cross product for a Vector2
template <>
PyObject * PyVector<Vector2>::py_cross2D( PyObject * args )
{
	PyObject * pOther = PyVector<Vector2>::pyNew( args );
	if (pOther == NULL)
	{
		return NULL;
	}

	const Vector2 otherV = ((PyVector<Vector2>*)pOther)->getVector();
	const Vector2 thisV = this->getVector();
	float result = thisV[0] * otherV[1] - thisV[1] * otherV[0];

	Py_DECREF( pOther );
	return Script::getData( result );
}


/// Return the 2D cross product for a Vector3
template <>
PyObject * PyVector<Vector3>::py_cross2D( PyObject * args )
{
	PyObject * pOther = PyVector<Vector3>::pyNew( args );
	if (pOther == NULL)
	{
		return NULL;
	}

	const Vector3 otherV = ((PyVector<Vector3>*)pOther)->getVector();
	const Vector3 thisV = this->getVector();
	float result = thisV[0] * otherV[2] - thisV[2] * otherV[0];

	Py_DECREF( pOther );
	return Script::getData( result );
}

/**
 *	This method returns the distance to the input Vector.
 */
template <class V>
PyObject * PyVector<V>::py_distTo( PyObject * args )
{
	V v;

	if (PyTuple_Size( args ) != 1 ||
		Script::setData( PyTuple_GET_ITEM( args, 0 ), v, "argument 1" ) != 0)
	{
		if (Script::setData( args, v, "argument 1" ) != 0)
			return NULL;
	}

	return PyFloat_FromDouble( (this->getVector() - v).length() );
}




/**
 *	This method returns the distance to the input Vector.
 */
template <class V>
PyObject * PyVector<V>::py_distSqrTo( PyObject * args )
{
	V v;

	if (PyTuple_Size( args ) != 1 ||
		Script::setData( PyTuple_GET_ITEM( args, 0 ), v, "argument 1" ) != 0)
	{
		if (Script::setData( args, v, "argument 1" ) != 0)
			return NULL;
	}

	return PyFloat_FromDouble( (this->getVector() - v).lengthSquared() );
}


/**
 *	This method returns the distance between the points in the XZ plane. It is
 *	only implemented for Vector3.
 */
template <class V>
PyObject * PyVector<V>::py_flatDistTo( PyObject * /*args*/ )
{
	PyErr_SetString( PyExc_TypeError,
			"No flatDistTo for this type of vector" );
	return NULL;
}


/**
 *	This method returns the distance squared between the points in the XZ plane.
 *	It is only implemented for Vector3.
 */
template <>
PyObject * PyVector<Vector3>::py_flatDistTo( PyObject * args )
{
	Vector3 v;

	if (PyTuple_Size( args ) != 1 ||
		Script::setData( PyTuple_GET_ITEM( args, 0 ), v, "argument 1" ) != 0)
	{
		if (Script::setData( args, v, "argument 1" ) != 0)
			return NULL;
	}

	const Vector3 thisV = this->getVector();
	float x = thisV.x - v.x;
	float z = thisV.z - v.z;

	return PyFloat_FromDouble( sqrt( x*x + z*z ) );
}


/**
 *	This method returns the distance squared between the points in the XZ plane.
 *	It is only implemented for Vector3.
 */
template <class V>
PyObject * PyVector<V>::py_flatDistSqrTo( PyObject * /*args*/ )
{
	PyErr_SetString( PyExc_TypeError,
			"No flatDistSqrTo for this type of vector" );
	return NULL;
}


/**
 *	This method returns the distance between the points in the XZ plane. It is
 *	only implemented for Vector3.
 */
template <>
PyObject * PyVector<Vector3>::py_flatDistSqrTo( PyObject * args )
{
	Vector3 v;

	if (PyTuple_Size( args ) != 1 ||
		Script::setData( PyTuple_GET_ITEM( args, 0 ), v, "argument 1" ) != 0)
	{
		if (Script::setData( args, v, "argument 1" ) != 0)
			return NULL;
	}

	const Vector3 thisV = this->getVector();
	float x = thisV.x - v.x;
	float z = thisV.z - v.z;

	return PyFloat_FromDouble( x*x + z*z );
}


/**
 *	This method sets the vector to be a unit vector pointing to the given pitch and yaw.
 *	It is only implemented for Vector3.
 */
template <class V>
PyObject * PyVector<V>::py_setPitchYaw( PyObject * /*args*/ )
{
	PyErr_SetString( PyExc_TypeError,
			"No setPitchYaw for this type of vector" );
	return NULL;
}


/**
 *	This method sets the vector to be a unit vector pointing to the given pitch and yaw.
 *	It is only implemented for Vector3.
 */
template <>
PyObject * PyVector<Vector3>::py_setPitchYaw( PyObject * args )
{
	float pitch, yaw;
	if (PyTuple_Size( args ) != 2 ||
		Script::setData( PyTuple_GET_ITEM( args, 0 ), pitch ) != 0 ||
		Script::setData( PyTuple_GET_ITEM( args, 1 ), yaw ) != 0)
	{
		return NULL;
	}

	Vector3 v = this->getVector();
	v.setPitchYaw(pitch, yaw);
	if (!this->safeSetVector( v ))
		return NULL;

	Py_Return;
}


template <class V>
PyObject * PyVector<V>::py___getstate__( PyObject * args )
{
	V v = this->getVector();
	return PyString_FromStringAndSize( (char *)&v, sizeof( V ) );
}

template <class V>
PyObject * PyVector<V>::py___setstate__( PyObject * args )
{
	if (PyTuple_Check( args ) &&
		PyTuple_Size( args ) == 1)
	{
		PyObject * pStr = PyTuple_GET_ITEM( args, 0 );

		if (PyString_Check( pStr ) &&
			(PyString_Size( pStr ) == sizeof( V )))
		{
			V v;
			memcpy( &v, PyString_AsString( pStr ), sizeof( V ) );

			if (!this->safeSetVector( v ))
			{
				return NULL;
			}

			Py_Return;
		}
	}

	PyErr_SetString( PyExc_TypeError,
		"__setstate__ called with invalid type\n" );
	return NULL;
}


/**
 *	PyVector yaw method
 */
template <class V>
PyObject * PyVector<V>::pyGet_yaw()
{
	Py_Return;
}

template <>
PyObject * PyVector<Vector3>::pyGet_yaw()
{
	return Script::getData( this->getVector().yaw() );
}

/**
 *	PyVector pitch method
 */
template <class V>
PyObject * PyVector<V>::pyGet_pitch()
{
	Py_Return;
}

template <>
PyObject * PyVector<Vector3>::pyGet_pitch()
{
	return Script::getData( this->getVector().pitch() );
}


template <class V>
PyObject * PyVector<V>::pyGet_x()
{
	return Script::getData( this->getVector()[0] );
}


template <class V>
PyObject * PyVector<V>::pyGet_y()
{
	return Script::getData( this->getVector()[1] );
}


template <class V>
PyObject * PyVector<V>::pyGet_z()
{
	MF_ASSERT( NUMELTS >= 3 );
	return Script::getData( this->getVector()[2] );
}


template <class V>
PyObject * PyVector<V>::pyGet_w()
{
	MF_ASSERT( NUMELTS >= 4 );
	return Script::getData( this->getVector()[3] );
}


template <class V>
int PyVector<V>::pySet_x( PyObject * value )
{
	V v = this->getVector();
	int result = Script::setData( value, v[0] );
	if (result == 0)
	{
		if (!this->safeSetVector( v ))
			return -1;
	}
	return result;
}


template <class V>
int PyVector<V>::pySet_y( PyObject * value )
{
	V v = this->getVector();
	int result = Script::setData( value, v[1] );
	if (result == 0)
	{
		if (!this->safeSetVector( v ))
			return -1;
	}
	return result;
}


template <class V>
int PyVector<V>::pySet_z( PyObject * value )
{
	V v = this->getVector();
	int result = Script::setData( value, v[2] );
	if (result == 0)
	{
		if (!this->safeSetVector( v ))
			return -1;
	}
	return result;
}


template <class V>
int PyVector<V>::pySet_w( PyObject * value )
{
	V v = this->getVector();
	int result = Script::setData( value, v[3] );
	if (result == 0)
	{
		if (!this->safeSetVector( v ))
			return -1;
	}
	return result;
}



template <class V>
PyNumberMethods * PyVector_tp_as_number()
{
	static PyNumberMethods tp_as_number = {
		PyVector<V>::_py_add,				//binaryfunc nb_add;
		PyVector<V>::_py_subtract,			//binaryfunc nb_subtract;
		PyVector<V>::_py_multiply,			//binaryfunc nb_multiply;
		PyVector<V>::_py_divide,			//binaryfunc nb_divide;
		0,									//binaryfunc nb_remainder;
		0,									//binaryfunc nb_divmod;
		0,									//ternaryfunc nb_power;
		PyVector<V>::_py_negative,			//unaryfunc nb_negative;
		PyVector<V>::_py_positive,			//unaryfunc nb_positive;
		0,									//unaryfunc nb_absolute;
		PyVector<V>::_py_nonzero,			//inquiry nb_nonzero;
		0,									//unaryfunc nb_invert;
		0,									//binaryfunc nb_lshift;
		0,									//binaryfunc nb_rshift;
		0,									//binaryfunc nb_and;
		0,									//binaryfunc nb_xor;
		0,									//binaryfunc nb_or;
		0,									//coercion nb_coerce;
		0,									//unaryfunc nb_int;
		0,									//unaryfunc nb_long;
		0,									//unaryfunc nb_float;
		0,									//unaryfunc nb_oct;
		0,									//unaryfunc nb_hex;
		PyVector<V>::_py_inplace_add,		//binaryfunc nb_inplace_add;
		PyVector<V>::_py_inplace_subtract,	//binaryfunc nb_inplace_subtract;
		PyVector<V>::_py_inplace_multiply,	//binaryfunc nb_inplace_multiply;
		PyVector<V>::_py_inplace_divide,	//binaryfunc nb_inplace_divide;
		0,									//binaryfunc nb_inplace_remainder;
		0,									//ternaryfunc nb_inplace_power;
		0,									//binaryfunc nb_inplace_lshift;
		0,									//binaryfunc nb_inplace_rshift;
		0,									//binaryfunc nb_inplace_and;
		0,									//binaryfunc nb_inplace_xor;
		0,									//binaryfunc nb_inplace_or;
		// Added in release 2.2
		0,									//binaryfunc nb_floor_divide;
		0,									//binaryfunc nb_true_divide;
		0,									//binaryfunc nb_inplace_floor_divide;
		0,									//binaryfunc nb_inplace_true_divide;
	};
	return &tp_as_number;
}

template <class V>
PySequenceMethods * PyVector_tp_as_sequence()
{
	static PySequenceMethods tp_as_sequence =
	{
		PyVector<V>::_py_sq_length,		/* sq_length */
		0,								/* sq_concat */
		0,								/* sq_repeat */
		PyVector<V>::_py_sq_item,		/* sq_item */
		PyVector<V>::_py_sq_slice,		/* sq_slice */
		PyVector<V>::_py_sq_ass_item,	/* sq_ass_item */
		0,								/* sq_ass_slice */
		0,								/* sq_contains */
		0,								/* sq_inplace_concat */
		0,								/* sq_inplace_repeat */
	};
	return &tp_as_sequence;
}

template <class V> const char * PyVector_fullName();

/*~ function Math.Vector2
 *	@components{ all }
 *
 *	This function creates a Vector with 2 elements.  It can either take 2 floats, or a single
 *	float which is applied to both components.
 *
 *	@return	the newly created Vector2.
 */
template <> const char * PyVector_fullName< Vector2 >() { return "Math.Vector2"; }

/*~ function Math.Vector3
 *	@components{ all }
 *
 *	This function creates a Vector with 3 elements.  It can either take 3 floats, or a single
 *	float which is applied to all components.
 *
 *	@return	the newly created Vector3.
 */
template <> const char * PyVector_fullName< Vector3 >() { return "Math.Vector3"; }

/*~ function Math.Vector4
 *	@components{ all }
 *
 *	This function creates a Vector with 4 elements.  It can either take 4 floats, or a single
 *	float which is applied to all components.
 *
 *	@return	the newly created Vector4.
 */
template <> const char * PyVector_fullName< Vector4 >() { return "Math.Vector4"; }

template <class V>
const char * PyVector_baseName()
{
	const char * path = PyVector_fullName<V>();
	const char * pBase = strrchr( path, '.' );
	return pBase ? (pBase + 1) : path;
}

int PyVector_tp_compare( PyObject * v, PyObject * w )
{
	if (PyVector< Vector2 >::Check( v ) &&
		(PyVector< Vector2 >::Check( w )))
	{
		const Vector2 & a = ((PyVector<Vector2> *)v)->getVector();
		const Vector2 & b = ((PyVector<Vector2> *)w)->getVector();
		return (a < b) ? -1 : (b < a) ? 1 : 0;
	}
	if (PyVector< Vector3 >::Check( v ) &&
		(PyVector< Vector3 >::Check( w )))
	{
		const Vector3 & a = ((PyVector<Vector3> *)v)->getVector();
		const Vector3 & b = ((PyVector<Vector3> *)w)->getVector();
		return (a < b) ? -1 : (b < a) ? 1 : 0;
	}
	if (PyVector< Vector4 >::Check( v ) &&
		(PyVector< Vector4 >::Check( w )))
	{
		const Vector4 & a = ((PyVector<Vector4> *)v)->getVector();
		const Vector4 & b = ((PyVector<Vector4> *)w)->getVector();
		return (a < b) ? -1 : (b < a) ? 1 : 0;
	}

	return 0;
}

template <class V>
PyTypePlus PyVector<V>::s_type_ =
{
	PyObject_HEAD_INIT(&PyType_Type)
	0,                              // ob_size
	PyVector_fullName<V>(),			// tp_name
	sizeof(PyVectorCopy<V>),        // tp_basicsize
	0,                              // tp_itemsize

	// methods
	_tp_dealloc,                    // tp_dealloc
	0,                              // tp_print
	0,                              // tp_getattr
	0,                              // tp_setattr
	PyVector_tp_compare,            // tp_compare
	_pyStr,                         // tp_repr
	PyVector_tp_as_number<V>(),		// tp_as_number
	PyVector_tp_as_sequence<V>(),	// tp_as_sequence
	0,                              // tp_as_mapping
	0,                              // tp_hash
	0,                              // tp_call
	_pyStr,
	_tp_getattro,
	_tp_setattro,

	// To stop compiler warnings
	0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES /* | Py_TPFLAGS_HAVE_INPLACEOPS*/,
	0, 0, 0, 0, 0, 0, 0,
	0,	// tp_methods
	0,
	0, 0, 0, 0, 0, 0, 0,
	0,			// tp_alloc
	PyVector<V>::_tp_new,			// tp_new
	0, 0, 0, 0, 0, 0, 0
};



template <class V>
PY_BEGIN_METHODS( PyVector<V> )

	/*~ function PyVector.set
	 *	@components{ all }
	 *
	 *	This function sets the value of a PyVector to the specified value.  It can
	 *	take several different styles of argument:
	 *
	 *	- It can take a PyVector, which sets this vector equal to the arguent.
	 *
	 *	- It can take a tuple, with the same size as the vector, assigning the
	 *	first element to x, the second to y, etc.
	 *
	 *	- It can take a floating point number, which will be assigned to all
	 *	components of the vector.
	 *
	 *	@return None
	 */
	PY_METHOD( set )

	/*~ function PyVector.scale
	 *	@components{ all }
	 *
	 *	Returns the value of this vector, mutiplied by a scalar, leaving this
	 *	vector unaffected.
	 *
	 *	@param	s	the scalar to multiply by.
	 *
	 *	@return		returns the Vector3 product of the scalar with the vector.
	 */
	PY_METHOD( scale )

	/*~ function PyVector.dot
	 *	@components{ all }
	 *
	 *	This function performs a dot product between this vector and the specified
	 *	vector, and returns the product as a float. It doesn't effect this vector.
	 *
	 *	Dot product is defined to be the sum of the products of the individual
	 *	components, ie:
	 *
	 *	x*x + y*y + z*z
	 *
	 *	@param	rhs		The vector to dot this vector with.
	 *
	 *	@return			The float which is the dot product.
	 */
	PY_METHOD( dot )

	/*~ function PyVector.normalise
	 *	@components{ all }
	 *
	 *	This function normalizes this vector (scales it so that its length is
	 *	exactly 1).
	 *
	 *	@return		None
	 */
	PY_METHOD( normalise )

	/*~ function PyVector.tuple
	 *	@components{ all }
	 *
	 *	Returns the vector converted to a tuple of appropriate size. That is, a
	 *	Vector3 is converted to a 3-tuple, a vector4 to 4-tuple.
	 *
	 *	@return		the tupple representing the vector.
	 */
	PY_METHOD( tuple )

	/*~ function PyVector.list
	 *	@components{ all }
	 *
	 *	This function returns the vector converted to a list of the appropriate
	 *	size.  A Vector3 is converted to a list of length three, etc.
	 *
	 *	@return		The list representation of the vector.
	 */
	PY_METHOD( list )

	/*~ function PyVector.cross2D
	 *	@components{ all }
	 *
	 *	This function returns the magnitude of the cross product between two
	 *	Vector2s, or the cross product of the 2d representation (x, z) of two
	 *	vector3s.
	 *
	 *	In the case of vector2s, the formula is v1.x * v2.y - v1.y * v2.x
	 *
	 *	In the case of vector3s, the formula is v1.x * v2.z - v1.z * v2.x
	 *
	 *	@param	v2	The vector on the right hand side of the cross product.
	 *
	 *	@return		The magnitude of the cross product (a float).
	 */
	PY_METHOD( cross2D )

	/*~	function PyVector.distTo
	 *	@components{ all }
	 *
	 *	This function returns the distance between two vectors.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	PY_METHOD( distTo )

	/*~	function PyVector.distSqrTo
	 *	@components{ all }
	 *
	 *	This function returns the square of the distance between two vectors.  This
	 *	is often used for comparisons between two distances, because it saves the
	 *	computational expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the square of the distance between the two vectors, as a float.
	 */
	PY_METHOD( distSqrTo )

	/*~ function PyVector.flatDistTo
	 *	@components{ all }
	 *
	 *	This function calculates the distance between the points in the XZ plane.
	 *	It is only implemented for Vector3.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	PY_METHOD( flatDistTo )

	/*~ function PyVector.flatDistSqrTo
	 *	@components{ all }
	 *
	 *	This function calculates the distance squared between the points in the XZ
	 *	plane. It is only implemented for Vector3.  This is often used for
	 *	comparisons between two distances, because it saves the computational
	 *	expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance squared between the two vectors, as a float.
	 */
	PY_METHOD( flatDistSqrTo )

	/*~ function PyVector.setPitchYaw
	 *	@components{ all }
	 *
	 *	This function sets the vector to be a unit vector pointing to the given
	 *	pitch and yaw. It is only implemented for Vector3.
	 *
	 *	@param	pitch	pitch angle in radians (float).
	 *	@param	yaw		yaw angle in radians (float).
	 *
	 */
	PY_METHOD( setPitchYaw )

	// PY_METHOD( __reduce__ )
	PY_METHOD( __getstate__ )
	PY_METHOD( __setstate__ )
PY_END_METHODS()

template <class V>
typename PyVector< V >::ThisAttributeMap PyVector< V >::s_attributes_;

// Common vector stuff
template <class V> void PyVector< V >::s_InitAttributes_()
{
	typedef PyVector< V > ThisClass;

	PY_ATTRIBUTE( isReference )
	PY_ATTRIBUTE( isReadOnly )

	PY_ATTRIBUTE( length )

	/*~	attribute PyVector.lengthSquared
	 *	@components{ all }
	 *	This is the square of the length represented by the vector
	 *	@type	Read-only float
	 */
	PY_ATTRIBUTE( lengthSquared )

	PY_ATTRIBUTE( x )
	PY_ATTRIBUTE( y )

	if (NUMELTS == 3)
	{
		/*~	attribute PyVector.yaw
		 *	@components{ all }
		 *	Avaliable only for the Vector3, this returns the yaw component of the Vector3.
		 *	@type	Read-only float
		 */
		PY_ATTRIBUTE( yaw )

		/*~	attribute PyVector.pitch
		 *	@components{ all }
		 *	Avaliable only for the Vector3, this returns the pitch component of the Vector3.
		 *	@type	Read-only float
		 */
		PY_ATTRIBUTE( pitch )
	}

	if (NUMELTS >= 3)
	{
		PY_ATTRIBUTE( z )
	}

	if (NUMELTS >= 4)
	{
		PY_ATTRIBUTE( w )
	}

PY_END_ATTRIBUTES()


// -----------------------------------------------------------------------------
// Section: Special comment section for Vector[234]
// -----------------------------------------------------------------------------

/*
 * There is not appropriate place to document Python Vector[234] classes
 * because the corresponding C++ class is a template, so just leave them here.
 */

/*~	class Math.Vector2
 *	@components{ all }
 *	The Vector2 class is a 2-element vector. It can be created using
 *	Math.Vector2() function.
 *
 *	The elements of Vector2 are referred as members x and y.
 *
 *	Vector2 supports following operations:
 *	Addtion: ex. v1 = v2 + v3
 *	Subtraction: ex. v1 = v2 - v3
 *	Negative: ex. v1 = -v2
 *	Positive: ex. v1 = +v2
 *	True-value test: ex. if v1: statements. In this case v1 is true if v1.lengthSquared > 0.f
 *	In-place addition: ex. v1 += v2
 *	In-place subtraction: ex. v1 -= v2
 *	Length: ex. len(v1). In this case always returns 2.
 *	Index: ex. v1[0] equals to v1.x, v1[1] equals to v1.y.
 *	Slice: ex. v1[0:1] returns a tuple contains v1.x, v1[0:2] returns a Vector2 eqauls to v1 itself.
 *	Indexed Assignment: ex. v1[0] = 3.2 equals to v1.x = 3.2
 */
	/*~ function Vector2.set
	 *	@components{ all }
	 *
	 *	This function sets the value of a Vector2 to the specified value.  It can
	 *	take several different styles of argument:
	 *
	 *	- It can take a Vector2, which sets this vector equal to the argument.
	 *
	 *	- It can take a tuple, with the same size as the vector, assigning the
	 *	first element to x and the second to y.
	 *
	 *	- It can take a floating point number, which will be assigned to all
	 *	components of the vector.
	 *
	 *	@return None
	 */
	/*~ function Vector2.scale
	 *	@components{ all }
	 *
	 *	Returns the value of this vector, mutiplied by a scalar, leaving this
	 *	vector unaffected.
	 *
	 *	@param	s	the scalar to multiply by.
	 *
	 *	@return		returns the Vector2 product of the scalar with the vector.
	 */
	/*~ function Vector2.dot
	 *	@components{ all }
	 *
	 *	This function performs a dot product between this vector and the specified
	 *	vector, and returns the product as a float. It doesn't effect this vector.
	 *
	 *	Dot product is defined to be the sum of the products of the individual
	 *	components, ie:
	 *
	 *	x*x + y*y
	 *
	 *	@param	rhs		The vector to dot this vector with.
	 *
	 *	@return			The float which is the dot product.
	 */
	/*~ function Vector2.normalise
	 *	@components{ all }
	 *
	 *	This function normalizes this vector (scales it so that its length is
	 *	exactly 1).
	 *
	 *	@return		None
	 */
	/*~ function Vector2.tuple
	 *	@components{ all }
	 *
	 *	Returns the vector converted to a tuple of 2 elements.
	 *
	 *	@return		the tuple representing the vector.
	 */
	/*~ function Vector2.list
	 *	@components{ all }
	 *
	 *	This function returns the vector converted to a list of 2 elements.
	 *
	 *	@return		The list representation of the vector.
	 */
	/*~ function Vector2.cross2D
	 *	@components{ all }
	 *
	 *	This function returns the magnitude of the cross product between two
	 *	Vector2s.
	 *
	 *	The formula is v1.x * v2.y - v1.y * v2.x
	 *
	 *	@param	v2	The vector on the right hand side of the cross product.
	 *
	 *	@return		The magnitude of the cross product (a float).
	 */
	/*~	function Vector2.distTo
	 *	@components{ all }
	 *
	 *	This function returns the distance between two vectors.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	/*~	function Vector2.distSqrTo
	 *	@components{ all }
	 *
	 *	This function returns the square of the distance between two vectors.  This
	 *	is often used for comparisons between two distances, because it saves the
	 *	computational expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the square of the distance between the two vectors, as a float.
	 */
	/*~	attribute Vector2.length
	 *	@components{ all }
	 *	This is the length that is represented by the vector
	 *	@type	Read-only float
	 */
	/*~	attribute Vector2.lengthSquared
	 *	@components{ all }
	 *	This is the square of the length represented by the vector
	 *	@type	Read-only float
	 */

/*~	class Math.Vector3
 *	@components{ all }
 *	The Vector3 class is a 3-element vector. It can be created using
 *	Math.Vector3() function.
 *
 *	The elements of Vector3 are referred as members x, y and z.
 *
 *	Vector3 supports following operations:
 *	Addtion: ex. v1 = v2 + v3
 *	Subtraction: ex. v1 = v2 - v3
 *	Cross product: ex. v1 = v2 * v3
 *	Negative: ex. v1 = -v2
 *	Positive: ex. v1 = +v2
 *	True-value test: ex. if v1: statements. In this case v1 is true if v1.lengthSquared > 0.f
 *	In-place Addition: ex. v1 += v2
 *	In-place subtraction: ex. v1 -= v2
 *	In-place cross product: ex. v1 *= v2
 *	Length: ex. len(v1). In this case always returns 3.
 *	Index: ex. v1[0] equals to v1.x, v1[1] equals to v1.y, etc.
 *	Slice: ex. v1[0:1] returns a tuple contains v1.x, v1[0:2] returns a Vector2 consists of x and y.
 *	Indexed Assignment: ex. v1[0] = 3.2 equals to v1.x = 3.2
 */
	/*~ function Vector3.set
	 *	@components{ all }
	 *
	 *	This function sets the value of a Vector3 to the specified value.  It can
	 *	take several different styles of argument:
	 *
	 *	- It can take a Vector3, which sets this vector equal to the argument.
	 *
	 *	- It can take a tuple, with the same size as the vector, assigning the
	 *	first element to x, the second to y and the third to z.
	 *
	 *	- It can take a floating point number, which will be assigned to all
	 *	components of the vector.
	 *
	 *	@return None
	 */
	/*~ function Vector3.scale
	 *	@components{ all }
	 *
	 *	Returns the value of this vector, mutiplied by a scalar, leaving this
	 *	vector unaffected.
	 *
	 *	@param	s	the scalar to multiply by.
	 *
	 *	@return		returns the Vector3 product of the scalar with the vector.
	 */
	/*~ function Vector3.dot
	 *	@components{ all }
	 *
	 *	This function performs a dot product between this vector and the specified
	 *	vector, and returns the product as a float. It doesn't effect this vector.
	 *
	 *	Dot product is defined to be the sum of the products of the individual
	 *	components, ie:
	 *
	 *	x*x + y*y + z*z
	 *
	 *	@param	rhs		The vector to dot this vector with.
	 *
	 *	@return			The float which is the dot product.
	 */
	/*~ function Vector3.normalise
	 *	@components{ all }
	 *
	 *	This function normalizes this vector (scales it so that its length is
	 *	exactly 1).
	 *
	 *	@return		None
	 */
	/*~ function Vector3.tuple
	 *	@components{ all }
	 *
	 *	Returns the vector converted to a tuple of 3 elements.
	 *
	 *	@return		the tuple representing the vector.
	 */
	/*~ function Vector3.list
	 *	@components{ all }
	 *
	 *	This function returns the vector converted to a list of 3 elements.
	 *
	 *	@return		The list representation of the vector.
	 */
	/*~ function Vector3.cross2D
	 *	@components{ all }
	 *
	 *	This function returns the magnitude of the cross product between two
	 *	Vector3.
	 *
	 *	The formula is v1.x * v2.z - v1.z * v2.x
	 *
	 *	@param	v2	The vector on the right hand side of the cross product.
	 *
	 *	@return		The magnitude of the cross product (a float).
	 */
	/*~	function Vector3.distTo
	 *	@components{ all }
	 *
	 *	This function returns the distance between two vectors.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	/*~	function Vector3.distSqrTo
	 *	@components{ all }
	 *
	 *	This function returns the square of the distance between two vectors.  This
	 *	is often used for comparisons between two distances, because it saves the
	 *	computational expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the square of the distance between the two vectors, as a float.
	 */
	/*~ function Vector3.flatDistTo
	 *	@components{ all }
	 *
	 *	This function calculates the distance between the points in the XZ plane.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	/*~ function Vector3.flatDistSqrTo
	 *	@components{ all }
	 *
	 *	This function calculates the distance squared between the points in the XZ
	 *	plane. This is often used for comparisons between two distances, because
	 *	it saves the computational expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance squared between the two vectors, as a float.
	 */
	/*~	attribute Vector3.length
	 *	@components{ all }
	 *	This is the length that is represented by the vector
	 *	@type	Read-only float
	 */
	/*~	attribute Vector3.lengthSquared
	 *	@components{ all }
	 *	This is the square of the length represented by the vector
	 *	@type	Read-only float
	 */
	/*~	attribute Vector3.yaw
	 *	@components{ all }
	 *	This returns the yaw component of the Vector3.
	 *	@type	Read-only float
	 */
	/*~	attribute Vector3.pitch
	 *	@components{ all }
	 *	This returns the pitch component of the Vector3.
	 *	@type	Read-only float
	 */
/*~	class Math.Vector4
 *	@components{ all }
 *	The Vector4 class is a 4-element vector. It can be created using
 *	Math.Vector4() function.
 *
 *	The elements of Vector4 are referred as members x, y, z and w.
 *
 *	Vector4 supports following operations:
 *	Addtion: ex. v1 = v2 + v3
 *	Subtraction: ex. v1 = v2 - v3
 *	Negative: ex. v1 = -v2
 *	Positive: ex. v1 = +v2
 *	True-value test: ex. if v1: statements. In this case v1 is true if v1.lengthSquared > 0.f
 *	In-place Addition: ex. v1 += v2
 *	In-place subtraction: ex. v1 -= v2
 *	Length: ex. len(v1). In this case always returns 4.
 *	Index: ex. v1[0] equals to v1.x, v1[1] equals to v1.y, etc.
 *	Slice: ex. v1[0:1] returns a tuple contains v1.x, v1[0:3] returns a Vector3 consists of x, y and z.
 *	Indexed Assignment: ex. v1[3] = 3.2 equals to v1.w = 3.2
 */
	/*~ function Vector4.set
	 *	@components{ all }
	 *
	 *	This function sets the value of a Vector4 to the specified value.  It can
	 *	take several different styles of argument:
	 *
	 *	- It can take a Vector4, which sets this vector equal to the argument.
	 *
	 *	- It can take a tuple, with the same size as the vector, assigning the
	 *	first element to x, the second to y, the third to z and the last to w.
	 *
	 *	- It can take a floating point number, which will be assigned to all
	 *	components of the vector.
	 *
	 *	@return None
	 */
	/*~ function Vector4.scale
	 *	@components{ all }
	 *
	 *	Returns the value of this vector, mutiplied by a scalar, leaving this
	 *	vector unaffected.
	 *
	 *	@param	s	the scalar to multiply by.
	 *
	 *	@return		returns the Vector4 product of the scalar with the vector.
	 */
	/*~ function Vector4.dot
	 *	@components{ all }
	 *
	 *	This function performs a dot product between this vector and the specified
	 *	vector, and returns the product as a float. It doesn't effect this vector.
	 *
	 *	Dot product is defined to be the sum of the products of the individual
	 *	components, ie:
	 *
	 *	x*x + y*y + z*z + w*w
	 *
	 *	@param	rhs		The vector to dot this vector with.
	 *
	 *	@return			The float which is the dot product.
	 */
	/*~ function Vector4.normalise
	 *	@components{ all }
	 *
	 *	This function normalizes this vector (scales it so that its length is
	 *	exactly 1).
	 *
	 *	@return		None
	 */
	/*~ function Vector4.tuple
	 *	@components{ all }
	 *
	 *	Returns the vector converted to a tuple of 4 elements.
	 *
	 *	@return		the tuple representing the vector.
	 */
	/*~ function Vector4.list
	 *	@components{ all }
	 *
	 *	This function returns the vector converted to a list of 4 elements.
	 *
	 *	@return		The list representation of the vector.
	 */
	/*~	function Vector4.distTo
	 *	@components{ all }
	 *
	 *	This function returns the distance between two vectors.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the distance between the two vectors, as a float.
	 */
	/*~	function Vector4.distSqrTo
	 *	@components{ all }
	 *
	 *	This function returns the square of the distance between two vectors.  This
	 *	is often used for comparisons between two distances, because it saves the
	 *	computational expense of calculating a square root.
	 *
	 *	@param	v2	the vector to calculated the distance to, from this vector.
	 *
	 *	@return		the square of the distance between the two vectors, as a float.
	 */
	/*~	attribute Vector4.length
	 *	@components{ all }
	 *	This is the length that is represented by the vector
	 *	@type	Read-only float
	 */
	/*~	attribute Vector4.lengthSquared
	 *	@components{ all }
	 *	This is the square of the length represented by the vector
	 *	@type	Read-only float
	 */

#if 0
// Could use this and get rid of s_baseName
static const char * getbasename( const char * path )
{
	const char * pBase = strrchr( path, '.' );
	return pBase ? (pBase + 1) : path;
}
#endif

template <class V>
void PyVector_init( PyObject * pModule )
{
	PyTypeObject * pType = &PyVector< V >::s_type_;
	Py_INCREF( pType );
	PyModule_AddObject( pModule, PyVector_baseName<V>(), (PyObject *)pType );
}


/**
 * TODO: to be documented.
 */
class TempPyVectorInit : public Script::InitTimeJob
{
public:
	TempPyVectorInit() : Script::InitTimeJob( 0 ) {}

	virtual void init()
	{
		PyObject * pModule = PyImport_AddModule( "Math" );
		PyVector_init<Vector2>( pModule );
		PyVector_init<Vector3>( pModule );
		PyVector_init<Vector4>( pModule );
	}
};

static TempPyVectorInit pyVectorIniter;

// we don't want this kind of token...
#undef PY_CLASS_TOKEN
#define PY_CLASS_TOKEN( THIS_CLASS )


// -----------------------------------------------------------------------------
// Section: Module composition
// -----------------------------------------------------------------------------

/**
 *	Explicit instantiations for GCC weirdness
 */

/**
 *	This variable is a token which should be referenced by applications
 *	that want to use the math classes.
 */
int Math_token =	*(int*)(&PyVector<Vector2>::s_type_);


// -----------------------------------------------------------------------------
// Section: PyColour
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyColour )

PY_BEGIN_METHODS( PyColour )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyColour )

	PY_ATTRIBUTE_ALIAS( x, red )
	PY_ATTRIBUTE_ALIAS( y, green )
	PY_ATTRIBUTE_ALIAS( z, blue )
	PY_ATTRIBUTE_ALIAS( w, alpha )

PY_END_ATTRIBUTES()

PyObject * PyColour::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->PyVector< Vector4 >::pyGetAttribute( attr );
}

int PyColour::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->PyVector< Vector4 >::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: Vector4Provider
// -----------------------------------------------------------------------------

/*~ class Math.Vector4Provider
 *	@components{ all }
 *	Vector4Provider is the abstract base class for all objects which supply a
 *  dynamic Vector4. In many cases when the attribute of a particular object
 *  has a Vector4Provider assigned to it, it stores a reference to the original
 *	Vector4Provider rather than creating a copy.
 *
 *  Code Example:
 *  @{
 *  # This example creates a flashing light at ( 0, 10, 0 ).
 *  # The light flashes because the Vector4Provider assigned to its shader
 *  # attribute provides a dynamic Vector4, and the light uses this to
 *  # determine its colour.
 *
 *  # Import Math module
 *  import Math
 *
 *  # Create a light at positions ( 0, 10, 0 )
 *  newLight = BigWorld.PyChunkLight()
 *  newLight.position = ( i * 20, 10, 0 )
 *  newLight.outerRadius = 10
 *  newLight.visible = 1
 *
 *  # Set up a Vector4Animation, & apply it to the light.
 *  fadeV4 = Math.Vector4Animation()
 *  fadeV4.duration = 1.5
 *  fadeV4.keyframes = [ ( 0.0, ( 200, 0, 0, 0 ) ),    # red at 0.0s
 *                       ( 0.5, ( 0, 200, 0, 0 ) ),    # green at 0.5s
 *                       ( 1.0, ( 0, 0, 200, 0 ) ),    # blue at 1.0s
 *                       ( 1.5, ( 200, 0, 0, 0 ) ) ]   # red at 1.5s
 *  newLight.shader = fadeV4
 *  @}
 *
 *  When a tuple of four floats or a Vector4 is used in place of a
 *  Vector4Provider it is cast to a Vector4Basic, which is a simple static
 *  subclass of Vector4Provider. See the documentation for Vector4Basic for an
 *  example.
 *
 *  Vector4Providers can be used to provide dynamic input to PyChunkLight
 *  objects, GUI shaders, TintShaderPSA objects, and the PyModel dye system.
 *
 *	This base class provides no methods, and only one attribute, which is the
 *  Vector4 currently being provided.
 */
PY_TYPEOBJECT( Vector4Provider )

PY_BEGIN_METHODS( Vector4Provider )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Provider )

	/*~ attribute Vector4Provider.value
	 *	@components{ all }
	 *  The Vector4 currently being provided.
	 *  @type Read-Only Vector4
	 */
	PY_ATTRIBUTE( value )

PY_END_ATTRIBUTES()

Vector4Provider::Vector4Provider( PyTypePlus * pType ) :
	PyObjectPlus( pType )
{
}

PyObject * Vector4Provider::pyGet_value()
{
	Vector4 val;
	this->output( val );
	return Script::getData( val );
}

PyObject * Vector4Provider::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->PyObjectPlus::pyGetAttribute( attr );
}

int Vector4Provider::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->PyObjectPlus::pySetAttribute( attr, value );
}

// class to separate converters
/**
 * TODO: to be documented.
 */
class Vector4Provider_or_Vector4 : public Vector4Provider { };

PY_SCRIPT_CONVERTERS_DECLARE( Vector4Provider_or_Vector4 )
PY_SCRIPT_CONVERTERS( Vector4Provider_or_Vector4 )

// -----------------------------------------------------------------------------
// Section: Vector4Basic
// -----------------------------------------------------------------------------

/*~ class Math.Vector4Basic
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This is a static provider, and is
 *  therefore useful when a Vector4Provider which supplies a constant is
 *  required. No factory method exists to create an instance of this class,
 *  however one is automatically created whenever a tuple of 4 floats or a
 *  Vector4 is cast to a Vector4Provider. The following example shows this in
 *  action.
 *
 *  Code Example:
 *  @{
 *  >>> # This shows how Vector4Basic objects are used by BigWorld to cast
 *  >>> # tuples of four floats and Vector4 objects to Vector4Provider instances.
 *  >>>
 *  >>> # import Math module
 *  >>> import Math
 *	>>>
 *  >>> # create a tuple of four floats
 *  >>> tuple4 = ( 1.0, 2.0, 3.0, 4.5 )
 *  >>>
 *  >>> # create a Vector4
 *  >>> vector4 = Math.Vector4( 2.0, 3.0, 4.0, 5.5 )
 *  >>>
 *  >>> # use both the Vector4 and the tuple of four floats as Vector4Provider
 *  >>> # objects by setting them to the source attributes of a Vector4Product
 *  >>> v4p = Math.Vector4Product()
 *  >>> v4p.a = tuple4
 *  >>> v4p.b = vector4
 *  >>>
 *  >>> # show that the Vector4 and the tuple of four floats have both been cast
 *  >>> # to Vector4Basic objects
 *  >>> print v4p.a, v4p.a.value
 *  Vector4Basic at 0x0AB298B0 (1.000000, 2.000000, 3.000000, 4.500000)
 *  >>> print v4p.b, v4p.b.value
 *  Vector4Basic at 0x09EE75E8 (2.000000, 3.000000, 4.000000, 5.500000)
 *  @}
 */
PY_TYPEOBJECT( Vector4Basic )

PY_BEGIN_METHODS( Vector4Basic )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Basic )

	/*~ attribute Vector4Basic.value
	 *	@components{ all }
	 *	The contents of the Vector4Basic
	 *	@type Vector4
	 */
	PY_ATTRIBUTE( value )

PY_END_ATTRIBUTES()


/**
 *	Constructor
 */
Vector4Basic::Vector4Basic( const Vector4 & val, PyTypePlus * pType ) :
	Vector4Provider( pType ),
	value_( val )
{
}


PyObject * Vector4Basic::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->Vector4Provider::pyGetAttribute( attr );
}

int Vector4Basic::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->Vector4Provider::pySetAttribute( attr, value );
}


// converters for Vector4Provider - try it as a Vector4 first, then
// as a Vector4Provider.
int Script::setData( PyObject * pObject, Vector4Provider * & rpModel,
	const char * /*varName*/ )
{
	SmartPointer<PyObject> pUse = pObject;

	Vector4 v4;
	if (Script::setData( pObject, v4 ) == 0)
		pUse = SmartPointer<PyObject>( new Vector4Basic( v4 ), true );
	else
		PyErr_Clear();

	return Script::setData( &*pUse, (Vector4Provider_or_Vector4*&)rpModel );
}

PyObject * Script::getData( const Vector4Provider * pModel )
{
	return Script::getData( (Vector4Provider_or_Vector4*)pModel );
}

int Script::setData( PyObject * pObject,
	SmartPointer<Vector4Provider> & rpModel, const char * /*varName*/ )
{
	SmartPointer<PyObject> pUse = pObject;

	Vector4 v4;
	if (Script::setData( pObject, v4 ) == 0)
		pUse = SmartPointer<PyObject>( new Vector4Basic( v4 ), true );
	else
		PyErr_Clear();

	return Script::setData( &*pUse,
		(SmartPointer<Vector4Provider_or_Vector4>&)rpModel );
}

PyObject * Script::getData( ConstSmartPointer<Vector4Provider> pModel )
{
	return Script::getData(
		*(ConstSmartPointer<Vector4Provider_or_Vector4>*)&pModel );
}


// -----------------------------------------------------------------------------
// Section: Vector4Product
// -----------------------------------------------------------------------------

/**
 * TODO: to be documented.
 */
class Vector4Product : public Vector4Provider
{
	Py_Header( Vector4Product, Vector4Provider )

public:
	Vector4Product( PyTypePlus * pType = &s_type_ ) { }

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( a_, a )
	PY_RW_ATTRIBUTE_DECLARE( b_, b )

	static Vector4Product * New()	{ return new Vector4Product(); }
	PY_AUTO_FACTORY_DECLARE( Vector4Product, END )

private:
	Vector4ProviderPtr	a_;
	Vector4ProviderPtr	b_;
};


/*~ class Math.Vector4Product
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This provides the componentwise
 *  product of the outputs of two Vector4Provider objects, which means that
 *  each component of the output Vector4 is equal to the product of the
 *  equivalent components of the two source Vector4 objects. For example, the
 *  componentwise product of ( 1, 2, 0.5, 0 ) and ( 2, 2, 2, 2 ) is
 *  ( 2, 4, 1, 0 ).
 *
 *  If a Vector4Product instance has only one source Vector4Provider objects is
 *  specified, then the source's value is provided. If no source
 *  is specified then it provides ( 0, 0, 0, 0 ).
 *
 *  Code Example:
 *  @{
 *  # This creates a flashing yellow light.
 *  # It demonstrates the use of a Vector4Product by multiplying a Vector4LFO,
 *  # which is used to provide the flash, with a Vector4Basic, which is used to
 *  # provide the colour.
 *
 *  # import Math module
 *  import Math
 *
 *  # create the light at ( 0, 10, 0 )
 *  light = BigWorld.PyChunkLight()
 *  light.outerRadius = 10
 *  light.position = ( 0, 10, 0 )
 *  light.visible = 1
 *
 *  # create the flashing Vector4LFO
 *  flash = Math.Vector4LFO()
 *  flash.period = 0.5
 *  flash.waveform = "SAWTOOTH"
 *  flash.phase = 0
 *  flash.amplitude = 1
 *
 *  # create the yellow Vector4Basic
 *  yellow = ( 200, 150, 0, 0 )
 *
 *  # create the Vector4Product and apply to the light
 *  yellowFlash = Math.Vector4Product()
 *  yellowFlash.a = flash
 *  yellowFlash.b = yellow
 *  light.shader = yellowFlash
 *  @}
 */
PY_TYPEOBJECT( Vector4Product )

PY_BEGIN_METHODS( Vector4Product )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Product )

	/*~ attribute Vector4Product.a
	 *	@components{ all }
	 *  Each component of the value of this is multiplied by the corresponding
	 *  component in b to determine the corresponding component in the
	 *  Vector4Product object's value.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( a )

	/*~ attribute Vector4Product.b
	 *	@components{ all }
	 *  Each component of the value of this is multiplied by the corresponding
	 *  component in a to determine the corresponding component in the
	 *  Vector4Product object's value.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( b )

PY_END_ATTRIBUTES()

/*~ function Math Vector4Product
 *	@components{ all }
 *  Creates a new instance of Vector4Product. This is a subclass of
 *  Vector4Provider which provides the componentwise product of the outputs of two
 *  Vector4Provider objects.
 *  @return The new Vector4Product
 */
PY_FACTORY( Vector4Product, Math )


/**
 *	This method delegates the tick method to our children.
 */
void Vector4Product::tick( float dTime )
{
	if (a_) a_->tick(dTime);
	if (b_) b_->tick(dTime);
}


/**
 *	Vector4Provider output method
 */
void Vector4Product::output( Vector4 & v )
{
	if (a_)
	{
		a_->output( v );
		if (b_)
		{
			Vector4 bv;
			b_->output( bv );
			v.parallelMultiply( bv );
		}
	}
	else
	{
		if (b_)
			b_->output( v );
		else
			v.set(0.f,0.f,0.f,0.f);
	}

}


/**
 *	Get python attribute
 */
PyObject * Vector4Product::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Provider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4Product::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Provider::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: Vector4TimeManipulator
// -----------------------------------------------------------------------------
/**
 * TODO: to be documented.
 */
class Vector4TimeManipulator : public Vector4Provider
{
	Py_Header( Vector4TimeManipulator, Vector4Provider )

public:
	Vector4TimeManipulator( PyTypePlus * pType = &s_type_ ) : lastValue_(0.f, 0.f, 0.f, 0.f) { }

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( manipulated_, manipulated )
	PY_RW_ATTRIBUTE_DECLARE( manipulator_, manipulator )

	static Vector4TimeManipulator * New()	{ return new Vector4TimeManipulator(); }
	PY_AUTO_FACTORY_DECLARE( Vector4TimeManipulator, END )

private:
	Vector4ProviderPtr	manipulated_;
	Vector4ProviderPtr	manipulator_;
	Vector4				lastValue_;
};


/*~ class Math.Vector4TimeManipulator
 *	@components{ all }
 *
 *	The Vector4TimeManipulator takes two Vector4Providers,
 *	the time manipulator and the manipulated. Time runs
 *	normally for the time manipulator, but the first
 *	element of its output is used to set the time for the
 *	manipulated.
 *
 *	As an example, by making the manipulator a Vector4LFO
 *	and the manipulated a Vector4Animator, the animation
 *	can cycle back and forwards according to the wave pattern
 *	for the LFO. Hook this up to a ColourShader, and the
 *	colour can cycle back and forwards.
 *
 *	A new Vector4TimeManipulator is created using
 *	Math.Vector4TimeManipulator method.
 */
PY_TYPEOBJECT( Vector4TimeManipulator )

PY_BEGIN_METHODS( Vector4TimeManipulator )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4TimeManipulator )

	/*~ attribute Vector4TimeManipulator.manipulated
	 *	@components{ all }
	 *	If set, this value will be manipulated by the manipulator over time.
	 *	@type Vector4Provider
	 */
	PY_ATTRIBUTE( manipulated )

	/*~ attribute Vector4TimeManipulator.manipulator
	 *	@components{ all }
	 *	If set, this value will manipulate the manipulated value over time.
	 *	@type Vector4Provider
	 */
	PY_ATTRIBUTE( manipulator )

PY_END_ATTRIBUTES()

/*~ function Math.Vector4TimeManipulator
 *	@components{ all }
 *	Creates and returns a new Vector4TimeManipulator, which can change the value of a
 *	Vector4Provider over time, based on another source Vector4Provider.
 */
PY_FACTORY( Vector4TimeManipulator, Math )


/**
 *	This method delegates the tick method to our children.
 */
void Vector4TimeManipulator::tick( float dTime )
{
	if (manipulator_) {
		manipulator_->tick( dTime );

		Vector4 value;
		manipulator_->output( value );

		if (manipulated_)
			manipulated_->tick( value[0] - lastValue_[0] );

		lastValue_ = value;
	} else if (manipulated_)
		manipulated_->tick( dTime );
}


/**
 *	Vector4Provider output method
 */
void Vector4TimeManipulator::output( Vector4 & v )
{
	if (manipulated_)
	{
		manipulated_->output( v );
	}
	else
	{
		v.set(0.f,0.f,0.f,0.f);
	}
}


/**
 *	Get python attribute
 */
PyObject * Vector4TimeManipulator::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Provider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4TimeManipulator::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Provider::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: Vector4Animation
// -----------------------------------------------------------------------------

// keyframe converters
typedef std::pair<float,Vector4ProviderPtr> Vector4Animation_Keyframe;

namespace Script
{
	int setData( PyObject * pObject, Vector4Animation_Keyframe & rpVal,
		const char * varName )
	{
		Vector4Animation_Keyframe t;
		bool good = false;
		if (PyTuple_Check( pObject ) && PyTuple_Size( pObject ) == 2)
		{
			int ret = 0;
			ret |= Script::setData( PyTuple_GET_ITEM( pObject, 0 ), t.first );
			ret |= Script::setData( PyTuple_GET_ITEM( pObject, 1 ), t.second );
			good = (ret == 0) && t.second;
		}
		if (!good)
		{
			PyErr_Format( PyExc_TypeError, "%s must be set to "
				"a tuple of a float and a Vector4Provider", varName );
			return -1;
		}

		rpVal = t;
		return 0;
	}

	PyObject * getData( const Vector4Animation_Keyframe & pVal )
	{
		PyObject * pTuple = PyTuple_New( 2 );
		PyTuple_SET_ITEM( pTuple, 0, Script::getData( pVal.first ) );
		PyTuple_SET_ITEM( pTuple, 1, Script::getData( pVal.second ) );
		return pTuple;
	}
};


/**
 *	This animates between a number of Vector4 providers.
 */
class Vector4Animation : public Vector4Provider
{
	Py_Header( Vector4Animation, Vector4Provider )
public:
	Vector4Animation( PyTypePlus * pType = &s_type_ );

	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( keyframesHolder_, keyframes )
	PY_RW_ATTRIBUTE_DECLARE( time_, time )
	PY_RW_ATTRIBUTE_DECLARE( duration_, duration )

	static Vector4Animation * New()		{ return new Vector4Animation(); }
	PY_AUTO_FACTORY_DECLARE( Vector4Animation, END )

	virtual void tick( float dTime );
	virtual void output( Vector4 & val );

	typedef Vector4Animation_Keyframe Keyframe;
	typedef std::vector< Vector4Animation_Keyframe > Keyframes;

private:

	Keyframes	keyframes_;
	PySTLSequenceHolder<Keyframes>	keyframesHolder_;

	float		time_;
	float		duration_;
};


/*~ class Math.Vector4Animation
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This animates between the values
 *  provided by a number of Vector4Provider objects.
 *
 *  Keyframe values are specified as a list of tuples containing a time and a
 *  Vector4Provider. These are assumed to be in ascending order, so if a
 *  keyframe has a time which is before that of the previous keyframe, the
 *  Vector4 which is provided will insantly snap to the new value, rather than
 *  transitioning smoothly. Also, the Vector4Animation object will have a value
 *  equal to that of the first keyframe from the start of each cycle until the
 *  first keyframe has been hit. Similarly, a Vector4Animation object's value
 *  will be equal to that of the last keyframe, between the time of the last
 *  keyframe, and the end of the cycle.
 *
 *  Code Example:
 *  @{
 *  # This example creates four PyChunkLight objects which demonstrate the
 *  # use of Vector4Animation objects by using them to supply colour
 *  # information to the lights.
 *
 *  # Import Math module
 *  import Math
 *
 *  # Create the lights at positions ( 0, 10, 0 ), ( 20, 10, 0 ),
 *  # ( 40, 10, 0 ), ( 60, 10, 0 ).
 *  lights = []
 *  for i in range( 4 ):
 *      newLight = BigWorld.PyChunkLight()
 *      newLight.position = ( i * 20, 10, 0 )
 *      newLight.outerRadius = 10
 *      newLight.visible = 1
 *      lights.append( newLight )
 *
 *  # Set up the first Vector4Animation, & apply it to the first light.
 *  # This simply demonstrates how to set up a Vector4Animation which
 *  # smoothly transitions across keyframes and loops without snapping.
 *  # The light should start red, fade to green, fade to blue, then fade
 *  # back to red again.
 *  fadeV4 = Math.Vector4Animation()
 *  fadeV4.duration = 1.5
 *  fadeV4.keyframes = [ ( 0.0, ( 200, 0, 0, 0 ) ),    # red at 0.0s
 *                       ( 0.5, ( 0, 200, 0, 0 ) ),    # green at 0.5s
 *                       ( 1.0, ( 0, 0, 200, 0 ) ),    # blue at 1.0s
 *                       ( 1.5, ( 200, 0, 0, 0 ) ) ]   # red at 1.5s
 *  lights[0].shader = fadeV4
 *
 *  # Set up the second Vector4Animation, & apply it to the second light.
 *  # This shows how keyframes are treated when they're not listed in ascending
 *  # order. The light should flash red, green, and blue, without transitioning
 *  # smoothly.
 *  flashV4 = Math.Vector4Animation()
 *  flashV4.duration = 1.5
 *  flashV4.keyframes = [ ( 0.5, ( 200, 0, 0, 0 ) ),    # red at 0.5s
 *                        ( 0.1, ( 0, 200, 0, 0 ) ),    # green at 0.1s
 *                        ( 1.0, ( 0, 200, 0, 0 ) ),    # green at 1.0s
 *                        ( 0.1, ( 0, 0, 200, 0 ) ) ]   # blue at 0.1s
 *  lights[1].shader = flashV4
 *
 *  # Set up the third Vector4Animation, & apply it to the third light.
 *  # This shows how the value provided by a Vector4Animation remains static
 *  # between the start of a cycle and the first keyframe, and between the last
 *  # keyframe and the end of a cycle. The light should hold red, smoothly
 *  # transition to blue, hold blue, then snap instantly back to red at the
 *  # beginning of the next cycle.
 *  endSnapV4 = Math.Vector4Animation()
 *  endSnapV4.duration = 1.5
 *  endSnapV4.keyframes = [ ( 0.5, ( 200, 0, 0, 0 ) ),    # red at 0.5s
 *                          ( 1.0, ( 0, 0, 200, 0 ) ) ]   # blue at 1.0s
 *  lights[2].shader = endSnapV4
 *
 *  # Set up the fourth Vector4Animation, & apply it to the fourth light.
 *  # This shows how dynamic Vector4Provider objects can be used to provide the
 *  # keyframes used by a Vector4Animation. The light should transition between
 *  # off, flashing white, and solid white.
 *  offV4 = ( 0, 0, 0, 0 )           # off keyframe
 *  flashWhiteV4 = Math.Vector4LFO() # flashing white keyframe
 *  flashWhiteV4.period = 0.2
 *  flashWhiteV4.waveform = "SINE"
 *  flashWhiteV4.phase = 0
 *  flashWhiteV4.amplitude = 200
 *  onV4 = ( 200, 200, 200, 200 )    # on keyframe
 *  flashFadeV4 = Math.Vector4Animation()
 *  flashFadeV4.duration = 6.0
 *  flashFadeV4.keyframes = [ ( 0.0, offV4 ),        # off at 0.0s
 *                            ( 2.0, flashWhiteV4 ), # flashing white at 2.0s
 *                            ( 4.0, onV4 ),         # on 4.0s
 *                            ( 6.0, offV4 ) ]       # off 6.0s
 *  lights[3].shader = flashFadeV4
 *  @}
 */
PY_TYPEOBJECT( Vector4Animation )

PY_BEGIN_METHODS( Vector4Animation )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Animation )

	/*~ attribute Vector4Animation.keyframes
	 *	@components{ all }
	 *  Each entry in this list contains a time to which the keyframe
	 *  applies, in seconds, and a Vector4Provider which provides the
	 *  keyframe value.
	 *  @type Read-Write list of tuples containing a float and a Vector4Provider
	 */
	PY_ATTRIBUTE( keyframes )

	/*~ attribute Vector4Animation.time
	 *	@components{ all }
	 *  The time that has passed since the animation
	 *  entered it's current cycle, in seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( time )

	/*~ attribute Vector4Animation.duration
	 *	@components{ all }
	 *  The total duration of the animation's cycle, in
	 *  seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( duration )

PY_END_ATTRIBUTES()

/*~ function Math Vector4Animation
 *	@components{ all }
 *  Creates a new instance of Vector4Animation. This is a subclass of
 *  Vector4Provider which animates between the values provided by
 *  a number of Vector4Provider objects.
 *  @return The new Vector4Animation
 */
PY_FACTORY( Vector4Animation, Math )

#ifdef _WIN32
#pragma warning (disable : 4355 )	// this used in base member initialiser list
#endif

Vector4Animation::Vector4Animation( PyTypePlus * pType ) :
	Vector4Provider( pType ),
	keyframesHolder_( keyframes_, this, true ),
	time_( 0.f ),
	duration_( 1.f )
{
}

PyObject * Vector4Animation::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->Vector4Provider::pyGetAttribute( attr );
}

int Vector4Animation::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->Vector4Provider::pySetAttribute( attr, value );
}

void Vector4Animation::tick( float dTime )
{
	time_ = fmodf( time_ + dTime, std::max( duration_, 0.001f ) );

	Keyframes::iterator iter = keyframes_.begin();

	while (iter != keyframes_.end())
	{
		iter->second->tick( dTime );

		iter++;
	}
}

void Vector4Animation::output( Vector4 & val )
{
	if (keyframes_.empty())
	{
		val = Vector4(0.f,0.f,0.f,0.f);
		return;
	}

	// note: it is up to the user to keep the keyframes in order
	Keyframes::iterator nit = keyframes_.begin();
	Keyframes::iterator oit = nit;
	while (nit != keyframes_.end() && nit->first < time_)
	{
		oit = nit;
		nit++;
	}

	// are we before the first?
	if (nit == oit)
	{
		nit->second->output( val );
	}
	// are we after the last?
	else if (nit == keyframes_.end())
	{
		oit->second->output( val );
	}
	// we must be between oit and nit then
	else
	{
		Vector4 ov, nv;
		oit->second->output( ov );
		nit->second->output( nv );
		float t = (time_ - oit->first) / (nit->first - oit->first);
#ifdef _WIN32
		XPVec4Lerp( &val, &ov, &nv, t );
#else
		val = ov + t * (nv - ov);
#endif
	}
}


// -----------------------------------------------------------------------------
// Section: Vector4Morph
// -----------------------------------------------------------------------------
class Vector4Morph : public Vector4Provider
{
	Py_Header( Vector4Morph, Vector4Provider )
public:

	Vector4Morph( PyTypePlus * pType = &s_type_ ):
		Vector4Provider( pType ),
		target_( 0.f ,0.f ,0.f ,0.f ),
		duration_( 1.f ),
		time_( 0.f ),
		old_( 0.f ,0.f ,0.f ,0.f )
	{
	};

	void tick( float dTime );
	void output( Vector4 & v );	

	const Vector4& target() const	{ return target_; }
	void target( const Vector4& t );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector4, target, target );

	float duration() const	{ return duration_; }
	void duration( float d );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, duration, duration );

	float time() const	{ return time_; }
	void time( float t );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, time, time );

	static Vector4Morph * New()	{ return new Vector4Morph; }
	PY_AUTO_FACTORY_DECLARE( Vector4Morph, END )

	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

private:
	Vector4 target_;	
	float	duration_;
	float	time_;
	Vector4 old_;
};


/*~ class Math.Vector4Morph
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This provides a Vector4
 *  value that morphs from the current value to the target value, over
 *	the specified duration.
 *	You can query its current value using the value attribute.
 *	
 *
 *  Code Example:
 *  @{
 *  # This creates a Vector4 that smoothly changes from value to value.
 *
 *  # import the Math module
 *  import Math
 * 
 *  v = Math.Vector4Morph()
 *  v.duration = 2.0
 *
 *  print Math.Vector4(v)
 *	# '0, 0, 0, 0' 
 *
 *	v.target = (4,4,4,4)
 *	BigWorld.callback( 1.0, partial( checkValue, v ) )
 *
 *	def checkValue( v ):
 *		print Math.Vector4(v)
 *		# '2, 2, 2, 2'
 *
 *		v.target = (2,3,1,9)
 *		v.time = v.duration
 *		print v.current
 *		# '2, 3, 1, 9'
 *  @}
 */
PY_TYPEOBJECT( Vector4Morph )

PY_BEGIN_METHODS( Vector4Morph )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Morph )
	
	/*~ attribute Vector4Morph.target
	 *	@components{ all }
	 *  The target value
	 *  @type Read-Write Vector4
	 */
	PY_ATTRIBUTE( target )

	/*~ attribute Vector4Morph.duration
	 *	@components{ all }
	 *  The time taken by the morpher to transition
	 *  to the new value
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( duration )

	/*~ attribute Vector4Morph.time
	 *	@components{ all }
	 *  The current time value
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( time )

PY_END_ATTRIBUTES()


/*~ function Math Vector4Morph
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This provides a Vector4
 *  value that morphs from the current value to the target value, over
 *	the specified duration.
 *	You can query its current value using the currentValue attribute.
 *	You can make it update immediately by using the reset() method.
 *  @return The new Vector4Morph
 */
PY_FACTORY( Vector4Morph, Math )


/**
 *	This method ticks the Vector4Morph, advancing time by the passed
 *	in frame time amount.  The time is clamped to never exceed the
 *	duration.
 *
 *	@param	float	The delta time, in seconds.
 */
void Vector4Morph::tick( float dTime )
{
	this->time( time_ + dTime );	
}


/**
 *	Vector4Morph output method.
 *	@param	v	The output Vector4
 */
void Vector4Morph::output( Vector4 & v )
{	
	//duration is guaranteed to be > 0.f because
	//it is clamped in the set duration method.
	float t = time_ / duration_;
	v = (t * target_) + ( (1.f-t) * old_ );
}


/**
 *	This method sets the target value, and resets
 *	the internal timer to 0.0 seconds.
 *
 *	@param	Vector4		The new target value.
 */
void Vector4Morph::target( const Vector4& v )
{
	this->output( this->old_ );
	target_ = v;
	time_ = 0.f;
}


/**
 *	Vector4Morph duration method.
 *	Note that if you set the duration to be less than the current time value,
 *	the time value is clamped to be equal to the new duration.
 *	Also, the percentage that time has gone through its duration is kept,
 *	based on the new duration value (the current output value doesn't change
 *	when you change the duration).
 *
 *	@param float	the new duration, its minimum being 0.0001 seconds.
 */
void Vector4Morph::duration( float d )
{
	float t = time_ / duration_;

	duration_ = std::max( 0.0001f, d );

	//call the time function, in order to clamp it to sensible values.
	this->time( t * duration_ );	
}


/**
 *	Vector4Morph time method.  The time is clamped between 0 and duration_.
 *
 *	@param float	the new time.
 */
void Vector4Morph::time( float t )
{
	time_ = Math::clamp( 0.0001f, t, duration_ );
}


PyObject * Vector4Morph::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return this->Vector4Provider::pyGetAttribute( attr );
}

int Vector4Morph::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return this->Vector4Provider::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: Vector4LFO
// -----------------------------------------------------------------------------

/**
 * TODO: to be documented.
 */
class Vector4LFO : public Vector4Provider
{
	Py_Header( Vector4LFO, Vector4Provider )

public:
	enum Waveform
	{
		WAVE_SQUARE,
		WAVE_TRIANGLE,
		WAVE_SINE,
		WAVE_SAWTOOTH
	};

	PY_BEGIN_ENUM_MAP( Waveform, WAVE_ )
		PY_ENUM_VALUE( WAVE_SQUARE )
		PY_ENUM_VALUE( WAVE_TRIANGLE )
		PY_ENUM_VALUE( WAVE_SINE )
		PY_ENUM_VALUE( WAVE_SAWTOOTH )
	PY_END_ENUM_MAP()

	Vector4LFO( PyTypePlus * pType = &s_type_ ):
		waveform_( WAVE_SINE ),
		period_( 1.f ),
		a_( 1.f ),
		phase_( 0.f ),
		time_( 0.f ),
		parity_( 0 )
	{}

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( period_, period )
	PY_RW_ATTRIBUTE_DECLARE( a_, amplitude )
	PY_RW_ATTRIBUTE_DECLARE( phase_, phase )
	PY_RW_ATTRIBUTE_DECLARE( time_, time )
	PY_DEFERRED_ATTRIBUTE_DECLARE( waveform )

	static Vector4LFO * New()	{ return new Vector4LFO(); }
	PY_AUTO_FACTORY_DECLARE( Vector4LFO, END )

private:
	Waveform waveform_;
	float	period_;
	float	a_;
	float	phase_;
	float	time_;
	uint8	parity_;
};

// declare the enum converter functions
PY_ENUM_CONVERTERS_DECLARE( Vector4LFO::Waveform )


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE Vector4LFO::

PY_RW_ATTRIBUTE_DECLARE( waveform_, waveform )

/*~ class Math.Vector4LFO
 *	@components{ all }
 *  This is a subclass of Vector4Provider. This provides Vector4 values using
 *  a Low Frequency Oscillator. Instances of this class can provide vectors
 *  which follow square, triangle, sine or sawtooth waveforms. Each component
 *  of the Vector4 provided by a Vector4LFO will be equal to the output of
 *  the Vector4LFO object's single oscillator. Consequently, all four
 *  components will have the same value. Note also that the waveform produced
 *  by a Vector4LFO will occupy the range between 0 and it's amplitude
 *  attribute. This means that the centre of a waveform produced by a
 *  Vector4LFO with an amplitude of 2 would be 1.
 *
 *  Code Example:
 *  @{
 *  # This creates a flashing light whose brightness follows a sine wave.
 *
 *  # import the Math module
 *  import Math
 *
 *  # create the light at ( 0, 0, 0 )
 *  light = BigWorld.PyChunkLight()
 *  light.position = ( 0, 0, 0 )
 *  light.outerRadius = 10
 *  light.visible = 1
 *
 *  # create the Vector4LFO, & use it to provide the light's colour
 *  # The Vector4LFO provides a sine wave between the values 0 and 200,
 *  # with a period of 2.0 seconds.
 *  v = Math.Vector4LFO()
 *  v.period = 2.0
 *  v.waveform = "SINE"
 *  v.phase = 0
 *  v.amplitude = 200
 *  light.shader = v
 *  @}
 */
PY_TYPEOBJECT( Vector4LFO )

PY_BEGIN_METHODS( Vector4LFO )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4LFO )

	/*~ attribute Vector4LFO.period
	 *	@components{ all }
	 *  The time taken by the oscillator to complete
	 *  an entire wave, in seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( period )

	/*~ attribute Vector4LFO.amplitude
	 *	@components{ all }
	 *  The upper bound of the wave's range. The
	 *  lower bound is 0. This can be less that 0, which causes the wave to be
	 *  inverted.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( amplitude )

	/*~ attribute Vector4LFO.phase
	 *	@components{ all }
	 *  The wave's offset from time 0, in seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( phase )

	/*~ attribute Vector4LFO.waveform
	 *	@components{ all }
	 *  Specifies the shape of the wave. This may be one of the following values:
	 *  "SINE", "SQUARE", "TRIANGLE", "SAWTOOTH".
	 *  @type Read-Write string
	 */
	PY_ATTRIBUTE( waveform )

	/*~ attribute Vector4LFO.time
	 *	@components{ all }
	 *  The time that has passed since the wave entered it's current cycle, in
	 *  seconds.
	 *  @type Read-Write float
	 */
	PY_ATTRIBUTE( time )

PY_END_ATTRIBUTES()

PY_ENUM_MAP( Vector4LFO::Waveform )
PY_ENUM_CONVERTERS_CONTIGUOUS( Vector4LFO::Waveform )

/*~ function Math Vector4LFO
 *	@components{ all }
 *  Creates a new instance of Vector4LFO. This is a subclass of
 *  Vector4Provider which provides Vector4 values using a Low Frequency
 *  Oscillator. Instances of this class can provide vectors which follow
 *  square, triangle, sine or sawtooth  shaped waveforms.
 *  @return The new Vector4LFO
 */
PY_FACTORY( Vector4LFO, Math )


void Vector4LFO::tick( float dTime )
{
	time_ = time_ + dTime;
	parity_++;
}

/**
 *	Vector4LFO output method
 */
void Vector4LFO::output( Vector4 & v )
{
	float x = 0.f;

	switch( waveform_ )
	{
	case WAVE_SQUARE:
		if (period_ == 0) {
			x = 0;
		}
		else
		{
			time_ = fmodf( time_, period_ );
			float t = time_ + phase_;
			bool positive = false;
			if ( period_ > 0.f )
				positive = (t-period_/2.f > 0.f);
			else
				positive = !!(parity_%2);
			x = positive ? a_ : 0.f;
		}
		break;
	case WAVE_TRIANGLE:
		if (period_ == 0) {
			x = 0;
		}
		else
		{
			time_ = fmodf( time_, period_ );
			float t = time_ + phase_;
			float f = 1.f / period_;
			bool positive = (t-period_/2.f > 0.f);
			x = fmodf( t*f, 1.f );
			if ( !positive )
				x = 1.f - x;
			x = x * a_;
		}
		break;
	case WAVE_SINE:
		if (period_ == 0) {
			x = 0;
		}
		else
		{
			float t = fmodf(time_ + phase_, period_);
			t = (t / period_) * 2.f * MATH_PI;
			x = (sinf( t ) * 0.5f + 0.5f) * a_;
		}
		break;
	case WAVE_SAWTOOTH:
		if (period_ == 0) {
			x = 0;
		}
		else
		{
			time_ = fmodf( time_, period_ );
			float t = time_ + phase_;
			float f = 1.f / period_;
			x = fmodf( t*f, 1.f ) * a_;
		}
		break;
	}

	v.set( x,x,x,x );
}


/**
 *	Get python attribute
 */
PyObject * Vector4LFO::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Provider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4LFO::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Provider::pySetAttribute( attr, value );
}



// -----------------------------------------------------------------------------
// Section: Vector4MatrixAdaptor
// -----------------------------------------------------------------------------
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE
// #define PY_ATTR_SCOPE Vector4MatrixAdaptor::

/**
 * TODO: to be documented.
 */
class Vector4MatrixAdaptor : public MatrixProvider
{
	Py_Header( Vector4MatrixAdaptor, MatrixProvider )

public:
	enum Style
	{
		STYLE_XYZ_SCALE,
		STYLE_XY_SCALE,
		STYLE_X_ROTATE,
		STYLE_Y_ROTATE,
		STYLE_Z_ROTATE
	};

	PY_BEGIN_ENUM_MAP( Style, STYLE_ )
		PY_ENUM_VALUE( STYLE_XYZ_SCALE )
		PY_ENUM_VALUE( STYLE_XY_SCALE )
		PY_ENUM_VALUE( STYLE_X_ROTATE )
		PY_ENUM_VALUE( STYLE_Y_ROTATE )
		PY_ENUM_VALUE( STYLE_Z_ROTATE )
	PY_END_ENUM_MAP()

	Vector4MatrixAdaptor( PyTypePlus * pType = &s_type_ ):
		MatrixProvider( &s_type_ ),
		style_( STYLE_XY_SCALE ),
		source_( NULL )
	{}

	virtual void tick( float dTime );
	virtual void matrix( Matrix & m ) const;

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( source_, source )
	PY_DEFERRED_ATTRIBUTE_DECLARE( style )

	static Vector4MatrixAdaptor * New()	{ return new Vector4MatrixAdaptor(); }
	PY_AUTO_FACTORY_DECLARE( Vector4MatrixAdaptor, END )

private:
	Style style_;
	Vector4ProviderPtr	source_;
};

// declare the enum converter functions
PY_ENUM_CONVERTERS_DECLARE( Vector4MatrixAdaptor::Style )


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE Vector4MatrixAdaptor::
PY_RW_ATTRIBUTE_DECLARE( style_, style )

/*~ class Math.Vector4MatrixAdaptor
 *	@components{ all }
 *	As a matrix adaptor, this class produces a MatrixProvider representation
 *	based upon a source Vector4Provider and it's rotational or scalable style.
 *
 *	A new Vector4MatrixAdaptor is created using Math.Vector4MatrixAdaptor
 *	method.
 */
PY_TYPEOBJECT( Vector4MatrixAdaptor )

PY_BEGIN_METHODS( Vector4MatrixAdaptor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4MatrixAdaptor )

	/*~	attribute Vector4MatrixAdaptor.source
	 *	@components{ all }
	 *	Stores the values that this MatrixProvider will represent.
	 *	@type Vector4Provider
	 */
	PY_ATTRIBUTE( source )

	/*~	attribute Vector4MatrixAdaptor.style
	 *	@components{ all }
	 *	Specifies the rotational or scale style that the source is to represent in this MatrixProvider.
	 *	Possible styles are:
	 *
	 *	"XYZ_SCALE"
	 *	"XY_SCALE"
	 *	"X_ROTATE"
	 *	"Y_ROTATE"
	 *	"Z_ROTATE"
	 *
	 *	@type Vector4Provider
	 */
	PY_ATTRIBUTE( style )

PY_END_ATTRIBUTES()

PY_ENUM_MAP( Vector4MatrixAdaptor::Style )
PY_ENUM_CONVERTERS_CONTIGUOUS( Vector4MatrixAdaptor::Style )

/*~	function Math.Vector4MatrixAdaptor
 *	@components{ all }
 *	Creates and returns a new Vector4MatrixAdaptor, which is a MatrixProvider that represents
 *	a Vector4Provider, based upon its designated style/purpose.
 */
PY_FACTORY( Vector4MatrixAdaptor, Math )


void Vector4MatrixAdaptor::tick( float dTime )
{
	if ( source_ && source_.hasObject() )
		source_->tick( dTime );
}

/**
 *	Vector4MatrixAdaptor output method
 */
void Vector4MatrixAdaptor::matrix( Matrix & m ) const
{
	if ( !source_ )
		return;

	Vector4 v;
	source_->output( v );

	switch( style_ )
	{
	case STYLE_XYZ_SCALE:
		m.setScale( v.x, v.y, v.z );
		break;
	case STYLE_XY_SCALE:
		m.setScale( v.x, v.y, 1.f );
		break;
	case STYLE_X_ROTATE:
		m.setRotateX( v.x );
		break;
	case STYLE_Y_ROTATE:
		m.setRotateY( v.y );
		break;
	case STYLE_Z_ROTATE:
		m.setRotateZ( v.z );
		break;
	}
}


/**
 *	Get python attribute
 */
PyObject * Vector4MatrixAdaptor::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return MatrixProvider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4MatrixAdaptor::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return MatrixProvider::pySetAttribute( attr, value );
}

#undef PY_ATTR_SCOPE



// -----------------------------------------------------------------------------
// Section: Vector4Swizzle
// -----------------------------------------------------------------------------
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE
// #define PY_ATTR_SCOPE Vector4Swizzle::

/**
 * TODO: to be documented.
 */
class Vector4Swizzle : public Vector4Provider
{
	Py_Header( Vector4Swizzle, Vector4Provider )

public:
	Vector4Swizzle( PyTypePlus * pType = &s_type_ ):
	x_( NULL ),
	y_( NULL ),
	z_( NULL ),
	w_( NULL )
	{}

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( x_, x )
	PY_RW_ATTRIBUTE_DECLARE( y_, y )
	PY_RW_ATTRIBUTE_DECLARE( z_, z )
	PY_RW_ATTRIBUTE_DECLARE( w_, w )


	static Vector4Swizzle * New()	{ return new Vector4Swizzle; }
	PY_AUTO_FACTORY_DECLARE( Vector4Swizzle, END )

private:
	Vector4ProviderPtr	x_;
	Vector4ProviderPtr	y_;
	Vector4ProviderPtr	z_;
	Vector4ProviderPtr	w_;
};

/*~ class Math.Vector4Swizzle
 *	@components{ all }
 *	The Vector4Swizzle class takes four Vector4Providers and combines
 *	the x component of each into a single Vector4Provider.
 *
 *	A new Vector4Swizzle is created using Math.Vector4Swizzle method.
 */
PY_TYPEOBJECT( Vector4Swizzle )

PY_BEGIN_METHODS( Vector4Swizzle )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Swizzle )

	/*~ attribute Vector4Swizzle.x
	 *	@components{ all }
	 *  Provides the x component for the output, taken from the input x component.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( x )

	/*~ attribute Vector4Swizzle.y
	 *	@components{ all }
	 *  Provides the y component for the output, taken from the input x component.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( y )

	/*~ attribute Vector4Swizzle.z
	 *	@components{ all }
	 *  Provides the z component for the output, taken from the input x component.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( z )

	/*~ attribute Vector4Swizzle.w
	 *	@components{ all }
	 *  Provides the w component for the output, taken from the input x component.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( w )

PY_END_ATTRIBUTES()

/*~ function Math.Vector4Swizzle
 *	@components{ all }
 *	Creates and returns a new Vector4Swizzle, which is used to combine the x component of four Vector4Providers.
 */
PY_FACTORY( Vector4Swizzle, Math )


void Vector4Swizzle::tick( float dTime )
{
	if ( x_ )
		x_->tick( dTime );
	if ( y_ && y_!= x_ )
		y_->tick( dTime );
	if ( z_ && z_!=x_ && z_!=y_ )
		z_->tick( dTime );
	if ( w_ && w_!=z_ && w_!=y_ && w_!=x_ )
		w_->tick( dTime );
}

/**
 *	Vector4Swizzle output method
 */
void Vector4Swizzle::output( Vector4 & m )
{
	Vector4 out;

	if ( x_ )
	{
		x_->output( out );
		m.x = out.x;
	}
	if ( y_ )
	{
		y_->output( out );
		m.y = out.x;
	}
	if ( z_ )
	{
		z_->output( out );
		m.z = out.x;
	}
	if ( w_ )
	{
		w_->output( out );
		m.w = out.x;
	}
}


/**
 *	Get python attribute
 */
PyObject * Vector4Swizzle::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Provider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4Swizzle::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Provider::pySetAttribute( attr, value );
}

#undef PY_ATTR_SCOPE



// -----------------------------------------------------------------------------
// Section: Vector4Combiner
// -----------------------------------------------------------------------------
#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE
// #define PY_ATTR_SCOPE Vector4Combiner::

/**
 * TODO: to be documented.
 */
class Vector4Combiner : public Vector4Provider
{
	Py_Header( Vector4Combiner, Vector4Provider )

public:
	enum Fn
	{
		FN_MULTIPLY,
		FN_DIVIDE,
		FN_ADD,
		FN_SUBTRACT,
		FN_DOT,
		FN_MIN,
		FN_MAX,
	};

	PY_BEGIN_ENUM_MAP( Fn, FN_ )
		PY_ENUM_VALUE( FN_MULTIPLY )
		PY_ENUM_VALUE( FN_DIVIDE )
		PY_ENUM_VALUE( FN_ADD )
		PY_ENUM_VALUE( FN_SUBTRACT )
		PY_ENUM_VALUE( FN_DOT )
		PY_ENUM_VALUE( FN_MIN )
		PY_ENUM_VALUE( FN_MAX )
	PY_END_ENUM_MAP()

	Vector4Combiner( PyTypePlus * pType = &s_type_ ):
		fn_( FN_ADD ),
		a_( NULL ),
		b_( NULL )
	{}

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_DEFERRED_ATTRIBUTE_DECLARE( fn )
	PY_RW_ATTRIBUTE_DECLARE( a_, a );
	PY_RW_ATTRIBUTE_DECLARE( b_, b );

	static Vector4Combiner * New()	{ return new Vector4Combiner; }
	PY_AUTO_FACTORY_DECLARE( Vector4Combiner, END )

private:
	Fn	fn_;
	Vector4ProviderPtr	a_;
	Vector4ProviderPtr	b_;
};

// declare the enum converter functions
PY_ENUM_CONVERTERS_DECLARE( Vector4Combiner::Fn )


#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE Vector4Combiner::
PY_RW_ATTRIBUTE_DECLARE( fn_, fn )

/*~ class Math.Vector4Combiner
 *	@components{ all }
 *	Combines two Vector4Providers based on given function
 *
 *	A new Vector4Combiner is created using Math.Vector4Combiner
 *	method.
 */
PY_TYPEOBJECT( Vector4Combiner )

PY_BEGIN_METHODS( Vector4Combiner )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Combiner )

	/*~ attribute Vector4Combiner.fn
	 *	@components{ all }
	 *  Designates how to combine the a and b Vector4 inputs.
	 *	Possible values are:
	 *
	 *	"MULTIPLY"
	 *	"DIVIDE"
	 *	"ADD"
	 *	"SUBTRACT"
	 *	"DOT"
	 *	"MIN"
	 *	"MAX"
	 *
	 *	@type string
	 */
	PY_ATTRIBUTE( fn )

	/*~ attribute Vector4Combiner.a
	 *	@components{ all }
	 *  Provides the first component for the operation.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( a )

	/*~ attribute Vector4Combiner.b
	 *	@components{ all }
	 *  Provides the second component for the operation.
	 *  @type Read-Write Vector4Provider
	 */
	PY_ATTRIBUTE( b )

PY_END_ATTRIBUTES()

PY_ENUM_MAP( Vector4Combiner::Fn )
PY_ENUM_CONVERTERS_CONTIGUOUS( Vector4Combiner::Fn )

/*~ function Math.Vector4Combiner
 *	@components{ all }
 *	Creates and returns a new Vector4Combiner, which is used to
 *	combine two Vector4Providers based on a given function.
 */
PY_FACTORY( Vector4Combiner, Math )


void Vector4Combiner::tick( float dTime )
{
	if ( a_ )
		a_->tick( dTime );
	if ( b_ && b_ != a_ )
		b_->tick( dTime );
}

/**
 *	Vector4Combiner output method
 */
void Vector4Combiner::output( Vector4 & v )
{
	if ( !(a_ && b_) )
		return;

	a_->output( v );
	Vector4 b;
	b_->output( b );

	switch( fn_ )
	{
	case FN_MULTIPLY:
		v *= b;
		break;
	case FN_DIVIDE:
		v /= b;
		break;
	case FN_ADD:
		v += b;
		break;
	case FN_SUBTRACT:
		v -= b;
		break;
	case FN_DOT:
		{
			float s = v.dotProduct( b );
			v.set( s,s,s,s );
			break;
		}
	case FN_MIN:
		v.x = std::min( v.x, b.x );
		v.y = std::min( v.y, b.y );
		v.z = std::min( v.z, b.z );
		v.w = std::min( v.w, b.w );
		break;
	case FN_MAX:
		v.x = std::max( v.x, b.x );
		v.y = std::max( v.y, b.y );
		v.z = std::max( v.z, b.z );
		v.w = std::max( v.w, b.w );
		break;
	}
}


/**
 *	Get python attribute
 */
PyObject * Vector4Combiner::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Combiner::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4Combiner::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Combiner::pySetAttribute( attr, value );
}

#undef PY_ATTR_SCOPE

#define PY_ATTR_SCOPE

/**
 *	This class is a Vector4 that can be used as a register by Vector4Shader
 */
class Vector4Register : public Vector4Provider
{
	Py_Header( Vector4Register, Vector4Provider )
public:
	Vector4Register( uint8 index, Vector4 & value, PyTypePlus * pType = &s_type_ ) :
		Vector4Provider( pType ),
		index_( index ),
		value_( value )
	{
	}

	virtual void output( Vector4 & val ) { val = value_; }

	uint8 index()		{ return index_; }
	Vector4 & value()	{ return value_; }
private:
	uint8 index_;
	Vector4 & value_;
};

PY_TYPEOBJECT( Vector4Register )

PY_BEGIN_METHODS( Vector4Register )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Register )
PY_END_ATTRIBUTES()

// -----------------------------------------------------------------------------
// Section: Vector4Shader
// -----------------------------------------------------------------------------
// Some magic numbers used below
#define MAX_TEMP_REGISTERS	63
#define NUM_USER_REGISTERS	64
#define UNUSED_REGISTER		63
#define BASE_USER_REGISTER	64
#define NUM_REGISTERS		128

/**
 * TODO: to be documented.
 */
class Vector4Shader : public Vector4Provider
{
	Py_Header( Vector4Shader, Vector4Provider )

public:
	//Note - if you add opcodes, then please update the python file
	//res/common/V4ShaderHelper.py
	enum Op
	{
		//one param
		OP_MOVE = 0,
		OP_RECIPROCAL,
		OP_BIAS,
		OP_COMPLEMENT,
		//two params
		OP_MULTIPLY,
		OP_DIVIDE,
		OP_ADD,
		OP_SUBTRACT,
		OP_DOT,
		OP_MIN,
		OP_MAX,
		OP_SGE,
		OP_SLT
	};


	Vector4Shader( PyTypePlus * pType = &s_type_ )
	{}

	virtual void tick( float dTime );
	virtual void output( Vector4 & m );

	//output registers and temporary registers are implemented on the python side of things.
	//this simplifies the interface considerably.
	void		addOp( uint8 opcode, Vector4ProviderPtr outReg, Vector4ProviderPtr i1, Vector4ProviderPtr i2 = NULL );
	PY_AUTO_METHOD_DECLARE ( RETVOID, addOp, ARG(uint8,ARG(Vector4ProviderPtr,ARG(Vector4ProviderPtr,OPTARG(Vector4ProviderPtr,NULL,END)))));

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	static Vector4Shader * New()	{ return new Vector4Shader; }
	PY_AUTO_FACTORY_DECLARE( Vector4Shader, END )

	static Vector4ProviderPtr getRegister( uint8 r );
	PY_AUTO_MODULE_STATIC_METHOD_DECLARE( RETDATA, getRegister, MAX_ARG( uint8, MAX_TEMP_REGISTERS - 1, END ) )

private:
	class Instruction
	{
	public:
		uint8	op_;
		uint8	out_;	//output register. always a temp register.
		uint8	in_[2];	//input register.
	};

	typedef std::vector<Instruction> Instructions;
	Instructions	instructions_;
	std::vector<Vector4ProviderPtr> uniqueInputs_;
	static Vector4ProviderPtr pr_[MAX_TEMP_REGISTERS];
	static Vector4 r_[NUM_REGISTERS];
	//addInputVector returns the index of the register.
	uint8 addInputVector( Vector4ProviderPtr v, bool mustBeTemp = false );
};

// declare the enum converter functions
PY_ENUM_CONVERTERS_DECLARE( Vector4Shader::Op )

/*~ class Math.Vector4Shader
 *	@components{ all }
 *	The Vector4Shader implements a Vector4Provider that can be used to emulate a pixel
 *	shader implementation for 2D GUI components.
 *
 *	The following code example show how to create a flickering light, for example
 *	when a spot fire occurs for "duration" and fades out gracefully.
 *
 *	code:
 *	@{
 *	import BigWorld
 *	from Math import Vector4Shader
 *	from Math import Vector4LFO
 *	from Math import Vector4Animation
 *	from Helpers.V4ShaderConsts import _mul
 *	from Helpers.V4ShaderConsts import _r0
 *	from Helpers.V4ShaderConsts import _r1
 *
 *
 *	#and in class FlickeringLight...
 *
 *	self.light = BigWorld.PyChunkLight()
 *	self.light.innerRadius = self.innerRadius
 *	self.light.outerRadius = self.outerRadius
 *
 *	self.lightFaderAnim = Vector4Animation()
 *	self.lightFaderAnim.time = 0
 *	self.lightFaderAnim.duration = duration
 *	col = Vector4(self.colour)
 *	self.lightFaderAnim.keyframes = [(0.0,(0,0,0,0)),(0.25,col),
 *		(duration*0.5,col),(duration,(0,0,0,0))]
 *
 *	lfo1 = Vector4LFO()
 *	lfo1.period = 0.4
 *	lfo2 = Vector4LFO()
 *	lfo2.period = 0.149
 *	self.lightFader = Vector4Shader()
 *
 *	#First we multiply lfo1 by lfo2, and place the result in
 *	#the temporary register _r1
 *	self.lightFader.addOp( _mul, _r1, lfo1, lfo2 )
 *
 *	#Then we multiply the light fader animation to _r1, placing
 *	#the result in _r0 ( the final output register )
 *	self.lightFader.addOp( _mul, _r0, self.lightFaderAnim, _r1 )
 *
 *	self.light.source = source.model.root
 *	self.light.visible = 1
 *	self.light.shader = self.lightFader
 *	@}
 */
PY_TYPEOBJECT( Vector4Shader )

PY_BEGIN_METHODS( Vector4Shader )

	/*~ function Vector4Shader.addOp
	 *	@components{ all }
	 *  This method adds a single operation to the shader.  An operation may require one or
	 *	two parameters to achieve a result.  Valid operations are (opcode - description):
	 *
	 *	One parameter:
	 *	0 - MOVE
	 *	1 - RECIPROCAL
	 *	2 - BIAS
	 *	3 - COMPLEMENT
	 *
	 *	Two parameters:
	 *	4 - MULTIPLY
	 *	5 - DIVIDE
	 *	6 - ADD
	 *	6 - SUBTRACT
	 *	7 - DOT
	 *	8 - MIN
	 *	9 - MAX
	 *	10 - SGE
	 *	11 - SLT
	 *
	 *	@param	opcode	One of the above values as an uint8
	 *	@param	outreg	Temporary output register as Vector4ProviderPtr to store result
	 *	@param	param1	First parameter as Vector4ProviderPtr
	 *	@param	param2	Optional second parameter as Vector4ProviderPtr, used as required
	 */
	PY_METHOD( addOp )

PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( Vector4Shader )
PY_END_ATTRIBUTES()

/*~ function Math.Vector4Shader
 *	@components{ all }
 *	Creates and returns a new Vector4Shader, which used to
 *	perform pixel shader-like operations on a Vector4Provider.
 */
PY_FACTORY( Vector4Shader, Math )

Vector4ProviderPtr Vector4Shader::pr_[MAX_TEMP_REGISTERS];	//note - smartpointers default to NULL
Vector4 Vector4Shader::r_[NUM_REGISTERS];
static bool V4ShaderTempPtrsInited = false; //...but it's best to be explicit


/**
 *	This method ticks all the unique input vector4Providers we know about.
 *	This ensures each one is ticked only once.
 */
void Vector4Shader::tick( float dTime )
{
	std::vector<Vector4ProviderPtr>::iterator it = uniqueInputs_.begin();
	std::vector<Vector4ProviderPtr>::iterator end = uniqueInputs_.end();
	while ( it != end )
	{
		Vector4ProviderPtr v = *it++;
		v->tick(dTime);
	}
}


/**
 *	This method provides a python interface to retrieve
 *	references to temporary registers.  This is used by
 *	V4ShaderHelper at static initialisation time.
 */
Vector4ProviderPtr Vector4Shader::getRegister( uint8 r )
{
	if ( !V4ShaderTempPtrsInited )
	{
		for ( int i=0; i<MAX_TEMP_REGISTERS; i++ )
		{
			pr_[i] = Vector4ProviderPtr( new Vector4Register( i, r_[i] ), true );
		}
		V4ShaderTempPtrsInited = true;
	}

	MF_ASSERT( r < MAX_TEMP_REGISTERS );
	return pr_[r];
}

/*~	function Math.getRegister
 *	@components{ all }
 *
 *	This method provides a python interface to retrieve references to temporary registers.  The value is
 *	returned as a Vector4ProviderPtr, so the register can also be updated by changing the returned value.
 *
 *	@param	register	The temporary register to access as an int (eg, 1-16)
 *
 *	@return	A Vector4ProviderPtr containing the contents of the given register
 */
PY_MODULE_STATIC_METHOD( Vector4Shader, getRegister, Math )


/**
 *	This method is private.  It converts a vector4Provider into its register number.
 *	It checks if the incoming v4p is one of the temporary registers, or a new input
 *	register.
 */
uint8 Vector4Shader::addInputVector( Vector4ProviderPtr v, bool mustBeTemp )
{
	//no pointer - no register.
	if ( !v.getObject() )
		return UNUSED_REGISTER;

	//check if the V4Provider is one of our temp registers.
	//if so, return a value from 0 to MAX_TEMP_REGISTERS
	if (Vector4Register::Check( v.getObject() ))
	{
		//all the temp registers point to Vector4Basics.
		Vector4Register * v4r = static_cast<Vector4Register*>(v.getObject());
		return v4r->index();
	}

	if ( mustBeTemp )
	{
		ERROR_MSG( "Vector4Shader::addInputVector - register that must be temporary was not\n" );
		return UNUSED_REGISTER;
	}

	//only insert a new unique input if necessary.
	//return the index of the user register, from 1 .. n
	std::vector<Vector4ProviderPtr>::iterator it = uniqueInputs_.begin();
	std::vector<Vector4ProviderPtr>::iterator end = uniqueInputs_.end();
	std::vector<Vector4ProviderPtr>::iterator found = std::find( it, end, v );
	if ( found == end )
	{
		if (uniqueInputs_.size() == NUM_USER_REGISTERS)
		{
			ERROR_MSG( "Vector4Shader::addInputVector - shader has exceeded \
					   the maximum number of user registers\n" );
			return UNUSED_REGISTER;
		}
		uniqueInputs_.push_back(v);
		return uniqueInputs_.size() -1 + BASE_USER_REGISTER;
	}
	return ( found - uniqueInputs_.begin() + BASE_USER_REGISTER );
}


/**
 *	This method adds an opcode to the shader.  The second and third register params are optional.
 */
void Vector4Shader::addOp( uint8 opcode, Vector4ProviderPtr outReg, Vector4ProviderPtr reg1, Vector4ProviderPtr reg2 )
{
	Instruction ins;
	ins.op_ = opcode;

	ins.out_ = addInputVector( outReg, true );
	ins.in_[0] = addInputVector( reg1 );
	ins.in_[1] = addInputVector( reg2 );

	instructions_.push_back( ins );
}

/**
 *	Vector4Shader output method
 */
void Vector4Shader::output( Vector4 & finalOutput )
{
	float s;	//temporary scalar value

	//Calculate each of the used input vector4providers once only.
	std::vector<Vector4ProviderPtr>::iterator uit = uniqueInputs_.begin();
	std::vector<Vector4ProviderPtr>::iterator uend = uniqueInputs_.end();
	int idx = BASE_USER_REGISTER;
	while ( uit != uend )
	{
		Vector4ProviderPtr& v = *uit++;
		v->output(r_[idx++]);
	}

	Instructions::iterator it = instructions_.begin();
	Instructions::iterator end = instructions_.end();

	while ( it != end )
	{
		Instruction& i = *it++;

		//get pointers to the vector4s we are using.
		Vector4& out = r_[i.out_];
		Vector4& i0 = r_[i.in_[0]];
		Vector4& i1 = r_[i.in_[1]];

		switch( i.op_ )
		{
		case OP_MOVE:
			out = i0;
			break;
		case OP_RECIPROCAL:
			out.x = 1.f / i0.x;
			out.y = 1.f / i0.y;
			out.z = 1.f / i0.z;
			out.w = 1.f / i0.w;
			break;
		case OP_BIAS:
			out.x = i0.x - 0.5f;
			out.y = i0.y - 0.5f;
			out.z = i0.z - 0.5f;
			out.w = i0.w - 0.5f;
			break;
		case OP_COMPLEMENT:
			out.x = 1.f - i0.x;
			out.y = 1.f - i0.y;
			out.z = 1.f - i0.z;
			out.w = 1.f - i0.w;
			break;
		case OP_MULTIPLY:
			out = i0 * i1;
			break;
		case OP_DIVIDE:
			out = i0 / i1;
			break;
		case OP_ADD:
			out = i0 + i1;
			break;
		case OP_SUBTRACT:
			out = i0 - i1;
			break;
		case OP_DOT:
			s = i0.dotProduct(i1);
			out.set( s,s,s,s );
			break;
		case OP_MIN:
			out.x = std::min( i0.x, i1.x );
			out.y = std::min( i0.y, i1.y );
			out.z = std::min( i0.z, i1.z );
			out.w = std::min( i0.w, i1.w );
			break;
		case OP_MAX:
			out.x = std::max( i0.x, i1.x );
			out.y = std::max( i0.y, i1.y );
			out.z = std::max( i0.z, i1.z );
			out.w = std::max( i0.w, i1.w );
			break;
		case OP_SGE:
			out.x = (i0.x >= i1.x ? 1.f : 0.f);
			out.y = (i0.x >= i1.y ? 1.f : 0.f);
			out.z = (i0.x >= i1.z ? 1.f : 0.f);
			out.w = (i0.x >= i1.w ? 1.f : 0.f);
			break;
		case OP_SLT:
			out.x = (i0.x < i1.x ? 1.f : 0.f);
			out.y = (i0.x < i1.y ? 1.f : 0.f);
			out.z = (i0.x < i1.z ? 1.f : 0.f);
			out.w = (i0.x < i1.w ? 1.f : 0.f);
			break;
		}
	}

	finalOutput = r_[0];
}


/**
 *	Get python attribute
 */
PyObject * Vector4Shader::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return Vector4Provider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int Vector4Shader::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return Vector4Provider::pySetAttribute( attr, value );
}

#undef PY_ATTR_SCOPE

// script_math.cpp
