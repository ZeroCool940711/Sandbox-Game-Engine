/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#pragma warning (disable:4786)

#include "mfxexp.hpp"
#include "visual_envelope.hpp"
#include "mfxexp.hpp"
#include "utility.hpp"
#include "resmgr/bwresource.hpp"
#include "resmgr/bin_section.hpp"
#include "resmgr/quick_file_writer.hpp"

#include "iparamm2.h"
#include "iskin.h"

#ifndef CODE_INLINE
#include "visual_envelope.ipp"
#endif

extern TimeValue timeNow();
// -----------------------------------------------------------------------------
// Section: VisualEnvelope
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
VisualEnvelope::VisualEnvelope()
{
}


/**
 *	Destructor.
 */
VisualEnvelope::~VisualEnvelope()
{
}

void VisualEnvelope::getScaleValues( std::vector< Point3 > & scales )
{
	MatrixVector::iterator it = initialTransforms_.begin();
	MatrixVector::iterator end = initialTransforms_.end();

	while (it != end)
	{
		Matrix3& m = *it++;
		scales.push_back( Point3( 1.f / Length( m.GetRow(0) ),
								  1.f / Length( m.GetRow(1) ),
								  1.f / Length( m.GetRow(2) ) ) );
	}
}

bool VisualEnvelope::collectInitialTransforms(IPhysiqueExport* phyExport)
{
	bool ret = true;

	NodeVector::iterator it = boneNodes_.begin();
	NodeVector::iterator end = boneNodes_.end();

	while (it != end && ret)
	{
		INode * node = *it++;
		Matrix3 itm;
		ret = (MATRIX_RETURNED == phyExport->GetInitNodeTM( node, itm ) );
		if (ret)
		{
			initialTransforms_.push_back(itm);
		}
	}
	return ret;
}

void VisualEnvelope::normaliseInitialTransforms()
{
	MatrixVector::iterator it = initialTransforms_.begin();
	MatrixVector::iterator end = initialTransforms_.end();

	while (it != end)
	{
		Matrix3& m = *it++;
		m = normaliseMatrix( m );
	}
}

void VisualEnvelope::scaleBoneVertices( std::vector< Point3 >& scales )
{
	BoneVVector::iterator it = boneVertices_.begin();
	BoneVVector::iterator end = boneVertices_.end();

	while (it != end)
	{
		BoneVertex& bv = *it++;
		assert( uint(bv.boneIndex_) < scales.size() );
		bv.position_ = bv.position_ * scales[ bv.boneIndex_ ];
	}
}

void VisualEnvelope::initialPoseVertices()
{
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		BloatVertex& bloatV = *it++;
		BoneVertex& boneV = boneVertices_[ bloatV.vertexIndex ];
		bloatV.pos = initialTransforms_[ boneV.boneIndex_ ] * boneV.position_;
	}
}

void VisualEnvelope::relaxedPoseVertices()
{

	MatrixVector normalInvert;

	for (uint i = 0; i < initialTransforms_.size(); i++ )
	{
		Matrix3 m = Inverse( initialTransforms_[i] );
		m.SetRow(3, Point3::Origin);
		normalInvert.push_back( m );
	}

	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();

	while (it!=end)
	{
		BloatVertex& bloatV = *it++;
		BoneVertex& boneV = boneVertices_[ bloatV.vertexIndex ];
		bloatV.pos = boneV.position_;
		bloatV.normal = normalInvert[ boneV.boneIndex_ ] * bloatV.normal;
		bloatV.normal = Normalize( bloatV.normal );
	}

	for (uint i = 0; i < morphTargets_.size(); i++)
	{
		MorphTarget& mt = morphTargets_[i];
		for (uint j = 0; j < mt.verts.size(); j++)
		{
			MorphVertex& mv = mt.verts[j];
			BoneVertex& boneV = boneVertices_[mv.vertexIndex];
			mv.delta = initialObjectTransform_ * mv.delta;
			mv.delta =  normalInvert[ boneV.boneIndex_ ] * mv.delta;
		}
	}
}

