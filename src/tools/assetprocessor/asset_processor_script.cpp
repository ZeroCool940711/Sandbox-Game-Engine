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

#include "tchar.h"
#include "asset_processor_script.hpp"
#include "cstdmf/unique_id.hpp"
#include "pyscript/script.hpp"
#include "physics2/bsp.hpp"
#include "moo/init.hpp"
#include "moo/visual.hpp"
#include "moo/visual_common.hpp"
#include "moo/node.hpp"
#include "moo/visual_channels.hpp"
#include "moo/managed_effect.hpp"
#include "moo/texture_manager.hpp"
#include "moo/primitive_file_structs.hpp"
#include "moo/primitive_manager.hpp"
#include "moo/visual_manager.hpp"
#include "moo/vertices_manager.hpp"
#include "moo/render_context.hpp"
#include "resmgr/auto_config.hpp"


static const LPCTSTR APP_NAME = (LPCTSTR)"AssetProcessor";

std::string baseName( const std::string& resourceID )
{
	std::string baseName = resourceID.substr(
		0, resourceID.find_last_of( '.' ) );
	for (uint ni = 0; ni < baseName.size(); ni++)
		if (baseName[ni] == '\\') baseName[ni]='/';	
	return baseName;
}

//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: Window procedure for the game window
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch (msg) {

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE)
		{
			PostQuitMessage(0);
		}
		break;
				
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}


namespace AssetProcessorScript
{

void init()
{
	// Need this for shader include paths config
	AutoConfig::configureAllFrom( "resources.xml" );

	// Create moo, we don't care if render context init fails because we create
	// a new one below;
	Moo::init();

	//Create a render context and device, so that we can properly load
	//visuals etc.
	HINSTANCE hInst = ::GetModuleHandle(NULL);
	HCURSOR cursor = LoadCursor( hInst, MAKEINTRESOURCE(0) );
	WNDCLASS wc = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst, NULL, cursor, NULL, NULL, APP_NAME };
    if( !RegisterClass( &wc ) )
        return;
	HWND hWnd = CreateWindow( APP_NAME, APP_NAME, WS_OVERLAPPED, 0, 0, 256, 256, NULL, NULL, hInst, NULL );

	Moo::rc().createDevice( hWnd );

	Moo::VisualChannel::initChannels();

	DataSectionPtr ptr = BWResource::instance().openSection("shaders/formats");
	if (ptr)
	{
		DataSectionIterator it = ptr->begin();
		DataSectionIterator end = ptr->end();
		while (it != end)
		{
			std::string format = (*it)->sectionName();
			uint32 off = format.find_first_of( "." );
			format = format.substr( 0, off );
			Moo::VertexDeclaration::get( format );
			it++;
		}
	}

	// Initialise the module
	PyObject * pRMModule = PyImport_AddModule( "_AssetProcessor" );	
}


void fini()
{
	Moo::rc().releaseDevice();

	// Close render context.
	Moo::fini();
}


/**
 *	This method adds the triangles defined by the geometry structure to the
 *	given triangle set (transformed by the given matrix)
 *
 *	@param materialIDs	vector of materialIds.  Each primitive group will have
 *						a corresponding materialID.  The index of the material
 *						in the list is used as the material kind, and will later
 *						be mapped to materials in the visual.
 *
 *	@return the number of degenerate triangles found
 */
