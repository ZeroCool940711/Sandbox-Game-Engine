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
#include "worldeditor/project/space_map_timestamp_cache.hpp"
#include "worldeditor/project/space_helpers.hpp"
#include "cstdmf/debug.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )


/**
 *	Constructor.
 */
SpaceMapTimestampCache::SpaceMapTimestampCache():
	pLastModified_( NULL ),
	cacheName_( "" )	
{
}


/**
 *	This method lets us know information about the space we are now
 *	looking at.  In particular we can work out the cache name and the
 *	expected size of the timsestamp cache file.
 *
 *	@param	info	SpaceInformation structure.
 */
void SpaceMapTimestampCache::spaceInformation( const SpaceInformation& info )
{	
	info_ = info;
	cacheName_ = info_.spaceName_ + "/space.thumbnail.timestamps";

	load();
}


/**
 *	This method loads the cache off disk.
 */
void SpaceMapTimestampCache::load()
{
	if ( cacheName_ == "" )
		return;	

	pLastModified_ = BWResource::instance().rootSection()->readBinary( cacheName_ );
	size_t bytesRequired = (1+info_.gridWidth_*info_.gridHeight_)*sizeof(uint64);

	/* If the file exists, we must check that it is valid and is big enough
     * to contain all the data for the tiles in the space.
	 * TODO: we probably should do this for other calls to readBinary
	*/

	if (pLastModified_)
	{
		if (pLastModified_->len() != bytesRequired) 
		{
			ERROR_MSG( "ERROR: Corrupt Space Map Timestamp Cache File:%s"
			" will be deleted\n", cacheName_.c_str());
			if (::DeleteFile( BWResource::resolveFilename( cacheName_).c_str() ) == 0) 
			{
				ERROR_MSG("ERROR: unable to delete file:%s\n", cacheName_.c_str());
			} 
			else
			{
				BWResource::instance().purge( cacheName_ );
			}
			pLastModified_ = NULL;	
		}		
	}
	if (!pLastModified_)
	{
		pLastModified_ = 
			new BinaryBlock
			( 
				NULL, 
				bytesRequired,
				"BinaryBlock/SpaceMapTimestampCache"
			);
		//make sure the first bit of data does not contain ascii "<" and thus
		//be mistaken by the filesystem for text xml.
		uint32* pData = (uint32*)pLastModified_->data();
		pData[0] = 0xffffffff;
		pData[1] = 0xffffffff;
	}
}


/**
 *	This method saves the cache to disk.
 */
void SpaceMapTimestampCache::save()
{
	if ( pLastModified_ )
	{
		BinSectionPtr pSection = new BinSection( cacheName_, pLastModified_ );
		pSection->save( cacheName_ );		
	}
}


/**
 *	This method saves a temporary copy of the cache to disk.  This
 *	is needed when the space map is swapped out of usage, but the
 *	user hasn't pressed save yet and so we don't want to overwrite
 *	the actual on-disk cache.
 */
void SpaceMapTimestampCache::saveTemporaryCopy()
{
	cacheName_ = info_.spaceName_ + "/space.thumbnail.temp_timestamps";
	this->save();
	cacheName_ = info_.spaceName_ + "/space.thumbnail.timestamps";
}


/**
 *	This method loads our temporary copy of the cache from disk.  This
 *	is needed when the space map is swapped back into usage but before
 *	the user has pressed save.
 */
void SpaceMapTimestampCache::loadTemporaryCopy()
{
	cacheName_ = info_.spaceName_ + "/space.thumbnail.temp_timestamps";
	this->load();
	::DeleteFile( BWResource::resolveFilename( cacheName_ ).c_str() );
	cacheName_ = info_.spaceName_ + "/space.thumbnail.timestamps";
}


/**
 *	This method returns the date of the tile in the cache.
 *
 *	@param	tileNum		1-dimensional index of the tile to lookup
 *	@return	uint64		uint64 representation of the cache tile time.
 */
uint64 SpaceMapTimestampCache::cacheModifyTime( uint32 tileNum ) const
{
	MF_ASSERT( pLastModified_ )
	uint64* pData = (uint64*)(pLastModified_->data());
	return pData[tileNum+1];
}


/**
 *	This method sets the timestamp in the cache for a particular tile to now.
 *
 *	@param	gridX		x position of the tile ( in chunk coordinates )
 *	@param	gridZ		z position of the tile ( in chunk coordinates )
 */
void SpaceMapTimestampCache::touch( int16 gridX, int16 gridZ )
{	
	uint64 modTime = this->now();
	this->cacheModifyTime( tileNum(gridX, gridZ), modTime );
}


/**
 *	This method returns the timestamp in the cache for a particular tile.
 *
 *	@param	gridX		x position of the tile ( in chunk coordinates )
 *	@param	gridZ		z position of the tile ( in chunk coordinates )
 *
 *	@return	uint64		uint64 representation of the cache tile time.
 */
uint64 SpaceMapTimestampCache::cacheTime( int16 gridX, int16 gridZ ) const
{
	return this->cacheModifyTime( tileNum(gridX, gridZ) );
}


/**
 *	This method sets the timestamp in the cache for a particular tile to a
 *	particular time.
 *
 *	@param	tileNum		1-dimensional index of the tile to lookup
 *	@param	modTime		uint64 representation of the time to set for the tile.
 */
void SpaceMapTimestampCache::cacheModifyTime( uint32 tileNum, uint64 modTime )
{
	MF_ASSERT( pLastModified_ )
	uint64* pData = (uint64*)(pLastModified_->data());
	pData[tileNum+1] = modTime;
}


/**
 *	This method returns the 1-dimensional tile number for a particular tile.
 *
 *	@param	gridX		x position of the tile ( in chunk coordinates )
 *	@param	gridZ		z position of the tile ( in chunk coordinates )
 *
 *	@return	uint32		1-dimesional tile index at the given coordinates.
 */
uint32 SpaceMapTimestampCache::tileNum( int16 gridX, int16 gridZ ) const
{
	MF_ASSERT( gridX < info_.gridWidth_ );
	MF_ASSERT( gridZ < info_.gridHeight_ );
	uint16 biasedX, biasedZ;
	::biasGrid(info_.localToWorld_,gridX,gridZ,biasedX,biasedZ);
	uint32 tileNum = biasedX+biasedZ*info_.gridWidth_;
	return tileNum;
}


/**
 *	This method returns the current time.
 *
 *	@return	uint64		uint64 representation of the current time.
 */
uint64 SpaceMapTimestampCache::now() const
{
	SYSTEMTIME systemTime;
	FILETIME fileTime;
	::GetSystemTime( &systemTime );
	::SystemTimeToFileTime( &systemTime, &fileTime );
	return this->getUint64(fileTime);
}
