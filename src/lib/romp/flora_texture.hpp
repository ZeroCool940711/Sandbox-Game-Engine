/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef FLORA_TEXTURE_HPP
#define FLORA_TEXTURE_HPP

#define MAX_ECOTYPES	16

/**
 * TODO: to be documented.
 */
class CompressedMipMapCalculator
{
public:
	/**
	 * TODO: to be documented.
	 */
	struct MipData
	{
		uint32 offset_;
		uint32 numBytesPerRow_;
		uint32 numRows_;
		uint32 numBytes_;
	};

	/**
	 * TODO: to be documented.
	 */
	struct DDSData
	{
		uint32 numBytes_;
		uint32 numMipMaps_;
	};

	CompressedMipMapCalculator( bool includeHeader );
	void dimensions( int width, int height );
	int width()	{ return w_; }
	int height()	{ return h_; }
	void mipMap( int mipmapLevel, MipData& ret );
	uint32 ddsSize();
	uint32 dataSize();
	uint32 numMipMaps();
private:
	bool includeHeader_;
	int w_;
	int h_;
	DDSData data_;
	std::vector<MipData> mipmaps_;
};

/**
 *	This class manages one huge texture that is used to draw flora.
 *	When ecotypes come into scope, they are assigned an area of the texture map,
 *	and swapped in.
 *
 *	If we stick to the following rules:
 *	- no more than 16 active ecotypes ( practically 12 is about the limit )
 *	- no more than 512K dynamic texture memory ( give or take around 40K )
 *  (lets say we steal 170K from somewhere for mipmaps)
 *
 *	and assuming we use compressed textures with mip-maps
 *
 *	Then each ecotype can have it's very own
 *	- 256x128 texture map
 *	- 512x64 texture map
 */
class FloraTexture : public Moo::DeviceCallback
{
public:
	FloraTexture();
	~FloraTexture();

	bool	init( DataSectionPtr pSection );
	
	void	activate();
	void	deactivate();

	void	createUnmanagedObjects();
	void	deleteUnmanagedObjects();	

	//Texture management - these are called when ecotypes go into/out of scope.
	Vector2& allocate( uint8 ecotypeID, Moo::BaseTexturePtr pTexture );
	void deallocate( uint8 ecotypeID );

	float	width() const	{ return (float)width_; }
	int		blocksWide() const	{ return numBlocksWide_; }
	int		blocksHigh() const	{ return numBlocksHigh_; }

	void	drawDebug();

private:

	int		widthPerBlock_;		// texture width per ecotype
	int		heightPerBlock_;	// texture height per ecotype
	int		width_;				// width of texture containing all ecotypes
	int		height_;			// height of texture containing all ecotypes
	int		numBlocksWide_;		// number of blocks across the main texture
	int		numBlocksHigh_;		// number of blocks up/down the main texture

	struct Block
	{
		Vector2 offset_;
		int pixelOffsetX_;
		int pixelOffsetY_;
		int ecotype_;
		std::string texName_;
	};

	Block	blocks_[MAX_ECOTYPES];

	void	swapInBlock( int idx, Moo::BaseTexturePtr pTexture );	

	CompressedMipMapCalculator	largeMap_;
	CompressedMipMapCalculator	mediumMap_;
	CompressedMipMapCalculator	smallMap_;

	Moo::EffectConstantValuePtr textureSetter_;
	bool						isActive_;

	//temporary
	BYTE*	textureMemory_;
	ComObjectWrap<DX::Texture>	pTexture_;
};

#endif
