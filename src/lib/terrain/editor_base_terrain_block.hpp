/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef TERRAIN_EDITOR_BASE_TERRAIN_BLOCK_HPP
#define TERRAIN_EDITOR_BASE_TERRAIN_BLOCK_HPP

#include "base_terrain_block.hpp"
#include "terrain_data.hpp"

#ifdef EDITOR_ENABLED

namespace Terrain
{
    class HorizonShadowMap;
    class TerrainHoleMap;
    class TerrainTextureLayer;    
}

namespace Terrain
{
    /**
     *  This is the base interface for all terrain blocks that can be edited.
     */
    class EditorBaseTerrainBlock : public BaseTerrainBlock
    {
    public:
		// constants
		static const uint32 BLENDS_MIN_VALUE_THRESHOLD	= 8;
		static const size_t INVALID_LAYER				= (size_t)-1;

        /**
         *  Default constructor
         */
		EditorBaseTerrainBlock();

        /**
         *  This function saves the terrain block to a DataSection.
         *
         *  @param dataSection      A pointer to the DataSection to save to.
         *  @returns                True if the save was successful, false if the
         *                          save failed.
         */
        virtual bool save(DataSectionPtr dataSection) const = 0;

        /**
         *  This function saves the terrain block to a file.
         *
         *  @param filename         The file to save to.
         *  @returns                True if the save was successful, false if the
         *                          save failed.
         */
        virtual bool save(std::string const &filename) const = 0;

        /**
         *  This function gets the number of texture layers.
         *
         *  @returns                The number of texture layers.
         */
        virtual size_t numberTextureLayers() const = 0;

        /**
         *  This function gets the maximum number of texture layers.
         *
         *  @returns                The maximum number of texture layers.
         */
        virtual size_t maxNumberTextureLayers() const = 0;

        /**
         *  This function adds a new, empty texture layer and returns it's
		 *  index. It might fail if the maximum number of textures has been
		 *  reached already.
         *
		 *  @param blendWidth		The width of the blend, or 0 to use the
		 *							current terrain settings
		 *  @param blendHeight		The height of the blend, or 0 to use the
		 *							current terrain settings
         *  @returns                The index of the new layer, INVALID_LAYER 
		 *							otherwise.
         */
        virtual size_t insertTextureLayer(
			uint32 blendWidth = 0, uint32 blendHeight = 0 ) = 0;

        /**
         *  This function removes a texture layer.
         *  
         *  @param idx              The index of the layer to remove.
         *  @returns                True if the removal was ok, false otherwise.
         */
        virtual bool removeTextureLayer(size_t idx) = 0;

        /**
         *  This function gets the given texture layer.
         *
         *  @param idx              The index of the layer to get.
         *  @returns                The texture layer at the given index.
         */
        virtual TerrainTextureLayer &textureLayer(size_t idx) = 0;

        /**
         *  This function gets the given texture layer.
         *
         *  @param idx              The index of the layer to get.
         *  @returns                The texture layer at the given index.
         */
        virtual TerrainTextureLayer const &textureLayer(size_t idx) const = 0;

        /**
         *  This function draws the terrain ignoring any holes.
         */
        virtual void drawIgnoringHoles(bool setTextures = true) = 0;

		/**
		 * This function refreshes all data that would be affected by a height map
		 * change.
		 */
		virtual void setHeightMapDirty() = 0;

		/**
		 * This function refreshes normals.
		 */
		virtual void rebuildNormalMap( NormalMapQuality normalMapQuality ) = 0;

		/**
		 * This function refreshes lod texture.
		 *
		 * @param	transform	The world transform of this block.
		 * @returns				True if texture was rebuilt successfully, false 
		 *						otherwise.
		 */
		virtual bool rebuildLodTexture( const Matrix& transform  ) = 0;

		/**
		 *	This is used to flag that any LOD texture is now invalid and 
		 *	needs rebuilding.
		 */
		virtual void setLodTextureDirty( bool state ) = 0;

		/**
		 *	This function sets the resource name.
		 *
		 *  @param id				The name of the resource used.
		 */
        void resourceName(std::string const &id);

		/**
		 *  This function gets the resource name.
		 * 
		 *  @return					The name of the resource used.
		 */
        std::string const &resourceName() const;

		/**
		 *	This find the texture layer with the given texture and projections 
		 *	(if projections apply).
		 *
		 *  @param texture			The name of the texture.
		 *  @param uProj			The u-projection.
		 *  @param vProj			The v-projection.
		 *  @param epsilon			Alsmost equal epsilon from projections.
		 *  @param startIdx			The index to start searching from.
		 *  @returns				The index of the matching layer, or 
		 *							INVALID_LAYER if no such layer exists.
		 */
		size_t findLayer
		(
			std::string				const &texture, 
			Vector4					const &uProjection,
			Vector4					const &vProjection,
			float					epsilon,
			size_t					startIdx = 0
		) const;

		/**
		 *	This finds any matching texture layer.
		 *
		 *  @param layer			The layer to find.
		 *  @param epsilon			Epsilon for projection comparison.
		 *  @param startIdx			The index to start searching from.
		 *  @returns				The index of the matching layer, or 
		 *							INVALID_LAYER if no such layer exists.
		 */
		size_t findLayer
		(
			TerrainTextureLayer		const &layer, 
			float					epsilon,
			size_t					startIdx = 0
		) const;

		/**
		 *	This finds the layer that contributes the least to the block's
		 *  final colors.
		 *
		 *  @returns		The index of the weakest layer, or INVALID_LAYER if 
		 *					it has no layers.
		 */
		virtual size_t findWeakestLayer() const;

		/**
		 *	This removes duplicate layers and combines them into a single
		 *	layer.
		 *
		 *  @param epsilon			Epsilon for projection comparison.
		 *  @returns				True if any layers were modified.
		 */
		bool optimiseLayers(float epsilon);

		/**
		 * Rebuild combined texture layers from this object's texture layers.
		 */
		virtual void rebuildCombinedLayers
		(
			bool compressTextures          = true, 
			bool generateDominantTextureMap = true 
		) = 0;

        /**
         *  Set the selection key to use in marquee selection
         *
         *  @param key        Selection key, usualy the chunk item pointer
         */
		void setSelectionKey( DWORD key )
		{
			selectionKey_ = key;
		}

        /**
         *  Set the drawSelection flag so blocks are drawn using the selection
         *  key, for marquee selection.
         *
         *  @param draw        true to draw in selection mode, false otherwise
         */
		static void drawSelection( bool draw )
		{
			s_drawSelection_ = draw;
		}

	protected:
		static bool s_drawSelection_;
		DWORD selectionKey_;

    private:
        std::string             resourceName_;
    };

    typedef SmartPointer<EditorBaseTerrainBlock> EditorBaseTerrainBlockPtr;
} // namespace Terrain

#endif // EDITOR_ENABLED

#endif // TERRAIN_EDITOR_BASE_TERRAIN_BLOCK_HPP
