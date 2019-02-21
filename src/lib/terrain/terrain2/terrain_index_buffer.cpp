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
#include "cstdmf/profiler.hpp"
#include "terrain_index_buffer.hpp"

DECLARE_DEBUG_COMPONENT2( "Moo", 0 );

namespace
{
	static bool	s_drawDegeneratesOnly	= false;
	static bool s_enableDrawPrim		= true;
	static bool s_enableSetState		= true;
	static bool	s_watchesAdded			= false;
}

namespace Terrain
{

PROFILER_DECLARE( TerrainIndexBuffer_SetLodConstants, "TerrainIndexBuffer SetLodConstants" );

// -----------------------------------------------------------------------------
// Section: TerrainIndexBuffer
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TerrainIndexBuffer::TerrainIndexBuffer( uint32 quadCountX, uint32 quadCountZ ) :
	quadCountX_( quadCountX ),
	quadCountZ_( quadCountZ ),
	indexCount_( quadCountX * quadCountZ * 6 )
{
	BW_GUARD;
	// this is how many indexes needed for degenerate triangles around the 
	// main block.
	int degenerateIndexCount = ( (quadCountX / 2) * 3 );

	// now calculate how many we need for around each sub-block - its half.
	// We can't draw any SB degenerates if there will be less than 3 indices.
	subBlockDegIndexCount_ = degenerateIndexCount >= 6 ? 
								degenerateIndexCount / 2 : 0;

	if ( !s_watchesAdded )
	{
		MF_WATCH( "Render/Terrain/Terrain2/Draw Degenerates Only", s_drawDegeneratesOnly,
			Watcher::WT_READ_WRITE,
			"Only draw degenerate triangles between lods." );

		MF_WATCH( "Render/Performance/DrawPrim TerrainIndexBuffer", s_enableDrawPrim,
			Watcher::WT_READ_WRITE,
			"Allow TerrainIndexBuffer to call drawIndexedPrimitive()." );

		MF_WATCH( "Render/Performance/SetState TerrainIndexBuffer", s_enableSetState,
			Watcher::WT_READ_WRITE,
			"Allow TerrainIndexBuffer to set render state." );

		s_watchesAdded = true;
	}
}

/**
 *	Destructor.
 */
TerrainIndexBuffer::~TerrainIndexBuffer()
{
	BW_GUARD;
	// Remove our index buffer from the pool
	// Create a token from the dimensions.
	SimpleMutexHolder smh( s_mutex_ );
	uint64 token = uint64(quadCountX_) | (uint64(quadCountZ_) << 32 );

	IndexBufferMap::iterator it = s_buffers_.find( token );
	if (it != s_buffers_.end())
	{
		s_buffers_.erase( it );
	}
}

/*
 *	This method creates the index buffer and fills it.
 *	It is called when the device is created or recreated.
 */
void TerrainIndexBuffer::createManagedObjects()
{
	BW_GUARD;
	if (Moo::rc().device())
	{
		uint32 indexCount = 
			(indexCount_ + (subBlockDegIndexCount_ * 4 * 4) );

		if ( SUCCEEDED( pIndexBuffer_.create( indexCount, 
					D3DFMT_INDEX32, D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED, 
					"Terrain/IndexBuffer" ) ) )
		{
			Moo::IndicesReference ir = pIndexBuffer_.lock();

			if ( ir.valid() )
			{
				generateIndices( TLO_TILES_SWIZZLED, 
					(TerrainIndexBuffer::IndexType*)ir.indices(),
					indexCount * sizeof( TerrainIndexBuffer::IndexType ) );
				pIndexBuffer_.unlock();
				// Add the buffer to the preload list so that it can get uploaded
				// to video memory
				pIndexBuffer_.addToPreloadList();
			}
		}
	}
}

void TerrainIndexBuffer::deleteManagedObjects()
{
	BW_GUARD;
	pIndexBuffer_.release();
}

/*
 *	This method generates the indices for the terrain cell.
 *	The indices are in list form.
 *	TODO: Add degenerates after main body of triangle list
 */
void TerrainIndexBuffer::generateIndices( TriangleListOrder order, IndexType* pOut, uint32 bufferSize ) const
{
	BW_GUARD;
	IndexType	vertexRow	= IndexType(quadCountX_ + 1);
	IndexType*	pCurrent	= pOut;
	
	if ( order == TLO_TILES_ROWS )
	{
		// Generate the indices for this lod level
		for (uint32 z = 0; z < quadCountZ_; z++)
		{
			IndexType currentBaseIndex = IndexType(z) * vertexRow;

			// fill in the quads for this row 
			for (uint32 x = 0; x < quadCountX_; x++)
			{
				if ((x ^ z) & 1)
				{
					*(pCurrent++) = currentBaseIndex;
					*(pCurrent++) = currentBaseIndex + vertexRow;
					*(pCurrent++) = currentBaseIndex + 1;
					*(pCurrent++) = currentBaseIndex + 1;
					*(pCurrent++) = currentBaseIndex + vertexRow;
					*(pCurrent++) = currentBaseIndex + vertexRow + 1;
					currentBaseIndex += 1;
				}
				else
				{
					*(pCurrent++) = currentBaseIndex;
					*(pCurrent++) = currentBaseIndex + vertexRow;
					*(pCurrent++) = currentBaseIndex + vertexRow + 1;
					*(pCurrent++) = currentBaseIndex + vertexRow + 1;
					*(pCurrent++) = currentBaseIndex + 1;
					*(pCurrent++) = currentBaseIndex;
					currentBaseIndex += 1;
				}
			}
		}
	}
	else if (order = TLO_TILES_SWIZZLED )
	{
		// get log2 of quadCount, this will be used for shifting. quadCountX_
		// must be a power of 2 or this will be inaccurate.
		uint32 n = quadCountX_;
		uint32 ln= 0;			// this is log2(n).
		while ( n >>= 1 ) ln++;
		// Test
		MF_ASSERT( 1 << ln == quadCountX_ && "quadCountX_ is not a power of two!" );

		for (uint32 q = 0; q < (quadCountX_ * quadCountZ_); q++)
		{
			IndexType x = 0;
			IndexType z = 0;
			IndexType xmask = 1;
			IndexType zmask = 2;
			
			for (uint32 shift = 0; shift < ln; shift++)
			{
				if (q & xmask)
				{
					x += 1 << shift;
				}
				if (q & zmask)
				{
					z += 1 << shift;
				}

				xmask <<= 2;
				zmask <<= 2;
			}
			
			IndexType currentBaseIndex = z * vertexRow + x;
			
			if ((x ^ z) & 1)
			{
				*(pCurrent++) = currentBaseIndex;
				*(pCurrent++) = currentBaseIndex + vertexRow;
				*(pCurrent++) = currentBaseIndex + 1;
				*(pCurrent++) = currentBaseIndex + 1;
				*(pCurrent++) = currentBaseIndex + vertexRow;
				*(pCurrent++) = currentBaseIndex + vertexRow + 1;
			}
			else
			{
				*(pCurrent++) = currentBaseIndex;
				*(pCurrent++) = currentBaseIndex + vertexRow;
				*(pCurrent++) = currentBaseIndex + vertexRow + 1;
				*(pCurrent++) = currentBaseIndex + vertexRow + 1;
				*(pCurrent++) = currentBaseIndex + 1;
				*(pCurrent++) = currentBaseIndex;
			}
		}
	}

	// make sure we haven't overrun (or underrun) buffer
	MF_ASSERT( (pCurrent - pOut) == int32(indexCount_) );

	if ( subBlockDegIndexCount_ > 0 )
	{
		// Generate sub block degenerates, updating pCurrent
		IndexType subQuadsX = IndexType( quadCountX_ >> 1 );
		IndexType subQuadsZ = IndexType( quadCountZ_ >> 1 );

		generateDegenerates( 0, 0, subQuadsX, subQuadsZ, 
							vertexRow, pCurrent );
		generateDegenerates( subQuadsX, 0, subQuadsX + subQuadsX, subQuadsZ, 
							vertexRow, pCurrent );
		generateDegenerates( 0, subQuadsZ, subQuadsX, subQuadsZ + subQuadsZ, 
							vertexRow, pCurrent );
		generateDegenerates( subQuadsX, subQuadsZ, subQuadsX + subQuadsX, subQuadsZ + subQuadsZ, 
							vertexRow, pCurrent );
	}

	// make sure we haven't overrun (or underrun) buffer
	MF_ASSERT( (pCurrent - pOut) == bufferSize/sizeof(IndexType) );
}

void TerrainIndexBuffer::generateDegenerates( IndexType xStart, IndexType zStart, 
											  IndexType xEnd,  IndexType zEnd,	
											  IndexType rowSize, IndexType*& pBuffer ) const
{
	BW_GUARD;
	// degenerates for the positive x direction
	for (IndexType z = zStart; z < zEnd; z += 2)
	{
		IndexType index = z * rowSize + xEnd;
		*(pBuffer++) = index;
		*(pBuffer++) = index + rowSize;
		*(pBuffer++) = index + rowSize + rowSize;
	}

	// degenerates for the negative x direction
	for (IndexType z = zStart; z < zEnd; z += 2)
	{
		IndexType index = z * rowSize + xStart;
		*(pBuffer++) = index;
		*(pBuffer++) = index + rowSize + rowSize;
		*(pBuffer++) = index + rowSize;
	}

	IndexType offset = IndexType(rowSize * zEnd);

	// degenerates for the positive z direction
	for (IndexType x = xStart; x < xEnd; x += 2)
	{
		*(pBuffer++) = x + offset;
		*(pBuffer++) = x + offset + 2;
		*(pBuffer++) = x + offset + 1;
	}

	offset = IndexType(rowSize * zStart);

	// degenerates for the negative z direction
	for (IndexType x = xStart; x < xEnd; x += 2)
	{
		*(pBuffer++) = x + offset;
		*(pBuffer++) = x + offset + 1;
		*(pBuffer++) = x + offset + 2;
	}
}

/**
 *	This method sets the indices on the device.
 *	@return true if successful
 */
bool TerrainIndexBuffer::setIndices()
{
	BW_GUARD;
	bool res = false;

	if (!pIndexBuffer_.valid())
		createManagedObjects();

	if (pIndexBuffer_.valid())
	{
		pIndexBuffer_.set();
		res = true;
	}
	return res;
}

inline void SetLodConstants( Moo::EffectMaterialPtr	pMaterial, 
							 const Vector2&			morphValue )
{
	BW_GUARD;
	if ( !s_enableSetState )
		return;

	ID3DXEffect* pEffect	= pMaterial->pEffect()->pEffect();

	PROFILER_BEGIN( TerrainIndexBuffer_SetLodConstants );
	SAFE_SET( pEffect, Float, "lodStart",	morphValue.x );
	SAFE_SET( pEffect, Float, "lodEnd",		morphValue.y );
	PROFILER_END();

	pMaterial->commitChanges();
}

/**
 *	This method draws the triangles using the indexbuffer supplied
 *	and an external vertex buffer.
 *
 *	@param pMaterial        Material to render.
 *	@param morphRanges      Morph parameters for the vertex shader.
 *	@param neighbourMasks   Masks representing neighbouring relative LoD differences.
 *  @param subBlockMask     the mask which specifies which sub-blocks to draw.
 *	                        buffer to render. 
 */
void TerrainIndexBuffer::draw(	Moo::EffectMaterialPtr	pMaterial,
								const Vector2&			morphRanges,
								const NeighbourMasks&	neighbourMasks,
								uint8					subBlockMask	)
{ 
	BW_GUARD;
	const uint32 vertexCount= (quadCountX_ + 1) * (quadCountZ_ + 1);

	// offset into index buffer, degenerates start after the main indexes.
	const uint32 subBlockDegenStart	= indexCount_;

	// All specified, draw the indexes as normal.
	SetLodConstants( pMaterial, morphRanges );

	if ( subBlockMask == 0xF )
	{
		if ( !s_drawDegeneratesOnly && s_enableDrawPrim )
		{	
			Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 
				0, vertexCount, 0, indexCount_ / 3 );
		}
	}