int populateWorldTriangles( Moo::Visual::Geometry& geometry, 
							RealWTriangleSet & ws,
							const Matrix & m,
							std::vector<std::string>& materialIDs )
{
	int degenerateCount = 0;

	Moo::VerticesPtr verts = geometry.vertices_;
	Moo::PrimitivePtr prims = geometry.primitives_;

	if ((verts->format() != "xyznuv" && verts->format() != "xyznuvtb" ) ) return 0;


	const Moo::Vertices::VertexPositions& vertices = verts->vertexPositions();

	uint32 nIndices = 0;
	D3DPRIMITIVETYPE pt = prims->primType();

	if( pt == D3DPT_TRIANGLELIST )
	{
		for( uint32 i = 0; i < prims->nPrimGroups(); i++ )
		{
			const Moo::PrimitiveGroup& pg = prims->primitiveGroup( i );
			nIndices = max( uint32(pg.startIndex_ + ( pg.nPrimitives_ * 3 )), nIndices );
		}
	}
	else if( pt == D3DPT_TRIANGLESTRIP )
	{
		for( uint32 i = 0; i < prims->nPrimGroups(); i++ )
		{
			const Moo::PrimitiveGroup& pg = prims->primitiveGroup( i );
			nIndices = max( uint32(pg.startIndex_ + ( pg.nPrimitives_ ) + 2 ), nIndices );
		}
	}

	const Moo::IndicesHolder& indicesHolder = prims->indices();
	if ( pt == D3DPT_TRIANGLELIST )
	{
		for( uint32 i = 0; i < geometry.primitiveGroups_.size(); i++ )
		{			
			//flags stored in a generated BSP is the index of the material ID,
			//until we remap them to material kind / collision flags later.
			//we search backwards because the most recently added materialIDs
			//will correspond to the primitive groups in this geometry.
			WorldTriangle::Flags flags(0);
			for ( uint32 n = materialIDs.size(); n > 0 ; n-- )
			{
				if ( geometry.primitiveGroups_[i].material_->identifier() ==
						materialIDs[n-1] )
				{
					flags = (WorldTriangle::Flags)(n-1);
				}
			}			

			const Moo::PrimitiveGroup& pg = prims->primitiveGroup(
				geometry.primitiveGroups_[i].groupIndex_ );

			for( uint32 i = 0; int(i) < pg.nPrimitives_; i++ )
			{
				int pi = pg.startIndex_ + i * 3;
				Vector3 v1 = m.applyPoint( vertices[ indicesHolder[ pi ] ] );
				Vector3 v2 = m.applyPoint( vertices[ indicesHolder[ pi + 1 ] ] );
				Vector3 v3 = m.applyPoint( vertices[ indicesHolder[ pi + 2 ] ] );

				// Check for degenerate triangles

				WorldTriangle triangle( v1, v2, v3, flags );

				if (triangle.normal().length() < 0.001f)
				{
					degenerateCount++;
				}
				else
				{
					ws.push_back( triangle );
				}
			}
		}
	}
	else if ( pt == D3DPT_TRIANGLESTRIP )
	{
		for( uint32 i = 0; i < geometry.primitiveGroups_.size(); i++ )
		{
			//flags stored in a generated BSP is the index of the material ID,
			//until we remap them to material kind / collision flags later.
			//we search backwards because the most recently added materialIDs
			//will correspond to the primitive groups in this geometry.
			WorldTriangle::Flags flags(0);
			for ( uint32 n = materialIDs.size(); n > 0 ; n-- )
			{
				if ( geometry.primitiveGroups_[i].material_->identifier() ==
						materialIDs[n-1] )
				{
					flags = (WorldTriangle::Flags)(n-1);
				}
			}
			
			const Moo::PrimitiveGroup& pg = prims->primitiveGroup(
				geometry.primitiveGroups_[i].groupIndex_ );

			for( uint32 i = 0; int(i) < pg.nPrimitives_; i++ )
			{
				int pi = pg.startIndex_ + i;
				uint32 t1, t2, t3;

				if( i & 1 )
				{
					t1 = 2;
					t2 = 1;
					t3 = 0;
				}
				else
				{
					t1 = 0;
					t2 = 1;
					t3 = 2;
				}

				Vector3 v1 = m.applyPoint( vertices[ indicesHolder[ pi + t1 ] ] );
				Vector3 v2 = m.applyPoint( vertices[ indicesHolder[ pi + t2 ] ] );
				Vector3 v3 = m.applyPoint( vertices[ indicesHolder[ pi + t3 ] ] );

				WorldTriangle triangle( v1, v2, v3, flags );

				if (triangle.normal().length() < 0.001f)
				{
					degenerateCount++;
				}
				else
				{
					ws.push_back( triangle );
				}
			}
		}
	}

	return degenerateCount;
}


