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
#include "worldeditor/editor/station_node_link_locator.hpp"
#include "worldeditor/editor/chunk_obstacle_locator.hpp"
#include "worldeditor/collisions/closest_obstacle_no_edit_stations.hpp"
#include "worldeditor/world/items/editor_chunk_entity.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "gizmo/snap_provider.hpp"
#include <limits>


namespace
{
    /**
     *  This is a helper class that finds the closest linkable chunk item.
     */
    class ClosestLinkableCatcher : public CollisionCallback
    {
    public:
        explicit ClosestLinkableCatcher(StationNodeLinkLocator::Type type) : 
            chunkItem_(NULL),
            distance_(std::numeric_limits<float>::max()),
            type_(type)
        {
        }

        /*virtual*/ int operator()
        ( 
            ChunkObstacle   const &obstacle,
		    WorldTriangle   const &/*triangle*/, 
            float           dist
        )
        {
            if (dist < distance_)
            {
                distance_ = dist;

                EditorChunkItem *item = 
                    (ChunkItem *)obstacle.pItem().getObject();
                
                if 
                (
                    (type_ & StationNodeLinkLocator::LOCATE_ENTITIES) != 0
                    &&
                    item->isEditorEntity()
                )
                {
                    EditorChunkEntity *entity = (EditorChunkEntity *)item;
                    int idx = entity->patrolListPropIdx();
                    if (idx != -1)
                    {
                        chunkItem_ = obstacle.pItem();
                    }
                }
                else if 
                (
                    (type_ & StationNodeLinkLocator::LOCATE_NODES) != 0
                    &&
                    item->isEditorChunkStationNode()
                )
                {
                    chunkItem_ = obstacle.pItem();
                }           
            }
            return COLLIDE_BEFORE;
        }

        ChunkItemPtr                    chunkItem_;
        float                           distance_;
        StationNodeLinkLocator::Type    type_;
    };
}


/**
 *  StationNodeLinkLocator constructor.
 */
/*explicit*/ StationNodeLinkLocator::StationNodeLinkLocator
(
    Type    type        /*= LOCATE_BOTH*/
) :
    chunkItem_(NULL),
    initialPos_(true),
    type_(type)
{
    subLocator_ = 
        new ChunkObstacleToolLocator(ClosestObstacleNoEditStations::s_default);
}


/**
 *  StationNodeLinkLocator destrcutor.
 */
/*virtual*/ StationNodeLinkLocator::~StationNodeLinkLocator()
{
}


/**
 *  Calculate the location given a ray through worldray.
 *
 *  @param worldRay     The world ray.
 *  @param tool         The originating tool.
 */
/*virtual*/ void 
StationNodeLinkLocator::calculatePosition
(
    Vector3             const &worldRay, 
    Tool                &tool
)
{
    chunkItem_ = NULL; // reset item

    // Allow the sub-locator to find the position first:
	subLocator_->calculatePosition(worldRay, tool);
	transform_ = subLocator_->transform();

	Vector3 extent = 
        Moo::rc().invView().applyToOrigin() 
        +
		worldRay*Moo::rc().camera().farPlane();

    ClosestLinkableCatcher clc(type_);

	float distance = 
        ChunkManager::instance().cameraSpace()->collide
        ( 
		    Moo::rc().invView().applyToOrigin(),
		    extent,
		    clc
        );

    chunkItem_ = clc.chunkItem_;
}


/**
 *  Return the selected chunk item.
 */
ChunkItemPtr StationNodeLinkLocator::chunkItem()
{
    return chunkItem_;
}
