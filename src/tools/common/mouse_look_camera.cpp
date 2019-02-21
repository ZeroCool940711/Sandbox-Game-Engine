/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

//#include "cstdmf/pch.hpp"
#include "pch.hpp"

#include "mouse_look_camera.hpp"
#include "resmgr/string_provider.hpp"

#ifndef CODE_INLINE
#include "mouse_look_camera.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "Editor", 0 )



/**
 *	Constructor.
 */
MouseLookCamera::MouseLookCamera()
	: pitch_( 0.f ),
	  yaw_( 0.f ),
	  isCursorHidden_( false ),
	  lastPos_( 0.f, 0.f, 0.f ),
	  lastYaw_( 0.f ),
	  lastPitch_( 0.f ),
      limit_( Vector3( -10000.f, -10000.f, -10000.f ),
              Vector3( 10000.f, 10000.f, 10000.f ) )
{
	lastCursorPosition_.x = -1;
	lastCursorPosition_.y = -1;

	keyDown_.insert( std::make_pair( KeyEvent::KEY_W, false ) );
	keyDown_.insert( std::make_pair( KeyEvent::KEY_S, false ) );
	keyDown_.insert( std::make_pair( KeyEvent::KEY_A, false ) );
	keyDown_.insert( std::make_pair( KeyEvent::KEY_D, false ) );
	keyDown_.insert( std::make_pair( KeyEvent::KEY_Q, false ) );
	keyDown_.insert( std::make_pair( KeyEvent::KEY_E, false ) );
}


/**
 *	Destructor
 */
MouseLookCamera::~MouseLookCamera()
{
}


/**
 *	This method sets the view matrix for the mouse look camera.
 *	The polar coordinates are calculated thereafter.
 *
 *	@param v	The new view matrix for the camera.
 */
void MouseLookCamera::view( const Matrix & v )
{
	view_ = v;
	viewToPolar();
}


/**
 * This method updates the camera.
 *
 * If keys were not handled by the camera, then we reset our state.
 * Because the mouse look camera tests the keyboard states directly,
 * (which it must do because it interprets continuously held down keys ),
 * it must handle the case where the focus has been lost to the camera.
 *
 * @param dTime the change in time since the last frame
 */
void MouseLookCamera::update( float dTime, bool activeInputHandler )
{
	if ( activeInputHandler )
		handleInput( dTime );

	if ( !InputDevices::hasFocus() )
	{
		KeyDownMap::iterator it = keyDown_.begin();
		KeyDownMap::iterator end = keyDown_.end();
		while ( it != end )
		{
			(it++)->second = false;
		}
	}

    polarToView();

	if ( InputDevices::hasFocus() )
	{
		if (isCursorHidden_ && !InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ))
		{
			isCursorHidden_ = false;
			while (::ShowCursor( TRUE ) < 0) {}
		}
		else if (!isCursorHidden_ && InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ))
		{
			isCursorHidden_ = true;
			while (::ShowCursor( FALSE ) > -1) {}
		}
	}
	else if (isCursorHidden_)
	{
		isCursorHidden_ = false;
		while (::ShowCursor( TRUE ) < 0) {}
	}
}


/**
 * This method handles key events.  Here, we update the keyDown_ array,
 * indicating that we got a key down, and therefore should process repeats.
 */
bool MouseLookCamera::handleKeyEvent( const KeyEvent& event )
{
	// only move if control is not depressed
	if (event.isCtrlDown())
		return false;

	//update the key down map.
	bool handled = false;

	KeyDownMap::iterator found = keyDown_.find( event.key() );

	if ( found != keyDown_.end() )
	{
		if ( event.isKeyDown() )
		{
			found->second = true;
			handled = true;
		}
		else
		{
			if ( found->second )
			{
				found->second = false;
				handled = true;
			}
		}
	}

	return handled;
}


/**
 * This method handles mouse events.  Here, we update the pitch and yaw
 * information.
 */
bool MouseLookCamera::handleMouseEvent( const MouseEvent & event )
{
	bool handled = false;

    if ( InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ) )
    {
		if ( event.dy() != 0 )
		{
        	if ( invert_ )
				pitch_ += ( event.dy() * -0.01f );
            else
            	pitch_ += ( event.dy() * 0.01f );
			handled = true;
		}

		if ( event.dx() != 0 )
		{
			yaw_ += ( event.dx() * 0.01f );
			handled = true;
		}
    }

	return handled;
}


/**
 *	This method reads the keyboard and moves the camera around.
 *
 *	@param dTime	The change in time since the last frame.
 */