void VisualEnvelope::createVertexList( VertexVector& vertices )
{
	VertexXYZNUVI v;
	BVVector::iterator it = vertices_.begin();
	BVVector::iterator end = vertices_.end();
	while (it!=end)
	{
		BloatVertex& bv = *it++;
		bv.pos *= ExportSettings::instance().unitScale();
		v.pos[0] = bv.pos.x;
		v.pos[1] = bv.pos.z;
		v.pos[2] = bv.pos.y;
		v.normal[0] = bv.normal.x;
		v.normal[1] = bv.normal.z;
		v.normal[2] = bv.normal.y;
		v.uv[0] = bv.uv.x;
		v.uv[1] = bv.uv.y;
		v.index = float( 3 * boneVertices_[ bv.vertexIndex ].boneIndex_ );
		assert( v.index / 3 < boneNodes_.size() ); 
		vertices.push_back( v );
	}
}

bool VisualEnvelope::init( INode* node, MFXNode* root )
{
	bool ret = false;

	Modifier* pPhyMod = MFXExport::findPhysiqueModifier( node );
	Modifier* pSkinMod = MFXExport::findSkinMod( node );
	if (pPhyMod)
	{
		IPhysiqueExport* phyExport = (IPhysiqueExport*)pPhyMod->GetInterface( I_PHYINTERFACE );
		if (phyExport)
		{
			IPhyContextExport* contextExport = (IPhyContextExport*)phyExport->GetContextInterface( node );
			if( contextExport )
			{
				// Set the blending precision to be single bone influence.
				contextExport->AllowBlending( TRUE );
				contextExport->ConvertToRigid( TRUE );

				int nVertices = contextExport->GetNumberVertices();
				for( int i = 0; i < nVertices; i++ )
				{
					// Get the vertex interface for the vertex
					IPhyVertexExport *vex = contextExport->GetVertexInterface( i );
					if (vex->GetVertexType() == RIGID_NON_BLENDED_TYPE)
					{
						IPhyRigidVertex *rv = (IPhyRigidVertex*) vex;

						INode *n = rv->GetNode();

						if (n == NULL)
						{
							// Something very bad is after happening, quit now.
							MessageBox( GetForegroundWindow(),
								"Physique could not retrieve a node for one of the vertices.\n"
								"The animation may not appear as it should inside the BigWorld engine.",
								"Animation Exporter", MB_OK | MB_ICONEXCLAMATION );
						
							return false;
						}

						boneIndex( n );
					}
					else if (vex->GetVertexType() == RIGID_BLENDED_TYPE)
					{
						IPhyBlendedRigidVertex* rbv = (IPhyBlendedRigidVertex*) vex;
						int nNodes = rbv->GetNumberNodes();


						typedef std::multimap< float, int > IndexWeights;
						IndexWeights indexWeights;

						Point3 p(0, 0, 0);

						for (int i = 0; i < nNodes; i++)
						{
							Matrix3 transform;
							INode* n = rbv->GetNode(i);

							if( n == NULL )
								continue;

							boneIndex(n);
						}
					}
					else
					{
						MessageBox( GetForegroundWindow(),
							"VisualEnvelope::init - unknown vertex type in physiqued object\n",
							"Visual Exporter", MB_OK | MB_ICONEXCLAMATION );
						return false;
					}
					contextExport->ReleaseVertexInterface( vex );
				}
				phyExport->ReleaseContextInterface( contextExport );
			}
		}
		pPhyMod->ReleaseInterface(I_PHYINTERFACE, phyExport);

		for (NodeVector::size_type i = 0; i < boneNodes_.size(); i++)
		{
			MFXNode* x = root->find( boneNodes_[ i ] );
			x->include( true );
		}
		ret = true;
	}
	else if (pSkinMod)
	{
		ISkin* pSkin = (ISkin*)pSkinMod->GetInterface( I_SKIN );
		if (pSkin)
		{
			ISkinContextData* pContext = pSkin->GetContextInterface(node);
			if (pContext)
			{
				int nVertices = pContext->GetNumPoints();
				for (int i = 0; i < nVertices; i++)
				{
					int nNodes = pContext->GetNumAssignedBones(i);

					typedef std::multimap< float, int > IndexWeights;
					IndexWeights indexWeights;

					for (int j = 0; j < nNodes; j++)
					{
						Matrix3 transform;
						INode* n = pSkin->GetBone(pContext->GetAssignedBone(i, j));

						assert( n != NULL );

						boneIndex(n);
					}
				}
			}
		}

		for (NodeVector::size_type i = 0; i < boneNodes_.size(); i++)
		{
			MFXNode* x = root->find( boneNodes_[ i ] );
			x->include( true );
		}
		ret = true;
	}

	captureMorphTargets( node );

	return ret;
}

