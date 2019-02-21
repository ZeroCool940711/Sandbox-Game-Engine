/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "bsp_generator.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/primitive_file.hpp"
#include "physics2/bsp.hpp"
#include "moo/node.hpp"
#include "moo/primitive_file_structs.hpp"
#include "moo/vertex_formats.hpp"


DECLARE_DEBUG_COMPONENT2( "Exporter", 0 )

/**
 *	Class used to load in a set of vertices and allow access to
 *	the vertices through the array access operator.
 */
class Vertices
{
public:
	/** Default constructor */
	Vertices() : nVertices_( 0 ),
		pVertices_( NULL ),
		vertexStride_( 0 )
	{
	};

	/**
	 *	This method loads in vertices given a resourceID.
	 *
	 *	@param	resourceID	The resource to get the vertices from.
	 *	@return	Success or failure.
	 */
	bool load( const std::string & resourceID )
	{
		// find our data
		uint noff = resourceID.find( ".primitives/" );
		if (noff < resourceID.size())
		{
			// everything is normal
			noff += 11;
			std::string fileName = resourceID.substr( 0, noff );
			std::string sectionName = resourceID.substr( noff+1 );

			DataSectionPtr pFile =
				PrimitiveFile::get( resourceID.substr( 0, noff ) );
			if (!pFile.hasObject())
			{
				ERROR_MSG( "Vertices::load - No primitive file %s\n", resourceID.substr( 0, noff ).c_str() );
				return false;
			}
			pData_ = pFile->readBinary( resourceID.substr( noff+1 ) );
		}
		else
		{
			// find out where the data should really be stored
			std::string fileName, partName;
			splitOldPrimitiveName( resourceID, fileName, partName );

			// read it in from this file
			pData_ = fetchOldPrimitivePart( fileName, partName );
		}

		// Open the binary resource for these vertices
		if (pData_)
		{
			// Get the vertex header
			const Moo::VertexHeader* pVH =
				reinterpret_cast< const Moo::VertexHeader* >( pData_->data() );
			nVertices_ = pVH->nVertices_;

			pVertices_ = reinterpret_cast< const char * >( pVH + 1 );

			std::string format( pVH->vertexFormat_ );

			if (format == "xyznuv")
			{
				vertexStride_ = sizeof( Moo::VertexXYZNUV );
			}
			else if (format == "xyznuvi")
			{
				vertexStride_ = sizeof( Moo::VertexXYZNUVI );
			}
			else if (format == "xyznuvtb")
			{
				vertexStride_ = sizeof( Moo::VertexXYZNUVTB );
			}
			else if (format == "xyznuvitb")
			{
				vertexStride_ = sizeof( Moo::VertexXYZNUVITB );
			}
			else if (format == "xyznuviiiww" )
			{
				vertexStride_ = sizeof( Moo::VertexXYZNUVIIIWW );
			}
			else
			{
				ERROR_MSG( "Vertices::load: Unknown vertex format %s\n",
						format.c_str() );
				return false;
			}
		}
		else
		{
			ERROR_MSG( "Vertices::load: pData_ is NULL\n" );
			return false;
		}

		return true;
	}

	/**
	 *	This method is used to access vertices by index.
	 *
	 *	@param	index	The index of the vertex to be accessed.
	 *	@return	The positon of the accessed vertex.
	 */
	const Vector3 & operator[]( int index ) const
	{
		return *reinterpret_cast<const Vector3 *>( pVertices_ + vertexStride_ * index );
	}

private:
	uint32			nVertices_;
	const char *	pVertices_;
	BinaryPtr		pData_;
	int				vertexStride_;
};


/**
 *	Class used to encapsulate the loading and accessing of primitive
 *	groups.
 */
class Primitive
{
public:
	enum Type
	{
		PT_UNKNOWN,
		PT_TRIANGLE_LIST
	};

	typedef unsigned short Index;

	/** Default constructor */
	Primitive() : primType_( PT_UNKNOWN ),
		nIndices_( 0 ),
		pIndices_( NULL )
	{};