/**
 *	This method returns the material identifier for the given section.
 *	If none can be found, it returns the string "empty".  This has been
 *	chosen because "empty" is also the name chosen by the exporters when
 *	no material has been assigned, and the default mfm is given instead.
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
 *	This method generates a bsp for the given visual resourceID.
 *	It ignores any existing BSP information, and simply returns the
 *	new BSP via the pBSP reference passed in.  It also returns
 *	the number of degenerate triangles (triangle that are not added
 *	to the BSP).  This is used for checking the size of the BSP against
 *	the size of the visual later on.
 *
 *	@param	resourceID	name of the visual file
 *	@param	pBSP		[out] reference to a BSP Proxy Ptr that will contain
 *						the resultant generated BSP.
 *	@param	materialIDs [out] returned ordered vector of material IDs. 
 *	@param	nVisualTris [out] number of triangles in the visual
 *	@param	nDengerateTris [out] number of triangles in the visual that
 *						were not added to the BSP.
 *	@return string	error or success string.
 */
std::string generateBSP2( const std::string& resourceID, 
						 Moo::BSPProxyPtr& pBSP,
						 std::vector<std::string>& materialIDs,
						 uint32& nVisualTris,
						 uint32& nDegenerateTris )
{
	materialIDs.clear();
	nDegenerateTris = 0;
	nVisualTris = 0;
	std::string ret;
	uint32 nPGroups = 0;
	Moo::Visual::RenderSetVector renderSets_;	

	// open the resource
	DataSectionPtr root = BWResource::instance().rootSection()->openSection( resourceID );
	if (!root)
	{
		ret = "Couldn't open visual ";
		ret += resourceID;
		return ret;
	}
	
	std::string baseName = ::baseName( resourceID );

	// Load our primitives	
	std::string primitivesFileName = baseName + ".primitives/";

	RealWTriangleSet	tris;	
	Moo::NodePtr rootNode_;

	// load the hierarchy
	DataSectionPtr nodeSection = root->openSection( "node" );
	if (nodeSection)
	{
		rootNode_ = new Moo::Node;
		rootNode_->loadRecursive( nodeSection );
	}
	else
	{
		// If there are no nodes in the resource create one.
		rootNode_ = new Moo::Node;
		rootNode_->identifier( "root" );
	}

	// open the renderesets
	std::vector< DataSectionPtr > renderSets;
	root->openSections( "renderSet", renderSets );

	if (renderSets.size() == 0)
	{
		ret = "No rendersets in visual ";
		ret += resourceID;
		return ret;
	}

	// iterate through the rendersets and create them
	std::vector< DataSectionPtr >::iterator rsit = renderSets.begin();
	std::vector< DataSectionPtr >::iterator rsend = renderSets.end();
	while (rsit != rsend)
	{
		DataSectionPtr renderSetSection = *rsit;
		++rsit;
		Moo::Visual::RenderSet renderSet;

		// Read worldspace flag
		renderSet.treatAsWorldSpaceObject_ = renderSetSection->readBool( "treatAsWorldSpaceObject" );
		std::vector< std::string > nodes;

		// Read the node identifiers of all nodes that affect this renderset.
		renderSetSection->readStrings( "node", nodes );

		// Are there any nodes?
		if (nodes.size())
		{
			// Iterate through the node identifiers.
			std::vector< std::string >::iterator it = nodes.begin();
			std::vector< std::string >::iterator end = nodes.end();
			while (it != end)
			{
				// Find the current node.
				Moo::NodePtr node = rootNode_->find( *it );
				if (!node)
				{
					ret = "Couldn't find node " + (*it) + " in " + resourceID;					
					return ret;					
				}

				// Add the node to the rendersets list of nodes
				renderSet.transformNodes_.push_back( node );
				++it;
			}
		}
		else
		{
			// The renderset is not affected by any nodes, so we will force it to be affected by the rootNode.
			renderSet.transformNodes_.push_back( rootNode_ );
		}

		// Calculate the first node's static world transform
		//  (for populateWorldTriangles)
		Moo::NodePtr pMainNode = renderSet.transformNodes_.front();
		renderSet.firstNodeStaticWorldTransform_ = pMainNode->transform();

		while (pMainNode != rootNode_)
		{
			pMainNode = pMainNode->parent();
			renderSet.firstNodeStaticWorldTransform_.postMultiply(
				pMainNode->transform() );
		}

		// Open the geometry sections
		std::vector< DataSectionPtr > geometries;
		renderSetSection->openSections( "geometry", geometries );

		if (geometries.size() == 0)
		{
			ret = "No geometry in renderset in " + resourceID;
			return ret;			
		}

		std::vector< DataSectionPtr >::iterator geit = geometries.begin();
		std::vector< DataSectionPtr >::iterator geend = geometries.end();

		// iterate through the geometry sections
		while (geit != geend)
		{
			DataSectionPtr geometrySection = *geit;
			++geit;
			Moo::Visual::Geometry geometry;

			// Get a reference to the vertices used by this geometry
			std::string verticesName = geometrySection->readString( "vertices" );
			if (verticesName.find_first_of( '/' ) >= verticesName.size())
				verticesName = primitivesFileName + verticesName;
			geometry.vertices_ = Moo::VerticesManager::instance()->get( verticesName );

			// Get a reference to the indices used by this geometry
			std::string indicesName = geometrySection->readString( "primitive" );
			if (indicesName.find_first_of( '/' ) >= indicesName.size())
				indicesName = primitivesFileName + indicesName;
			geometry.primitives_ = Moo::PrimitiveManager::instance()->get( indicesName );

			// Open the primitive group descriptions
			std::vector< DataSectionPtr > primitiveGroups;
			geometrySection->openSections( "primitiveGroup", primitiveGroups );

			std::vector< DataSectionPtr >::iterator prit = primitiveGroups.begin();
			std::vector< DataSectionPtr >::iterator prend = primitiveGroups.end();

			// Iterate through the primitive group descriptions
			while (prit != prend)
			{
				nPGroups++;
				DataSectionPtr primitiveGroupSection = *prit;
				++prit;

				// Read the primitve group data.
				Moo::Visual::PrimitiveGroup primitiveGroup;
				primitiveGroup.groupIndex_ = primitiveGroupSection->asInt();				

				materialIDs.push_back( getMaterialIdentifier(primitiveGroupSection) );
				
				Moo::EffectMaterial * pMat = new Moo::EffectMaterial();				
				primitiveGroup.material_ = pMat;
				// No need to load the material at this point.
				// all we need is the identifier
				//pMat->load( primitiveGroupSection->openSection( "material" ) );		
				pMat->identifier( materialIDs[materialIDs.size()-1] );						

				geometry.primitiveGroups_.push_back( primitiveGroup );
					
			}

			nVisualTris += geometry.nTriangles();

			// Add this geometry to our own private BSP tree
			nDegenerateTris += populateWorldTriangles( geometry, tris,
				renderSet.firstNodeStaticWorldTransform_, materialIDs );

			renderSet.geometry_.push_back( geometry );
		}

		renderSets_.push_back( renderSet );
	}

	
	if (!tris.empty())
	{
		pBSP = new Moo::BSPProxy( new BSPTree( tris ) );		
	}
	//else no BSP for you.  this means the vertices are skinned and should
	//not have a BSP.  this is fine.	

	return "";
}


