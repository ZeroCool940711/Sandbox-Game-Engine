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
#include "terrain_texture_layer2.hpp"

#include "../terrain_settings.hpp"
#include "../terrain_data.hpp"
#include "terrain_block2.hpp"
#include "moo/png.hpp"

#ifdef EDITOR_ENABLED
#include "terrain/terrain2/ttl2cache.hpp"
#endif // EDITOR_ENABLED

using namespace Terrain;
using namespace std;

DECLARE_DEBUG_COMPONENT2( "Terrain", 0)

/**
 *	This is the TerrainTextureLayer2 constructor.
 *
 *  @param terrainBlock		The underlying TerrainBlock2.
 *  @param blendWidth		The width of the blend, or 0 to use the current
 *							terrain settings
 *  @param blendHeight		The height of the blend, or 0 to use the current
 *							terrain settings
 */
/*explicit*/ TerrainTextureLayer2::TerrainTextureLayer2(
		CommonTerrainBlock2 &terrainBlock,
		uint32 blendWidth /*= 0*/, uint32 blendHeight /*= 0*/ ):
	terrainBlock_(&terrainBlock),
	blends_("Terrain/TextureLayer2/Image"),
	width_(blendWidth),
	height_(blendHeight),
   	lockCount_(0)
#ifdef EDITOR_ENABLED
	,state_(LOADING)
#endif
{
	BW_GUARD;
#ifndef MF_SERVER
	if ( width_ == 0 )
		width_ = terrainBlock_->settings()->blendMapSize();
	if ( height_ == 0 )
		height_ = terrainBlock_->settings()->blendMapSize();
#endif

	// Track memory usage
	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
							(uint)ResourceCounters::SYSTEM),
							(uint)sizeof(*this))
}


/**
 *	This is the TerrainTextureLayer2 destructor.
 */
TerrainTextureLayer2::~TerrainTextureLayer2()
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	if (TTL2Cache::instance() != NULL)
		TTL2Cache::instance()->delTextureLayer(this);
#endif
	// Track memory usage
	RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
							(uint)ResourceCounters::SYSTEM),
							(uint)sizeof(*this))
}


/**
 *	This gets the name of the texture used in the layer.
 *
 *  @returns	The name of the texture used to draw the layer.
 */
/*virtual*/ string TerrainTextureLayer2::textureName() const
{
    return textureName_;
}


/**
 *	This sets the texture used by the layer.  The layer is created using the
 *	default size.
 *
 *  @param filename		The name of the new texture.
 *  @returns			True if the texture could be set.
 */
/*virtual*/ bool TerrainTextureLayer2::textureName(string const &filename)
{
	BW_GUARD;	
#ifndef MF_SERVER
	Moo::BaseTexturePtr tex = Moo::TextureManager::instance()->get(filename);
    if (tex != NULL)
    {
        texture(tex, filename);
        return true;
    }
    else
    {
        return false;
    }
#else
	// Track memory usage
	RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool("texture layer",
							(uint)ResourceCounters::SYSTEM),
							(uint)textureName_.capacity())
	textureName_ = filename;

	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool("texture layer",
							(uint)ResourceCounters::SYSTEM),
							(uint)textureName_.capacity())
	return true;
#endif
}


#ifndef MF_SERVER


/**
 *	This gets the texture used by the layer.
 *
 *  @returns	The texture used by the layer.
 */
/*virtual*/ Moo::BaseTexturePtr TerrainTextureLayer2::texture() const
{
    return pTexture_;
}

/**
 *	This sets the texture used by the layer.
 *
 *  @param texture	The new texture.
 *  @param textureName	The name of the new texture's resource.
 */
