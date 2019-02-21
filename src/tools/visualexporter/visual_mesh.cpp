/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4530 )

#include "windows.h"
#include <algorithm>
#include "mfxexp.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/xml_section.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/quick_file_writer.hpp"
#include "morpher_holder.hpp"
#include "utility.hpp"
#include "expsets.hpp"
#include "wm3.h"
#include "ieditnormals.h"

#define OPTIMISATION (1)

#include <iterator>
#include <map>
#include <set>

#if MAX_RELEASE < 6000
#define MAX5
#endif

#ifndef MAX5
#include "idxmaterial.h"
#include "bmmlib.h"
#include "iparamm2.h"
#endif


#include "visual_mesh.hpp"

#include "NVMeshMender.h"
#include "iparamb2.h"


DECLARE_DEBUG_COMPONENT2( "VisualMesh", 0 );
static AutoConfigString s_defaultMaterial( "exporter/defaultMaterial" );

static std::map<VisualMesh::BloatVertex,std::set<VisualMesh::BVVector::size_type> > verticeSetMap;
static std::map<int,std::vector<VisualMesh::BVVector::size_type> > verticeIndexSetMap;

static inline uint32 packNormal( const Point3&nn )
{
	Point3 n = nn;
	n.Normalize();

	n.x = Math::clamp(-1.f, n.x, 1.f);
	n.y = Math::clamp(-1.f, n.y, 1.f);
	n.z = Math::clamp(-1.f, n.z, 1.f);

	if (ExportSettings::instance().useLegacyOrientation())
	{
		return	( ( ( (uint32)(-n.y * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.z * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(-n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}
	else
	{
		return	( ( ( (uint32)(n.y * 511.0f) )  & 0x3ff ) << 22L ) |
				( ( ( (uint32)(n.z * 1023.0f) ) & 0x7ff ) << 11L ) |
				( ( ( (uint32)(n.x * 1023.0f) ) & 0x7ff ) <<  0L );
	}		
}

namespace
{
	bool endsWith( const std::string& s, const std::string& postfix )
	{
		if (s.length() < postfix.length())
			return false;

		return (s.substr(s.length() - postfix.length()) == postfix);
	}
}

TimeValue timeNow()
{
	return ExportSettings::instance().staticFrame() * GetTicksPerFrame();
}

// -----------------------------------------------------------------------------
// Section: VisualMesh
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualMesh::VisualMesh() :
	identifier_( "Scene Root" ),
	snapVertices_( false ),
	largeIndices_( false )
{
}


/**
 *	Destructor.
 */
VisualMesh::~VisualMesh()
{
}


/**
 *	Returns all resources used.
 *
 *	@param	retResources	The vector of all resources.
 */
void VisualMesh::resources( std::vector<std::string>& retResources )
{
	MaterialVector::iterator it = materials_.begin();
	MaterialVector::iterator end = materials_.end();
	while (it != end)
	{
		Material& m = *it++;
		m.resources( retResources );
	}
}


/**
 * This method finds the angle between two edges in a triangle.
 *
 * @param tri	The triangle.
 * @param index	The vertex reference index to find the angle from.
 * @return		The angle (in radians) between the two edges.
 */
float VisualMesh::normalAngle( const Triangle& tri, uint32 index )
{
	Point3 v1 = vertices_[tri.index[(index + 1 ) % 3]].pos - vertices_[tri.index[index]].pos;
	Point3 v2 = vertices_[tri.index[(index + 2 ) % 3]].pos - vertices_[tri.index[index]].pos;


	float len = Length( v1 );
	if (len == 0)
		return 0;
	v1 /= len;

	len = Length( v2 );
	if (len == 0)
		return 0;
	v2 /= len;

	float normalAngle = DotProd( v1, v2 );

	normalAngle = min( 1.f, max( normalAngle, -1.f ) );

	return acosf( normalAngle );	
}


/**
 *	This method adds a normal to all vertices that should be influenced by it.
 *
 *	@param normal the normal to add
 *	@param realIndex the index of the vertex in the max mesh.
 *	@param smoothingGroup the smoothinggroup of this normal.
 *	@param index the index of a vertex this normal has to influence.
 */
void VisualMesh::addNormal( const Point3& normal, int realIndex, int smoothingGroup, int index )
{
	if( verticeIndexSetMap.find( realIndex ) != verticeIndexSetMap.end() )
	{
		std::vector<BVVector::size_type>& verticeIndexSet = verticeIndexSetMap[ realIndex ];
		for( std::vector<BVVector::size_type>::iterator iter = verticeIndexSet.begin();
			iter != verticeIndexSet.end(); ++iter )
		{
			BloatVertex& bv = vertices_[ *iter ];
			if( bv.smoothingGroup & smoothingGroup || ((*iter) == index) )
				bv.normal += normal;
		}
	}
}


/**
 *	This method creates vertex normals for all vertices in the vertex list.
 */
void VisualMesh::createNormals( )
{
	TriangleVector::iterator it = triangles_.begin();
	TriangleVector::iterator end = triangles_.end();

	while (it!=end)
	{
		Triangle& tri = *it++;
		Point3 v1 = vertices_[tri.index[1]].pos - vertices_[tri.index[0]].pos;
		Point3 v2 = vertices_[tri.index[2]].pos - vertices_[tri.index[0]].pos;
		Point3 normal = v1^v2;
		normal = Normalize( normal );

		addNormal( normal * normalAngle( tri, 0), tri.realIndex[0], tri.smoothingGroup, tri.index[0] );
		addNormal( normal * normalAngle( tri, 1), tri.realIndex[1], tri.smoothingGroup, tri.index[1] );
		addNormal( normal * normalAngle( tri, 2), tri.realIndex[2], tri.smoothingGroup, tri.index[2] );
	}

	BVVector::iterator vit = vertices_.begin();
	BVVector::iterator vend = vertices_.end();
	while (vit != vend)
	{
		BloatVertex& bv = *vit++;
		bv.normal = Normalize( bv.normal );
	}
}


/**
 *	This method extracts the normals from the Normals modifier for
 *	pNode.
 *
 *	@param	pNode	The node to extract normal information from.
 */
void VisualMesh::getNormalsFromModifier( INode* pNode )
{
	IEditNormalsMod* pNormsMod = MFXExport::findEditNormalsMod( pNode );
	if (pNormsMod)
	{
		DEBUG_MSG( "MFXExport::preProcess - found edit normals modifier on object %s\n", pNode->GetName() );
		int nFaces = pNormsMod->EnfnGetNumFaces();

		for (int i = 0; i < nFaces; i++)
		{
			int faceDegree = pNormsMod->EnfnGetFaceDegree( i );
			std::vector< int > face;
			std::map< int, Point3* > vertexNormals;
			for (int j = 0; j < faceDegree; j++)
			{
				face.push_back( pNormsMod->EnfnGetVertexID( i, j ) );
				Point3* norm = NULL;
				if (pNormsMod->EnfnGetFaceNormalSpecified( i, j ))
				{
					int nID = pNormsMod->EnfnGetNormalID( i, j );
					norm = pNormsMod->EnfnGetNormal( nID, pNode, timeNow() );
				}
				vertexNormals[face.back()] = norm;
			}

			TriangleVector::iterator it = triangles_.begin();
			TriangleVector::iterator end = triangles_.end();
			int nMatching = 0;
			while (it != end)
			{
				if (std::find( face.begin(), face.end(), it->realIndex[0] ) != face.end() &&
					std::find( face.begin(), face.end(), it->realIndex[1] ) != face.end() &&
					std::find( face.begin(), face.end(), it->realIndex[2] ) != face.end())
				{
					if (vertexNormals[it->realIndex[0]] != NULL)
					{
						int ind = vertices_.size();
						vertices_.push_back( vertices_[it->index[0]] );
						vertices_.back().normal = Normalize(*vertexNormals[it->realIndex[0]]);
						it->index[0] = ind;
					}
					if (vertexNormals[it->realIndex[1]] != NULL)
					{
						int ind = vertices_.size();
						vertices_.push_back( vertices_[it->index[1]] );
						vertices_.back().normal = Normalize(*vertexNormals[it->realIndex[1]]);
						it->index[1] = ind;
					}
					if (vertexNormals[it->realIndex[2]] != NULL)
					{
						int ind = vertices_.size();
						vertices_.push_back( vertices_[it->index[2]] );
						vertices_.back().normal = Normalize(*vertexNormals[it->realIndex[2]]);
						it->index[2] = ind;
					}
				}
				it++;
			}
		}
	}
}


/**
 *	This method gets a max material and stores the material
 *	data we want to export.
 *
 *	@param	mtl	The max material being added.
 */
void VisualMesh::addMaterial( StdMat* mtl )
{
	Material m;
	m.ambient = mtl->GetAmbient( timeNow() );
	m.diffuse = mtl->GetDiffuse( timeNow() );
	m.specular = mtl->GetSpecular( timeNow() );
	m.selfIllum = mtl->GetSelfIllum( timeNow() );
	m.identifier = mtl->GetName();

	m.mapIdMeaning = 0;
	if( mtl->MapEnabled( ID_DI ) )
	{
		Texmap *map = mtl->GetSubTexmap( ID_DI );
		if (map && map->ClassID() == Class_ID( BMTEX_CLASS_ID, 0x00 ))
		{
			std::string submapName =
				unifySlashes( toLower( std::string( map->GetName() ) ) );
			std::string textureName =
					unifySlashes( ((BitmapTex *)map)->GetMapName() );

			if (submapName.length() > 4 &&
				submapName.substr( submapName.length()-4 ) == ".mfm")
			{
				m.mapIdentifier = submapName;
				m.mapIdMeaning = 2;
			}
			else
			{
				textureName = BWResolver::dissolveFilename( textureName );
				if (textureName.length())
				{
					m.mapIdentifier = textureName;
					m.mapIdMeaning = 1;
				}
			}
		}
	}

	materialRemap_.push_back( this->findOrAddMaterial( m ) );
}


/**
 *	This method gets a DirectX material and stores the material
 *	data we want to export.
 *
 *	@param	mtl	The DirectX material being added.
 */
void VisualMesh::addDXMaterial( Mtl* mtl )
{
#ifndef MAX5
	Material m;
	m.identifier = mtl->GetName();
	IDxMaterial * idxm = (IDxMaterial*)mtl->GetInterface(IDXMATERIAL_INTERFACE);
	m.fxFile_ = idxm->GetEffectFilename();
	IParamBlock2* pParamBlock = mtl->GetParamBlock(0);
	int nParams = pParamBlock->NumParams();
	TCHAR* localname = pParamBlock->GetLocalName();
	// iterate over the parameters
	for (int i = 0; i < nParams; i++)
	{
		// The parameter definitions contains information about how to 
		// interpret the parameter data.
		ParamDef& def = pParamBlock->GetParamDef(i);

        // If lightDir or lightColor, continue
        std::string defName = std::string( def.int_name );
        if ( defName == "lightDir" || defName == "lightColor" )
            continue;

		switch (def.type)
		{
			case TYPE_BOOL:
			{
				BOOL b = pParamBlock->GetInt( i );
				m.boolOverrides_.push_back( std::make_pair( std::string( def.int_name ), b  ) );
				break;
			}
			case TYPE_FLOAT:
			{
				float f = pParamBlock->GetFloat( i );
				m.floatOverrides_.push_back( std::make_pair( std::string( def.int_name ), f  ) );
				break;
			}
			case TYPE_INT:
			{
				int j = pParamBlock->GetInt( i );
				m.intOverrides_.push_back( std::make_pair( std::string( def.int_name ), j ) );
				break;
			}
			case TYPE_FRGBA:
			{
				Point4 v = pParamBlock->GetPoint4( i );
				m.vectorOverrides_.push_back( std::make_pair( std::string( def.int_name ), v ) );
				break;
			}
			case TYPE_POINT4:
			{
				Point4 v = pParamBlock->GetPoint4( i );
				m.vectorOverrides_.push_back( std::make_pair( std::string( def.int_name ), v ) );
				break;
			}
			case TYPE_BITMAP:
			{
				PBBitmap* b = pParamBlock->GetBitmap( i );
				if (b && std::string(b->bi.Name()) != std::string("None"))
					m.textureOverrides_.push_back( std::make_pair( std::string( def.int_name ), std::string( b->bi.Name() ) ) );
				break;
			}
		}
	}
	
	materialRemap_.push_back( this->findOrAddMaterial( m ) );
#endif
}


/**
 *	This method gathers all the materials contained in a specific node.
 *
 *	@param	node	The node to gather materials from.
 */
void VisualMesh::gatherMaterials( INode* node )
{
	Mtl* mtl = node->GetMtl();
	if (mtl)
	{
		if (mtl->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
		{
			addMaterial( (StdMat*)mtl );
		}
#ifndef MAX5
		else if (mtl->GetInterface(IDXMATERIAL_INTERFACE))
		{
			addDXMaterial( mtl );
		}
#endif
		else if (mtl->NumSubMtls())
		{
			for (int i = 0; i < mtl->NumSubMtls(); i++)
			{
				Mtl* submat = mtl->GetSubMtl( i );
				if (submat == NULL)
				{
					Material m;
					materialRemap_.push_back( this->findOrAddMaterial( m ) );
				}
				else if (submat->ClassID() == Class_ID(DMTL_CLASS_ID, 0))
				{
					addMaterial( (StdMat*)submat );
				}
#ifndef MAX5
				else if (submat->GetInterface(IDXMATERIAL_INTERFACE))
				{
					addDXMaterial( submat );
				}
#endif
				else
				{
					Material m;
					m.identifier = submat->GetName();
					materialRemap_.push_back( this->findOrAddMaterial( m ) );
				}
			}
		}
		else
		{
			Material m;
			m.identifier = mtl->GetName();
			materialRemap_.push_back( this->findOrAddMaterial( m ) );
		}
	}
	else
	{
		Material m;
		materialRemap_.push_back( this->findOrAddMaterial( m ) );
	}

	if (!materialRemap_.size())
	{
		materialRemap_.push_back( 0 );
	}
}


/**
 *	This method finds or adds the given visual material to our list.
 *
 *	@param	mat	Material to be added.
 *	@return	Index of mat in materials_.
 */
int VisualMesh::findOrAddMaterial( const Material & mat )
{
	uint32 i;
	for (i = 0; i < materials_.size(); i++)
	{
		if (materials_[i] == mat) return i;
	}

	materials_.push_back( mat );
	return i;
}


/**
 *	This method swaps the triangle winding order for all triangles.
 */
void VisualMesh::flipTriangleWindingOrder()
{
	TriangleVector::iterator it = triangles_.begin();
	TriangleVector::iterator end = triangles_.end();

	while (it!=end)
	{
		Triangle& tri = *it++;
		swap( tri.index[1], tri.index[2] );
		swap( tri.realIndex[1], tri.realIndex[2] );
	}
}


VisualMesh::BVVector* g_verts = 0;

bool triangleSorter( const VisualMesh::Triangle& t1, const VisualMesh::Triangle& t2 )
{
	//return t1.materialIndex < t2.materialIndex;

	if (t1.materialIndex == t2.materialIndex)
		return (*g_verts)[t1.index[0]].meshIndex < (*g_verts)[t2.index[0]].meshIndex;
	else
		return t1.materialIndex < t2.materialIndex;
}


/**
 *	This method sorts the triangles.
 */
void VisualMesh::sortTriangles()
{
	g_verts = &vertices_;
	std::sort( triangles_.begin(), triangles_.end(), triangleSorter );

	if (triangles_.size())
	{
		std::vector<int> vertexIndices(vertices_.size(), -1);
		BVVector vs;

		int mIndex = triangles_.front().materialIndex;
		for (int i = 0; i < triangles_.size(); i++)
		{
			Triangle& tri = triangles_[i];
			if (tri.materialIndex != mIndex)
			{
				mIndex = tri.materialIndex;
				vertexIndices.resize( 0 );
				vertexIndices.resize( vertices_.size(), -1 );
			}
			for (int j = 0; j < 3; j++)
			{
				int ind = tri.index[j];
				if (vertexIndices[ind] == -1)
				{
					vertexIndices[ind] = vs.size();
					vs.push_back( vertices_[ind] );
				}
				tri.index[j] = vertexIndices[ind];
			}
		}
		vertices_ = vs;
	}
}


/**
 *	This method creates our primitive groups.
 *
 *	@param	primGroups	The output primitivegroups.
 *	@param	indices		The output indices.
 */
bool VisualMesh::createPrimitiveGroups( PGVector& primGroups, Moo::IndicesHolder& indices )
{
	if (triangles_.size())
	{
		int mIndex = triangles_.front().materialIndex;

		int firstVertex = triangles_.front().index[0];
		int lastVertex = triangles_.front().index[0];
		int firstTriangle = 0;

		int offset = 0;
		indices.setSize( triangles_.size() * 3, Moo::IndicesReference::bestFormat( vertices_.size() ) );

		for (int i = 0; i <= triangles_.size(); i++)
		{
			Triangle& tri = i == triangles_.size() ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || tri.materialIndex != mIndex)
			{
				Moo::PrimitiveGroup pg;
				pg.startVertex_ = firstVertex;
				pg.nVertices_ = lastVertex - firstVertex + 1;
				pg.startIndex_ = firstTriangle * 3;
				pg.nPrimitives_ = i - firstTriangle;
				primGroups.push_back( pg );	
				firstVertex = tri.index[0];
				lastVertex = tri.index[0];
				firstTriangle = i;
				materials_[ mIndex ].inUse = true;
				mIndex = tri.materialIndex;
			}
			if (i!=triangles_.size())
			{
				indices.set( offset, tri.index[0] );
				++offset;
				indices.set( offset, tri.index[1] );
				++offset;
				indices.set( offset, tri.index[2] );
				++offset;
				firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
				lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
			}
		}

		//Mesh particles add an extra 15 primitive groups, defined by the index
		//of the vertices.
		if (ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)
		{
			return this->createMeshParticlePrimGroups(primGroups);		
		}
	}
	return true;
}


/**
 *	This method adds 15 primitive groups, based on meshIndex.
 *	This allows the mesh particles to be drawn all together (prim group 0)
 *	or one-by-one (prim group 1 - 15 incl.), which is used when doing
 *	slower, correct sorting of alpha-blended mesh particles.
 *
 *	@param	primGroups	The vector of prim groups.
 *	@return	Success or failure.
 */
bool VisualMesh::createMeshParticlePrimGroups( PGVector& primGroups )
{
	//Mesh particles should have one primitive group, representing the
	//entire mesh.
	//MF_ASSERT( primGroups.size() == 1 )
	if (primGroups.size() != 1)
	{
		MessageBox( GetForegroundWindow(),
					"VisualMesh::createMeshParticlePrimGroups - Mesh particles must use only 1 material/mesh.\n",
					"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
		return false;
	}

	if (triangles_.size())
	{
		Triangle& tri = triangles_[0];
		
		int firstVertex = tri.index[0];
		int lastVertex = firstVertex;
		int mIndex = vertices_[firstVertex].meshIndex;
		int firstTriangle = 0;

		for (int i = 0; i <= triangles_.size(); i++)
		{
			Triangle& tri = i == triangles_.size() ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || vertices_[tri.index[0]].meshIndex != mIndex)
			{
				Moo::PrimitiveGroup pg;
				pg.startVertex_ = firstVertex;
				pg.nVertices_ = lastVertex - firstVertex + 1;
				pg.startIndex_ = firstTriangle * 3;
				pg.nPrimitives_ = i - firstTriangle;
				primGroups.push_back( pg );

				firstVertex = lastVertex = tri.index[0];
				mIndex = vertices_[firstVertex].meshIndex;				
				firstTriangle = i;
			}

			firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
			lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
		}		
	}

	int nPrimGroups = primGroups.size();

	//Mesh particles should now always have 16 primitive groups.
	//Made this an assert because the MeshParticleRenderer assumes
	//there are 16 primitivegroups.
	MF_ASSERT( primGroups.size() == 16 )

	return true;
}


/**
 *	This method creates our output vertex list.
 *
 *	@vertices The output vertex list.
 */
void VisualMesh::createVertexList( VertexVector& vertices )
{
	VertexXYZNUV v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		if (ExportSettings::instance().useLegacyOrientation())
		{
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
			v.normal[0] = -bv.normal.x;
			v.normal[1] = bv.normal.z;
			v.normal[2] = -bv.normal.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
			v.normal[0] = bv.normal.x;
			v.normal[1] = bv.normal.z;
			v.normal[2] = bv.normal.y;
		}

		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		vertices.push_back( v );
	}
}


/**
 *	This method creates our output vertex list.
 *
 *	@vertices The output vertex list.
 */
void VisualMesh::createVertexList( IndexedVertexVector& vertices )
{
	VertexXYZNUVI v;

	BVVector::iterator lastit = vertices_.begin();
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		MF_ASSERT( lastit->meshIndex <= it->meshIndex );
		lastit = it;

		BloatVertex& bv = *it++;

		bv.pos *= ExportSettings::instance().unitScale();

		if (ExportSettings::instance().useLegacyOrientation())
		{
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
			v.normal[0] = -bv.normal.x;
			v.normal[1] = bv.normal.z;
			v.normal[2] = -bv.normal.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
			v.normal[0] = bv.normal.x;
			v.normal[1] = bv.normal.z;
			v.normal[2] = bv.normal.y;
		}

		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.index = (float) bv.meshIndex * 3;

		vertices.push_back( v );		
	}
}


/**
 *	This method creates our output vertex list.
 *
 *	@vertices The output vertex list.
 */
void VisualMesh::createVertexList( TBVertexVector& vertices )
{
	VertexXYZNUVTB v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		if (ExportSettings::instance().useLegacyOrientation())
		{
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
		}

		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.tangent = packNormal( bv.tangent );
		v.binormal = packNormal( bv.binormal );
		vertices.push_back( v );
	}
}


/**
 *	This method creates our output vertex list.
 *
 *	@vertices The output vertex list.
 */
void VisualMesh::createVertexList( IndexedTBVertexVector& vertices )
{
	VertexXYZNUVITB v;

	BVVector::iterator lastit = vertices_.begin();
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		MF_ASSERT( lastit->meshIndex <= it->meshIndex );
		lastit = it;

		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();

		if (ExportSettings::instance().useLegacyOrientation())
		{
			v.pos[0] = -bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = -bv.pos.y;
		}
		else
		{
			v.pos[0] = bv.pos.x;
			v.pos[1] = bv.pos.z;
			v.pos[2] = bv.pos.y;
		}

		v.normal = packNormal( bv.normal );
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.tangent = packNormal( bv.tangent );
		v.binormal = packNormal( bv.binormal );
		v.index = (float) bv.meshIndex * 3;

		vertices.push_back( v );		
	}
}


struct PG
{
	VisualMesh::BVVector verts;
	VisualMesh::TriangleVector tris;
};


/**
 *	This method calculates the tangents and binormals for each
 *	vertex in the mesh.
 */
void VisualMesh::makeBumped()
{
	if (triangles_.size())
	{

		// separate the primitive groups.
		std::vector< PG > pgs;

		int mIndex = triangles_.front().materialIndex;

		int firstVertex = triangles_.front().index[0];
		int lastVertex = triangles_.front().index[0];
		int firstTriangle = 0;

		for (int i = 0; i <= triangles_.size(); i++)
		{
			Triangle& tri = i == triangles_.size() ? triangles_.back() : triangles_[i];
			if (i == triangles_.size() || tri.materialIndex != mIndex)
			{
				PG pg;
				pg.verts.assign( vertices_.begin() + firstVertex, vertices_.begin() + lastVertex + 1 );
				for (int j = firstTriangle; j < i; j++)
				{
					Triangle t = triangles_[ j ];
					t.index[0] -= firstVertex;
					t.index[1] -= firstVertex;
					t.index[2] -= firstVertex;
					pg.tris.push_back( t );
				}

				if (i != triangles_.size())
				{
					firstVertex = tri.index[0];
					lastVertex = tri.index[0];
					firstTriangle = i;
					mIndex = tri.materialIndex;
				}

				pgs.push_back( pg );
			}
			if (i!=triangles_.size())
			{
				firstVertex = min( firstVertex, min( tri.index[0], min( tri.index[1], tri.index[2] ) ) );
				lastVertex  = max( lastVertex,  max( tri.index[0], max( tri.index[1], tri.index[2] ) ) );
			}
		}
		
		// bump map the primitivegroups individually
		for (int i = 0; i < pgs.size(); i++)
		{
			PG& p = pgs[i];
			TriangleVector& tris = p.tris;
			BVVector& verts = p.verts;

			std::vector< MeshMender::Vertex > theVerts;
			std::vector< unsigned int > theIndices;
			std::vector< unsigned int > mappingNewToOld;
		
			for (int j = 0; j < verts.size(); j++)
			{
				MeshMender::Vertex v;
				v.pos.x = verts[j].pos.x;
				v.pos.y = verts[j].pos.z;
				v.pos.z = verts[j].pos.y;
				v.normal.x = verts[j].normal.x;
				v.normal.y = verts[j].normal.z;
				v.normal.z = verts[j].normal.y;
				v.s = verts[j].uv.x;
				v.t = 1.f - verts[j].uv.y;
				//meshmender will computer normals, tangents, and binormals, no need to fill those in.
				//however, if you do not have meshmender compute the normals, you _must_ pass in valid
				//normals to meshmender
				theVerts.push_back(v);
			}

			for (int j = 0; j < tris.size(); j++)
			{
				theIndices.push_back( tris[j].index[0] );
				theIndices.push_back( tris[j].index[1] );
				theIndices.push_back( tris[j].index[2] );
			}

			MeshMender mender;
			mender.Mend( theVerts, theIndices, mappingNewToOld,
				0.5f, 0.5f, 0.5f, 
				0,
				MeshMender::DONT_CALCULATE_NORMALS,
				MeshMender::DONT_RESPECT_SPLITS,
				ExportSettings::instance().fixCylindrical() ? 
						MeshMender::FIX_CYLINDRICAL : 
						MeshMender::DONT_FIX_CYLINDRICAL );

			BVVector newVerts; // new vector as we still need to copy the extra data from verts
			for( int j = 0; j < (int)theVerts.size(); ++j )
			{
				BloatVertex bv;
				bv.pos.x = theVerts[j].pos.x;
				bv.pos.y = theVerts[j].pos.z;
				bv.pos.z = theVerts[j].pos.y;
				bv.normal.x = theVerts[j].normal.x;
				bv.normal.y = theVerts[j].normal.z;
				bv.normal.z = theVerts[j].normal.y;
				bv.binormal.x = theVerts[j].binormal.x;
				bv.binormal.y = theVerts[j].binormal.z;
				bv.binormal.z = theVerts[j].binormal.y;
				bv.tangent.x = theVerts[j].tangent.x;
				bv.tangent.y = theVerts[j].tangent.z;
				bv.tangent.z = theVerts[j].tangent.y;
				bv.uv.x = theVerts[j].s;
				bv.uv.y = 1.f - theVerts[j].t;
				bv.smoothingGroup = verts[ mappingNewToOld[j] ].smoothingGroup;
				bv.vertexIndex = verts[ mappingNewToOld[j] ].vertexIndex;
				newVerts.push_back( bv );
			}

			verts.clear();

			for( int j = 0; j < (int)newVerts.size(); ++j )
				verts.push_back( newVerts[j] );

			Triangle tri = tris.front();
			tris.clear();
			for (int j = 0; j < theIndices.size(); j += 3)
			{
				tri.index[0] = theIndices[j];
				tri.index[1] = theIndices[j + 1];
				tri.index[2] = theIndices[j + 2];
				tris.push_back( tri );
			}
		}
		
		// stitch the primitivegroups back together
		vertices_.clear();
		triangles_.clear();

		for (int i = 0; i < pgs.size(); i++)
		{
			PG& p = pgs[i];
			TriangleVector& tris = p.tris;
			BVVector& verts = p.verts;
			int firstVert = vertices_.size();

			vertices_.insert( vertices_.end(), verts.begin(), verts.end() );
			for (int j = 0; j < tris.size(); j++)
			{
				Triangle tri = tris[j];
				tri.index[0] += firstVert;
				tri.index[1] += firstVert;
				tri.index[2] += firstVert;
				triangles_.push_back( tri );
			}
		}
	}
}

static bool closeEnough( float a, float b )
{
	return fabs( a - b ) < 0.0001f;
}

static bool pointCloseEnough( Point3 a, Point3 b )
{
	return closeEnough( a.x, b.x )
		&& closeEnough( a.y, b.y )
		&& closeEnough( a.z, b.z );
}

static bool pointCloseEnough( Point2 a, Point2 b )
{
	return closeEnough( a.x, b.x )
		&& closeEnough( a.y, b.y );
}

static bool directionCloseEnough( Point3 a, Point3 b )
{
	// 0.02 == 2 * pi / 360 (roughly);
	return (DotProd( a, b ) > cos(0.02)) || (a == b);
}

static bool closeEnough( const VisualMesh::BloatVertex& a, const VisualMesh::BloatVertex& b )
{
	return  pointCloseEnough( a.pos, b.pos ) &&
		directionCloseEnough( a.normal, b.normal ) &&
		pointCloseEnough( a.uv, b.uv ) &&
//		a.smoothingGroup == b.smoothingGroup &&
		directionCloseEnough( a.binormal, b.binormal ) &&
		directionCloseEnough( a.tangent, b.tangent ) &&
		a.meshIndex == b.meshIndex;
}

#define TC_OPTIMISATION
#ifdef TC_OPTIMISATION
#include <hash_map>

inline uint64 hashKey( const Point3& v )
{
	uint64 x = uint64(v.x);
	uint64 y = uint64(v.y);
	uint64 z = uint64(v.z);
	x <<= 20;
	y <<= 10;
	return (x + y + z);
}
#endif//TC_OPTIMISATION
// }


/**
 *	This method removes duplicate vertices.
 */
void VisualMesh::removeDuplicateVertices()
{
// Raymond : Merge TC's optimisation {
#ifdef TC_OPTIMISATION
	int j1 = 0;
// In VS 2002, hash_multimap is inside namespace std
// In VS 2003, hash_multimap is inside namespace stdext
// I cannot find an elegant way for that, so MACRO is the rescue
#if	_MSC_VER < 1300
	#error this file is intended to be compiled with VC 2002 or above
#endif//_MSC_VER < 1300

#if _MSC_VER > 1300
	stdext::hash_multimap <uint64, int> vertexMap;
	stdext::hash_multimap <uint64, int> :: const_iterator lowerBound, upperBound, iter;
	stdext::hash_map <int, int> vertexRemap;
#else //_MSC_VER > 1300
	std::hash_multimap <uint64, int> vertexMap;
	std::hash_multimap <uint64, int> :: const_iterator lowerBound, upperBound, iter;
	std::hash_map <int, int> vertexRemap;
#endif//_MSC_VER > 1300
	typedef std::pair <uint64, int> Int64_Pair;
	typedef std::pair <int, int> Int_Pair;
	for (int j1 = 0; j1 < vertices_.size(); j1++)
	{
		BloatVertex& bv1 = vertices_[j1];
		uint64 key = hashKey( bv1.pos );
		int found = -1;
		lowerBound = vertexMap.lower_bound( key );
		upperBound = vertexMap.upper_bound( key );
		for (iter = lowerBound; iter != upperBound; iter++)
		{
			int hashedIndex = iter->second;
			BloatVertex& bv2 = vertices_[ hashedIndex ];
			if (closeEnough( bv1, bv2 ))
			{
				found = hashedIndex;
				break;
			}
		}
		if (found == -1)
		{
			vertexMap.insert( Int64_Pair( key, j1 ) );
			vertexRemap.insert( Int_Pair( j1, j1 ) );
		}
		else
			vertexRemap.insert( Int_Pair( j1, found ) );
	}
#endif//TC_OPTIMISATION
// }

	int nDupes = 0;
	// iterate through all triangles
	for (int i = 0; i < (int)triangles_.size(); ++i)
	{
		Triangle& tri = triangles_[i];
// Raymond : Merge TC's optimisation {
#ifdef TC_OPTIMISATION
		tri.index[0] = vertexRemap[tri.index[0]];
		tri.index[1] = vertexRemap[tri.index[1]];
		tri.index[2] = vertexRemap[tri.index[2]];
#endif//TC_OPTIMISATION
#ifndef TC_OPTIMISATION
// }


		// for each vertex
		for (int j = 0; j < 3; ++j)
		{
			// find if there is an earlier vertex that matches
			for (int k = 0; k < tri.index[j]; ++k)
			{
				// make the triangle point at that one instead
				if (closeEnough( vertices_[tri.index[j]], vertices_[k] ))
					tri.index[j] = k;
			}
		}
// Raymond : Merge TC's optimisation {
#endif//TC_OPTIMISATION
// }
	}

	// (sortTriangles will removed the unreferenced vertices)
}


/**
 *	This method saves the visual mesh.
 *
 *	@param pVisualSection	DataSection of the .visual file.
 *	@param spExistingVisual	DataSection of the existing visual to get the material
							from.
 *	@param primitiveFile	The filename of the primitive record.
 *	@param useIdentifier	Flag use identifier.
 */
bool VisualMesh::save( DataSectionPtr pVisualSection,
	DataSectionPtr spExistingVisual,
	const std::string& primitiveFile,
	bool useIdentifier )
{
	std::string idPrefix;
	if (useIdentifier) idPrefix = identifier_ + ".";

	DataSectionPtr pFile = BWResource::openSection( primitiveFile );
	if (!pFile)
		pFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualMesh" ) );


	if (pFile)
	{
		flipTriangleWindingOrder();
		sortTriangles();
		
		if (ExportSettings::instance().bumpMapped() &&
			!(ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES))
		{
			makeBumped();
		}

		removeDuplicateVertices();
		sortTriangles();

		PGVector primGroups;
		Moo::IndicesHolder indices;

		if (!createPrimitiveGroups( primGroups, indices ))
			return false;

		Moo::IndexHeader ih;
		largeIndices_ = indices.entrySize() != 2;
		bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_), largeIndices_ ? "list32" : "list" );
		ih.nIndices_ = indices.size();
		ih.nTriangleGroups_ = primGroups.size();

		QuickFileWriter f;
		f << ih;
		f.write( indices.indices(), indices.size() * indices.entrySize() );
		f << primGroups;

		//pFile->updateBinary( idPrefix + "indices", f.output() );
		pFile->writeBinary( idPrefix + "indices", f.output() );
		
		f = QuickFileWriter();

		if (ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)
		{
			IndexedVertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = vertices.size();
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvi" );
			f << vh;
			f << vertices;
		}		
		else if (ExportSettings::instance().bumpMapped())
		{
			TBVertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = vertices.size();
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvtb" );
			f << vh;
			f << vertices;
		}
		else
		{
			VertexVector vertices;
			createVertexList( vertices );
			Moo::VertexHeader vh;
			vh.nVertices_ = vertices.size();
			bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuv" );
			f << vh;
			f << vertices;
		}

		if (morphTargets_.size())
		{
			writeMorphTargets( f );
		}

		if (morphTargets_.size())
			//pFile->updateBinary( idPrefix + "mvertices", f.output() );
			pFile->writeBinary( idPrefix + "mvertices", f.output() );
		else
			//pFile->updateBinary( idPrefix + "vertices", f.output() );
			pFile->writeBinary( idPrefix + "vertices", f.output() );

		pFile->save( primitiveFile );

		DataSectionPtr pRenderSet = pVisualSection->newSection( "renderSet" );
		if (pRenderSet)
		{
			pRenderSet->writeBool( "treatAsWorldSpaceObject", false );
			pRenderSet->writeString( "node", identifier_ );
			DataSectionPtr pGeometry = pRenderSet->newSection( "geometry" );
			if (pGeometry)
			{
				if (morphTargets_.size())
					pGeometry->writeString( "vertices", idPrefix + "mvertices" );
				else
					pGeometry->writeString( "vertices", idPrefix + "vertices" );
				pGeometry->writeString( "primitive", idPrefix + "indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();

				uint32 primGroup = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];;
					if (mat.inUse) mat.save( pGeometry, spExistingVisual, primGroup++ );
				}
			}
		}
	}
	return true;
}


