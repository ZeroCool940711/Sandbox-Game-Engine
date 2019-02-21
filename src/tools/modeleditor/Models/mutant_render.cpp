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

#include "model/node_tree.hpp"
#include "model/super_model.hpp"
#include "model/super_model_animation.hpp"

#include "romp/sky.hpp"
#include "romp/geometrics.hpp"
#include "romp/labels.hpp"

#include "moo/software_skinner.hpp"
#include "moo/dynamic_vertex_buffer.hpp"

#include "math/convex_hull.hpp"

#include "physics2/bsp.hpp"

#include "me_consts.hpp"

#include "mutant.hpp"

DECLARE_DEBUG_COMPONENT2( "Mutant_Draw", 0 )

float Mutant::drawModel( bool atOrigin, float atDist /* = -1.f */, bool drawIt /* = true */ )
{
	if (!superModel_) return atDist;

	// Using auto-pointer to make sure our SkyBoxScopedSetup get's deleted on
	// exit, if it's ever created (isSkyBox_ == true).
	std::auto_ptr< SkyBoxScopedSetup > skySetup;
	if (isSkyBox_)
	{
		// Create a SkyBoxScopedSetup to get the viewport right and avoid 
		// z-fighting.
		skySetup.reset( new SkyBoxScopedSetup );
	}

	Moo::rc().push();
	if (!atOrigin)
		Moo::rc().preMultiply( transform( groundModel_, centreModel_ ));
		
	if (virtualDist_ != -1)
	{
		atDist = virtualDist_;
	}
		
	// We want the latest fashion information from the action queue
	if ( !animMode_ )
	{
		recreateFashions( true );
		fashions_.insert( fashions_.end(), actionQueue_.fv().begin(), actionQueue_.fv().end() );
	}
	
	atDist = superModel_->draw( &fashions_, 0, atDist, drawIt );

	if (!foundRootNode_)
	{
		initialRootNode_ = getCurrentRootNode();
		foundRootNode_ = true;
	}

	Moo::rc().pop();

	return atDist;
}

void Mutant::drawOriginalModel()
{
	if (!superModel_) return;

	if ( currAnims_.empty() ) return;
		
	Moo::rc().push();
    Moo::rc().preMultiply( transform( groundModel_, centreModel_ ) );

	std::map< size_t, std::vector<Moo::AnimationChannelPtr> > oldChannels;

	AnimIt animEnd = currAnims_.end();
	
	// OK let's now replace the animation channels with the uncompressed ones
	AnimIt animIt = currAnims_.begin();
	for (;animIt != animEnd; ++animIt)
	{
		if (animations_.find(animIt->second) == animations_.end()) 
			continue;

		SuperModelAnimationPtr superModelAnim = animations_[animIt->second].animation;

		if (superModelAnim == NULL) 
			continue;

		ModelAnimation* modelAnim = superModelAnim->pSource(*superModel_);

		if (modelAnim == NULL) 
			continue;

		Moo::AnimationPtr anim = modelAnim->getMooAnim();

		if (anim == NULL) 
			continue;

		animations_[animIt->second].uncompressAnim( anim, oldChannels[animIt->first] );
	}
		
	// Now draw the model
	superModel_->draw( &fashions_, 0, virtualDist_ );

	// Finally put everything back as it was...
	animIt = currAnims_.begin();
	for (;animIt != animEnd; ++animIt)
	{
		if (animations_.find(animIt->second) == animations_.end()) 
			continue;

		SuperModelAnimationPtr superModelAnim = animations_[animIt->second].animation;

		if (superModelAnim == NULL) 
			continue;

		ModelAnimation* modelAnim = superModelAnim->pSource(*superModel_);

		if (modelAnim == NULL) 
			continue;

		Moo::AnimationPtr anim = modelAnim->getMooAnim();

		if (anim == NULL) 
			continue;

		animations_[animIt->second].restoreAnim( anim, oldChannels[animIt->first] );
	}

	Moo::rc().pop();
}

