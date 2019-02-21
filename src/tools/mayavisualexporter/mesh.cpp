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

#include "mesh.hpp"
#include "resmgr/bwresource.hpp"
#include "maya/MFnAttribute.h"

#include "ExportIterator.h"
#include "utility.hpp"
#include <maya/MFnAttribute.h>

template<typename MObj>
bool isObjectVisible( MObj& obj )
{
	bool visible = true;
	MStatus status;
	MPlug plug = obj.findPlug( "visibility", &status );
	if( status == MS::kSuccess )
		plug.getValue( visible );
	if( visible )
	{
		MPlug plug = obj.findPlug( "v", &status );
		if( status == MS::kSuccess )
			plug.getValue( visible );
	}

	if ( visible )
	{
		MPlugArray mpa;
		obj.getConnections(mpa);
		for (unsigned int i=0; i<mpa.length(); i++)
		{
			if ( (std::string(mpa[i].name().asChar()).find("drawOverride") != std::string::npos) && mpa[i].isCompound() )
			{
				bool ovrEnabled = false;
				mpa[i].child(5).getValue(ovrEnabled);
				if (ovrEnabled)
					mpa[i].child(6).getValue(visible);
			
				break;
			}
		}
	}

	return visible;
}

Mesh::Mesh()
{
	MStatus status;
	ExportIterator it( MFn::kMesh, &status );

	if( status == MStatus::kSuccess )
	{
		for( ; it.isDone() == false; it.next() )
		{
			bool visible = true;
			MFnMesh mesh( it.item(), &status );
			if( status != MS::kSuccess )
				visible = false;
			else
				visible = isObjectVisible( mesh );

			if( visible && mesh.parentCount() == 1 )
			{
				MFnTransform transform( mesh.parent( 0 ), &status );
				if( status == MS::kSuccess )
					visible = isObjectVisible( transform );
			}
			// ignore intermediate meshes
			if( visible && mesh.isIntermediateObject() == false )
				_meshes.append( MDagPath::getAPathTo( it.item() ) );
		}
	}
}

Mesh::~Mesh()
{
	finalise();
}

uint32 Mesh::count()
{
	return (uint32)_meshes.length();
}

