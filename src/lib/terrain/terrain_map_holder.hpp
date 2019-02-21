/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef TERRAIN_TERRAIN_MAP_HOLDER_HPP
#define TERRAIN_TERRAIN_MAP_HOLDER_HPP

#include "terrain_map.hpp"

namespace Terrain
{
    /**
     *  This object can be used to safely use lock/unlock a TerrainMap.
     */
    template<typename TERRAINMAP>
    class TerrainMapHolder
    {
    public:
        TerrainMapHolder(TERRAINMAP *terrainMap, bool readOnly);

        TerrainMapHolder(TerrainMapHolder const &other);

        ~TerrainMapHolder();

        TerrainMapHolder &operator=(TerrainMapHolder const &other);

    private:
        TERRAINMAP          *terrainMap_;
        bool                readOnly_;
    };
}


/**
 *  Implementation
 */

template<typename TERRAINMAP>
inline /*explicit*/ 
Terrain::TerrainMapHolder<TERRAINMAP>::TerrainMapHolder
(
    TERRAINMAP              *terrainMap,
    bool                    readOnly
):
    terrainMap_(terrainMap),
    readOnly_(readOnly)
{
    if (terrainMap_ != NULL)
        terrainMap_->lock(readOnly);
}


template<typename TERRAINMAP>
inline 
Terrain::TerrainMapHolder<TERRAINMAP>::TerrainMapHolder(TerrainMapHolder const &other):
    terrainMap_(other.terrainMap_),
    readOnly_(other.readOnly_)
{
    if (terrainMap_ != NULL)
        terrainMap_->lock(readOnly_);
}


template<typename TERRAINMAP>
inline 
Terrain::TerrainMapHolder<TERRAINMAP>::~TerrainMapHolder()
{
    if (terrainMap_ != NULL)
    {
        terrainMap_->unlock();
        terrainMap_ = NULL;
    }
}


template<typename TERRAINMAP>
inline
Terrain::TerrainMapHolder<TERRAINMAP> &
Terrain::TerrainMapHolder<TERRAINMAP>::operator=
(
    TerrainMapHolder const &other
)
{
    if (this != &other)
    {
        if (terrainMap_ != NULL)
            terrainMap_->unlock();
        terrainMap_ = other.terrainMap_;
        readOnly_ = other.readOnly_;
        if (terrainMap_ != NULL)
            terrainMap_->lock(readOnly_);
    }
    return *this;
}

#endif // TERRAIN_TERRAIN_MAP_HOLDER_HPP
