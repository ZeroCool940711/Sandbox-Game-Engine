/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef LOCK_MAP_HPP
#define LOCK_MAP_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "moo/com_object_wrap.hpp"
#include "moo/moo_dx.hpp"
#include "resmgr/datasection.hpp"


/**
 *	This class manages and displays a view of the currently
 *	locked areas of the whole space.
 *
 *	User locked areas are displayed in blue, areas locked by
 *	other users are shown in red.
 */
class LockMap
{
public:
	LockMap();
	~LockMap();

	void gridSize( uint32 width, uint32 height );
	void setTexture( uint8 textureStage );
	bool init( DataSectionPtr pSection );
	void updateLockData( uint32 width, uint32 height, uint8* lockData );
    ComObjectWrap<DX::Texture> lockTexture() { return lockTexture_; }
private:
	void releaseLockTexture();
	void createLockTexture();
	
	ComObjectWrap<DX::Texture>	lockTexture_;
	uint32						gridWidth_;
	uint32						gridHeight_;

	uint32						colourLocked_;
	uint32						colourLockedEdge_;
	uint32						colourLockedByOther_;
	uint32						colourUnlocked_;
};


#endif // LOCK_MAP_HPP