void MouseLookCamera::handleInput( float dTime )
{
	Matrix oldView = view_;
	bool viewChanged = false;
	view_.invert();

	Vector3 forward = view_.applyToUnitAxisVector( 2 );
	Vector3 up = view_.applyToUnitAxisVector( 1 );
	Vector3 right = view_.applyToUnitAxisVector( 0 );

	float movementSpeed;

	//if ( !InputDevices::isKeyDown( KeyEvent::KEY_CAPSLOCK ) )
	if ( (::GetKeyState( VK_CAPITAL ) & 0x0001) == 0 )
		movementSpeed = speed();
	else
		movementSpeed = turboSpeed();

	//frame rate independent speed, but capped
	if ( dTime < 0.1f )
		movementSpeed *= dTime;
	else
		movementSpeed *= 0.1f;


	if ( InputDevices::isKeyDown( KeyEvent::KEY_W ) )
	{
		if ( keyDown_[KeyEvent::KEY_W] )
		{
			//move forward
			view_._41 += forward.x * movementSpeed;
			view_._42 += forward.y * movementSpeed;
			view_._43 += forward.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_W] = false;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_S ) )
	{
		if ( keyDown_[KeyEvent::KEY_S] )
		{
			//move back
			view_._41 -= forward.x * movementSpeed;
			view_._42 -= forward.y * movementSpeed;
			view_._43 -= forward.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_S] = false;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_A ) )
	{
		if ( keyDown_[KeyEvent::KEY_A] )
		{
			//move left
			view_._41 -= right.x * movementSpeed;
			view_._42 -= right.y * movementSpeed;
			view_._43 -= right.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_A] = false;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_D ) )
	{
		if ( keyDown_[KeyEvent::KEY_D] )
		{
			//move right
			view_._41 += right.x * movementSpeed;
			view_._42 += right.y * movementSpeed;
			view_._43 += right.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_D] = false;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_E ) )
	{
		if ( keyDown_[KeyEvent::KEY_E] )
		{
			//move up
			view_._41 += up.x * movementSpeed;
			view_._42 += up.y * movementSpeed;
			view_._43 += up.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_E] = false;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_Q ) )
	{
		if ( keyDown_[KeyEvent::KEY_Q] )
		{
			//move down
			view_._41 -= up.x * movementSpeed;
			view_._42 -= up.y * movementSpeed;
			view_._43 -= up.z * movementSpeed;
			viewChanged = true;
		}
	}
	else
	{
		keyDown_[KeyEvent::KEY_Q] = false;
	}

	if (viewChanged)
	{
		// Cap the camera position
		view_._41 = Math::clamp( limit_.minBounds().x, view_._41, limit_.maxBounds().x );
		view_._42 = Math::clamp( limit_.minBounds().y, view_._42, limit_.maxBounds().y );
		view_._43 = Math::clamp( limit_.minBounds().z, view_._43, limit_.maxBounds().z );
	}

	if ( InputDevices::hasFocus() )
		if ( InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ) )
		{
			// Keep cursor's click position
			if ( lastCursorPosition_.x == -1 && lastCursorPosition_.y == -1 )
				::GetCursorPos( &lastCursorPosition_ );
			::SetCursorPos( lastCursorPosition_.x, lastCursorPosition_.y );
		}
		else
		{
			if ( lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1 )
			{
				lastCursorPosition_.x = -1;
				lastCursorPosition_.y = -1;
			}
		}
	else
		if ( lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1 )
		{
			lastCursorPosition_.x = -1;
			lastCursorPosition_.y = -1;
		}

	if (viewChanged)
	{
		view_.invert();
	}
	else
	{
		// Nothing changed, restore old view.
		view_ = oldView;
	}
}


/**
 *	This method calculates the pitch and yaw from the inverse view matrix.
 */
void MouseLookCamera::viewToPolar()
{
	Matrix invView( view_ );
    invView.invert();
	Vector3 * dir = (Vector3*)&invView.applyToUnitAxisVector(2);

	pitch_ = -atan2f( dir->y,	sqrtf( dir->z * dir->z + dir->x * dir->x ) );
	yaw_ = atan2f( dir->x, dir->z );
}


/**
 *	This method calculates the view matrix from our pitch and yaw,
 *	and the current camera position
 */
void MouseLookCamera::polarToView()
{
	Matrix oldView = view_;
	//calculate the view matrix
	view_.invert();

	Vector3 pos( view_.applyToOrigin() );

	if (!almostEqual( pos.x, lastPos_.x ) || !almostEqual( pos.y, lastPos_.y ) || !almostEqual( pos.z, lastPos_.z ) ||
		!almostEqual( yaw_, lastYaw_ ) || !almostEqual( pitch_, lastPitch_ ))
	{
		lastPos_ = pos;
		lastYaw_ = yaw_;
		lastPitch_ = pitch_;

		view_.setIdentity();

		Matrix rot;
		XPMatrixRotationYawPitchRoll( &rot, yaw_, pitch_, 0.f );

		view_.setTranslate( pos );
		view_.preMultiply( rot );

		view_.invert();
	}
	else
	{
		view_ = oldView;
	}
}


/**
 *	This method copies view_, pitch_ and yaw_ from another camera
 */
void MouseLookCamera::view( const MouseLookCamera& other )
{
	view_ = other.view_;
	pitch_ = other.pitch_;
	yaw_ = other.yaw_;
}


/**
 *	This function is the output stream operator for MouseLookCamera.
 */
std::ostream& operator<<(std::ostream& o, const MouseLookCamera& t)
{
	o << L("COMMON/MOUSE_LOOK_CAMERA/OUTPUT");
	return o;
}

// mouse_look_camera.cpp