void VisualEnvelope::save( DataSectionPtr spVisualSection, const std::string& primitiveFile )
{
	DataSectionPtr spFile = BWResource::openSection( primitiveFile );
	if (!spFile)
		spFile = new BinSection( primitiveFile, new BinaryBlock( 0, 0, "BinaryBlock/VisualEnvelope" ) );

	if (spFile)
	{
		flipTriangleWindingOrder();
		sortTriangles();

		PGVector primGroups;
		IndexVector indices;

		createPrimitiveGroups( primGroups, indices );

		IndexHeader ih;
		bw_snprintf( ih.indexFormat, sizeof(ih.indexFormat), "list" );
		ih.nIndices = indices.size();
		ih.nTriangleGroups = primGroups.size();

		QuickFileWriter f;
		f << ih;
		f << indices;
		f << primGroups;

		spFile->writeBinary( identifier_ + ".indices", f.output() );

		f = QuickFileWriter();

		VertexVector vertices;
		createVertexList( vertices );
		VertexHeader vh;
		vh.nVertices = vertices.size();
		bw_snprintf( vh.vertexFormat, sizeof(vh.vertexFormat), "xyznuvi" );
		f << vh;
		f << vertices;

		writeMorphTargets( f );

		if (morphTargets_.size())
			spFile->writeBinary( identifier_ + ".mvertices", f.output() );
		else
			spFile->writeBinary( identifier_ + ".vertices", f.output() );

		spFile->save( primitiveFile );

		DataSectionPtr spRenderSet = spVisualSection->newSection( "renderSet" );
		if (spRenderSet)
		{
			spRenderSet->writeBool( "treatAsWorldSpaceObject", true );
//			spRenderSet->writeString( "node", identifier_ );
			for (uint i = 0; i < boneNodes_.size(); i++)
			{
				DataSectionPtr spNode = spRenderSet->newSection( "node" );
				if (spNode)
					spNode->setString( trimWhitespaces( boneNodes_[i]->GetName() ) );
//				spRenderSet->addString( "node", trimWhitespaces( boneNodes_[i]->GetName() ) );
			}


			DataSectionPtr spGeometry = spRenderSet->newSection( "geometry" );
			if (spGeometry)
			{
				if (morphTargets_.size())
					spGeometry->writeString( "vertices", identifier_ + ".mvertices" );
				else
					spGeometry->writeString( "vertices", identifier_ + ".vertices" );
				spGeometry->writeString( "primitive", identifier_ + ".indices" );

				MaterialVector::iterator it = materials_.begin();
				MaterialVector::iterator end = materials_.end();

				uint32 primGroup = 0;
				for (uint32 i = 0; i < materials_.size(); i++)
				{
					Material& mat = materials_[i];;
					if (mat.inUse)
					{
						DataSectionPtr spPG = spGeometry->newSection( "primitiveGroup" );
						if (spPG)
						{
							spPG->setInt( primGroup++ );
							spPG->writeString( "material/identifier", mat.identifier );
							spPG->writeFloat( "material/selfIllumination", mat.selfIllum );
							if (mat.hasMap)
								spPG->writeString( "material/texture", mat.mapIdentifier );
						}
					}
				}
			}
		}
	}
}


int VisualEnvelope::boneIndex( INode* node )
{
	NodeVector::iterator it = std::find( boneNodes_.begin(), boneNodes_.end(), node );

	if (it != boneNodes_.end())
	{
		int ret = 0;
		while (it != boneNodes_.begin())
		{
			++ret;
			--it;
		}
		return ret;
	}

	boneNodes_.push_back( node );
	return boneNodes_.size() - 1;
}

// visual_envelope.cpp
