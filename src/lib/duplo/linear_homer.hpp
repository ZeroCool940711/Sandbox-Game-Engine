/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef LinearHomer_HPP
#define LinearHomer_HPP

#include "motor.hpp"
#include "pyscript/script_math.hpp"
#include "cstdmf/aligned.hpp"

/**
 *	This class is a motor for a model that sets the position
 *	of the model based on a signal.
 *
 *	This class actually sets the transform of a model to the
 *	given MatrixProvider.
 */
class LinearHomer : public Motor, public Aligned
{
	Py_Header( LinearHomer, Motor )

public:
	LinearHomer( PyTypePlus * pType = &LinearHomer::s_type_ );
	~LinearHomer();

	PY_FACTORY_DECLARE()
	PY_RW_ATTRIBUTE_DECLARE( target_, target )
	PY_RW_ATTRIBUTE_DECLARE( acceleration_, acceleration )
	PY_RW_ATTRIBUTE_DECLARE( velocity_, velocity )
	PY_RW_ATTRIBUTE_DECLARE( proximityCallback_, proximityCallback )
	PY_RW_ATTRIBUTE_DECLARE( proximity_, proximity )
	PY_RW_ATTRIBUTE_DECLARE( tripTime_, tripTime )
	PY_RW_ATTRIBUTE_DECLARE( align_, align )
	PY_RW_ATTRIBUTE_DECLARE( up_, up )
	PY_RW_ATTRIBUTE_DECLARE( revDelay_, revDelay )
	PY_RW_ATTRIBUTE_DECLARE( offsetProvider_, offsetProvider )
	PY_RW_ATTRIBUTE_DECLARE( pitchRollBlendInTime_, pitchRollBlendInTime )
	PY_RW_ATTRIBUTE_DECLARE( blendOutTime_, blendOutTime )


	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

	virtual void rev( float dTime );
private:
	MatrixProviderPtr	target_;
	Vector3				velocity_;
	float				acceleration_;
	float				proximity_;
	PyObject*			proximityCallback_;
	float				tripTime_;
	Vector3				up_;
	MatrixProviderPtr	align_;
	int					revDelay_;
	MatrixProviderPtr	offsetProvider_;
	float				pitchRollBlendInTime_;
	float				pitchRollBlendIn_;
	bool				updatePitchRollRef_;
	Matrix				pitchRollRef_;
	float				blendOutTime_;
	float				blendOut_;

	void makePCBHappen();
};

#endif