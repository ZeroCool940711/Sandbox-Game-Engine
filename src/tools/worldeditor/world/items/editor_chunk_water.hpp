/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef EDITOR_CHUNK_WATER_HPP
#define EDITOR_CHUNK_WATER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "chunk/chunk_water.hpp"


/**
 *	This class is the editor version of a ChunkWater
 */
class EditorChunkWater : public EditorChunkSubstance<ChunkWater>, public Aligned
{	
	static VLOFactory factory_;
	static bool create( Chunk * pChunk, DataSectionPtr pSection, std::string uid );
	
	static ChunkItemFactory	oldWaterFactory_;
	static ChunkItemFactory::Result oldCreate( Chunk * pChunk, DataSectionPtr pSection );
public:
	EditorChunkWater( std::string uid );
	~EditorChunkWater();

	void toss( );
	void dirty();
	bool load( DataSectionPtr pSection, Chunk * pChunk );

	virtual void cleanup();
	virtual void saveFile( Chunk* chunk=NULL );
	virtual void save( );
	virtual void drawRed(bool val);
	virtual void highlight(bool val);
	virtual const Matrix & origin();
	virtual const Matrix & edTransform();	
	virtual const Matrix & localTransform( );
	virtual void edDelete( ChunkVLO* instigator );
	virtual bool edSave( DataSectionPtr pSection );
	virtual const char * edClassName() { return "Water"; }
	virtual const Matrix & localTransform( Chunk * pChunk );
//	virtual bool edTransform( const Matrix & m, bool transient );
	virtual bool edEdit( class ChunkItemEditor & editor, const ChunkItemPtr pItem);
	
	virtual bool visibleInside() const;

	virtual bool visibleOutside() const;

	virtual void draw( ChunkSpace* pSpace );

	static const std::vector<EditorChunkWater*>& instances() { return instances_; }
	static SimpleMutex &instancesMutex() { return instancesMutex_; }

	static void fini();
private:
	EditorChunkWater( const EditorChunkWater& );
	EditorChunkWater& operator=( const EditorChunkWater& );

	virtual void addAsObstacle() { }
	virtual ModelPtr reprModel() const;
	virtual void updateLocalVars( const Matrix & m );
	virtual void updateWorldVars( const Matrix & m );
	virtual const char * sectName() const { return "water"; }
	virtual const char * drawFlag() const { return "render/drawChunkWater"; }	


	bool			changed_;
	Vector2			size2_;
	Vector3			worldPos_;
	float			localOri_;
	Matrix			transform_;
	Matrix			origin_;
	Matrix			scale_;

	static std::vector<EditorChunkWater*> instances_;
	static SimpleMutex					  instancesMutex_;
};


typedef SmartPointer<EditorChunkWater> EditorChunkWaterPtr;


#endif // EDITOR_CHUNK_WATER_HPP
