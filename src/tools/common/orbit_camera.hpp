/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef ORBIT_CAMERA_HPP
#define ORBIT_CAMERA_HPP

#include <iostream>
#include "base_camera.hpp"

/**
 * This class implements an orbit camera that
 * always looks at a given location.
 * In interactive mode, the mouse swings the camera
 * around the location.
 * In animation mode, the mouse automatically pans
 * around the location.
 */
class OrbitCamera : public BaseCamera
{
public:
	OrbitCamera();
	~OrbitCamera();

	void origin( const Vector3& val )
	{
		origin_ = val;
		viewToPolar();
	}

	const Vector3& origin() const
	{
		return origin_;
	}

	void interactive( bool state )
	{
		interactive_ = state;
	}

    void view( const Matrix & );
	const Matrix & view() const;

	void update( float dTime, bool activeInputHandler = true );
	bool handleKeyEvent( const KeyEvent& event );
	bool handleMouseEvent( const MouseEvent & event );

	void rotDir( int dir ) { rotDir_ = dir; }
	int rotDir() const { return rotDir_; }

	void orbitSpeed( float speed ) { orbitSpeed_ = speed; }
	float orbitSpeed() { return orbitSpeed_; }

private:
	void handleInput( float dTime );
    void viewToPolar();
	void polarToView();

	void adjustPitch( float dPitch );
	void constrainPitch();

	OrbitCamera(const OrbitCamera&);
	OrbitCamera& operator=(const OrbitCamera&);

	Vector3	origin_;
	bool	interactive_;
    int		rotDir_;

	//polar coordinates
    float	pitch_;
    float	yaw_;
	float	radius_;

	//Stuff to handle key events correctly
	typedef std::map< KeyEvent::Key, bool>	KeyDownMap;
	KeyDownMap	keyDown_;

	// For hiding and showing the mouse cursor
	bool isCursorHidden_;

	/**
	 * Where the cursor was when we started looking around,
	 * so we can restore it to here when done
	 */
	POINT lastCursorPosition_;

	float orbitSpeed_;

	friend std::ostream& operator<<(std::ostream&, const OrbitCamera&);
};

#ifdef CODE_INLINE
#include "mouse_look_camera.ipp"
#endif

typedef SmartPointer<OrbitCamera> OrbitCameraPtr;

#endif
/*mouse_look_camera.hpp*/