/*virtual*/ void
TerrainTextureLayer2::texture
(
	Moo::BaseTexturePtr		texture,
    string const&			textureName
)
{
	BW_GUARD;
	// TODO: pass the blend sizes from the GUI.
	// Track memory usage
	RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
							(uint)ResourceCounters::SYSTEM),
							(uint)textureName_.capacity())
    textureName_ = textureName;
	RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
							(uint)ResourceCounters::SYSTEM),
							(uint)textureName_.capacity())

	pTexture_    = texture;

    blends_.resize( width_, height_, 0 );

    TerrainTextureLayer::defaultUVProjections(uProjection_, vProjection_);
}
#endif


/**
 *	This is called to determine whether layers of this type support setting UV
 *	projections.
 *
 *  @returns		true.
 */
/*virtual*/ bool TerrainTextureLayer2::hasUVProjections() const
{
    return true;
}


/**
 *	This returns the u-projection of the layer.
 *
 *  @returns		The u-projection axis of the layer.
 */
/*virtual*/ Vector4 const &TerrainTextureLayer2::uProjection() const
{
    return uProjection_;
}


/**
 *	This is used to set the u-projection of the layer.
 *
 *  @param u		The new u axis.
 */
/*virtual*/ void TerrainTextureLayer2::uProjection(Vector4 const &u)
{
    uProjection_ = u;
}


/**
 *	This returns the v-projection of the layer.
 *
 *  @returns		The v-projection axis of the layer.
 */
/*virtual*/ Vector4 const &TerrainTextureLayer2::vProjection() const
{
    return vProjection_;
}


/**
 *	This is used to set the v-projection of the layer.
 *
 *  @param v		The new v axis.
 */
/*virtual*/ void TerrainTextureLayer2::vProjection(Vector4 const &v)
{
    vProjection_ = v;
}


/**
 *	This gets the width of the layer.
 *
 *  @returns		The width of the layer.
 */
/*virtual*/ uint32 TerrainTextureLayer2::width() const
{
    return width_;
}


/**
 *	This gets the height of the layer.
 *
 *  @returns		The height of the layer.
 */
/*virtual*/ uint32 TerrainTextureLayer2::height() const
{
    return height_;
}


#ifdef EDITOR_ENABLED

/**
 *	This is called before the layer's image is read or written to.
 *
 *  @param readOnly	Is the layer being locked for reading?
 *  @returns		True.
 */
/*virtual*/ bool TerrainTextureLayer2::lock(bool readOnly)
{
    BW_GUARD;
	if (lockCount_++ == 0)
	{
#ifdef EDITOR_ENABLED
		if (TTL2Cache::instance() != NULL && (state_ & LOADING) == 0)
			TTL2Cache::instance()->onLock(this, readOnly);
#endif // EDITOR_ENABLED
	}
    return true;
}


/**
 *	This gets the layer's image.  This has to be called within a lock/unlock
 *	pair.
 *
 *  @returns		The layer's image.
 */
/*virtual*/ TerrainTextureLayer2::ImageType &TerrainTextureLayer2::image()
{
    BW_GUARD;
	MF_ASSERT(lockCount_ != 0);
    return blends_;
}


/**
*	This is called when the layer's image is not needed for reading/writing.
*
*  @returns		True.
*/
/*virtual*/ bool TerrainTextureLayer2::unlock()
{
	BW_GUARD;
	if (--lockCount_ == 0)
	{
#ifdef EDITOR_ENABLED
		if (TTL2Cache::instance() != NULL && (state_ & LOADING) == 0)
			TTL2Cache::instance()->onUnlock(this);
		// In the editor, blends can be resized in certain cases, so check
		// the size.
		if ( blends_.width() != width_ ||
			blends_.height() != height_ )
		{
			width_ = blends_.width();
			height_ = blends_.height();
			if (TTL2Cache::instance() != NULL && state_ != LOADING)
				TTL2Cache::instance()->delTextureLayer(this);
		}
#endif // EDITOR_ENABLED
	}
	return true;
}