	// Render each sub-block as specified by mask. Note we'll always draw
	// sub-block degenerates, but not necessarily the sub-block (because that
	// might have been draw above).
	const uint32 subBlockIndexCount	= indexCount_ /4 ;
	for ( uint32 i = 0; i < 4; i++ )
	{
		if ( !(subBlockMask & ( 1 << i )))
			continue;

		// Draw sub-blocks only if main block wasn't drawn above
		if (	!s_drawDegeneratesOnly	&&
				s_enableDrawPrim		&&
				subBlockMask != 0xF		)
		{
			
			uint32	offset = subBlockIndexCount * i;

			Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 
				0, vertexCount, offset, subBlockIndexCount / 3 );
		}
		
		// Early out if there are no degenerates for given sub-block.
		if (neighbourMasks[i+1]		== 0 ||
			subBlockDegIndexCount_	== 0)
			continue;

		for ( uint32 j = 0; j < 4; j++ )
		{
			// skip main block degenerates
			uint32 doffset = subBlockDegenStart;
			doffset += ( ( i * 4 ) + ( j ) ) * subBlockDegIndexCount_;

			// for each sub block side, if there are degenerates...
			if ( neighbourMasks[i+1] & ( 1 << j ) && s_enableDrawPrim )
			{
				Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 
					0, vertexCount, doffset, subBlockDegIndexCount_ / 3 );
			}
		}
	}
}

/**
 *	This static method gets a a terrainindexbuffer of certain dimensions from our
 *	pool of index buffers, if an index buffer is not in the pool a new one is created.
 */
TerrainIndexBuffer* TerrainIndexBuffer::get( uint32 quadCountX, uint32 quadCountZ )
{
	BW_GUARD;
	TerrainIndexBuffer* pBuffer = NULL;
	
	// Create a token from the dimensions.
	uint64 token = uint64(quadCountX) | (uint64(quadCountZ) << 32 );

	SimpleMutexHolder smh( s_mutex_ );
	IndexBufferMap::iterator it = s_buffers_.find( token );
	if (it == s_buffers_.end())
	{
		pBuffer = new TerrainIndexBuffer( quadCountX, quadCountZ );
		s_buffers_.insert( std::make_pair( token, pBuffer ) );
	}
	else
	{
		pBuffer = it->second;
	}
	return pBuffer;
}

TerrainIndexBuffer::IndexBufferMap TerrainIndexBuffer::s_buffers_;
SimpleMutex TerrainIndexBuffer::s_mutex_;

};