/**
 * This method saves the visual mesh in XML format.  Note that this
 * method is intended for debugging purposes only.
 *
 * @param	xmlFile	The filename of the XML file.
 */
bool VisualMesh::savePrimXml( const std::string& xmlFile )
{
	DataSectionPtr spFileRoot = BWResource::openSection( xmlFile );
	if (!spFileRoot)
		spFileRoot = new XMLSection( "VisualEnvelopePrimXml" );

	DataSectionPtr spFile = spFileRoot->newSection( identifier_ );

	flipTriangleWindingOrder();
	sortTriangles();
	
	if (ExportSettings::instance().bumpMapped() &&
		!(ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES))
	{
		makeBumped();
	}

	removeDuplicateVertices();
	sortTriangles();

	PGVector primGroups;
	Moo::IndicesHolder indices;

	if (!createPrimitiveGroups( primGroups, indices ))
		return false;

	Moo::IndexHeader ih;
	largeIndices_ = indices.entrySize() != 2;
	bw_snprintf( ih.indexFormat_, sizeof(ih.indexFormat_), largeIndices_ ? "list32" : "list" );
	ih.nIndices_ = indices.size();
	ih.nTriangleGroups_ = primGroups.size();

	// Print Index Header contents here
	DataSectionPtr pIH = spFile->newSection( "IndexHeader" );
	pIH->writeString( "IndexFormat", ih.indexFormat_ );
	pIH->writeInt( "NumIndices", ih.nIndices_ );
	pIH->writeInt( "NumPrimGroups", ih.nTriangleGroups_ );

	// Write out the indices
	DataSectionPtr pIndices = spFile->newSection( "Indices" );
	for (uint32 i = 0; i < indices.size(); i++)
	{
		DataSectionPtr pIndex = pIndices->newSection( "Index" );
		pIndex->setInt( (uint32)indices[i] );
	}
	
	DataSectionPtr pPrimGroups = spFile->newSection( "PrimGroups" );
	PGVector::iterator pgvIt;
	for (pgvIt = primGroups.begin(); pgvIt != primGroups.end(); ++pgvIt)
	{
		DataSectionPtr pPrimGroup = pPrimGroups->newSection( "PrimGroup" );
		pPrimGroup->writeInt( "NumPrimGroups", pgvIt->nPrimitives_ );
		pPrimGroup->writeInt( "NumVertices", pgvIt->nVertices_ );
		pPrimGroup->writeInt( "StartIndex", pgvIt->startIndex_ );
		pPrimGroup->writeInt( "StartVertex", pgvIt->startVertex_ );
	}

	if (ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)
	{
		IndexedVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = vertices.size();
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvi" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		IndexedVertexVector::iterator ivvIt;
		for (ivvIt = vertices.begin(); ivvIt != vertices.end(); ++ivvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(ivvIt->pos[0], ivvIt->pos[1], ivvIt->pos[2]) );
			pVertex->writeVector3( "Normal", Vector3(ivvIt->normal[0], ivvIt->normal[1], ivvIt->normal[2]) );
			pVertex->writeVector2( "UV", Vector2(ivvIt->uv[0], ivvIt->uv[1]) );
			pVertex->writeFloat( "Index", ivvIt->index );
		}
	}		
	else if (ExportSettings::instance().bumpMapped())
	{
		TBVertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = vertices.size();
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuvtb" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		TBVertexVector::iterator tbvvIt;
		for (tbvvIt = vertices.begin(); tbvvIt != vertices.end(); ++tbvvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(tbvvIt->pos[0], tbvvIt->pos[1], tbvvIt->pos[2]) );
			pVertex->writeLong( "Normal", tbvvIt->normal );
			pVertex->writeVector2( "UV", Vector2(tbvvIt->uv[0], tbvvIt->uv[1]) );
			pVertex->writeLong( "Tangent", tbvvIt->tangent );
			pVertex->writeLong( "Binormal", tbvvIt->binormal );
		}
	}
	else
	{
		VertexVector vertices;
		createVertexList( vertices );
		Moo::VertexHeader vh;
		vh.nVertices_ = vertices.size();
		bw_snprintf( vh.vertexFormat_, sizeof(vh.vertexFormat_), "xyznuv" );

		// Print Vertex Header contents here
		DataSectionPtr pVH = spFile->newSection( "VertexHeader" );
		pVH->writeString( "VertexFormat", vh.vertexFormat_ );
		pVH->writeInt( "NumVertices", vh.nVertices_ );

		DataSectionPtr pVertices = spFile->newSection( "Vertices" );
		VertexVector::iterator vvIt;
		for (vvIt = vertices.begin(); vvIt != vertices.end(); ++vvIt)
		{
			DataSectionPtr pVertex = pVertices->newSection( "Vertex" );
			pVertex->writeVector3( "Pos", Vector3(vvIt->pos[0], vvIt->pos[1], vvIt->pos[2]) );
			pVertex->writeVector3( "Normal", Vector3(vvIt->normal[0], vvIt->normal[1], vvIt->normal[2]) );
			pVertex->writeVector2( "UV", Vector2(vvIt->uv[0], vvIt->uv[1]) );
		}
	}

	if (morphTargets_.size())
	{
		// TODO - Need to create an XML outputting version of morph targets
		//writeMorphTargetsXml( f );
	}

	spFileRoot->save( xmlFile );

	return true;
}