/**
 *	This method replaces BSP data in a .primitives file with BSP2 data.
 */
std::string replaceBSPData( const std::string& visualName,
							Moo::BSPProxyPtr pGeneratedBSP,
							std::vector<std::string>& bspMaterialIDs )
{
	// Make sure we don't have any of the file hanging around in the cache
	BWResource::instance().purgeAll();

	std::string ret;
	std::string primResName =
		BWResource::removeExtension( visualName ) + ".primitives";
	const std::string tempBSPName( "\\temp.bsp" );
	const _TCHAR wTempBSPName[] = _T("\\temp.bsp");
	bool result = pGeneratedBSP->pTree()->save( tempBSPName );

	// We may now have a .bsp file, move it into the .primitives file
	DataSectionPtr ds = BWResource::openSection( tempBSPName );
	BinaryPtr bp;

	if (ds && (bp = ds->asBinary()))
	{
		DataSectionPtr p = BWResource::openSection( primResName );
		if (p)
		{
			p->writeBinary( "bsp2", bp );

			DataSectionPtr materialIDsSection = BWResource::openSection( "temp_bsp_materials.xml", true );
			materialIDsSection->writeStrings( "id", bspMaterialIDs );
			p->writeBinary( "bsp2_materials", materialIDsSection->asBinary() );

			p->deleteSection( "bsp" );
			p->save( primResName );
		}
		else
		{
			ret = "Unable to open primitives file ";
			ret += primResName;
			return ret;
		}

		// Clean up the .bsp file		
		::DeleteFile( wTempBSPName );
		ret = "BSP2 data created successfully for ";
		ret += visualName;
		return ret;
	}
	else
	{
		return "File error during generation of BSP data";
	}
}


};	//namespace


