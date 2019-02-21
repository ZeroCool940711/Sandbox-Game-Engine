/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef ECOTYPE_HPP
#define ECOTYPE_HPP

namespace Moo
{
	class BaseTerrainBlock;
	typedef SmartPointer<BaseTerrainBlock> BaseTerrainBlockPtr;
	class Visual;
	typedef SmartPointer<Visual> VisualPtr;
	class BaseTexture;
	typedef SmartPointer<BaseTexture> BaseTexturePtr;
}


/**
 *	This class represents a single ecotype in the flora system.
 *	Each ecotype has a unique ID that is baked into the terrain
 *	by the world editor.
 *	Each ecotype may have a single entry in the collated flora
 *	texture map, and is allocated a fixed number of vertices.
 *	Each ectoype has a generator class that provides geometry
 *	at any given location.
 */
class Ecotype
{
public:	
	typedef uint8 ID;
	static const uint32 ID_AUTO = 255;

	Ecotype( class Flora* flora );
	~Ecotype();
	
	void init( DataSectionPtr allEcotypesSection, uint8 id );
	void finz();

	//public member that will be set to false once the resources
	//have been loaded in the backgroudn thread.
	bool isLoading_;
	//public member that will be set to false when the ecotype
	//deallocates itself
	bool isInited_;
	const std::string& soundTag() const	{ return tag_; }
	void offsetUV( Vector2& offset );
	const Vector2& uvOffset() { return uvOffset_; }
	uint32 generate(		
		class FloraVertexContainer* pVerts,
		uint32 id,
		uint32 maxVerts,
		const Matrix& objectToWorld,
		const Matrix& objectToChunk,
		BoundingBox& bb );
	bool isEmpty() const;
	uint8 id() const								{ return id_; }
	std::string& textureResource()					{ return textureResource_; }
	void textureResource( const std::string& t )	{ textureResource_ = t; }
	Moo::BaseTexturePtr pTexture()					{ return pTexture_; }
	void pTexture( Moo::BaseTexturePtr p )			{ pTexture_ = p; }
	void tag( const std::string& t )				{ tag_ = t; }
	void generator( class EcotypeGenerator* g )		{ generator_ = g; }

	class EcotypeGenerator* generator() const	{ return generator_; }

	class Flora* flora() const { return flora_; }

	//reference counting
	void incRef();
	void decRef();

	static SimpleMutex s_deleteMutex_;

private:	
	void grabTexture();
	void freeTexture();
	struct BkLoadInfo
	{					
		class BackgroundTask * loadingTask_;
		Ecotype* ecotype_;
		DataSectionPtr pSection_;
	};
	static void backgroundInit( void * );
	static void onBackgroundInitComplete( void * );
	std::string textureResource_;
	Moo::BaseTexturePtr pTexture_;
	std::string tag_;
	Vector2 uvOffset_;	
	int		refCount_;
	uint8	id_;
	class EcotypeGenerator* generator_;
	class Flora* flora_;
	BkLoadInfo* loadInfo_;
};


#endif //#ifndef ECOTYPE_HPP