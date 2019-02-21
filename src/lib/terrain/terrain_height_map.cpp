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
#include "terrain_height_map.hpp"

using namespace Terrain;

TerrainHeightMap::Iterator TerrainHeightMap::iterator(int32 x, int32 y)
{
    return Iterator(*this, x, y, xVisibleOffset(), zVisibleOffset());
}

/*virtual*/ float TerrainHeightMap::heightAt(float x, float z) const
{
    BW_GUARD;
	// TODO: Make more accurate
    // This implementation uses bicubic interpolation, so it's not entirely
    // accurate.
    TerrainHeightMap *myself = const_cast<TerrainHeightMap *>(this);

#ifdef EDITOR_ENABLED
	myself->lock(true);
#endif

	float result = myself->image().getBicubic(x, z);

#ifdef EDITOR_ENABLED
	myself->unlock();
#endif

    return result;
}

float TerrainHeightMap::slopeAt(int x, int z) const
{
    Vector3 n = normalAt(x, z);
    return RAD_TO_DEG(::acosf(Math::clamp(-1.0f, n.y, +1.0f)));
}

float TerrainHeightMap::slopeAt(float x, float z) const
{
    Vector3 n = normalAt(x, z);
    return RAD_TO_DEG(::acosf(Math::clamp(-1.0f, n.y, +1.0f)));
}
