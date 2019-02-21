/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef TERRAIN_LOD_CONTROLLER_HPP
#define TERRAIN_LOD_CONTROLLER_HPP

#include "math/vector3.hpp"

namespace Terrain
{

class TerrainBlock2;

/**
 *	This class implements the lod controller for the terrain, 
 *	it manages all the different lod styles for the terrain, such
 *	as the vertex lod, the texture lod, the normal map lod etc.
 */
class TerrainLodController : public ReferenceCount
{
public:
	TerrainLodController();
	~TerrainLodController();

	void	cameraPosition( const Vector3& value );
	
	void	addBlock( TerrainBlock2* pBlock, const Vector3& centre );
	void	delBlock( TerrainBlock2* pBlock );

private:
	Vector3		cameraPosition_;
	int32		blockFocusX_;
	int32		blockFocusZ_;
	int32		blockWindowX_;
	int32		blockWindowZ_;
	int32		windowDims_;
	bool		initialised_;

	typedef		std::vector<TerrainBlock2*> Blocks;
	Blocks		activeBlocks_;

	typedef		std::pair<int32, int32> LocationToken;
	typedef		std::map<LocationToken, TerrainBlock2*> BlockMap;

	BlockMap	homeless_;
};

class BasicTerrainLodController
{
public:	
	BasicTerrainLodController();

	void	setCameraPosition( const Vector3& position );
	void	addBlock( TerrainBlock2* pBlock, const Vector3& worldPosition );
	bool	delBlock( TerrainBlock2* pBlock );

	static BasicTerrainLodController& instance();

private:
	uint32		getNumBlocks() const { return blocks_.size(); }

	// This is the list of block entries, so controller may update them at will.
	typedef		std::pair< Vector3, TerrainBlock2* > BlockEntry; 
	typedef		std::vector< BlockEntry > BlockContainer;
	BlockContainer	blocks_;

	// This serialises access to the list, so we don't try to update a partially
	// inserted block entry.
	SimpleMutex		accessMutex_;
};

};

#endif // TERRAIN_LOD_CONTROLLER_HPP