/**
 *	Returns all resources used by the material.
 *
 *	@param	retResources	The vector to be filled with resources.
 */
void VisualMesh::Material::resources( std::vector<std::string>& retResources )
{
	if (this->fxFile_.length())
	{
		retResources.push_back( unifySlashes( toLower( fxFile_ ) ) );
		for (uint32 i = 0; i < textureOverrides_.size(); i++)
		{
			std::string textureName = unifySlashes( toLower( textureOverrides_[i].second ) );
			retResources.push_back( textureName );
		}
	}
	else
	{
		if (mapIdentifier.length() > 4 && mapIdMeaning != 2)
		{
			std::string mfmID = mapIdentifier.substr( 0, mapIdentifier.length() - 4 ) + ".mfm";			
			DataSectionPtr mfmsect = BWResource::openSection( mfmID );
			if (mfmsect)
			{
				if ((mfmsect->openSection("mfm") ||
					mfmsect->openSection("fx") ||
					mfmsect->openSection("property")) &&
					!(mfmsect->openSection("Src_Blend_type") ||
					mfmsect->openSection("Dest_Blend_type") ||
					mfmsect->openSection("Self_Illumination") ||
					mfmsect->openSection("Alpha_blended")))
				{
					mapIdentifier = mfmID;
					mapIdMeaning = 2;
				}
			}
		}

		if (mapIdMeaning == 1)
		{
			if (mapIdentifier.length())
			{
				retResources.push_back( mapIdentifier );
			}
		}
		else if (mapIdMeaning == 2)
		{
			if (mapIdentifier.length())
			{
				retResources.push_back( mapIdentifier );
			}
		}
		else// mapIdMeaning == 0
		{
			if( !s_defaultMaterial.value().empty() )
			{
				retResources.push_back( s_defaultMaterial.value() );
			}
		}
	}
}


