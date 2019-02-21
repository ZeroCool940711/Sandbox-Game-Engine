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

#include "terrain_hole_functor.hpp"

#include "worldeditor/undo_redo/terrain_hole_map_undo.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "gizmo/tool.hpp"
#include "gizmo/undoredo.hpp"

#include "romp/flora.hpp"

#include "terrain/terrain_hole_map.hpp"

#include "chunk/base_chunk_space.hpp"


PY_TYPEOBJECT( TerrainHoleFunctor )

PY_BEGIN_METHODS( TerrainHoleFunctor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( TerrainHoleFunctor )
	PY_ATTRIBUTE( fillNotCut )
PY_END_ATTRIBUTES()

PY_FACTORY_NAMED( TerrainHoleFunctor, "TerrainHoleFunctor", Functor )

FUNCTOR_FACTORY( TerrainHoleFunctor )


/**
 *	Constructor.
 *
 *	@param pType		The type.
 */
/*explicit*/ TerrainHoleFunctor::TerrainHoleFunctor( PyTypePlus * pType ):
	TerrainFunctor( pType ),
	falloff_( 0.f ),
	fillNotCut_( false ),
	applying_( false )
{
}


/** 
 *	This method updates the height pole hole functor.  If the left mouse button 
 *	is down, the functor is applied.
 *
 *	@param dTime	The change in time since the last frame.
 *	@param tool		The tool that we are using.
 */
void TerrainHoleFunctor::update( float dTime, Tool & tool )
{
	if (dragHandler().isDragging(MouseDragHandler::KEY_LEFTMOUSE))
	{
		if (!applying_)
		{
			beginApplying();
			applying_ = true;
		}

		//per update calculations
		falloff_ = 2.0f / tool.size();

		// Doing all this offset shennanigans is necessary because although the 
		// user "points" at quads, the underlying mesh is still vertex based : 
		// the quad actually touched hangs off the corner of the vertex.		
		Matrix savedTransform = tool.locator()->transform();
		Matrix newTransform( savedTransform );		

		float size = float( GRID_RESOLUTION ) / 
			float( WorldManager::instance().pTerrainSettings()->holeMapSize() );
		float offset = -size / 2.0f;
		newTransform._41 += offset;
		newTransform._43 += offset;

		tool.locator()->transform( newTransform );
		TerrainFunctor::apply( tool );
		tool.locator()->transform( savedTransform );
	}
	else
	{
		if (applying_)
		{
			endApplying();
			applying_ = false;

			UndoRedo::instance().barrier( "Terrain Hole Change", true );

			Flora::floraReset();
		}
	}
}


/**
 *	This returns whether the functor is being applied.
 *
 *	@return				True if the functor is being applied, false otherwise.
 */
bool TerrainHoleFunctor::applying() const
{ 
	return applying_; 
}


/**
 *	This method is called before any of perheightpole class are made.
 *	It saves the current hole map in the undo/redo list
 *
 *  @param chunkTerrain	The terrain to get the format of.
 *	@param format		This is set to the format of the terrain.
 */
void TerrainHoleFunctor::getBlockFormat(
    const EditorChunkTerrain &		chunkTerrain,
    TerrainUtils::TerrainFormat &	format ) const
{
	const Terrain::TerrainHoleMap & holeMap = 
		chunkTerrain.block().holeMap();
	format.polesWidth	= holeMap.width();
	format.polesHeight	= holeMap.height();
	format.blockWidth	= holeMap.width();
	format.blockHeight	= holeMap.height();
	format.poleSpacingX	= Terrain::BLOCK_SIZE_METERS / format.blockWidth;
	format.poleSpacingY	= Terrain::BLOCK_SIZE_METERS / format.blockHeight;
}


/**
 *	This is called whenever a new terrain is touched by the tool, it can be 
 *	used to save the undo/redo buffer for example.
 *
 *  @param ect		The new terrain touched.
 */
void TerrainHoleFunctor::onFirstApply(EditorChunkTerrain &ect)
{
	UndoRedo::instance().add( new TerrainHoleMapUndo( &ect.block(), ect.chunk() ) );
}


/**
 *	This method cuts out or fills in a hole
 *
 *	@param	chunkTerrain The terrain that is being considered.
 *	@param	toolOffset	The offset of the tool.
 *	@param	chunkOffset	The offset of the chunk.
 *  @param  format		The format of the area being edited.
 *  @param  minx		The minimum x coord of the area of the block to change.
 *  @param  minz		The minimum z coord of the area of the block to change.
 *  @param  maxx		The maximum x coord of the area of the block to change.
 *  @param  maxz		The maximum z coord of the area of the block to change.
 */
void TerrainHoleFunctor::applyToSubBlock(
	EditorChunkTerrain &		chunkTerrain,
	const Vector3 &				/*toolOffset*/,
	const Vector3 &				/*chunkOffset*/,
	const TerrainUtils::TerrainFormat &/*format*/,
	int32						minx,
	int32						minz,
	int32						maxx,
	int32						maxz )
{
	Terrain::TerrainHoleMap &holeMap = chunkTerrain.block().holeMap();
	Terrain::TerrainHoleMapHolder holder( &holeMap, false );
	Terrain::TerrainHoleMap::ImageType &image = holeMap.image();

	// These x & z might be outside of the terrain coordinate for this
	// chunk. Make sure they are in range.
	minx = Math::clamp< int32 >( 0, minx, minx );
	minz = Math::clamp< int32 >( 0, minz, minz );
	maxx = Math::clamp< int32 >( maxx, maxx, (int32)image.width() - 1 );
	maxz = Math::clamp< int32 >( maxz, maxz, (int32)image.height() - 1 );

	for (int32 z = minz; z <= maxz; ++z)
	{
		for (int32 x = minx; x <= maxx; ++x)
		{
			image.set( x, z, !fillNotCut_ );
		}
	}
	WorldManager::instance().markTerrainShadowsDirty( chunkTerrain.chunk() );
}


/**
 *	This is called when the tool has finished being applied for one frame.
 *
 *	@param tool			The tool being applied.
 */
void TerrainHoleFunctor::onApplied( Tool & /*tool*/ )
{
}


/**
 *	This is for every chunk that the tool was applied to in the current frame.
 *
 *	@param chunkTerrain	A chunk that the tool was applied to.
 */
void TerrainHoleFunctor::onLastApply( EditorChunkTerrain & chunkTerrain )
{
}


/**
 *	This gets an attribute for Python.
 *
 *	@param pAttr	The attribute to get.
 *	@return			The attribute.
 */
PyObject * TerrainHoleFunctor::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return TerrainFunctor::pyGetAttribute( attr );
}


/**
 *	This sets an attribute from Python.
 *
 *	@param pAttr	The attribute to set.
 *	@param pValue	The new value of the attribute.
 */
int TerrainHoleFunctor::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return TerrainFunctor::pySetAttribute( attr, value );
}


/**
 *	This is the static Python factory method.
 *
 *  @param pArgs	The creation arguments.
 *	@return			A new TerrainHoleFunctor.
 */
PyObject * TerrainHoleFunctor::pyNew( PyObject * /*pArgs*/ )
{
	return new TerrainHoleFunctor();
}