/*~ function AssetProcessor updateBSPVersion
 *  This function is a tool which can be used to udpate the version of the
 *	bsp file used by a visual.  It converts any .bsp file and 'bsp' section
 *	in a primitives file to a 'bsp2' section in a primitives file.  It does
 *	not delete any old '.bsp' files, these must be removed from your source
 *	repository manually.
 *
 *  @param visulaName A string containing the name (including path) of the
 *  visual file for which you'd like to upgrade the bsp.
 *
 *  @return ErrorString A string describing the error, or "" to indicate
 *	no error.  Errors will be produced if the existing visual uses a custom
 *	bsp.  There is no way to automatically upgrade a custom bsp, this must
 *	be done manually be re-exporting the visual file.
 */
static std::string updateBSPVersion( const std::string& visualName )
{
	std::string ret;	

	//First check whether this visual already has bsp2 data.  If so, we are done.
	bool wasNewVersion = false;
	bool wasBrokenBSP2 = false;	
	Moo::VisualLoader< Moo::Visual > loader( baseName( visualName ) );
	// Load BSP Tree
	Moo::BSPMaterialIDs 	bspMaterialIDs;
	Moo::Visual::BSPTreePtr 		pFoundBSP;	
	bool bspRequiresMapping = loader.loadBSPTree( pFoundBSP, bspMaterialIDs );

	wasNewVersion = bspRequiresMapping;
	 
	if (wasNewVersion)
	{
		if (bspMaterialIDs.empty())
		{
			//re-export as long as there isn't a custom bsp.
			wasBrokenBSP2 = true;			
		}
		else
		{
			//hooray - this has already been updated.
			return "";
		}
	}

	bool originalFileHadBSP = (pFoundBSP != NULL);	
	int nFoundBSPTriangles = 0;
	if (originalFileHadBSP)
	{		
		nFoundBSPTriangles = pFoundBSP->pTree()->triangles().size();		
	}

	//We will check the number of triangles is the same as in the visual.  If it's not,
	//then it's may be a custom BSP.  However since meshes may have degenerate
	//triangles, the bsp triangle count and the visual's triangle count may be
	//different also.  So we need to construct a BSP anyway, and check that the
	//number of post-degenerate triangles matches the number of tris in the visual.	
	Moo::BSPProxyPtr pGeneratedBSP = NULL;
	uint32 nVisualTris = 0;
	uint32 nDegenerateTris = 0;
	ret = AssetProcessorScript::generateBSP2(
		visualName,
		pGeneratedBSP,
		bspMaterialIDs,
		nVisualTris,
		nDegenerateTris );

	if (!ret.empty())
	{
		//an error occurred while generating the BSP.  leave now.
		return ret;
	}
	
	if (pGeneratedBSP)
	{			
		//Note about triangle count check.  If the generated BSP size is the same
		//as the existing BSP size, then that's a pretty good check that we are
		//not using a custom BSP.  For further certainty, we also check that the
		//number of triangles in the existing BSP plus the number of degenerate
		//triangles equals the number of triangles in the visual.
		//
		//And if there was no existing BSP, then just create one.	
		int nGeneratedBSPTriangles = pGeneratedBSP->pTree()->triangles().size();
		bool triCountEqual = ( nGeneratedBSPTriangles == nFoundBSPTriangles &&
			(nFoundBSPTriangles + nDegenerateTris) == nVisualTris );

		if ( triCountEqual || (nFoundBSPTriangles == 0) )
		{			
			//save BSP2 data and clean up existing data.	
			AssetProcessorScript::replaceBSPData( visualName, pGeneratedBSP, bspMaterialIDs );
			ret = "";
		}
		else
		{
			ret = visualName;
			ret += " has a custom BSP.  Please re-export this visual manually.";			
		}
	}
	else
	{
		//generating the BSP from the visual will return NULL if the visual only
		//contains skinned / mesh particle vertices, or only contained degenerate
		//triangles.  This is ok.
		if (originalFileHadBSP)
		{
			//Can't create a new BSP from the vertices but the old file had a BSP.
			ret = visualName;

			if (nFoundBSPTriangles > 0)
			{				
				//This suggests that the object is skinned but somebody created a
				//custom BSP.				
				ret += " has a custom BSP.  Please re-export this visual manually.";
				return ret;
			}
			else
			{
				//Can't create a new BSP from the vertices but the old file had a BSP,
				//which was 0 length.  This is a weird case, and means it's most likely
				//all degenerate triangles.
				if (nDegenerateTris > 0)
				{					
					ret += " has a zero-sized BSP.  This is probably due to "
						"the visual only containing degenerate triangles.  "
						"Please check the visual and re-export it manually.";
				}
				else
				{
					ret += " has a zero-sized BSP, but used to have a BSP "
						"(which also was zero sized).  The visual either "
						"has no triangles or there is some other error. "
						"Please check."	;					
				}
				return ret;		
			}
		}
		else
		{
			//Both new and old files do not have BSPs.  This is ok because the
			//uses a vertex format that does not produced BSPs (object was skinned)
			return "";
		}
	}

	return ret;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, updateBSPVersion, ARG( std::string, END ), _AssetProcessor )