	/**
	 *	This method loads in primives given a resourceID.
	 *
	 *	@param	resourceID	The resource to get the primives from.
	 *	@return	Success or failure.
	 */
	bool load( const std::string & resourceID )
	{
		bool result = false;

		uint noff = resourceID.find( ".primitives/" );
		if (noff < resourceID.size())
		{
			// everything is normal
			noff += 11;
			pData_ = PrimitiveFile::get( resourceID.substr( 0, noff ) )->
				readBinary( resourceID.substr( noff+1 ) );
		}
		else
		{
			// find out where the data should really be stored
			std::string fileName, partName;
			splitOldPrimitiveName( resourceID, fileName, partName );

			// read it in from this file
			pData_ = fetchOldPrimitivePart( fileName, partName );
		}

		// Open the binary resource for this primitive
		if (pData_)
		{
			// Get the index header
			const Moo::IndexHeader* pIH =
				reinterpret_cast< const Moo::IndexHeader* >( pData_->data() );

			const std::string format( pIH->indexFormat_ );

			// Create the right type of primitive
			if (format == "list")
			{
				primType_ = PT_TRIANGLE_LIST;
				nIndices_ = pIH->nIndices_;
				pIndices_ = reinterpret_cast< const Index * >( pIH + 1 );

				const Index * pSrc = pIndices_ + nIndices_;

				// Get the primitive groups
				const Moo::PrimitiveGroup* pGroup =
					reinterpret_cast< const Moo::PrimitiveGroup* >( pSrc );

				for (int i = 0; i < pIH->nTriangleGroups_; i++)
				{
					primGroups_.push_back( *(pGroup++) );
				}

				result = true;
			}
			else if (format == "list32")
			{
				primType_ = PT_TRIANGLE_LIST;
				nIndices_ = pIH->nIndices_;
				pIndices_ = reinterpret_cast< const Index * >( pIH + 1 );

				const Index * pSrc = pIndices_ + nIndices_*2;

				// Get the primitive groups
				const Moo::PrimitiveGroup* pGroup =
					reinterpret_cast< const Moo::PrimitiveGroup* >( pSrc );

				for (int i = 0; i < pIH->nTriangleGroups_; i++)
				{
					primGroups_.push_back( *(pGroup++) );
				}

				result = true;
			}
			else
			{
				ERROR_MSG( "Unsupported index format %s\n", format.c_str() );
			}
		}
		else
		{
			ERROR_MSG( "Primitive::load: Failed to read binary resource: %s\n",
					resourceID.c_str() );
		}

		return result;
	}

	Type				primType() const	{ return primType_; }
	uint32				nPrimGroups() const { return primGroups_.size(); }
	int					nIndices() const	{ return nIndices_; }
	const Index*		pIndices() const	{ return pIndices_; }
	const Moo::PrimitiveGroup&	primitiveGroup(uint32 i) const
											{ return primGroups_[ i ]; }

private:
	typedef std::vector< Moo::PrimitiveGroup > PrimGroupVector;

	PrimGroupVector	primGroups_;
	const Index *	pIndices_;
	BinaryPtr 		pData_;
	Type			primType_;
	int				nIndices_;
};


/**
 *	Primitive group structure.
 */
class PrimitiveGroup
{
public:
	uint32	groupIndex_;
	WorldTriangle::Flags	materialFlags_;
};

typedef std::vector< PrimitiveGroup > PrimitiveGroups;


/**
 *	This function is used to populate the world triangles.
 *
 *	@param	ws				The returned set of triangles.
 *	@param	m				The transformation matrix.
 *	@param	vertices		The vertices.
 *	@param	primitives		The primitives.
 *	@param	primitiveGroups	The primitive groups.
 */
