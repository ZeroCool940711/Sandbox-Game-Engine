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
#include "rotation_gizmo.hpp"
#include "chunk/chunk_item.hpp"
#include "chunk/chunk.hpp"
#include "tool.hpp"
#include "tool_manager.hpp"
#include "general_properties.hpp"
#include "current_general_properties.hpp"
#include "item_functor.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/effect_visual_context.hpp"
#include "moo/visual_channels.hpp"
#include "resmgr/auto_config.hpp"
#include "resmgr/resource_cache.hpp"
#include "moo/visual_manager.hpp"

static AutoConfigString s_gizmoVisual( "editor/rotationGizmo" );

extern Moo::Colour g_unlit;
extern bool g_showHitRegion;

class RotationShapePart : public ShapePart
{
public:
	RotationShapePart( const Moo::Colour& col, int axis ) 
	:	colour_( col )
	{
		Vector3 normal( 0, 0, 0 );
		normal[axis] = 1.f;
		planeEq_ = PlaneEq( normal, 0 );
	}

	const Moo::Colour& colour() const { return colour_; }
	const PlaneEq & plane() const{ return planeEq_; }

private:
	Moo::Colour	colour_;
	PlaneEq		planeEq_;
};

Matrix normaliseMatrix( const Matrix& m )
{
	Matrix m2 = m;
	m2[0].normalise();
	m2[1].normalise();
	m2[2].normalise();
	return m2;
}


// -----------------------------------------------------------------------------
// Section: RotationGizmo
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
RotationGizmo::RotationGizmo( MatrixProxyPtr pMatrix, int enablerModifier, int disablerModifier ) :
	active_( false ),
	currentPart_( NULL ),
	pMatrix_( pMatrix ),
	drawMesh_( NULL ),
	lightColour_( 0, 0, 0, 0 ),
	enablerModifier_( enablerModifier ),
	disablerModifier_( disablerModifier )
{
	if ( !s_gizmoVisual.value().empty() )
	{
		drawMesh_ = Moo::VisualManager::instance()->get( s_gizmoVisual );
		ResourceCache::instance().addResource( drawMesh_ );
	}	

	float inner = 6.f;
	float outer = 6.5f;
	Matrix m;
	m.setIdentity();
	selectionMesh_.transform( m );
	selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0xff0000, new RotationShapePart( Moo::Colour( 1, 0, 0, 0 ), 2 ) );

	m.setRotateY( DEG_TO_RAD( 90 ) );
	selectionMesh_.transform( m );
	selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0x00ff00, new RotationShapePart( Moo::Colour( 0, 1, 0, 0 ), 0 ) );

	m.setRotateX( DEG_TO_RAD( 90 ) );
	selectionMesh_.transform( m );
	selectionMesh_.addDisc( Vector3( 0, 0, 0 ), inner, outer, 0x0000ff, new RotationShapePart( Moo::Colour( 0, 0, 1, 0 ), 1 ) );

	if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_OBJECT )
		invObjectTransform_.setIdentity();
	else
		invObjectTransform_.invert( normaliseMatrix( objectTransform() ) );
}


/**
 *	Destructor.
 */
RotationGizmo::~RotationGizmo()
{

}