void Mutant::drawBoundingBoxes()
{
	if (!superModel_) return;

	/*Vector3 minB ( modelBB_.minBounds() - frameBB_.minBounds() );
	Vector3 maxB ( modelBB_.maxBounds() - frameBB_.maxBounds() );
	
	if (minB.length() + maxB.length() > 0.01f)
	{
		Moo::rc().push();
		Moo::rc().preMultiply( transform( groundModel_, centreModel_ ) );
		Geometrics::wireBox( frameBB_, 0x00ffff00 );
		Moo::rc().pop();
	}*/
	
	Moo::rc().push();
	Matrix m;
	if (hasRootNode_)
	{
		m = Matrix::identity;
		Vector3 t( getCurrentRootNode() - initialRootNode_ );
		m.setTranslate( t );
	}
	else
	{
		m = transform( groundModel_, centreModel_ );
	}
	Moo::rc().preMultiply( m );
	Geometrics::wireBox( visibilityBB_, 0x000000ff );
	Geometrics::wireBox( modelBB_, 0x00ffff00 );
	Moo::rc().pop();
}

/**
 *	draws the skeleton of this model
 */
void Mutant::drawSkeleton()
{
	if (!superModel_) return;
	
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	// draw skeleton for each model
	for (int i=0; i<superModel_->nModels(); ++i)
	{
		Model * model = superModel_->curModel( i );
		if (model != NULL)
		{
			NodeTreeIterator it  = model->nodeTreeBegin();
            NodeTreeIterator end = model->nodeTreeEnd();

            //Advance past the base node
            if (it != end) it++;

			while (it != end)
			{
				// skip leaf nodes
				std::string nodeName = it->pData->pNode->identifier();
				if ( ( nodeName.substr(0, 3) != std::string("HP_") ) && ( nodeName.find("BlendBone", 0) == nodeName.npos) )
				{
					Vector3	fromPos = it->pParentTransform->applyToOrigin();

					const Matrix & nodeTrans = it->pData->pNode->worldTransform();
					Vector3 toPos = nodeTrans.applyToOrigin();

					// draw arrow body
					Geometrics::drawLine( fromPos, toPos, 0x0000ff00 );

					// draw arrow head
					Vector3 vDir  = (toPos - fromPos);
                    float length = 0.1f * vDir.length(); // Make the arrowhead proportional to the length of the bone.
					vDir.normalise();
					Vector3 ortog = vDir.crossProduct( nodeTrans[1] );
					ortog.normalise();
					Geometrics::drawLine( toPos, toPos - vDir * length + ortog * length, 0x00ff0000 );
					Geometrics::drawLine( toPos, toPos - vDir * length - ortog * length, 0x00ff0000 );
				}
				it++;
			}
		}
	}
	Moo::rc().pop();
}

