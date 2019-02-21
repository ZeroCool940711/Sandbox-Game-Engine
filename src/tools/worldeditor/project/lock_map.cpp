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
#include "worldeditor/project/lock_map.hpp"
#include "worldeditor/project/world_editord_connection.hpp"
#include "moo/render_context.hpp"
#include "math/colour.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )


inline std::string errorAsString( HRESULT hr )
{
	char s[1024];
	//TODO : find out the DX9 way of getting the error string
	//D3DXGetErrorString( hr, s, 1000 );
	bw_snprintf( s, sizeof(s), "Error %lx\0", hr );
	return std::string( s );
}

const Vector4 COLOUR_LOCKED( 64, 64, 255, 128 );
const Vector4 COLOUR_LOCKED_EDGE( 32, 32, 255, 128 );
const Vector4 COLOUR_LOCKED_BY_OTHER( 255, 2, 2, 128 );
const Vector4 COLOUR_UNLOCKED( 0, 0, 0, 0 );

LockMap::LockMap():
	gridWidth_(0),
	gridHeight_(0),
	colourLocked_( Colour::getUint32( COLOUR_LOCKED ) ),
	colourLockedEdge_( Colour::getUint32( COLOUR_LOCKED_EDGE ) ),
	colourLockedByOther_( Colour::getUint32( COLOUR_LOCKED_BY_OTHER ) ),
	colourUnlocked_( Colour::getUint32( COLOUR_UNLOCKED ) )
{
}


LockMap::~LockMap()
{
	releaseLockTexture();
}


bool LockMap::init( DataSectionPtr pSection )
{
	if ( pSection )
	{
		colourLocked_ = Colour::getUint32(
			pSection->readVector4("colourLocked", COLOUR_LOCKED));
		colourLockedEdge_ = Colour::getUint32(
			pSection->readVector4("colourLockedEdge", COLOUR_LOCKED_EDGE));
		colourLockedByOther_ = Colour::getUint32(
			pSection->readVector4("colourLockedByOther", COLOUR_LOCKED_BY_OTHER));
		colourUnlocked_ = Colour::getUint32(
			pSection->readVector4("colourUnlocked", COLOUR_UNLOCKED));
	}
	return true;
}


void LockMap::setTexture( uint8 textureStage )
{
	Moo::rc().setTexture( textureStage, lockTexture_.pComObject() );
}


void LockMap::gridSize( uint32 width, uint32 height )
{
	if ( width==gridWidth_ && height==gridHeight_ )
		return;

	releaseLockTexture();

	gridWidth_ = width;
	gridHeight_ = height;

	createLockTexture();
}


void LockMap::updateLockData( uint32 width, uint32 height, uint8* lockData )
{
	//Do we need to recreate the lock texture?
	bool recreate = !lockTexture_.pComObject();
	recreate |= ( gridWidth_ != width );
	recreate |= ( gridHeight_ != height );

	if (recreate)
	{
		this->gridSize( width, height );
		if ( !lockTexture_.pComObject() )
			return;
	}

	//Update the data therein.
	D3DLOCKED_RECT lockedRect;
	HRESULT hr = lockTexture_->LockRect( 0, &lockedRect, NULL, 0 );
	if (FAILED(hr))
	{
		ERROR_MSG( "LockMap::updateLockData - unable to lock texture  D3D error %s\n", errorAsString(hr).c_str() );
		return;
	}
	
	uint32* pixels = (uint32*) lockedRect.pBits;
	for (uint32 h = 0; h < height; h++)
	{
		for (uint32 w = 0; w < width; w++)
		{
			int pix = h * width + w;
			if (lockData[pix] == BWLock::GS_WRITABLE_BY_ME)
				// Lock by the current user
				pixels[pix] = colourLockedEdge_;
			else if (lockData[pix] == BWLock::GS_LOCKED_BY_ME)
				// Writable by the current user
				pixels[pix] = colourLocked_;
			else if (lockData[pix] == BWLock::GS_NOT_LOCKED)
				// Not locked
				pixels[pix] = colourUnlocked_;
			else
				// Locked by someone else
				pixels[pix] = colourLockedByOther_;
		}
	}	

	hr = lockTexture_->UnlockRect( 0 );
	if (FAILED(hr))
	{
		ERROR_MSG( "LockMap::updateLockData - unable to unlock texture.  D3D error %s\n", errorAsString(hr).c_str() );
	}
}


void LockMap::releaseLockTexture()
{
	lockTexture_ = NULL;
}


void LockMap::createLockTexture()
{
	if ( gridWidth_==0 || gridHeight_==0 )
	{
		ERROR_MSG( "LockMap: Invalid dimensions: %d x %d\n", gridWidth_, gridHeight_ );
		return;
	}

	lockTexture_ = NULL; // Release any lock texture

	lockTexture_ = Moo::rc().createTexture( gridWidth_, gridHeight_ , 1,
		0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, "texture/lock terrain" );
}