bool Mesh::initialise( uint32 index, bool objectSpace )
{
	if( index > _meshes.length()-1 )
		return false;
	
	finalise();
	
	_fullName = _meshes[index].fullPathName().asChar();
	_name = _fullName.substr( _fullName.find_last_of( "|" ) +  1 );
	MStatus stat;
	MFnTransform trans(_meshes[index].transform(), &stat );
	if (stat == MS::kSuccess)
	{
		_nodeName = trans.fullPathName().asChar();
		_nodeName = _nodeName.substr( _nodeName.find_last_of( "|" ) + 1 );
	}
	else
		_nodeName = "Scene Root";


	// set to first frame of the animation
	MAnimControl control;
	
	// store current time
	MTime time = control.currentTime();
	
	control.setCurrentTime( control.animationStartTime() );

	MFnMesh mesh( _meshes[index] );

	MSpace::Space space = objectSpace ? MSpace::kObject : MSpace::kWorld;
	
	// get vertex positions
	MPointArray vertexList;
	mesh.getPoints( vertexList, space );
	
	for( uint32 i = 0; i < vertexList.length(); ++i )
	{
		vertexList[i].cartesianize();
		
		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			_positions.push_back(
				Point3(
					-static_cast<float>( vertexList[i].x ),
					static_cast<float>( vertexList[i].y ),
					static_cast<float>( vertexList[i].z ) ) );
		}
		else
		{
			_positions.push_back(
				Point3(
					static_cast<float>( vertexList[i].x ),
					static_cast<float>( vertexList[i].y ),
					-static_cast<float>( vertexList[i].z ) ) );
		}

	}
	
	// get normals
	MFloatVectorArray normalList;
	mesh.getNormals( normalList, space );
	
	for( uint32 i = 0; i < normalList.length(); ++i )
	{
		_normals.push_back( Point3() );
		normalList[i].get( &(_normals.back().x) );

		if (ExportSettings::instance().useLegacyOrientation())
		{
			// This legacy code came from the 1.7.2 release
			_normals.back().x = -_normals.back().x;
		}
		else
		{
			_normals.back().z = -_normals.back().z;
		}
	}
	
	// get texture coordinates
	MFloatArray uList;
	MFloatArray vList;
	mesh.getUVs( uList, vList );
	
	if( uList.length() != vList.length() )
		return false;
	
	for( uint32 i = 0; i < uList.length(); ++i )
	{
		_uvs.push_back( Point2( uList[i], vList[i] ) );
	}
	
	// get the shaders from mesh instance 0 (assuming all instances share the same materials)
	// TODO : this may have to be changed.
	
	if( mesh.instanceCount( true ) < 1 )
		return false;
	
	MObjectArray shaders;
	MIntArray materialIndexes;
	mesh.getConnectedShaders( 0, shaders, materialIndexes );

	// create the polygon face std::vector
	for( uint32 p = 0; p < (uint32)mesh.numPolygons(); ++p )
	{
		MIntArray positionIndexes;
		mesh.getPolygonVertices( p, positionIndexes );
		
		MIntArray normalIndexes;
		mesh.getFaceNormalIds( p, normalIndexes );

		// Not sure if this would ever happen, but check just in case.
		if( positionIndexes.length() > normalIndexes.length()  )
			return false; // TODO : error?
		
		// Triangulate polygon, assumes the polygon is a fan.
		for (uint i = 0; i < (positionIndexes.length()-2); i++)
		{
			// Note: order of normals is not the same as the order of positions
			_faces.push_back( Face() );

			int index1 = i + 1;
			int index2 = i + 2;
			
			_faces.back().positionIndex[0] = positionIndexes[0];
			_faces.back().positionIndex[1] = positionIndexes[index1];
			_faces.back().positionIndex[2] = positionIndexes[index2];

			_faces.back().normalIndex[0] = normalIndexes[0];
			_faces.back().normalIndex[1] = normalIndexes[index1];
			_faces.back().normalIndex[2] = normalIndexes[index2];

			_faces.back().materialIndex = materialIndexes[p];

			mesh.getPolygonUVid( p, 0, _faces.back().uvIndex[0] );
			mesh.getPolygonUVid( p, index1, _faces.back().uvIndex[1] );
			mesh.getPolygonUVid( p, index2, _faces.back().uvIndex[2] );
		}
	}
	
	// get material parameters
	for( uint32 m = 0; m < shaders.length(); ++m )
	{
		// find the surface shader for the material
		MFnDependencyNode materials( shaders[m] );
		MPlug shaderPlug = materials.findPlug( "surfaceShader" );
		
		if( shaderPlug.isNull() == false )
		{
			MPlugArray shaderPlugSources;
			shaderPlug.connectedTo( shaderPlugSources, true, false );
			
			for( uint32 i = 0; i < shaderPlugSources.length(); ++i )
			{
				MPlug plug = shaderPlugSources[i];
				
				// get the base shader name
				_materials.push_back( material() );
				_materials.back().name = plug.name().asChar();
				_materials.back().name = _materials.back().name.substr( 0, _materials.back().name.find( '.' ) );
				
				// get the first texture on the shader as the colour std::map (TODO : support other maps?)
				for( MItDependencyGraph it( plug, MFn::kFileTexture, MItDependencyGraph::kUpstream ); it.isDone() == false; it.next() )
				{
					MObject node = it.thisNode();
					MFnDependencyNode texture( node );
					MString file;
					texture.findPlug( "fileTextureName" ).getValue( file );

					// check for mfm
					std::string mfmName = BWResource::removeExtension( file.asChar() ) + ".mfm";
					if ( BWResource::fileExists( BWResolver::resolveFilename( mfmName ) ) )
					{
						_materials.back().mapFile = mfmName;
						_materials.back().mapIdMeaning = 2;		// .mfm magic number
						_materials.back().fxFile = "";
					}
					// assign it
					else if( _materials.back().mapFile == "" )
					{
						_materials.back().mapFile = file.asChar();
						_materials.back().mapIdMeaning = 1;		// bitmap magic number
						// what type of fx file to use?
					}
				}
			}
		}
	}
	
	// restore initial time
	control.setCurrentTime( time );
	
	return true;
}

void Mesh::finalise()
{
	_name = "";
	_nodeName = "";
	_positions.clear();
	_normals.clear();
	_uvs.clear();
	_faces.clear();
	_materials.clear();
}

MDagPathArray& Mesh::meshes()
{
	return _meshes;
}

std::string Mesh::name()
{
	return _name;
}

std::string Mesh::nodeName()
{
	return _nodeName;
}

std::string Mesh::fullName()
{
	return _fullName;
}

std::vector<Point3>& Mesh::positions()
{
	return _positions;
}

std::vector<Point3>& Mesh::normals()
{
	return _normals;
}

std::vector<Point2>& Mesh::uvs()
{
	return _uvs;
}

std::vector<Face>& Mesh::faces()
{
	return _faces;
}

std::vector<material>& Mesh::materials()
{
	return _materials;
}