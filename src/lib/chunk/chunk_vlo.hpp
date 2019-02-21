/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef CHUNK_VLO_HPP
#define CHUNK_VLO_HPP

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "chunk_item.hpp"

#include "romp/water.hpp"

/**
 *
 */
//TODO: Combine VLOFactory and ChunkItemFactory
/**
 *	This class is a factory for VLO items, used by the actual VLO (not the references).
 *  e.g. ChunkWater
 */
class VLOFactory
{
public:
	typedef bool (*Creator)( Chunk * pChunk, DataSectionPtr pSection, std::string uid );

	VLOFactory( const std::string & section,
		int priority = 0,
		Creator creator = NULL );

	virtual bool create( Chunk * pChunk, DataSectionPtr pSection, std::string uid ) const;

	int priority() const							{ return priority_; }

private:
	int		priority_;
	Creator creator_;
};

class ChunkVLO;

/**
 * The actual large object, created when a reference is encountered.
 */
class VeryLargeObject;
typedef SmartPointer<VeryLargeObject> VeryLargeObjectPtr;
class VeryLargeObject : public SafeReferenceCount
{
public:
	typedef StringHashMap<VeryLargeObjectPtr> UniqueObjectList;
	typedef std::list<ChunkVLO*> ChunkItemList;

	VeryLargeObject();
	VeryLargeObject( std::string uid, std::string type );
	~VeryLargeObject();

	void setUID( std::string uid );

//TODO: would be much nicer to have an editor class rather than these ifdef's
#ifdef EDITOR_ENABLED	
	ChunkItemList chunkItems() const;

	virtual void cleanup() {}
	virtual void saveFile(Chunk* pChunk=NULL) {}
	virtual void save() {}
	virtual void drawRed(bool val) {}
	virtual void highlight(bool val) {}
	virtual void edDelete( ChunkVLO* instigator );	
	virtual const char * edClassName() { return "VLO"; }
	virtual const Matrix & edTransform() { return Matrix::identity; }
	virtual bool edEdit( class ChunkItemEditor & editor, const ChunkItemPtr pItem ) { return false; }
	virtual bool edShouldDraw();
	virtual std::string type() { return type_; }

	virtual bool isObjectCreated() const;

	virtual bool visibleInside() const { return true; }

	virtual bool visibleOutside() const { return true; }

	static std::string generateUID();

	static void updateSelectionMark() { currentSelectionMark_++; }
	static uint32 selectionMark() { return currentSelectionMark_; }
	virtual bool edCheckMark(uint32 mark)
	{
		if (mark == selectionMark_)
			return false;
		else
		{
			selectionMark_ = mark;
			return true;
		}
	}

	static void deleteUnused();
	static void saveAll();

#endif //EDITOR_ENABLED

	virtual void objectCreated();

	bool shouldRebuild() { return rebuild_; }
	void shouldRebuild( bool rebuild ) { rebuild_ = rebuild; }

	virtual void dirty() {}
	virtual void draw( ChunkSpace* pSpace ) {}
	virtual void lend( Chunk * pChunk ) {}
	virtual void unlend( Chunk * pChunk ) {}
	virtual void updateLocalVars( const Matrix & m ) {}
	virtual void updateWorldVars( const Matrix & m ) {}	
	virtual const Matrix & origin() { return Matrix::identity; }
	virtual const Matrix & localTransform( ) { return Matrix::identity; }	
	virtual const Matrix & localTransform( Chunk * pChunk ) { return Matrix::identity; }
	virtual void sway( const Vector3 & src, const Vector3 & dst, const float diameter ) {}

	virtual BoundingBox chunkBB( Chunk* pChunk ) { return BoundingBox::s_insideOut_; };

	std::string getUID() const { return uid_; }
	// TODO: a dangerous function this is.............fix it I will

	void addItem( ChunkVLO* item );
	void removeItem( ChunkVLO* item);
	BoundingBox& boundingBox() { return bb_; }	
	ChunkVLO* containsChunk( const Chunk * pChunk ) const;
#ifdef EDITOR_ENABLED
	void section( const DataSectionPtr pVLOSection )
	{ 
		dataSection_ = pVLOSection;
	}	
	DataSectionPtr section() { return dataSection_; }

	static VeryLargeObjectPtr getObject( const std::string& uid )	
	{
		// The tool uses UniqueIDs, yet VLOs converts IDs to lowercase, which
		// are then used for filenames, the map, etc, so ensuring lowercase.
		std::string uidTemp = uid;
		std::transform(
				uidTemp.begin(), uidTemp.end(), uidTemp.begin(), tolower );
		return s_uniqueObjects_[uidTemp];
	}
#else

	static VeryLargeObjectPtr getObject( const std::string& uid )	
	{
		return s_uniqueObjects_[uid];
	}
#endif //EDITOR_ENABLED
	
	virtual void syncInit(ChunkVLO* pVLO) {}

	static UniqueObjectList s_uniqueObjects_;
protected:
	friend class EditorChunkVLO;

	std::string chunkPath_;
	BoundingBox bb_;
	std::string uid_;
	std::string type_;
	ChunkItemList itemList_;
#ifdef EDITOR_ENABLED
	DataSectionPtr dataSection_;
	bool listModified_;
	bool objectCreated_;
#endif //EDITOR_ENABLED

private:
	bool rebuild_;
#ifdef EDITOR_ENABLED
	uint32 selectionMark_;	
	static uint32 currentSelectionMark_;
#endif //EDITOR_ENABLED
};


/**
 * Reference to the large object .... lives in a chunk (one per chunk per VLO)
 */
class ChunkVLO : public ChunkItem
{
public:
	explicit ChunkVLO( WantFlags wantFlags = WANTS_NOTHING );
	~ChunkVLO();	

	virtual void draw();		
	virtual void objectCreated() { }
	virtual void lend( Chunk * pChunk );
	virtual void toss( Chunk * pChunk );
	virtual void removeCollisionScene() {}	
	virtual void updateTransform( Chunk * pChunk ) {}
//	virtual bool legacyLoad( DataSectionPtr pSection, Chunk * pChunk, std::string& type );
	virtual void sway( const Vector3 & src, const Vector3 & dst, const float diameter );

	bool load( DataSectionPtr pSection, Chunk * pChunk );

#ifdef EDITOR_ENABLED
	//TODO: move into editorVLO!!!! 
	bool root() const { return creationRoot_; }
	void root(bool val) { creationRoot_ = val; }
	bool createVLO( DataSectionPtr pSection, Chunk* pChunk );
	bool createLegacyVLO( DataSectionPtr pSection, Chunk* pChunk, std::string& type );	
	bool cloneVLO( DataSectionPtr pSection, Chunk* pChunk, VeryLargeObjectPtr pSource );
	bool buildVLOSection( DataSectionPtr pObjectSection, Chunk* pChunk, std::string& type, std::string& uid ); 
#endif //EDITOR_ENABLED

	static bool loadItem( Chunk* pChunk, DataSectionPtr pSection );

	VeryLargeObjectPtr object() const{ return pObject_; }
	static void registerFactory( const std::string & section,
				const VLOFactory & factory );

	static void fini();
	virtual void syncInit();
	
protected:
	VeryLargeObjectPtr		pObject_;
private:
	typedef StringHashMap<const VLOFactory*> Factories;

	bool				dirty_;
	bool				creationRoot_;
	static Factories	*pFactories_;

	// VLO reference factory...
	DECLARE_CHUNK_ITEM( ChunkVLO )
};


#endif // CHUNK_VLO_HPP