/**
 *	Checks that the effect file exists.
 *
 *	@param	fxName		The effect file name.
 *	@param	fxPostfix	The suffix being used.
 *	@return	Success or failure.
 */
bool VisualMesh::Material::checkFilename( std::string& fxName, const std::string& fxPostfix )
{
	fxName = unifySlashes( this->fxFile_ );
	if (endsWith( fxName, ".fx" ))
	{
		std::string s = fxName.substr( 0, fxName.length() - 3 );
		bool skinnedFX=false;
		uint skinPos = s.find("_skinned");

		std::string pre,post;

		if (skinPos != std::string::npos)
		{
			pre = s.substr(0, skinPos);
			if ( (skinPos+8) < s.size())
				post = s.substr(skinPos+8, s.size());

			skinnedFX = true;
		}
		
		if (BWResource::fileExists( pre + fxPostfix + post + ".fx" ))
		{
			fxName = pre + fxPostfix +  post + ".fx";
		}
		else if (skinnedFX)
			return false;
	}
	return true;
}


/**
 *	Save this material description into the given data section.
 *
 *	@param	pGeometry			The visual to save into.
 *	@param	spExistingVisual	The visual to be searched.
 *	@param	pgid				The prim group id.
 *	@param	skin				The skin type.
 *	@param	skinnedShader		Flag is skinned shader.
 */