bool RotationGizmo::draw( bool force )
{
	active_ = false;
	if 
	( 
		!force 
		&& 
		enablerModifier_ != ALWAYS_ENABLED
		&&
		( 
			(InputDevices::modifiers() & enablerModifier_) == 0 
			||
			(InputDevices::modifiers() & disablerModifier_) != 0 
		) 
	)
	{
		return false;
	}
	active_ = true;

	Moo::RenderContext& rc = Moo::rc();
	DX::Device* pDev = rc.device();

	rc.setPixelShader(NULL);

	if (drawMesh_)
	{
		rc.fogEnabled( false );
		Moo::LightContainerPtr pOldLC = rc.lightContainer();
		Moo::LightContainerPtr pLC = new Moo::LightContainer;
		pLC->ambientColour( lightColour_ );
		rc.lightContainer( pLC );
		rc.setPixelShader( NULL );		

		rc.push();
		rc.world( gizmoTransform() );
		drawMesh_->draw();
		rc.pop();

		rc.lightContainer( pOldLC );
		Moo::SortedChannel::draw();
	}
	
	if (!drawMesh_ || g_showHitRegion)
	{
		rc.setRenderState( D3DRS_NORMALIZENORMALS, TRUE );
		Moo::Material::setVertexColour();
		rc.fogEnabled( false );
		rc.setRenderState( D3DRS_LIGHTING, FALSE );
		rc.setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		rc.setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_DIFFUSE );
		rc.setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_TFACTOR );
		rc.setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		rc.setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		uint32 tfactor = lightColour_;
		rc.setRenderState( D3DRS_TEXTUREFACTOR, tfactor );

		pDev->SetTransform( D3DTS_WORLD, &gizmoTransform() );
		pDev->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		pDev->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );		
		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( Moo::VertexXYZND::fvf() );

		Moo::DynamicIndexBufferBase& dib = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
		Moo::IndicesReference ind = dib.lock( selectionMesh_.indices().size() );
		if (ind.size())
		{
			ind.fill( &selectionMesh_.indices().front(), selectionMesh_.indices().size() );
			dib.unlock();

			Moo::DynamicVertexBuffer< Moo::VertexXYZND >& dvb = Moo::DynamicVertexBuffer< Moo::VertexXYZND >::instance();
			Moo::VertexXYZND* verts = dvb.lock( selectionMesh_.verts().size() );	
			if (verts)
			{
				memcpy( verts, &selectionMesh_.verts().front(), sizeof( Moo::VertexXYZND ) * selectionMesh_.verts().size() );
				dvb.unlock();

				dvb.set();
				dib.indexBuffer().set();

				rc.drawIndexedPrimitive( D3DPT_TRIANGLELIST, dvb.lockIndex(), 0, selectionMesh_.verts().size(),
					dib.lockIndex(), selectionMesh_.indices().size() / 3 );
			}
		}
	}

	return true;
}

bool RotationGizmo::intersects( const Vector3& origin, const Vector3& direction, float& t )
{
	if ( !active_ )
	{
		currentPart_ = NULL;
		return false;
	}

	lightColour_ = g_unlit;
	if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_OBJECT )
		invObjectTransform_.setIdentity();
	else
		invObjectTransform_.invert( normaliseMatrix( objectTransform() ) );

	Matrix m = gizmoTransform();
	m.invert();

	Vector3 lo = m.applyPoint( origin );
	Vector3 ld = m.applyVector( direction );
	float l = ld.length();
	t *= l;
	ld /= l;;

	currentPart_ = (RotationShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );

	t /= l;

/*	currentPart_ = (RotationShapePart*)selectionMesh_.intersects(
		m.applyPoint( origin ),
		m.applyVector( direction ),
		&t );*/

	return currentPart_ != NULL;
}

void RotationGizmo::click( const Vector3& origin, const Vector3& direction )
{
	// handle the click


	if (currentPart_ != NULL) 
	{
		/*ToolViewPtr pView = NULL;
		if ( WorldEditor::instance().tool() )
		{
			pView = WorldEditor::instance().tool()->view( "chunkViz" );
		}*/

		PlaneEq	peq( Vector3( 0.f, 0.f, 0.f ), 0.f );
		peq = currentPart_->plane();
		peq = PlaneEq( this->objectTransform().applyToOrigin(), getCoordModifier().applyVector( peq.normal() ) );
		ToolPtr moveTool( new Tool(
			ToolLocatorPtr( new PlaneToolLocator( &peq ), true ),
			/*pView*/NULL,
			ToolFunctorPtr( new MatrixRotator( pMatrix_ ), true )
			), true );
		ToolManager::instance().pushTool( moveTool );
	}
}

void RotationGizmo::rollOver( const Vector3& origin, const Vector3& direction )
{
	// roll it over.
	if (currentPart_ != NULL)
	{
		lightColour_ = currentPart_->colour();
	}
	else
	{
		lightColour_ = g_unlit;
	}
}

Matrix RotationGizmo::objectTransform() const
{
	Matrix m;
	pMatrix_->getMatrix( m );
	m.translation(CurrentRotationProperties::averageOrigin());
	return m;
}

Matrix RotationGizmo::objectCoord() const
{
	Matrix coord;
	if (CurrentRotationProperties::properties().size() > 0)
		CurrentRotationProperties::properties()[0]->pMatrix()->getMatrix(coord);
	else
		coord = Matrix::identity;
	return coord;
}

Matrix RotationGizmo::getCoordModifier() const
{
	Matrix coord;
	if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_OBJECT )
	{
		return objectCoord();
	}
	else if( CoordModeProvider::ins()->getCoordMode() == CoordModeProvider::COORDMODE_VIEW )
	{
		coord = Moo::rc().invView();
	}
	else
	{
		coord.setIdentity();
	}
	return coord;
}


// rotation_gizmo.cpp