void Mutant::drawPortals()
{
	if (!superModel_) return;

	if (!currVisual_) return;

	//Load the portal material if we haven't already
	if (!pPortalMat_)
	{
		pPortalMat_ = new Moo::EffectMaterial;
		pPortalMat_->initFromEffect( "resources/effects/portal.fx" );
		if (!pPortalMat_) return; 
	}

	Moo::rc().setVertexShader( NULL );
	Moo::rc().device()->SetPixelShader( NULL );
					
	Moo::rc().setRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
    Moo::rc().setFVF( Moo::VertexXYZ::fvf() );

    // First of all lets try and handle the old portal system

    std::vector<DataSectionPtr>	spOldPortalSets;
    currVisual_->openSections( "portal", spOldPortalSets );
    std::vector<DataSectionPtr>::iterator opit = spOldPortalSets.begin();
	std::vector<DataSectionPtr>::iterator opend = spOldPortalSets.end();
	while (opit != opend)
	{
		DataSectionPtr portalSet = *opit++;

        std::vector<DataSectionPtr>	spPointSets;
        portalSet->openSections( "point", spPointSets );
        std::vector<DataSectionPtr>::iterator ptit = spPointSets.begin();
        std::vector<DataSectionPtr>::iterator ptend = spPointSets.end();

        std::vector<Vector3> points;

        // Iterate through our points
        while (ptit != ptend)
        {
                points.push_back( (*ptit++)->asVector3() );
        }

        //Now we need to reorder the points to be in the correct order

        PlaneEq basePlane( points[0], points[1], points[2] );
        Vector3 baseNorm = basePlane.normal();
        baseNorm.normalise();

        bool swapped;
        do
        {
			swapped = false;
			for (unsigned i=3; i<points.size(); i++)
			{
				PlaneEq testPlane( points[0], points[i-1], points[i] );
				Vector3 testNorm = testPlane.normal();
				testNorm.normalise();
				if ((testNorm + baseNorm).length() < 1.0f)
				{
					swapped = true;
					Vector3 temp = points[i];
					points[i] = points[i-1];
					points[i-1] = temp;
				}
			}
        }
        while (swapped);

        Moo::rc().device()->SetTransform( D3DTS_WORLD, &transform( groundModel_, centreModel_ ) );
		Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
        if (pPortalMat_->begin())
        {
            for (uint32 i = 0; i < pPortalMat_->nPasses(); i++)
            {
                pPortalMat_->beginPass( i );
                Moo::rc().device()->DrawPrimitiveUP(
                    D3DPT_TRIANGLEFAN,
                    points.size() - 2,
                    &points[0],
                    sizeof( Moo::VertexXYZ ) );
                pPortalMat_->endPass();
            }
            pPortalMat_->end();
        }
    }

    // OK, now lets try the new setup

    std::vector<DataSectionPtr>	spBoundarySets;
    currVisual_->openSections( "boundary", spBoundarySets );
    std::vector<DataSectionPtr>::iterator bit = spBoundarySets.begin();
	std::vector<DataSectionPtr>::iterator bend = spBoundarySets.end();

	// Iterate through our boundaries.
	while (bit != bend)
	{
		DataSectionPtr boundarySet = *bit++;

        //Get the normal and distance
        Vector3 norm = boundarySet->readVector3( "normal", Vector3(0,0,0) );
        float d = boundarySet->readFloat( "d", 0.0 );

        std::vector<DataSectionPtr>	spPortalSets;
        boundarySet->openSections( "portal", spPortalSets );
        std::vector<DataSectionPtr>::iterator pit = spPortalSets.begin();
        std::vector<DataSectionPtr>::iterator pend = spPortalSets.end();

        // Iterate through our portals
        while (pit != pend)
        {
            DataSectionPtr portalSet = *pit++;

            Vector3 uAxis = portalSet->readVector3( "uAxis", Vector3(0,0,0) );
            Vector3 vAxis = norm.crossProduct( uAxis );
	        Vector3 origin = norm * d / norm.lengthSquared();

            std::vector<DataSectionPtr>	spPointSets;
            portalSet->openSections( "point", spPointSets );
            std::vector<DataSectionPtr>::iterator ptit = spPointSets.begin();
            std::vector<DataSectionPtr>::iterator ptend = spPointSets.end();

            std::vector<Vector3> points;

            // Iterate through our points
            while (ptit != ptend)
            {
				Vector3 pt = (*ptit++)->asVector3();

				points.push_back( Vector3( uAxis * pt[0] + vAxis * pt[1] + origin ) );
            }

            Moo::rc().device()->SetTransform( D3DTS_WORLD, &transform( groundModel_, centreModel_ ) );
			Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
			Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
            if (pPortalMat_->begin())
	        {
		        for (uint32 i = 0; i < pPortalMat_->nPasses(); i++)
		        {
			        pPortalMat_->beginPass( i );
			        Moo::rc().device()->DrawPrimitiveUP(
						D3DPT_TRIANGLEFAN,
						points.size() - 2,
						&points[0],
						sizeof( Moo::VertexXYZ ) );
			        pPortalMat_->endPass();
		        }
		        pPortalMat_->end();
	        }
        }
	}
}

void Mutant::reloadBSP()
{
	if (!superModel_) return;

	Moo::VisualPtr visual = superModel_->topModel(0)->getVisual();
	if (visual)	
	{
		visual->updateBSP();

		verts_.resize( 0 );

		const BSPTree *tree = superModel_->topModel(0)->decompose();

		if (tree)
		{
			Moo::BSPTreeHelper::createVertexList(*tree, verts_, 0x000000FF);
		}
	}
}

void Mutant::drawBsp()
{
	if ( (!superModel_) || (verts_.size() == 0) ) return;

	Moo::rc().push();

	//set the transforms
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &transform( groundModel_, centreModel_ ) );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	Moo::rc().setVertexShader( NULL );
	Moo::rc().device()->SetPixelShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );

	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE);
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().fogEnabled( false );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLELIST, verts_.size() / 3, &verts_[0], sizeof( Moo::VertexXYZL ) );

	Moo::rc().pop();
}