/*~ function AssetProcessor upgradeHardskinnedVertices
 *  This function is a tool which can be used to change the vertex format
 *	of vertices used by a visual, upgrading hardskinned vertices to soft-
 *	skinned ones.  This process is necessary due to the removal of all
 *	hardskinned shaders.
 *
 *  @param visualName A string containing the name (including path) of the
 *  visual file for which you'd like to upgrade the vertices.
 *	@param retErrorString Returned error / info string
 *
 *  @return bool Returns true if the vertices have been changed and saved.
 */
static bool upgradeHardskinnedVertices( const std::string& resourceID, std::string& ret )
{
	// open the resource
	DataSectionPtr root = BWResource::instance().rootSection()->openSection( resourceID );
	if (!root)
	{
		ret = "Couldn't open visual ";
		ret += resourceID;
		return false;
	}
	
	std::string baseName = ::baseName( resourceID );	
	std::string primitivesFileName = baseName + ".primitives/";

	// open the renderesets
	std::vector< DataSectionPtr > renderSets;
	root->openSections( "renderSet", renderSets );

	if (renderSets.size() == 0)
	{
		ret = "No rendersets in visual ";
		ret += resourceID;
		return false;
	}

	ret = "";
	bool changed = false;

	// iterate through the rendersets and create them
	std::vector< DataSectionPtr >::iterator rsit = renderSets.begin();
	std::vector< DataSectionPtr >::iterator rsend = renderSets.end();
	while (rsit != rsend)
	{
		DataSectionPtr renderSetSection = *rsit;
		++rsit;
		Moo::Visual::RenderSet renderSet;		

		// Open the geometry sections
		std::vector< DataSectionPtr > geometries;
		renderSetSection->openSections( "geometry", geometries );

		if (geometries.size() == 0)
		{
			ret = "No geometry in renderset in " + resourceID;
			return false;			
		}

		std::vector< DataSectionPtr >::iterator geit = geometries.begin();
		std::vector< DataSectionPtr >::iterator geend = geometries.end();

		// iterate through the geometry sections
		while (geit != geend)
		{
			DataSectionPtr geometrySection = *geit;
			++geit;
			Moo::Visual::Geometry geometry;

			// Get a reference to the vertices used by this geometry
			std::string verticesName = geometrySection->readString( "vertices" );
			if (verticesName.find_first_of( '/' ) >= verticesName.size())
				verticesName = primitivesFileName + verticesName;
			geometry.vertices_ = Moo::VerticesManager::instance()->get( verticesName );

			if (geometry.vertices_->format() == "xyznuvi")
			{
				ret = verticesName + " has hard-skinned, non bumped vertices.  Changed to xyznuviiiww format.";
				geometry.vertices_->resaveHardskinnedVertices();
				changed = true;
			}
			else if (geometry.vertices_->format() == "xyznuvitb")
			{
				ret = verticesName + " has hard-skinned, bumped vertices.  Changed to xyznuviiiwwtb format.";
				geometry.vertices_->resaveHardskinnedVertices();
				changed = true;
			}
		}		
	}
	
	return changed;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, upgradeHardskinnedVertices, ARG( std::string, ARG( std::string, END )), _AssetProcessor )


