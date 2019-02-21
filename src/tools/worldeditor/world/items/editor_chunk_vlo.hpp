/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef EDITOR_CHUNK_VLO_HPP
#define EDITOR_CHUNK_VLO_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/items/editor_chunk_substance.hpp"
#include "worldeditor/project/world_editord_connection.hpp"
#include "chunk/chunk_vlo.hpp"
#include "resmgr/string_provider.hpp"


/**
 *	This class is the editor version of a ChunkVLO
 */
class EditorChunkVLO : public EditorChunkSubstance<ChunkVLO>, public Aligned
	, BWLock::WorldEditordConnection::Notification
{
	DECLARE_EDITOR_CHUNK_ITEM( EditorChunkVLO )
public:
	EditorChunkVLO( );	
	EditorChunkVLO( std::string type );	
	~EditorChunkVLO();

	bool load( const std::string& uid, Chunk * pChunk );
	bool load( DataSectionPtr pSection, Chunk * pChunk, std::string* errorString = NULL );	
	
	std::string edDescription()
	{
		std::string iDesc;
		if (pObject_)
			return L( "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_VLO/ED_DESCRIPTION", pObject_->edClassName() );
		return L( "WORLDEDITOR/WORLDEDITOR/CHUNK/EDITOR_CHUNK_VLO/VLO_REFENRENCE" );
	}

	virtual bool edCanAddSelection() const; 
	virtual bool edShouldDraw();
	virtual void draw();
	virtual void edPreDelete();
	virtual void edChunkSave();	
	virtual void edPostCreate();
	virtual void edSelectedBox( BoundingBox& bbRet ) const;
	virtual void toss( Chunk * pChunk );
	virtual void removeCollisionScene();
	virtual const Matrix & edTransform();	
	virtual bool edSave( DataSectionPtr pSection );	
	virtual void edCloneSection( Chunk* destChunk, const Matrix& destMatrixInChunk, DataSectionPtr destDS );
	virtual bool edPreChunkClone( Chunk* srcChunk, const Matrix& destChunkMatrix, DataSectionPtr chunkDS );
	virtual bool edIsPositionRelativeToChunk() {	return false;	}
	virtual bool edBelongToChunk();
	virtual void edPostClone( EditorChunkItem* srcItem );
	virtual void updateTransform( Chunk * pChunk );
	virtual bool edEdit( class ChunkItemEditor & editor );
	virtual bool edTransform( const Matrix & m, bool transient );
	virtual bool legacyLoad( DataSectionPtr pSection, Chunk * pChunk, std::string& type );
	virtual void changed();

	virtual bool edCheckMark( uint32 mark ) 
	{ 
		if (pObject_)
			return pObject_->edCheckMark(mark);
		return false;
	}

	static void fini();

protected:
	virtual void objectCreated();

private:
	EditorChunkVLO( const EditorChunkVLO& );
	EditorChunkVLO& operator=( const EditorChunkVLO& );

	void updateWorldVars( const Matrix & m );
	void updateLocalVars( const Matrix & m, Chunk * pChunk );	

	virtual void addAsObstacle() { }	
	virtual ModelPtr reprModel() const;
	virtual const char * sectName() const { return "vlo"; }
	virtual const char * drawFlag() const { return "render/drawVLO"; }
	virtual bool edAffectShadow() const {	return pOwnSect()->readString("type") != "water"; }

	std::string		uid_;
	std::string		type_;
	Vector3			localPos_;
	bool			colliding_;
	Matrix			transform_;
	Matrix			vloTransform_;	
	bool			drawTransient_;
	bool			readonly_;
	mutable bool	highlight_;
};


typedef SmartPointer<EditorChunkVLO> EditorChunkVLOPtr;


#endif // EDITOR_CHUNK_VLO_HPP