void Mutant::drawNormals( bool showNormals, bool showBinormals )
{
	if (!superModel_) return;

	//Determine a scale to use for the axes based on the total size of the model
	float scale = ((float)normalsLength_/50) * (modelBB_.maxBounds() - modelBB_.minBounds()).length() / 30.f;
	
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	Moo::rc().device()->SetTransform( D3DTS_WORLD     , &Moo::rc().world() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW      , &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	Moo::rc().setRenderState( D3DRS_FOGENABLE, FALSE );

	CustomMesh<Moo::VertexXYZL> mesh(D3DPT_LINELIST);
    Moo::VertexXYZL vertex;

	//set the transform
	for (int i=0; i<superModel_->nModels(); ++i)
	{
		Model * model = superModel_->curModel( i );
		if (model != NULL)
		{
			const Moo::Visual::RenderSetVector& set = model->getVisual()->renderSets();
			Moo::Visual::RenderSetVector::const_iterator it = set.begin();
			for (;it != set.end(); it++)
			{
				Vector3 transPos;

				Moo::Visual::GeometryVector::const_iterator g_it = (*it).geometry_.begin();

				const Moo::Visual::Geometry& geometry = *g_it;

				Moo::Vertices::VertexPositions points;
				Moo::Vertices::VertexNormals	 tangentspace;
				Moo::Vertices::VertexPositions normals;					
				Moo::Vertices::VertexNormals	 packedNormals;
				// Use the software skinner
				Moo::BaseSoftwareSkinner* pSkinner = geometry.vertices_->softwareSkinner();

				if (pSkinner)
				{
					Moo::DynamicVertexBufferBase2<Moo::VertexXYZNUVTBPC>& vb = Moo::DynamicVertexBufferBase2<Moo::VertexXYZNUVTBPC>::instance();
					Moo::VertexXYZNUVTBPC* pVerts = vb.lock( geometry.vertices_->nVertices() );
					pSkinner->transformVertices( pVerts, 0, geometry.vertices_->nVertices(), (*it).transformNodes_ );
					vb.unlock();
					for ( uint32 i = 0; i < geometry.vertices_->nVertices(); i++ )
					{
						points.push_back(pVerts[i].pos_);
						tangentspace.push_back(Moo::packNormal(pVerts[i].normal_));
						tangentspace.push_back(Moo::packNormal(pVerts[i].tangent_));
						tangentspace.push_back(Moo::packNormal(pVerts[i].binormal_));
					}
					transPos = Vector3::zero();
				}
				else
				{
					transPos = transform( groundModel_, centreModel_ )[3];
					points = geometry.vertices_->vertexPositions();
					tangentspace = geometry.vertices_->vertexNormals();
					normals = geometry.vertices_->vertexNormals2();					
					packedNormals = geometry.vertices_->vertexNormals3();
				}

				Moo::Vertices::VertexNormals::const_iterator n_it	 = tangentspace.begin();
				Moo::Vertices::VertexPositions::const_iterator n2_it = normals.begin();
				Moo::Vertices::VertexNormals::const_iterator n3_it	 = packedNormals.begin();

				bool drawTS  = tangentspace.size() > 0;
				bool packed  = packedNormals.size() > 0;
				bool regular = normals.size() > 0;
				
				Moo::Vertices::VertexPositions::const_iterator p_it = points.begin();
				
				for (;p_it != points.end(); p_it++)
				{						
					Moo::Colour ncol(1.f,1.f,0.f,1.f);
					Moo::Colour tcol(1.f,0.f,0.f,1.f);
					Moo::Colour bcol(0.f,0.f,1.f,1.f);

					const Vector3& point = (*p_it);

					Vector3 start = point + transPos;
					Vector3 end;
					Vector3 normal;

					if (drawTS)
					{
						uint32 intNormal = (*n_it);
						normal = Moo::unpackNormal(intNormal);
						n_it++;
					}
					else if (packed)
					{
						uint32 intNormal = (*n3_it);
						normal = Moo::unpackNormal(intNormal);
						n3_it++;
					}
					else if (regular)
					{
						normal = (*n2_it);
						n2_it++;
					}

					if ( showNormals )
					{
						end = point + normal*scale + transPos;
						vertex.colour_ = ncol;
						vertex.pos_ = start;
						mesh.push_back( vertex );
						vertex.pos_ = end;
						mesh.push_back( vertex );
					}

					if (drawTS && showBinormals)
					{
						uint32 intTangent = (*n_it);
						Vector3 tangent = Moo::unpackNormal(intTangent);
						n_it++;

						uint32 intBinormal = (*n_it);
						Vector3 binormal = Moo::unpackNormal(intBinormal);
						n_it++;
					
						end = point + tangent*scale + transPos;
						vertex.colour_ = tcol;
						vertex.pos_ = start;
						mesh.push_back( vertex );
						vertex.pos_ = end;
						mesh.push_back( vertex );

						end = point + binormal*scale + transPos;
						vertex.colour_ = bcol;
						vertex.pos_ = start;
						mesh.push_back( vertex );
						vertex.pos_ = end;
						mesh.push_back( vertex );
					}

					if ( !showBinormals )
					{ 
						// We need to do this to so that on the next pass we get the correct normal
						n_it++;
						n_it++;
					}
				}
			}
		}
	}
	mesh.draw();
	Moo::rc().pop();
}

/**
 *	draws the indicators on the hardpoints
 */
void Mutant::drawHardPoints()
{
	if (!superModel_) return;

	//Determine a scale to use for the axes based on the total size of the model
	float scale = (modelBB_.maxBounds() - modelBB_.minBounds()).length() / 30.f;
	
	Moo::rc().push();
	Moo::rc().world( Matrix::identity );

	//set the transform
	Vector3 transPos = transform( groundModel_, centreModel_ )[3];

	Labels * nodesIDs = new Labels;

	// draw hardpoints for each model
	for (int i=0; i<superModel_->nModels(); ++i)
	{
		Model * model = superModel_->curModel( i );
		if (model != NULL)
		{
			NodeTreeIterator it  = model->nodeTreeBegin();
            NodeTreeIterator end = model->nodeTreeEnd();

            //Advance past the base node
            if (it != end) it++;

			while (it != end)
			{
				std::string nodeName = it->pData->pNode->identifier();

				// see if this is a Hard Point
				if ( nodeName.substr(0, 3) == std::string("HP_") )
				{
					//If so draw the gizmo
					const Matrix & worldTrans = it->pData->pNode->worldTransform();

					//Calculate the position and vectors
					Vector3 worldPos = worldTrans[3];
					Vector3 x = worldPos + scale*worldTrans[0];
					Vector3 y = worldPos + scale*worldTrans[1];
					Vector3 z = worldPos + scale*worldTrans[2];

					// draw arrow body
					Geometrics::drawLine( worldPos, x, 0x0000ff00 );
					Geometrics::drawLine( worldPos, y, 0x000000ff );
					Geometrics::drawLine( worldPos, z, 0x00ff0000 );

					//draw the hard point's name
					nodesIDs->add( nodeName, worldPos );
				}
				it++;
			}
		}
	}
	Moo::SortedChannel::addDrawItem(nodesIDs);
	Moo::rc().pop();
}


/** This function caches all the points that are part of the custom hull.
  * Doing this operation each draw call on a custom hull is much to0 expensive.
  * Hence we need to do this on load. Of course models without a custom hull 
  * will not store any points.
  * This needs a great deal of optimising!!!! ONLY DO ON LOAD!
  */
void Mutant::calcCustomHull() 
{
	// just in case we load another model without a custom hull.
	customHullPoints_.clear();

	bool customHull = currVisual_->readBool( "customHull", false );

	if ( !customHull ) 
		return;

	std::vector<DataSectionPtr>	spBoundarySets;
    currVisual_->openSections( "boundary", spBoundarySets );
    std::vector<DataSectionPtr>::iterator bit = spBoundarySets.begin();
	std::vector<DataSectionPtr>::iterator bend = spBoundarySets.end();

	std::vector<PlaneEq> planes;

	// STEP 1:
	// Iterate through our boundaries and compile list of planes. O(P)
	while (bit != bend)
	{
		DataSectionPtr boundarySet = *bit++;

        //Get the normal and distance
        Vector3 norm = boundarySet->readVector3( "normal", Vector3(0,0,0) );
        float d = boundarySet->readFloat( "d", 0.0 );
		planes.push_back( PlaneEq( norm, d ) );
	}

	if ( planes.size() == 0 ) return;
	
	// STEP 2:
	// for each plane we need to store all of its intersections with other planes O(P^2)
	std::vector<std::vector<LineEq> > planeLines( planes.size() );

	for ( size_t i = 0; i < planes.size(); i++ )
	{
		for ( size_t j = 0; j < planes.size(); j++ )
		{
			if ( i == j )
				continue;

			LineEq line = planes.at( i ).intersect( planes.at( j ) );
			if ( ( line.normal() == Vector2( 0.f, 0.f ) ) && ( line.d() == 0.f ) )
				continue;

			planeLines.at( i ).push_back( line );
		}
	}

	// STEP 3:
	// now we need to compile a list of intersect points for each plane O(P*L^2)
	std::vector<std::vector<Vector3> > planePoints( planes.size() );

	for ( size_t i = 0; i < planeLines.size(); i++ )
	{
		std::vector<LineEq> lines = planeLines.at( i );
		for ( size_t j = 0; j < lines.size(); j++ )
		{
			for ( size_t k = j + 1; k < lines.size(); k++ )
			{
				if ( !lines.at( j ).isParallel( lines.at( k ) ) )
				{
					// find the point of intersect
					float intersectPoint = lines.at( j ).intersect( lines.at( k ) );
					Vector2 planePoint = lines.at( j ).param( intersectPoint );
					planePoints.at( i ).push_back( planes.at( i ).param( planePoint ) );
				}
			}
		}

		// STEP 4:
		// since the lines on the planes have infinite length there will be alot of 
		// bogus points calculated. What we need to do is clean up so that we only store
		// the points that we are interested in. O(L^2*P^2)
		std::vector<Vector3> keeperPoints;
		for ( size_t j = 0; j < planePoints.at( i ).size(); j++ )
		{			
			bool keep = true;
			for ( size_t k = 0; k < planes.size(); k++ )
			{
				// we dont compare the points with their own plane
				if ( i == k )
					continue;
				if ( almostEqual(planes.at( k ).distanceTo( planePoints.at( i ).at( j ) ), 0.f) )
					continue;
				if ( !planes.at( k ).isInFrontOf( planePoints.at( i ).at( j ) ) )
				{
					keep = false;
					break;
				}					
			}
			if ( keep )
			{
				// Add the point only if it has not been added already
				bool alreadyAdded = false;
				for ( size_t k = 0; k < keeperPoints.size(); k++ )
				{
					if ( almostEqual(keeperPoints[k], planePoints.at( i ).at( j )) )
					{
						alreadyAdded = true;
						break;
					}
				}
				if ( !alreadyAdded )
					keeperPoints.push_back( planePoints.at( i ).at( j ) );
			}
		}
		planePoints.at( i ) = keeperPoints;
	}

	// STEP 5:
	// Reorder the points so we can easily connect the dots without any criss crossing
	// uses same algorithm as in drawPortals.
	for ( size_t i = 0; i < planes.size(); i++ )
	{
		ConvexHull::GrahamScan gs( planePoints.at( i ) );
	}

	// STEP 6:
	// Store the custom hull points
	customHullPoints_ = planePoints;
}

/** Since the points are stored on a per plane basis
  * Joining the dots won't get the desired effect. Thus need to join them
  * using some convex hull method such as "rubberband".
  */
void Mutant::drawCustomHull() 
{
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	// for each plane
	for ( size_t i = 0; i < customHullPoints_.size(); i++ )
	{
		if ( !customHullPoints_.at( i ).size() )
			return;
		// join the dots!
		size_t j = 0; 
		for ( ; j < customHullPoints_.at( i ).size() - 1; j++ )
			Geometrics::drawLine( customHullPoints_.at( i ).at( j ), customHullPoints_.at( i ).at( j + 1 ), 0x00ffff00 );
		Geometrics::drawLine( customHullPoints_.at( i ).at( j ), customHullPoints_.at( i ).at( 0 ), 0x00ffff00 );
	}
}

float Mutant::render( float dTime, int renderStates )
{
	if (!superModel_) return -1.f;

	float atDist = -1.f;

	if (renderStates & SHOW_BSP)
	{
		drawBsp();
	}
	else if (renderStates & SHOW_MODEL)
	{
		atDist = drawModel();
		
		if ((renderStates & SHOW_EDITOR_PROXY) && (editorProxySuperModel_))
		{
			editorProxySuperModel_->draw( 0, 0, atDist, true );
		}
	}

	if (renderStates & SHOW_BB)
	{
		drawBoundingBoxes();
	}

	if (renderStates & SHOW_SKELETON)
	{
		drawSkeleton();
	}

	if (renderStates & SHOW_PORTALS)
	{
		drawPortals();
	}

	bool showNormals = ( renderStates & SHOW_NORMALS ) != 0;
	bool showBinormals = ( renderStates & SHOW_BINORMALS ) != 0;
	if (showNormals || showBinormals)
	{
		drawNormals(showNormals, showBinormals);
	}

	if (renderStates & SHOW_HARD_POINTS)
	{
		drawHardPoints();
	}

	if (renderStates & SHOW_CUSTOM_HULL)
	{
		drawCustomHull();
	}

	actionQueue_.debugTick( dTime );
	if (renderStates & SHOW_ACTIONS)
	{
		actionQueue_.debugDraw();
	}

	return atDist;
}
