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

#include "moo/moo_math.hpp"
#include "input/input.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"

class BaseCamera : public PyObjectPlus, public Aligned
{
	Py_Header( BaseCamera, PyObjectPlus )

public:
	BaseCamera( PyTypePlus * pType = &s_type_ );
	virtual ~BaseCamera();

	//This method must calculate the view matrix
	virtual void update( float dTime, bool activeInputHandler = true ) = 0;
	virtual void render( float dTime );
	virtual bool handleKeyEvent( const KeyEvent & );
	virtual bool handleMouseEvent( const MouseEvent & );

    //common tool camera properties
    float	speed() const;
    void	speed( float s );

	float	turboSpeed() const;
    void	turboSpeed( float s );

    bool	invert() const;
    void	invert( bool state );


	virtual const Matrix & view() const;
	virtual void view( const Matrix & );

	virtual Vector3 position() const;
	virtual void position( const Vector3 & );

    void windowHandle( const HWND handle );

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_METHOD_DECLARE( py_update )
	PY_METHOD_DECLARE( py_render )
	PY_METHOD_DECLARE( py_handleKeyEvent )
	PY_METHOD_DECLARE( py_handleMouseEvent )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, speed, speed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, turboSpeed, turboSpeed )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, invert, invert )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Matrix, view, view )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector3, position, position )

protected:
	Matrix	view_;
    float	speed_[2];
    bool	invert_;

    HWND	windowHandle_;

private:
	BaseCamera(const BaseCamera&);
	BaseCamera& operator=(const BaseCamera&);
};

#ifdef CODE_INLINE
#include "base_camera.ipp"
#endif


typedef SmartPointer<BaseCamera> BaseCameraPtr;


#endif // BASE_CAMERA_HPP
