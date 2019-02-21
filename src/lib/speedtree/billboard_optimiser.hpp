/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SPEEDTREE_BB_OPTIMISER_HPP
#define SPEEDTREE_BB_OPTIMISER_HPP

// Module Interface
#include "speedtree_config_lite.hpp"

#include "speedtree_vertex_types.hpp"

// BW Tech Hearders
#include "cstdmf/smartpointer.hpp"
#include "moo/effect_material.hpp"
#include "moo/texture_aggregator.hpp"
#include "moo/moo_math.hpp"
#include "math/Matrix.hpp"

// DX Headers
#include <d3dx9math.h>

// STD Headers
#include <vector>
#include <map>

// Forwad declarations
namespace Moo { class BaseTexture; }

namespace speedtree {

/**
 *	The billboard optimiser. Optimises billboard rendering by compiling all
 *	billboards placed in the same area (grouped by location) as a single vertex
 *  buffer, minimising the draw call overhead.
 *  The different textures of each  tree type are all
 *	bunched together into a single texture atlas. Alpha test values, the only 
 *	remaining unique feature that differentiates each tree billboard are 
 *	uploaded as shader constants and processed in the vertex shader.
 */
class BillboardOptimiser
{
public:
	typedef SmartPointer<BillboardOptimiser> BillboardOptimiserPtr;

	/**
	 *	Holds all camera and light setup information 
	 *	needed to render a single frame.
	 */
	struct FrameData
	{
		FrameData(		
				const Matrix  & vpj,
				const Matrix  & ivw,
				const Vector3 & sdr,
				const Moo::Colour sdf,
				const Moo::Colour sam,
				bool              texTrees ) :
			viewProj_( vpj ),
			invView_( ivw ),
			sunDirection_( sdr ),
			sunDiffuse_( sdf ),
			sunAmbient_( sam ),
			texturedTrees_( texTrees )
		{}

		const Matrix  & viewProj_;
		const Matrix  & invView_;
		const Vector3 & sunDirection_;
		const Moo::Colour sunDiffuse_;
		const Moo::Colour sunAmbient_;
		const bool        texturedTrees_;
	};

	static void init( DataSectionPtr section );
	static void initFXFiles();
	static void fini();
	
	static BillboardOptimiserPtr retrieve( const Vector3 & pos );	

	static int addTreeType(
		Moo::BaseTexturePtr    texture, 
#if SPT_ENABLE_NORMAL_MAPS
		Moo::BaseTexturePtr    normalMap, 
#endif // SPT_ENABLE_NORMAL_MAPS
		const BBVertexBuffer & vertices,
		const char           * treeFilename,
		const float          * material,
		float                  leafLightAdj );
		
	static void delTreeType( int treeTypeId );
	
	int addTree(
		int             treeTypeId,
		const Matrix &  world,
#ifdef EDITOR_ENABLED
		float           alphaValue,
		const Vector4 & fogColour,
		float           fogNear,
		float           fogFar );
#else
		float           alphaValue );
#endif // EDITOR_ENABLED

	void removeTree( int treeID );
	void release( int treeID );
	
	void updateTreeAlpha(
		int             treeID,
#ifdef EDITOR_ENABLED
		float           alphaValue,
		const Vector4 & fogColour,
		float           fogNear,
		float           fogFar );
#else
		float           alphaValue );
#endif // EDITOR_ENABLED

	bool isFull() const;
	
	static void tick();
	static void update();
	static void drawAll( const FrameData & frameData );

	void releaseBuffer();


	void incRef() const;
	void decRef() const;
	int  refCount() const;

	static uint16		s_maxInstanceCount_;
	static float		s_mipBias_;
private:
	typedef std::vector<int> IntVector;
	
	/**
	 * Represents a tree type in the optimiser
	 */
	struct TreeType
	{
		IntVector              tileIds_;
		const BBVertexBuffer * vertices_;
		Vector4                diffuseNAdjust_;
		Vector3                ambient_;
	};

	/**
	 * Represents a single tree in a chunk.
	 */
	struct TreeInstance : Aligned
	{
		int            typeId_;
		Matrix         world_;
	};

	typedef std::map<int, TreeType> TreeTypeMap;
	typedef std::avector<TreeInstance> TreeInstanceVector;
	typedef std::vector<float> AlphaValueVector;

	BillboardOptimiser( const Vector3& pos );
	~BillboardOptimiser();

	static void getNearestSuitableTexCoords(
		DX::Texture	          * texture,
		const BillboardVertex * originalVertices,
		RECT                  & o_srcRect, 
		Vector2               * o_coordsOffsets );

	static void getNextBBBTSlot(
		const RECT & srcRect, 
		POINT      & o_dstPoint,
		Vector2    * o_dstTexCoords );
		
	static void renderIntoBBBT(
		DX::Texture	* texture,
		const RECT  & srcRect, 
		const POINT & dstPoint );

	static uint32 makeID( const Vector3& pos );

	// forbid copy
	BillboardOptimiser( const BillboardOptimiser & );
	const BillboardOptimiser & operator = ( const BillboardOptimiser & );

	class BillboardsVBuffer * vbuffer_;
	uint32					id_;

	TreeInstanceVector trees_;
	AlphaValueVector   alphas_;
	IntVector          freeSlots_;
	
	mutable int refCount_;

	static int			s_atlasMinSize_;
	static int			s_atlasMaxSize_;
	static int			s_atlasMipLevels_;
	static int			s_nextTreeTypeId_;	
	static TreeTypeMap	s_treeTypes_;

	typedef std::vector<BillboardOptimiser*> BBVector;
	typedef std::map<uint32, BBVector > ChunkBBMap;
	static SimpleMutex s_chunkBBMapLock_;
	static ChunkBBMap  s_chunkBBMap_;
	
	static bool s_saveTextureAtlas_;
	
	friend class BillboardsVBuffer;
};

} // namespace speedtree
#endif // SPEEDTREE_BB_OPTIMISER_HPP