/**
*	This is called to save the layer.
*
*  @param pSection	The DataSection to save to.
*	@returns		True if successfully saved.
*/
/*virtual*/ bool TerrainTextureLayer2::save(DataSectionPtr pSection) const
{
	BW_GUARD;
	MF_ASSERT( pSection );

	// Compress the blends to PNG format:
	BinaryPtr compData;
#ifdef EDITOR_ENABLED
	if (compressedBlend_ != NULL)
		compData = compressedBlend_;
	else
		compData = compressImage(blends_);
#else
	compData = compressImage(blends_);
#endif

	size_t dataSz = sizeof(BlendHeader) + compData->len() +
		sizeof(uint32) + sizeof(char)*(textureName_.length() + 1);

	std::vector<uint8> data;
	data.resize( dataSz, 0 );

	uint8 *addr = &data.front();

	BlendHeader* bh = (BlendHeader *)addr;
	bh->magic_ = BlendHeader::MAGIC;
	bh->width_ = width_;
	bh->height_ = height_;
	bh->bpp_ = 8;
	bh->uProjection_ = uProjection_;
	bh->vProjection_ = vProjection_;
	bh->version_ = BlendHeader::VERSION_PNG_BLENDS;
	addr += sizeof(BlendHeader);

	uint32 *texLen = (uint32 *)addr;
	*texLen = textureName_.length();
	addr += sizeof(uint32);

	char *tex = (char *)addr;
	::strncpy(tex, textureName_.c_str(), *texLen);
	addr += textureName_.length();

	uint8* pHeight = (uint8*)(addr);
	::memcpy(pHeight, compData->data(), compData->len());

	BinaryPtr binaryBlock = 
		new BinaryBlock(&data.front(), data.size(), "Terrain/TextureLayer2/BinaryBlock");
	pSection->setBinary( binaryBlock );

	return true;
}

#endif // EDITOR_ENABLED

/**
 *	This gets the layer's image.  This has to be called within a lock/unlock
 *	pair.
 *
 *  @returns		The layer's image.
 */
/*virtual*/ TerrainTextureLayer2::ImageType const &TerrainTextureLayer2::image() const
{
#ifdef EDITOR_ENABLED
    MF_ASSERT(lockCount_ != 0);
#endif
	return blends_;
}

/**
 *	This is called to load the layer.
 *
 *  @param dataSection	The DataSection to load from.
 *  @param error		If an error occurs during loading then this is set to
 *						an error string.
 *  @returns			True if successfully loaded.
 */
