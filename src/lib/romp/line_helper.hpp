/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef LINE_HELPER_HPP
#define LINE_HELPER_HPP

#include "custom_mesh.hpp"
#include "moo/vertex_formats.hpp"

/**
 *	This helper class manages debug line drawing.
 *	You can add single coloured lines to the list and
 *	it will take care of rendering them at an appropriate
 *	time.
 */
class LineHelper
{
public:
	static const uint32 POINT_LIMIT = 2000;

	LineHelper();
	~LineHelper();

	static LineHelper& instance();
	
	// draw a line in world space, with a colour and an amount of frames to live.
	void drawLine(	const Vector3& start, const Vector3& end, 
					Moo::PackedColour	colour	= 0xffffffff,
					uint32				life	= 0 );

	// draw a strip of lines given points. Nothing happens if length < 2.
	// Caller is responsible for ensuring length is correct.
	void drawLineStrip( const Vector3* points, uint32 length, 
		Moo::PackedColour	colour	= 0xffffffff,
		uint32				life	= 0 );

	// draw a triangle
	void drawTriangle( const Vector3& v0, const Vector3& v1, const Vector3& v2, 
		Moo::PackedColour	colour	= 0xffffffff,
		uint32				life	= 0);
	
	// draw an axis aligned bounding box
	// TODO: optimize by using strips, allow lifetime
	void drawBoundingBox( const BoundingBox& bb, const Matrix& world,
		Moo::PackedColour colour = 0xffffffff );

	// draw a line in screen space
	void drawLineScreenSpace( const Vector4& start, const Vector4& end, 
		Moo::PackedColour colour = 0xffffffff );

	void purge();

private:
	// types 

	// a type which owns a pointer to a mesh and a lifetime. When life = 0,
	// entry should be deleted.
	template <typename MESH_TYPE> struct MeshEntry
	{
		MeshEntry( uint32 life = 1 ) :
			mesh_( D3DPT_LINELIST ),
			life_( life )
		{
		}

		~MeshEntry()
		{
		}
	
		MESH_TYPE	 mesh_;
		uint32		 life_;
	};
	
	// Mesh types 
	typedef CustomMesh<Moo::VertexXYZL> MeshWS;			// world space
	typedef CustomMesh<Moo::VertexTL>	MeshSS;			// screen space

	// a entry type for a world space line
	typedef MeshEntry< MeshWS >			LineEntryWS;
	// list of pointers to entries which represent world space lines
	typedef std::list< LineEntryWS* >	MeshListWS;	
	
	// functions

	// draw each type of line
	void purgeScreenSpace();
	void purgeWorldSpace();
	
	// get a mesh to draw a line in given its lifetime this will create an entry
	// in bufferedLines_ if necessary. Caller doesn't own returned mesh.
	MeshWS* getMeshForLine( uint32 life );

	// data

	// world space buffered and immediate line holders
	MeshListWS					bufferedLines_;
	MeshWS						immediateLines_;		
	// screen space lines
	MeshSS						immediateLinesScreenSpace_;
};

#endif // LINE_HELPER_HPP