void populateWorldTriangles( RealWTriangleSet & ws, const Matrix & m,
		const Vertices & vertices, const Primitive & primitives,
		const PrimitiveGroups & primitiveGroups )
{
	Primitive::Type primitivesType = primitives.primType();

	if (primitivesType != Primitive::PT_TRIANGLE_LIST)
	{
		ERROR_MSG( "populateWorldTriangles: Bad primitives type\n" );
		return;
	}

	if (primitivesType == Primitive::PT_TRIANGLE_LIST)
	{
		bool bspTriangleCulled = false;

		PrimitiveGroups::const_iterator iter = primitiveGroups.begin();
		while (iter != primitiveGroups.end())
		{
			WorldTriangle::Flags flags = iter->materialFlags_;

			// -1 for flags is the non existent setting
			// in BigWorld 1.8 and later, this is not used, however this
			// code is retained in case we want to be able to associate
			// in the MAX file somehow that an area of a model never add
			// itself to the BSP.
			if (flags != (WorldTriangle::Flags) -1)
			{
				const Moo::PrimitiveGroup& pg =
					primitives.primitiveGroup( iter->groupIndex_ );

				for (int i = 0; i < pg.nPrimitives_; i++)
				{
					const Primitive::Index * pi = primitives.pIndices() + pg.startIndex_ + i * 3;
					MF_ASSERT( pi < ( primitives.pIndices() + primitives.nIndices() + 3 ) );

					Vector3 v1 = m.applyPoint( vertices[ *(pi    ) ] );
					Vector3 v2 = m.applyPoint( vertices[ *(pi + 1) ] );
					Vector3 v3 = m.applyPoint( vertices[ *(pi + 2) ] );

					// Check for degenerate triangles

					WorldTriangle triangle( v1, v2, v3, flags );

					if (triangle.normal().length() >= 0.0001f)
					{
						ws.push_back( triangle );
					}
					else
					{
						bspTriangleCulled = true;
					}
				}
			}

			iter++;
		}

		if (bspTriangleCulled)
		{
			INFO_MSG(
				"One or more BSP triangles have been culled.\n"
				"Creating a simplified BSP would fix the problem and improve performance inside BigWorld." );
		}
	}
}


/**
 *	This method returns the material identifier for the given section.
 *	If none can be found, it returns the string "empty".  This has been
 *	chosen because "empty" is also the name chosen by the exporters when
 *	no material has been assigned, and the default mfm is given instead.
 *
 *	@param	primitiveGroupSection	The primitive group to be searched.
 *
 *	@return	The identifier of the material.
 */
std::string getMaterialIdentifier( DataSectionPtr primitiveGroupSection )
{
	DataSectionPtr pMat = primitiveGroupSection->openSection( "material" );
	if (pMat)
	{
		return pMat->readString( "identifier", "empty" );
	}

	return "empty";
}


/**
 *	This function generates a bsp from a visual.
 *
 *	@param	visualName	The visual file to extract the BSP from.
 *	@param	bspName		The name of the BSP.
 *	@param	materialIDs	The list of materialIDs to be returned.
 *	@return	Success or failure.
 */