void VisualMesh::Material::save( DataSectionPtr pGeometry,
								DataSectionPtr spExistingVisual,
								uint32 pgid, 
								VisualMesh::Material::SkinType skin,
								bool skinnedShader )
{
	DataSectionPtr pPG = pGeometry->newSection( "primitiveGroup" );
	if (pPG)
	{
		pPG->setInt( pgid );		

		std::string idPostfix = skin == SOFT_SKIN ? "_skinned" : "";

		// copy existing bitmap/fx combination
		if ( !this->findAndCopyExistingMaterial( identifier + idPostfix, pPG, spExistingVisual ) )
		{
			DataSectionPtr pMaterial = pPG->newSection( "material" );
			pMaterial->writeString( "identifier", identifier + idPostfix );
			if (ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)
			{
				pMaterial->writeString( "fx", "shaders/std_effects/mesh_particle.fx" );
				if (mapIdentifier.length())
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( "diffuseMap" );
					pProp->writeString( "Texture", mapIdentifier );
				}
			}
			else if (this->fxFile_.length())
			{
				std::string fxName;
				std::string fxPostfix = idPostfix;
				if (!skinnedShader)
					fxPostfix = "";

				checkFilename(fxName, fxPostfix);

				pMaterial->writeString( "fx", BWResolver::dissolveFilename( fxName ) );
				for (uint32 i = 0; i < textureOverrides_.size(); i++)
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( textureOverrides_[i].first );
					std::string textureName = unifySlashes( textureOverrides_[i].second );
					pProp->writeString( "Texture", BWResolver::dissolveFilename( textureName ) );
				}
				for (uint32 i = 0; i < vectorOverrides_.size(); i++)
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( vectorOverrides_[i].first );
					pProp->writeVector4( "Vector4", *(Vector4*)(&vectorOverrides_[i].second) );
				}
				for (uint32 i = 0; i < boolOverrides_.size(); i++)
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( boolOverrides_[i].first );
					pProp->writeBool( "Bool", boolOverrides_[i].second == TRUE );
				}
				for (uint32 i = 0; i < floatOverrides_.size(); i++)
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( floatOverrides_[i].first );
					pProp->writeFloat( "Float", floatOverrides_[i].second );
				}
				for (uint32 i = 0; i < intOverrides_.size(); i++)
				{
					DataSectionPtr pProp = pMaterial->newSection( "property" );
					pProp->setString( intOverrides_[i].first );
					pProp->writeInt( "Int", intOverrides_[i].second );
				}

			}
			else
			{
				if (mapIdentifier.length() > 4 && mapIdMeaning != 2)
				{
					std::string mfmID = mapIdentifier.substr( 0, mapIdentifier.length() - 4 ) + ".mfm";
					DataSectionPtr mfmsect = BWResource::openSection( mfmID );
					if (mfmsect)
					{
						if ((mfmsect->openSection("mfm") ||
							mfmsect->openSection("fx") ||
							mfmsect->openSection("property")) &&
							!(mfmsect->openSection("Src_Blend_type") ||
							mfmsect->openSection("Dest_Blend_type") ||
							mfmsect->openSection("Self_Illumination") ||
							mfmsect->openSection("Alpha_blended")))
						{
							mapIdentifier = mfmID;
							mapIdMeaning = 2;
						}
					}
				}

				if (mapIdMeaning == 1)
				{
					if (skin == NO_SKIN)
					{
						std::string fx = "shaders/std_effects/lightonly";
						if (mapIdentifier.length() > 8 && 
							mapIdentifier.substr( mapIdentifier.length() - 8, 4 ) == "_afx")
						{
							fx += "_add";
						}
						//RA: changing the default shader for materials with a .tga file
						/*else if (mapIdentifier.length() > 4 && 
							mapIdentifier.substr( mapIdentifier.length() - 4, 4 ) == ".tga")
						{
							fx += "_alpha";
						}*/

						fx += ".fx";
						pMaterial->writeString( "fx", fx );
					}
					else
					{
						if (skinnedShader)
							pMaterial->writeString( "fx", "shaders/std_effects/lightonly_skinned.fx" );
						else
							pMaterial->writeString( "fx", "shaders/std_effects/lightonly.fx" );
					}
					
					if (mapIdentifier.length())
					{
						DataSectionPtr pProp = pMaterial->newSection( "property" );
						pProp->setString( "diffuseMap" );
						pProp->writeString( "Texture", mapIdentifier );
					}
				}
				else if (mapIdMeaning == 2)
				{
					pMaterial->writeString( "mfm", mapIdentifier );
				}
				else// mapIdMeaning == 0
				{
					if( !s_defaultMaterial.value().empty() )
						pMaterial->writeString( "mfm", s_defaultMaterial.value() );
				}
			}
		}
	}
}