/*~ function AssetProcessor getGuid
 *  This function generates a UniqueID of the form 81A9D1BF.4B8B622E.6F7081B3.0698330E
 *
 *  @return The UniqueID.
 */
static std::string getGuid()
{
	UniqueID guid = UniqueID::generate();
	return guid.toString();
}

PY_AUTO_MODULE_FUNCTION( RETDATA, getGuid, END, _AssetProcessor )


/*~ function AssetProcessor outsideChunkIdentifier
 *
 *	This method finds the identifier of a chunk given a position in world coordinates.
 *	This method was copied from chunk_format.hpp.
 *
 *	@param position		The position in world space coordinates to find the chunk
 *						identifier of.
 *
 *  @return The outside chunk identifier.
 */
static std::string outsideChunkIdentifier( const Vector3 &position )
{
	int gridX = int(floorf( position.x / 100.f ));
	int gridZ = int(floorf( position.z / 100.f ));

	char chunkIdentifierCStr[32];
	std::string gridChunkIdentifier;

    uint16 gridxs = uint16(gridX), gridzs = uint16(gridZ);
	if (uint16(gridxs + 4096) >= 8192 || uint16(gridzs + 4096) >= 8192)
	{
			bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%01xxxx%01xxxx/sep/",
					int(gridxs >> 12), int(gridzs >> 12) );
			gridChunkIdentifier = chunkIdentifierCStr;
	}
	if (uint16(gridxs + 256) >= 512 || uint16(gridzs + 256) >= 512)
	{
			bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%02xxx%02xxx/sep/",
					int(gridxs >> 8), int(gridzs >> 8) );
			gridChunkIdentifier += chunkIdentifierCStr;
	}
	if (uint16(gridxs + 16) >= 32 || uint16(gridzs + 16) >= 32)
	{
			bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%03xx%03xx/sep/",
					int(gridxs >> 4), int(gridzs >> 4) );
			gridChunkIdentifier += chunkIdentifierCStr;
	}
	bw_snprintf( chunkIdentifierCStr, sizeof(chunkIdentifierCStr), "%04x%04xo", int(gridxs), int(gridzs) );
	gridChunkIdentifier += chunkIdentifierCStr;

	return gridChunkIdentifier;
}

PY_AUTO_MODULE_FUNCTION( RETDATA, outsideChunkIdentifier,  ARG( Vector3, END ), _AssetProcessor )

