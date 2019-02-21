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
 *
 *	@ingroup Math
 */

#ifndef QUAT_HPP
#define QUAT_HPP

#include "mathdef.hpp"
#include "matrix.hpp"

/**
 *	This class is used to represent a quaternion. Quaternions are useful when
 *	working with angles in 3 dimensions.
 *
 *	@ingroup Math
 */
class Quaternion : public QuaternionBase
{
public:
	/// @name Construction/Destruction
	//@{
	Quaternion();
	Quaternion( const Matrix &m );
	Quaternion( float x, float y, float z, float w );
	Quaternion( const Vector3 &v, float w );
	//@}

	void			setZero();
	void			set( float x, float y, float z, float w );
	void			set( const Vector3 &v, float w );

	void			fromAngleAxis( float angle, const Vector3 &axis );

	void			fromMatrix( const Matrix &m );

	void			normalise();
	void			invert();
	void			minimise();

	void			slerp( const Quaternion& qStart, const Quaternion &qEnd,
							float t );

	void			multiply( const Quaternion& q1, const Quaternion& q2 );
	void			preMultiply( const Quaternion& q );
	void			postMultiply( const Quaternion& q );

	float			dotProduct( const Quaternion& q ) const;
	float			length() const;
	float			lengthSquared() const;
};

Quaternion  operator *( const Quaternion& q1, const Quaternion& q2 );
bool		operator ==( const Quaternion& q1, const Quaternion& q2 );

#ifdef CODE_INLINE
#include "quat.ipp"
#endif




#endif // QUAT_HPP
/*quat.hpp*/
