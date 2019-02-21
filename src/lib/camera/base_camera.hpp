/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef BASE_CAMERA_HPP
#define BASE_CAMERA_HPP

#include "math/matrix.hpp"

#include "input/input.hpp"
#include "input/event_converters.hpp"

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"

#include "network/basictypes.hpp"


/*~ class BigWorld.BaseCamera
 *
 *	This class is the abstract base class for all cameras.  It supplies basic
 *  functionality for setting a camera to a particular matrix, for updating and
 *  rendering the camera, and for reading the camera's position and direction.
 *
 *	There are three subclasses, CursorCamera, FlexiCam and FreeCamera.
 */
/**
 *	This class is the base pythonised camera from which all others
 *	are derived. When a camera is active, its handleKeyEvent and
 *	handleMouseEvent methods will get called according to the camera's
 *	place in the event distribution chain. Cameras should not poll
 *	for key down events in their update methods, or they may get some
 *	not intended for them.
 */
class BaseCamera : public PyObjectPlus, public InputHandler, public Aligned
{
	Py_Header( BaseCamera, PyObjectPlus )

public:
	BaseCamera( PyTypePlus * pType );
	~BaseCamera();

	virtual bool handleKeyEvent( const KeyEvent & event );
	virtual bool handleMouseEvent( const MouseEvent & event );

	void set( ConstSmartPointer<MatrixProvider> pMP );
	virtual void set( const Matrix & viewMatrix ) = 0;
	virtual void update( float dTime ) = 0;	///< updates view & invView
	void render();

	const Matrix & view() const;
	const Matrix & invView() const;

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_AUTO_METHOD_DECLARE( RETVOID, set, NZARG( MatrixProviderPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, update, ARG( float, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, render, END )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleKeyEvent, ARG( KeyEvent, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, handleMouseEvent, ARG( MouseEvent, END ) )

	PY_RO_ATTRIBUTE_DECLARE( invView_.applyToOrigin(), position )
	PY_RO_ATTRIBUTE_DECLARE( invView_.applyToUnitAxisVector(2), direction )

	MatrixProviderPtr viewMatrixProvider() const;
	PY_RO_ATTRIBUTE_DECLARE( this->viewMatrixProvider(), matrix );

	MatrixProviderPtr invViewMatrixProvider() const;
	PY_RO_ATTRIBUTE_DECLARE( this->invViewMatrixProvider(), invViewMatrix );
	PY_RW_ATTRIBUTE_DECLARE( spaceID_, spaceID )

	// This is used by NearPlaneClippingChecker.
	static bool checkCameraTooClose()
						{ return checkCameraTooClose_; }
	static void checkCameraTooClose( bool value )
						{ checkCameraTooClose_ = value; }

	SpaceID	spaceID() const;
	void spaceID( SpaceID spaceID );

	static bool sceneCheck(
		Vector3 & cameraPosInWS,
		const Vector3 & cameraLookAtInWS,
		const Vector3 & direction,
		const Vector3 & uprightDirection );

protected:
	Matrix	view_;
	Matrix	invView_;

	///	@name Protected Helper Methods.
	//@{

	//@}

private:
	BaseCamera(const BaseCamera&);
	BaseCamera& operator=(const BaseCamera&);

	mutable MatrixProviderPtr viewMatrixProvider_;
	mutable MatrixProviderPtr invViewMatrixProvider_;

	SpaceID	spaceID_;

	static bool checkCameraTooClose_;
};

typedef SmartPointer<BaseCamera> BaseCameraPtr;

PY_SCRIPT_CONVERTERS_DECLARE( BaseCamera )



#ifdef CODE_INLINE
#include "base_camera.ipp"
#endif


#endif // BASE_CAMERA_HPP