static PyObject* compileShader( const std::string& resourceID, PyObjectPtr macroCombination, bool force )
{
	using namespace Moo;

	EffectMacroSetting::MacroSettingVector macroSettings;
	EffectMacroSetting::getMacroSettings( macroSettings );

	std::string ret;
	//compileShaderCombinations( resourceID, macroSettings, 0, ret );

	if (! PyTuple_Check(macroCombination.get()) )
	{
		PyErr_SetString( PyExc_TypeError, "compileShader arg must be a tuple of integers.");
		Py_RETURN_NONE;
	}
	
	size_t comboLen = PyTuple_Size( macroCombination.get() );
	if ( comboLen != macroSettings.size() )
	{
		PyErr_SetString( PyExc_TypeError, "compileShader macroCombination tuple is the wrong length. Must be len(getShaderMacros).");
		Py_RETURN_NONE;
	}

	// Setup the macro settings
	for (size_t i = 0; i < comboLen; i++)
	{
		EffectMacroSetting::EffectMacroSettingPtr setting = macroSettings[i];
		long settingIdx = PyInt_AsLong (PyTuple_GetItem( macroCombination.get(), i ));

		setting->selectOption( settingIdx, true );
	}

	EffectMacroSetting::updateManagerInfix();

	// Compile
	std::string compileResult;

	ManagedEffectPtr effect = new ManagedEffect();	
	int success = effect->compile( resourceID, NULL, force, &compileResult ) ? 1 : 0;	
	return Py_BuildValue( "(is)", success, compileResult.c_str() );
}


PY_AUTO_MODULE_FUNCTION( RETOWN, compileShader,  ARG( std::string, ARG( PyObjectPtr, ARG( bool, END))), _AssetProcessor )

static PyObject * getShaderMacros()
{
	using namespace Moo;

	EffectMacroSetting::MacroSettingVector macroSettings;
	EffectMacroSetting::getMacroSettings( macroSettings );

	PyObject* ret = PyList_New( macroSettings.size() );

	for (size_t i = 0; i < macroSettings.size(); i++)
	{
		EffectMacroSetting::EffectMacroSettingPtr setting = macroSettings[i];
		PyObject* tuple = Py_BuildValue( "(si)", setting->macroName().c_str(), setting->numMacroOptions() );
		PyList_SetItem( ret, i, tuple );
	}

	return ret;
}

PY_AUTO_MODULE_FUNCTION( RETOWN, getShaderMacros, END, _AssetProcessor )

static PyObject* shaderNeedsRecompile( const std::string& resourceID, PyObjectPtr macroCombination )
{
	using namespace Moo;

	EffectMacroSetting::MacroSettingVector macroSettings;
	EffectMacroSetting::getMacroSettings( macroSettings );

	std::string ret;
	//compileShaderCombinations( resourceID, macroSettings, 0, ret );

	if (! PyTuple_Check(macroCombination.get()) )
	{
		PyErr_SetString( PyExc_TypeError, "shaderNeedsRecompile arg must be a tuple of integers.");
		Py_RETURN_NONE;
	}
	
	size_t comboLen = PyTuple_Size( macroCombination.get() );
	if ( comboLen != macroSettings.size() )
	{
		PyErr_SetString( PyExc_TypeError, "shaderNeedsRecompile macroCombination tuple is the wrong length. Must be len(getShaderMacros).");
		Py_RETURN_NONE;
	}

	// Setup the macro settings
	for (size_t i = 0; i < comboLen; i++)
	{
		EffectMacroSetting::EffectMacroSettingPtr setting = macroSettings[i];
		long settingIdx = PyInt_AsLong (PyTuple_GetItem( macroCombination.get(), i ));

		setting->selectOption( settingIdx, true );
	}

	EffectMacroSetting::updateManagerInfix();

	if (Moo::EffectManager::needRecompile( resourceID ))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}

PY_AUTO_MODULE_FUNCTION( RETOWN, shaderNeedsRecompile, ARG( std::string, ARG( PyObjectPtr, END)), _AssetProcessor )


extern int FullScreenBackBufferSetting_token;
extern int SkyLightMapSetting_token;

int s_macroSettingTokens =	FullScreenBackBufferSetting_token | 
							SkyLightMapSetting_token;