/**
 *	Look for the material named identifier in the existing visual file.  If found,
 *	then copy the material into the pPrimGroup section and return True.  Otherwise
 *	do nothing to pMaterial and return False.
 *	We use the pPrimGroup section to copy into, because the copy process includes
 *	copying the "material" node.
 *
 *	@param	identifier			The name of the material being searched for.
 *	@param	pPrimGroup			The prim group to copy into.
 *	@param	spExistingVisual	The visual to be searched.
 *	@return	Success of failure.
 */
bool VisualMesh::Material::findAndCopyExistingMaterial(
	const std::string& identifier,
	DataSectionPtr pPrimGroup,
	DataSectionPtr spExistingVisual ) const
{
	if (!spExistingVisual)
		return false;

	DataSectionPtr pExistingMaterial = this->findExistingMaterial( identifier, spExistingVisual );
	if (pExistingMaterial)
	{
		pPrimGroup->copy( pExistingMaterial, false );
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	Look for material named 'identifier' in the given data section, and
 *	return it if it exists.
 *
 *	@param	identifier			The name of the material being searched for.
 *	@param	spExistingVisual	The data section to be searched.
 *	@return	NULL if not found, otherwise the material data section.
 */
DataSectionPtr VisualMesh::Material::findExistingMaterial( const std::string& identifier,
														  DataSectionPtr spExistingVisual ) const
{
	if (!spExistingVisual)
		return NULL;

	std::vector<DataSectionPtr> renderSets;
	spExistingVisual->openSections( "renderSet", renderSets );
	for (uint32 r=0; r<renderSets.size(); r++)
	{
		std::vector<DataSectionPtr> geometries;
		renderSets[r]->openSections( "geometry", geometries );
		for (uint32 g=0; g<geometries.size(); g++)
		{
			std::vector<DataSectionPtr> primGroups;
			geometries[g]->openSections( "primitiveGroup", primGroups );
			for (uint32 p=0; p<primGroups.size(); p++)
			{
				DataSectionPtr pMaterial = primGroups[p]->openSection( "material" );
				if (pMaterial->readString( "identifier" ) == identifier)
					return primGroups[p];
			}
		}
	}

	return NULL;
}


/**
 *	This method adds this visual mesh to destination visual mesh.
 *
 *	@param	destMesh		The visual mesh we are being added to.
 *	@param	forceMeshIndex	Flag force mesh index.
 *	@param	worldSpace		Flag transform to world space.
 */
void VisualMesh::add( VisualMesh & destMesh, int forceMeshIndex, bool worldSpace )
{
	uint32 i;

	// find or add all our materials to the dest mesh
	RemapVector		reremap;
	for (i = 0; i < materials_.size(); i++)
	{
		reremap.push_back( destMesh.findOrAddMaterial( materials_[i] ) );
	}

	// append our transformed vertices to its vertices
	uint32 startVertex = destMesh.vertices_.size();
	for (i = 0; i < vertices_.size(); i++)
	{
		BloatVertex bv = vertices_[i];
		if (worldSpace)
			bv.pos = world_.PointTransform( bv.pos );
		bv.normal = Normalize( world_.VectorTransform( bv.normal ) );
		if (forceMeshIndex != -1)
			bv.meshIndex = forceMeshIndex;
		destMesh.vertices_.push_back( bv );
	}

	// append our remapped and shuffled triangles to its triangles
	for (i = 0; i < triangles_.size(); i++)
	{
		Triangle tr = triangles_[i];
		tr.index[0] += startVertex;
		tr.index[1] += startVertex;
		tr.index[2] += startVertex;
		tr.materialIndex = reremap[ tr.materialIndex ];
		destMesh.triangles_.push_back( tr );
	}
}


/**
 *	This method adds a vertex to our vertex list.
 *
 *	@param bv Vertex to add to the vertexlist.
 *	@return Index of the vertex in the vector.
 */
int VisualMesh::addVertex( const VisualMesh::BloatVertex& bv )
{
	if( verticeSetMap.find( bv ) != verticeSetMap.end() )
	{
		std::set<BVVector::size_type>& verticeSet = verticeSetMap[ bv ];
		for( std::set<BVVector::size_type>::iterator iter = verticeSet.begin();
			iter != verticeSet.end(); ++iter )
			if( vertices_[ *iter ] == bv )
			{
				BloatVertex& mbv = vertices_[ *iter ];
				mbv.smoothingGroup |= bv.smoothingGroup;
				return *iter;
			}
	}
	vertices_.push_back( bv );
	verticeSetMap[ bv ].insert( vertices_.size() - 1 );
	return vertices_.size() - 1;
}


/**
 *	This method writes the morph targets out to the passed in
 *	file writer.
 *
 *	@param	f	The file writer to write the morph targets to.
 */
void VisualMesh::writeMorphTargets( QuickFileWriter& f )
{
	ExportMorphHeader mh;
	mh.nMorphTargets = morphTargets_.size();
	mh.version = 0x100;
	
	f << mh;

	for (int i = 0; i < morphTargets_.size(); i++)
	{
		MorphTarget& mt = morphTargets_[i];
		ExportMorphTarget emt;
		emt.channelIndex = mt.channelIndex;
		bw_snprintf( emt.identifier, sizeof(emt.identifier), "%s", mt.identifier.c_str() );
		std::vector< ExportMorphVertex > emvs;

		std::vector< MorphVertex >::iterator it = mt.verts.begin();
		std::vector< MorphVertex >::iterator end = mt.verts.end();

		while (it != end)
		{
			MorphVertex& mv = *it++;
			ExportMorphVertex emv;

			if (ExportSettings::instance().useLegacyOrientation())
			{
				emv.delta[0] = -mv.delta.x * 100 * ExportSettings::instance().unitScale();
				emv.delta[1] = mv.delta.z * 100 * ExportSettings::instance().unitScale();
				emv.delta[2] = -mv.delta.y * 100 * ExportSettings::instance().unitScale();
			}
			else
			{
				emv.delta[0] = mv.delta.x * 100 * ExportSettings::instance().unitScale();
				emv.delta[1] = mv.delta.z * 100 * ExportSettings::instance().unitScale();
				emv.delta[2] = mv.delta.y * 100 * ExportSettings::instance().unitScale();
			}

			for (int j = 0; j < vertices_.size(); j++)
			{
				if (vertices_[j].vertexIndex == mv.vertexIndex)
				{
					emv.index = j;
					emvs.push_back( emv );
				}
			}
		}
		emt.nVertices = emvs.size();
		f << emt;
		f << emvs;
	}
}


/**
 *	This method captures the morph target information for the passed
 *	node using the Morpher modifier.
 *
 *	@param	node	The node to capture the morph targets from.
 */
void VisualMesh::captureMorphTargets( INode* node )
{
	Modifier* mod = MFXExport::findMorphModifier( node );
	if (mod)
	{
		Matrix3 meshMatrix = node->GetObjectTM( timeNow() );
		Matrix3 nodeMatrix = node->GetNodeTM( timeNow() );
		if( !ExportSettings::instance().allowScale() )
			nodeMatrix = normaliseMatrix( nodeMatrix );

		Matrix3 scaleMatrix = meshMatrix * Inverse( nodeMatrix );
		scaleMatrix.SetRow( 3, Point3::Origin );

		MorphR3* morph = static_cast<MorphR3*>(mod);
		for (int i = 0; i < MR3_NUM_CHANNELS; i++)
		{
			morphChannel& channel = morph->chanBank[i];
			if (channel.mActive)
			{
				MorphTarget mt;
				mt.channelIndex = i;
				mt.identifier = channel.mName;
				for (int j = 0; j < channel.mNumPoints; j++)
				{
					if (Length( channel.mDeltas[j] ) != 0.f)
					{
						MorphVertex mv;
						mv.delta = scaleMatrix * channel.mDeltas[j];
						mv.vertexIndex = j;
						mt.verts.push_back( mv );
					}
				}
				morphTargets_.push_back( mt );
			}
		}
	}
}


/**
 *	This method checks the material suffix to check if the material type
 *	matches the visual type, i.e. it checks that non-skinned meshes do not
 *	use skinned materials and vice-verse.
 *
 *	@param fxPostfix	The suffix to check for.
 *	@return				Success or failure.
 */
bool VisualMesh::checkDXMaterials( const std::string& fxPostfix )
{
	uint32 i;
	for (i = 0; i < materials_.size(); i++)
	{
		if ( (ExportSettings::instance().exportMode() != ExportSettings::NORMAL) && materials_[i].fxFile_.length())
		{
			std::string fxName;
			if (!materials_[i].checkFilename(fxName,fxPostfix))
			{
				MessageBox( GetForegroundWindow(),
						"VisualMesh::Material::save - Skinned shader applied to a non-skinned mesh and the required shader variant doesn't exist.\n",
						"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
				return false;
			}
		}
	}
	return true;
}


/**
 *	This method initalises the visual mesh.
 *
 *	@param node			The node of the mesh we want to export.
 *	@param checkMats	Flag to check the material of the node.
 *	@return				Success or failure.
 */
bool VisualMesh::init( INode* node, bool checkMats )
{
	verticeSetMap.clear();
	verticeIndexSetMap.clear();

	// Disable the Morpher modifier if it is present
	MorpherHolder morpherHolder(node);

	bool needDel = false;
	ConditionalDeleteOnDestruct<TriObject> triObject( MFXExport::getTriObject( node, timeNow(), needDel));
	if (! (&*triObject))
		return false;
	triObject.del( needDel );
	Mesh* mesh = &triObject->mesh;
	
	bool ret = false;
	if (mesh->getNumFaces() && mesh->getNumVerts())
	{
		identifier_ = trimWhitespaces( node->GetName() );

		gatherMaterials( node );
		bool hasUVs = mesh->getNumTVerts() && mesh->tvFace;

		if (checkMats && !checkDXMaterials(""))
			return false;

		if ( !hasUVs &&
				(ExportSettings::instance().bumpMapped() && 
				!(ExportSettings::instance().exportMode() == ExportSettings::MESH_PARTICLES)) )
		{
			std::string warningMessage(
				"VisualMesh::init - Bump mapping is enabled but mesh '" +
				identifier_ +
				"' has no UV coordinates.\n" );
			MessageBox( GetForegroundWindow(),
				warningMessage.c_str(),
				"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );

			return false;
		}



		BloatVertex bv;
		bv.normal = Point3::Origin;
		bv.uv = Point2::Origin;
		bv.binormal = Point3::Origin;
		bv.tangent = Point3::Origin;

		bool isStatic = ExportSettings::instance().exportMode() == ExportSettings::STATIC ||
			ExportSettings::instance().exportMode() == ExportSettings::STATIC_WITH_NODES;

		Matrix3 meshMatrix = node->GetObjectTM( timeNow() );
		Matrix3 nodeMatrix = node->GetNodeTM( timeNow() );
		if( !ExportSettings::instance().allowScale() )
			nodeMatrix = normaliseMatrix( nodeMatrix );

		Matrix3 scaleMatrix = isStatic ? meshMatrix : meshMatrix * Inverse( nodeMatrix );

		vertexPositions_.assign( mesh->verts, mesh->verts + mesh->getNumVerts() );

		for (int i = 0; i < mesh->getNumFaces(); i++)
		{
			Face f = mesh->faces[ i ];

			Triangle tr;
			tr.materialIndex = materialRemap_[ f.getMatID() % materialRemap_.size() ];
			tr.smoothingGroup = f.smGroup;
			for (int j = 0; j < 3; j++)
			{
				bv.pos = scaleMatrix * mesh->verts[ f.v[j] ];
				bv.vertexIndex = f.v[j];
				bv.smoothingGroup = f.smGroup;
				if (hasUVs)
				{
					bv.uv = reinterpret_cast<Point2&>( mesh->tVerts[ mesh->tvFace[i].t[j] ] );
					bv.uv.y = 1 - bv.uv.y;
				}
				bv.meshIndex = 0;

				if (snapVertices_)
					bv.pos = snapPoint3( bv.pos );
				tr.index[j] = addVertex( bv );
				tr.realIndex[j] = bv.vertexIndex;
			}
			triangles_.push_back( tr );
		}

		if (!isStatic)
			captureMorphTargets( node );

		bool nodeMirrored = isStatic ? false : isMirrored( nodeMatrix );
		bool meshMirrored = isMirrored( meshMatrix );

		if (nodeMirrored ^ meshMirrored)
			flipTriangleWindingOrder();
		
/*		if (isMirrored( meshMatrix ))
			flipTriangleOrder();
		
		if (isMirrored( nodeMatrix ))
			flipTriangleWindingOrder();*/

		for( BVVector::size_type i = 0; i < vertices_.size(); ++i )
			verticeIndexSetMap[ vertices_[ i ].vertexIndex ].push_back( i );

		createNormals( );
		getNormalsFromModifier( node );

		if (isStatic)
		{
			world_.IdentityMatrix();
		}
		else
		{
			world_ = nodeMatrix;
		}

		// initialise boundingbox
		if (snapVertices_)
			bb_.setBounds(	reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ 0 ].pos ) ),
							reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ 0 ].pos ) ) );
		else
			bb_.setBounds(	reinterpret_cast<const Vector3&>( world_ * vertices_[ 0 ].pos ),
							reinterpret_cast<const Vector3&>( world_ * vertices_[ 0 ].pos ) );
		for (uint32 i = 1; i < vertices_.size(); i++)
		{
			if (snapVertices_)
				bb_.addBounds( reinterpret_cast<const Vector3&>( snapPoint3( world_ * vertices_[ i ].pos ) ) );
			else
				bb_.addBounds( reinterpret_cast<const Vector3&>( world_ * vertices_[ i ].pos ) );
		}

		if (nodeMirrored)
			flipTriangleWindingOrder();

		ret = true;
	}
	
	return ret;
}