/*virtual*/ bool TerrainTextureLayer2::load(DataSectionPtr dataSection, string *error /*= NULL*/)
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	state_ = LOADING;
#endif

	bool ret = false;
	BinaryPtr pBinary = dataSection->asBinary();
	if (pBinary)
	{
		const BlendHeader* pHeader = (const BlendHeader*)pBinary->data();
		if (pHeader->magic_ == BlendHeader::MAGIC)
		{
			// Load common parts for version 1 and 2:
			uint8 *blendData = NULL;
			uint32 sizeData = 0;
			if
			(
				pHeader->version_ == BlendHeader::VERSION_RAW_BLENDS
				||
				pHeader->version_ == BlendHeader::VERSION_PNG_BLENDS
			)
			{
				uProjection_ = pHeader->uProjection_;
				vProjection_ = pHeader->vProjection_;
				blends_.resize(pHeader->width_, pHeader->height_);

				const uint32* stringLength = (const uint32*)(pHeader + 1);
				const char* texName = (const char*)(stringLength + 1);

				// Track memory usage
				RESOURCE_COUNTER_SUB(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
										(uint)ResourceCounters::SYSTEM),
										(uint)textureName_.capacity())
				textureName_.assign( texName, texName + *stringLength );

				// Do this to make sure the string length only goes up to the first \0
				textureName_ = textureName_.c_str();

				RESOURCE_COUNTER_ADD(	ResourceCounters::DescriptionPool("Terrain/TextureLayer2",
										(uint)ResourceCounters::SYSTEM),
										(uint)textureName_.capacity())
#ifndef MF_SERVER
				pTexture_ = Moo::TextureManager::instance()->get( textureName_, 
							true, true, true, "Terrain/TextureLayer2/TextureResource" );
#endif
				blendData = (uint8*)(texName + *stringLength );
				sizeData  = pBinary->len() - sizeof(BlendHeader) - 
								sizeof(char)*(*stringLength) - sizeof(uint32);
			}
			// Load blends from version 1 (raw data):
			if (pHeader->version_ == BlendHeader::VERSION_RAW_BLENDS)
			{
				blends_.copyFrom(blendData);
				ret = true;
			}
			// Load blends from version 2 (png compressed):
			else if(pHeader->version_ == BlendHeader::VERSION_PNG_BLENDS)
			{
				BinaryPtr compData = 
					new BinaryBlock
					( 
						blendData, 
						sizeData,
						"Terrain/TextureLayer2/BinaryBlock"
					);

				ret = decompressImage( compData, blends_ );

				if (ret)
				{
					MF_ASSERT(blends_.width()  == pHeader->width_ );
					MF_ASSERT(blends_.height() == pHeader->height_);
				}
				else
				{
					std::string err = "TerrainTextureLayer2::load: "
						"PNG blend data is invalid\n";

					if (error)
						*error = err;

					ERROR_MSG( err.c_str() );
				}

#ifdef EDITOR_ENABLED
				compressedBlend_ = compData;
				state_  = (State)(state_ | BLENDS | COMPRESSED);
#endif

			}
			else
			{
				char buffer[512];
				bw_snprintf( buffer, sizeof(buffer)/sizeof(char),
					"Blend data is incorrect version.  Wanted 1 or 2, got %u\n",
					pHeader->version_ );
				if (error != NULL)
					*error = buffer;
				ERROR_MSG( buffer );
			}
			width_ = pHeader->width_;
			height_ = pHeader->height_;
		}
		else
		{
			std::string err = "Blend data is invalid (incorrect header)\n";
			if (error != NULL)
				*error = err;
			ERROR_MSG( err.c_str() );
		}
	}
	return ret;
}


/**
 *	This is called once the TerrainTextureLayer2 has been loaded and combined
 *	etc.
 */
void TerrainTextureLayer2::onLoaded()
{
	BW_GUARD;	
#ifdef EDITOR_ENABLED
	if ((state_ & COMPRESSED) == 0)
		compressBlend();
	state_ = COMPRESSED;
#endif
	blends_.clear();
}


/**
 *	This returns the underlying block.
 *
 *  @returns		The underlying block.
 */
/*virtual*/ BaseTerrainBlock &TerrainTextureLayer2::baseTerrainBlock()
{
    return *terrainBlock_;
}


/**
 *	This returns the underlying block.
 *
 *  @returns		The underlying block.
 */
/*virtual*/ BaseTerrainBlock const &TerrainTextureLayer2::baseTerrainBlock() const
{
    return *terrainBlock_;
}



#ifdef EDITOR_ENABLED


/**
 *	This is called on a TerrainTextureLayer2 which has not been used lately.
 */
void TerrainTextureLayer2::compressBlend()
{
	BW_GUARD;
	if (!blends_.isEmpty())
	{
		compressedBlend_ = Moo::compressImage(blends_, "Terrain/TextureLayer2/CompressedBlends");

		blends_.clear();
		state_ = COMPRESSED;
	}
}


/**
 *	This is called on a TerrainTextureLayer2 which is about to be edited and hasn't been
 *  used lately.
 */
void TerrainTextureLayer2::decompressBlend()
{
	BW_GUARD;
	if (compressedBlend_ != NULL)
	{
		Moo::decompressImage(compressedBlend_, blends_);

		compressedBlend_ = NULL;
		state_ = BLENDS;
	}
}


/**
 *	This returns the state of the layer - offline, compressed etc.
 *
 *  @returns		The state of the layer.
 */
TerrainTextureLayer2::State TerrainTextureLayer2::state()
{
	return state_;
}

#endif