bool generateBSP( const std::string & visualName,
		const std::string & bspName,
		std::vector<std::string>& materialIDs )
{
	std::string primitivesName =
		visualName.substr( 0, visualName.find_last_of( '.' ) ) +
			".primitives/";
	std::replace( primitivesName.begin(), primitivesName.end(), '\\', '/' );

	// Make sure these are not in the cache.
	BWResource::instance().purge(visualName,true);
	BWResource::instance().purge(primitivesName,true);

	RealWTriangleSet tris;

	DataSectionPtr pRoot =
		BWResource::instance().rootSection()->openSection( visualName );

	if (!pRoot)
	{
		ERROR_MSG( "generateBSP: Failed to get %s\n",
				visualName.c_str() );

		return false;
	}

	Moo::NodePtr pRootNode = new Moo::Node;
	Moo::Node & rootNode = *pRootNode;

	DataSectionPtr pNodeSection = pRoot->openSection( "node" );
	if (pNodeSection)
	{
		rootNode.loadRecursive( pNodeSection );
	}
	else
	{
		rootNode.identifier( "root" );
	}

	// Iterate through the render set sections.
	DataSection::iterator iter = pRoot->begin();

	while (iter != pRoot->end())
	{
		if ((*iter)->sectionName() == "renderSet")
		{
			std::vector< Moo::NodePtr > transformNodes;
			DataSectionPtr pRenderSetSection = *iter;

			DataSection::iterator nodeIter = pRenderSetSection->begin();

			while (nodeIter != pRenderSetSection->end())
			{
				DataSectionPtr pNS = *nodeIter;
				if (pNS->sectionName() == "node")
				{
					Moo::NodePtr pNode =
						rootNode.find( pNS->asString() );
					MF_ASSERT( pNode );
					transformNodes.push_back( pNode );
				}

				nodeIter++;
			}

			if (transformNodes.empty())
			{
				transformNodes.push_back( &rootNode );
			}

			Moo::NodePtr pMainNode = transformNodes.front();
			Matrix firstNodeTransform = pMainNode->transform();

			while (pMainNode != &rootNode)
			{
				pMainNode = pMainNode->parent();
				firstNodeTransform.postMultiply( pMainNode->transform() );
			}

			DataSection::iterator geomIter = pRenderSetSection->begin();

			while (geomIter != pRenderSetSection->end())
			{
				DataSectionPtr pGeomSection = *geomIter;
				if (pGeomSection->sectionName() == "geometry")
				{
					PrimitiveGroups primitiveGroups;

					// Get a reference to the vertices used by this geometry
					std::string verticesName = pGeomSection->readString( "vertices" );
					if (verticesName.find_first_of( '/' ) >= verticesName.size())
						verticesName = primitivesName + verticesName;
					Vertices vertices;

					if (!vertices.load( verticesName ))
					{
						ERROR_MSG( "generateBSP: "
								"Failed to load vertices\n" );
						return false;
					}

					// Get a reference to the indices used by this geometry
					std::string indicesName = pGeomSection->readString( "primitive" );
					if (indicesName.find_first_of( '/' ) >= indicesName.size())
						indicesName = primitivesName + indicesName;
					Primitive primitive;

					if (!primitive.load( indicesName ))
					{
						ERROR_MSG( "ServerModel::generateBSP: "
								"Failed to load indices\n" );
						return false;
					}

					DataSection::iterator primitiveGroupIter = pGeomSection->begin();

					while (primitiveGroupIter != pGeomSection->end())
					{
						DataSectionPtr primitiveGroupSection = *primitiveGroupIter;
						if (primitiveGroupSection->sectionName() == "primitiveGroup")
						{
							// Read the primitive group data.
							PrimitiveGroup primitiveGroup;
							primitiveGroup.groupIndex_ =
								primitiveGroupSection->asInt();
							//in BigWorld 1.8, we save the index into the
							//material ID list as our material flags.
							//these are interpreted at runtime based on material settings that
							//may have changed in model editor etc.
							materialIDs.push_back( getMaterialIdentifier(primitiveGroupSection) );

							primitiveGroup.materialFlags_ = (WorldTriangle::Flags)(materialIDs.size() - 1);
							primitiveGroups.push_back( primitiveGroup );
						}

						primitiveGroupIter++;
					}

					populateWorldTriangles( tris, firstNodeTransform,
							vertices, primitive, primitiveGroups );
				}

				geomIter++;
			}
		}

		iter++;
	}

	BSPTree * pTree = new BSPTree( tris );
	bool result = pTree->save( bspName );
	delete pTree;

	if (!result)
	{
		ERROR_MSG( "generateBSP: failed to save '%s'\n", bspName.c_str() );
	}
#if 0
	// For testing.
	else
	{
		BinaryPtr bspData = BWResource::openSection( bspName )->asBinary();

		if (bspData)
		{
			BSPTree * pTree = new BSPTree();
			INFO_MSG( "generateBSP: Loading %s %s\n",
				bspName.c_str(),
				pTree->load( bspData ) ? "succeeded" : "failed" );
			delete pTree;
		}
	}
#endif

	return result;
}

// bsp_generator.cpp
