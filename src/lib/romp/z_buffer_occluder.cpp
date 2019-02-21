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
#include "cstdmf/debug.hpp"
#include "moo/effect_material.hpp"
#include "moo/texturestage.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/moo_dx.hpp"
#include "resmgr/auto_config.hpp"

#include "lens_effect.hpp"
#include "z_buffer_occluder.hpp"
#include "custom_mesh.hpp"

static AutoConfigString s_effect( "environment/ZBufferOccluderFX" );
static Moo::ManagedEffectPtr s_managedEffect;

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

#ifndef CODE_INLINE
#include "z_buffer_occluder.ipp"
#endif

#define MAX_LENS_FLARES				256

bool ZBufferOccluder::isAvailable()
{
	HRESULT hr = Moo::rc().device()->CreateQuery( D3DQUERYTYPE_OCCLUSION, NULL );
	return (hr==D3D_OK);
}


/*static*/ void ZBufferOccluder::init()
{
	s_managedEffect = Moo::EffectManager::instance().get( s_effect );
}

/*static*/ void ZBufferOccluder::fini()
{
	s_managedEffect = NULL;
}


ZBufferOccluder::ZBufferOccluder():
	helper_( MAX_LENS_FLARES ),
	helperZBuffer_( MAX_LENS_FLARES )
{
	mat_ = new Moo::EffectMaterial;
	mat_->initFromEffect( s_effect.value() );
}


ZBufferOccluder::~ZBufferOccluder()
{
}


void ZBufferOccluder::beginOcclusionTests()
{
	//reset used parameters.
	helper_.begin();	
	helperZBuffer_.begin();
}


/**
 *	This method sets the material state on the device.
 *	Note that since photon occluders are interleaved, we'
 *	must ensure we set our own device state for each draw.
 */
void ZBufferOccluder::setDeviceState()
{
	DX::Device* device = Moo::rc().device();
	static bool s_watchAdded = false;
	static bool s_viewFlareSources = false;
	static float s_pointSize = 3.f;
	if ( !s_watchAdded )
	{
		MF_WATCH ( "Render/View Flare Sources",
			s_viewFlareSources,
			Watcher::WT_READ_WRITE,
			"Visualise lens flare source geometry." );

		MF_WATCH ( "Render/Flare Source Size",
			s_pointSize,
			Watcher::WT_READ_WRITE,
			"Point size for lens flare source geometry." );

		s_watchAdded = true;
	}

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, s_viewFlareSources ? D3DCOLORWRITEENABLE_RED: 0 );
	
	device->SetTransform( D3DTS_WORLD, &Matrix::identity );
	device->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	device->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );	
	Moo::rc().setRenderState( D3DRS_POINTSCALEENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&s_pointSize) );
}


/**
 *	This method checks the ray between the photon source
 *	and the eye.
 *
 *	@return Returns the precentage of the flare that is visible.
 */
float ZBufferOccluder::collides( 
	const Vector3 & photonSourcePosition,
	const Vector3 & cameraPosition,
	const LensEffect& le )
{
	//get result index for this lens flare
	OcclusionQueryHelper::Handle queryHandle;
	OcclusionQueryHelper::Handle queryHandleZBuffer;

	queryHandle = helper_.handleFromId( le.id() );
	queryHandleZBuffer = helperZBuffer_.handleFromId( le.id() );
	
	this->setDeviceState();

	if (!mat_->begin())
	{
		ERROR_MSG( "Could not begin z buffer occluder material\n" );
		return 0.f;
	}

	//TODO : set the far plane flag on the material.
	D3DXHANDLE param = mat_->pEffect()->pEffect()->GetParameterByName(NULL, "clampToFarPlane");
	if (param)
		mat_->pEffect()->pEffect()->SetBool( param, le.clampToFarPlane() );	

	// Draw the OCCLUDED flare
	float occluded = 0.f;
	if (helperZBuffer_.beginQuery(queryHandleZBuffer))
	{		
		mat_->beginPass(0);			
		if (le.area() > 1.f)
			this->writeArea( photonSourcePosition, le.area() );
		else
			this->writePixel( photonSourcePosition );
		mat_->endPass();	

		//end the test
		helperZBuffer_.endQuery(queryHandleZBuffer);

		occluded = (float)helperZBuffer_.avgResult(queryHandleZBuffer);
	}
	//else
	//{
	//	ERROR_MSG( "Could not begin visibility test for OCCLUDED lens flare\n" );
	//}

	float full = 0.f;		
	if ( occluded != 0.f )
	{
		// Draw the FULL flare		
		if (helper_.beginQuery(queryHandle))
		{
			mat_->beginPass(1);
			if (le.area() > 1.f)
				this->writeArea( photonSourcePosition, le.area() );
			else
				this->writePixel( photonSourcePosition );
			mat_->endPass();

			//end the test
			helper_.endQuery(queryHandle);		

			full = (float)helper_.avgResult(queryHandle);
		}
		//else
		//{
		//	ERROR_MSG( "Could not begin visibility test for FULL lens flare\n" );
		//}
	}

	mat_->end();

	if ( full == 0.f )
	{
		//DEBUG_MSG( "Zbuffer says flare be invisible\n ------------\n" );
		return 0.f;
	}

	//DEBUG_MSG( "Zbuffer says flare is visible by %.2f\n ------------\n", OCCLUDED / FULL );
	return occluded / full;
}


void ZBufferOccluder::endOcclusionTests()
{
	helper_.end();
	helperZBuffer_.end();
}


void ZBufferOccluder::writePixel( const Vector3& source )
{
	//write outs 4 pixels into the z-buffer, with no colour at all.
	Moo::VertexXYZ vert;
	vert.pos_ = source;

	Moo::rc().setFVF(Moo::VertexXYZ::fvf());

	Moo::rc().drawPrimitiveUP( D3DPT_POINTLIST, 1, &vert, sizeof( Moo::VertexXYZ ) );
}


void ZBufferOccluder::writeArea( const Vector3& source, float size )
{
	//write out a pixel into the z-buffer, with no colour at all.
	static CustomMesh<Moo::VertexXYZ> mesh( D3DPT_TRIANGLEFAN );
	mesh.clear();
	Moo::VertexXYZ vert;	

	Vector3 src( source );
	src -= Moo::rc().invView().applyToUnitAxisVector(2);

	//this line accounts for the fact that due to the way we draw this mesh,
	//we have to scale the size compared to a farPlane default of 500.
	float fpFactor = Moo::rc().camera().farPlane() / 500.f;
	float h = (size / 2.f) * fpFactor;
	float w = h;
	Vector3 right = Moo::rc().invView().applyToUnitAxisVector(0) * w;
	Vector3 up = Moo::rc().invView().applyToUnitAxisVector(1) * h;
	vert.pos_ = src - right + up;
	mesh.push_back( vert );
	vert.pos_ = src + right + up;
	mesh.push_back( vert );
	vert.pos_ = src + right - up;
	mesh.push_back( vert );
	vert.pos_ = src - right - up;
	mesh.push_back( vert );		

	mesh.drawEffect();
}


std::ostream& operator<<(std::ostream& o, const ZBufferOccluder& t)
{
	o << "ZBufferOccluder\n";
	return o;
}

// z_buffer_occluder.cpp
