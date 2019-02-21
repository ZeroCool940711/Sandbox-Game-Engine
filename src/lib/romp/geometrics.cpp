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

#include "geometrics.hpp"
#include "moo/render_context.hpp"
#include "moo/vertex_formats.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "resmgr/auto_config.hpp"

#ifndef CODE_INLINE
#include "geometrics.ipp"
#endif


//The s_offset constant defines the correction factor for pixel-texel
//alignment when using screen-space quads.
static Vector2 s_offset( -0.5f, -0.5f );

static void projectInPlace( Vector4& v )
{
	v.w = 1.f / v.w;
	v.z *= v.w;
	v.x = (v.x * v.w * Moo::rc().halfScreenWidth()) + Moo::rc().halfScreenWidth();
	v.y = (-v.y * v.w * Moo::rc().halfScreenHeight()) + Moo::rc().halfScreenHeight();
}


/**
 * TODO: to be documented.
 */
class WorldViewProjSetter : public Moo::EffectConstantValue
{
public:
	~WorldViewProjSetter()
	{
		pEffectConstantValue_ = NULL;
	}

	bool operator()(ID3DXEffect* pEffect, D3DXHANDLE constantHandle)
	{
		Matrix wvp( Moo::rc().world() );
		wvp.postMultiply( Moo::rc().view() );
		wvp.postMultiply( Moo::rc().projection() );
		pEffect->SetMatrix( constantHandle, &wvp );
		return true;
	}

	static void set()
	{
		*instance()->pEffectConstantValue_ = &*instance();
	}

	static SmartPointer<WorldViewProjSetter> instance()
	{
		static SmartPointer<WorldViewProjSetter> pInstance = new WorldViewProjSetter;
		return pInstance;
	}

private:
	WorldViewProjSetter()
	{
		pEffectConstantValue_ = Moo::EffectConstantValue::get( "WorldViewProjection" );
	}

	Moo::EffectConstantValuePtr* pEffectConstantValue_;
};



static void setColourConstant( Moo::EffectMaterial& mat,
	const std::string& parmName,
	const Moo::Colour& colour )
{
	MF_ASSERT( mat.pEffect()->pEffect() );
	mat.pEffect()->pEffect()->SetVector( "diffuseColour", (Vector4*)&colour );
}


Geometrics::Geometrics()
{
}

Geometrics::~Geometrics()
{
}

Geometrics & Geometrics::instance()
{
	static Geometrics geometrics;

	return geometrics;
}


void Geometrics::drawShadowPoly( const Moo::Colour& colour )
{
	Moo::rc().setRenderState( D3DRS_STENCILENABLE , TRUE );
	Moo::rc().setRenderState( D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL );
	Moo::rc().setRenderState( D3DRS_STENCILREF, 0 );

	if( Moo::rc().device() )
	{
		DX::Device* device_ = Moo::rc().device();

		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		CustomMesh<Moo::VertexTL> mesh( D3DPT_TRIANGLESTRIP );
		Moo::VertexTL tlv;
		tlv.colour_ = colour;
		tlv.pos_.z = 0;
		tlv.pos_.w = 1;

		tlv.pos_.x = 0;
		tlv.pos_.y = 0;
		mesh.push_back( tlv );

		tlv.pos_.x = float( Moo::rc().screenWidth() );
		tlv.pos_.y = 0;
		mesh.push_back( tlv );

		tlv.pos_.x = 0;
		tlv.pos_.y = float( Moo::rc().screenHeight() );
		mesh.push_back( tlv );

		tlv.pos_.x = float( Moo::rc().screenWidth() );
		tlv.pos_.y = float( Moo::rc().screenHeight() );
		mesh.push_back( tlv );

		mesh.draw();
	}

	Moo::rc().setRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
	Moo::rc().setRenderState( D3DRS_STENCILENABLE , FALSE );
}


/**
 *	This method clears the given viewport.
 */
void Geometrics::clearSwizzledViewport()
{
	if( Moo::rc().device() )
	{
		DX::Viewport vp;
		Moo::rc().getViewport( &vp );

		Moo::rc().setIndices( NULL );
		Moo::rc().setRenderState( D3DRS_STENCILENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_STENCILFUNC, D3DCMP_ALWAYS );
		Moo::rc().setRenderState( D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE );
		Moo::rc().setRenderState( D3DRS_STENCILREF, 0x0  );
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		Moo::VertexTL tlv;
		CustomMesh<Moo::VertexTL> mesh( D3DPT_TRIANGLESTRIP );

		tlv.colour_ = 0xffffffff;
		tlv.pos_.z = 1.f;
		tlv.pos_.w = 1.f;

		tlv.pos_.x = (float)vp.X;
		tlv.pos_.y = (float)vp.Y;
		mesh.push_back( tlv );

		tlv.pos_.x = (float)(vp.X + vp.Width);
		tlv.pos_.y = (float)vp.Y;
		mesh.push_back( tlv );

		tlv.pos_.x = (float)vp.X;
		tlv.pos_.y = (float)(vp.Y + vp.Height);
		mesh.push_back( tlv );

		tlv.pos_.x = (float)(vp.X + vp.Width);
		tlv.pos_.y = (float)(vp.Y + vp.Height);
		mesh.push_back( tlv );

		mesh.draw();
		Moo::rc().setRenderState( D3DRS_STENCILENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	}
}


/**
 *	This method puts a screen-space rectangle into the 
 *	given CustomMesh.  The CustomMesh must be have the
 *	Moo::VertexTL format.
 *
 *	Note the rectangle is adjusted for precise pixel/texl mapping.
 */
void Geometrics::createRectMesh( const Vector2& topLeft,
	const Vector2& bottomRight,
	const Moo::Colour& colour,
	CustomMesh<Moo::VertexTL>& mesh )
{
	Moo::VertexTL tlv;

	tlv.colour_ = colour;
	tlv.pos_.z = 0;
	tlv.pos_.w = 1;

	tlv.pos_.x = topLeft.x + s_offset.x;
	tlv.pos_.y = topLeft.y + s_offset.y;
	mesh.push_back( tlv );

	tlv.pos_.x = bottomRight.x + s_offset.x;
	tlv.pos_.y = topLeft.y + s_offset.y;
	mesh.push_back( tlv );

	tlv.pos_.x = topLeft.x + s_offset.x;
	tlv.pos_.y = bottomRight.y + s_offset.y;
	mesh.push_back( tlv );

	tlv.pos_.x = bottomRight.x + s_offset.x;
	tlv.pos_.y = bottomRight.y + s_offset.y;
	mesh.push_back( tlv );
}


/**
 *	This method puts a screen-space rectangle into the 
 *	given CustomMesh.  The CustomMesh must be have the
 *	Moo::VertexTLUV format.
 *
 *	Note the rectangle is adjusted for precise pixel/texl mapping.
 */
void Geometrics::createRectMesh( const Vector2& topLeft,
	const Vector2& bottomRight,
	const Moo::Colour& colour,
	bool linearUV, 
	CustomMesh<Moo::VertexTLUV>& mesh )
{
	Moo::VertexTLUV tlv;
	tlv.colour_ = colour;
	tlv.pos_[2] = 0;
	tlv.pos_[3] = 1;	

	tlv.pos_[0] = topLeft.x + s_offset.x;
	tlv.pos_[1] = topLeft.y + s_offset.y;
	tlv.uv_[0] = linearUV ? topLeft.x : 0.f;
	tlv.uv_[1] = linearUV ? topLeft.y : 0.f;
	mesh.push_back( tlv );

	tlv.pos_[0] = bottomRight.x + s_offset.x;
	tlv.pos_[1] = topLeft.y + s_offset.y;
	tlv.uv_[0] = linearUV ? bottomRight.x : 1.f;
	tlv.uv_[1] = linearUV ? topLeft.y : 0.f;
	mesh.push_back( tlv );

	tlv.pos_[0] = topLeft.x + s_offset.x;
	tlv.pos_[1] = bottomRight.y + s_offset.y;
	tlv.uv_[0] = linearUV ? topLeft.x : 0.f;
	tlv.uv_[1] = linearUV ? bottomRight.y : 1.f;
	mesh.push_back( tlv );

	tlv.pos_[0] = bottomRight.x + s_offset.x;
	tlv.pos_[1] = bottomRight.y + s_offset.y;
	tlv.uv_[0] = linearUV ? bottomRight.x : 1.f;
	tlv.uv_[1] = linearUV ? bottomRight.y : 1.f;
	mesh.push_back( tlv );
}


/**
 *	This method puts a rectangle into the 
 *	given CustomMesh.  The CustomMesh must be have the
 *	Moo::VertexXYZUV format.
 */
void Geometrics::createRectMesh( const Vector2& topLeft,
	const Vector2& bottomRight,
	const Moo::Colour& colour,	
	CustomMesh<Moo::VertexXYZUV>& mesh )
{
	Moo::VertexXYZUV xyzuv;
	xyzuv.pos_[2] = 0;	

	xyzuv.pos_[0] = topLeft.x;
	xyzuv.pos_[1] = topLeft.y;
	xyzuv.uv_[0] = 0.f;
	xyzuv.uv_[1] = 0.f;
	mesh.push_back( xyzuv );

	xyzuv.pos_[0] = bottomRight.x;
	xyzuv.pos_[1] = topLeft.y;
	xyzuv.uv_[0] = 1.f;
	xyzuv.uv_[1] = 0.f;
	mesh.push_back( xyzuv );

	xyzuv.pos_[0] = topLeft.x;
	xyzuv.pos_[1] = bottomRight.y;
	xyzuv.uv_[0] = 0.f;
	xyzuv.uv_[1] = 1.f;
	mesh.push_back( xyzuv );

	xyzuv.pos_[0] = bottomRight.x;
	xyzuv.pos_[1] = bottomRight.y;
	xyzuv.uv_[0] = 1.f;
	xyzuv.uv_[1] = 1.f;
	mesh.push_back( xyzuv );
}


/**
 *	This methods sets up a simple vertex colouring scheme using
 *	the fixed function pipeline.
 */
void Geometrics::setVertexColour( const Moo::Colour& colour )
{
	if( Moo::rc().device() )
	{
		DX::Device* device_ = Moo::rc().device();

		bool hasAlpha = (colour.a != 0.f && colour.a != 1.f);

		Moo::rc().setPixelShader( NULL );

		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, hasAlpha ? TRUE : FALSE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, hasAlpha ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, hasAlpha ? D3DBLEND_SRCALPHA : D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, hasAlpha ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	}
}


/**
 *	This methods sets up a simple vertex colouring scheme using
 *	the fixed function pipeline, and applies a MODx2 blend mode.
 */
void Geometrics::setVertexColourMod2( const Moo::Colour& colour )
{
	if( Moo::rc().device() )
	{
		DX::Device* device_ = Moo::rc().device();

		bool hasAlpha = (colour.a != 0.f && colour.a != 1.f);

		Moo::rc().setPixelShader( NULL );

		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_DESTCOLOR );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, hasAlpha ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );
	}
}


/**
 *	This method draws a single coloured rectangle on the screen.
 *	@param topLeft the top left screen space position of the rect.
 *	@param bottomRight the bottom right screen space position of the rect.
 *	@param colour the colour to draw the rect in.
 */
void Geometrics::drawRect( const Vector2& topLeft,
		const Vector2& bottomRight, 
		const Moo::Colour& colour )
{
	CustomMesh<Moo::VertexTL> mesh( D3DPT_TRIANGLESTRIP );
	setVertexColour( colour );
	createRectMesh( topLeft, bottomRight, colour, mesh );
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
	mesh.draw();
	Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, TRUE );
}


/**
 *	This method draws a single rectangle on the screen with the given material.
 *	@param topLeft the top left screen space position of the rect.
 *	@param bottomRight the bottom right screen space position of the rect.
 *	@param material the material to draw the rect with.
 *	@param linearUV if the uv coords should follow the position rather go from 0..1
 */
void Geometrics::drawRect( const Vector2& topLeft,
		const Vector2& bottomRight,
		Moo::EffectMaterial& material,
		bool linearUV )
{
	DX::Device* device_ = Moo::rc().device();

	if( device_ )
	{
		Moo::VertexTUV tuv;
		tuv.pos_[2] = 0;
		tuv.pos_[3] = 1;
		CustomMesh< Moo::VertexTUV > mesh( D3DPT_TRIANGLESTRIP );

		tuv.pos_[0] = topLeft.x + s_offset.x;
		tuv.pos_[1] = topLeft.y + s_offset.y;
		tuv.uv_[0] = linearUV ? topLeft.x : 0.f;
		tuv.uv_[1] = linearUV ? topLeft.y : 0.f;
		mesh.push_back( tuv );

		tuv.pos_[0] = bottomRight.x + s_offset.x;
		tuv.pos_[1] = topLeft.y + s_offset.y;
		tuv.uv_[0] = linearUV ? bottomRight.x : 1.f;
		tuv.uv_[1] = linearUV ? topLeft.y : 0.f;
		mesh.push_back( tuv );

		tuv.pos_[0] = topLeft.x + s_offset.x;
		tuv.pos_[1] = bottomRight.y + s_offset.y;
		tuv.uv_[0] = linearUV ? topLeft.x : 0.f;
		tuv.uv_[1] = linearUV ? bottomRight.y : 1.f;
		mesh.push_back( tuv );

		tuv.pos_[0] = bottomRight.x + s_offset.x;
		tuv.pos_[1] = bottomRight.y + s_offset.y;
		tuv.uv_[0] = linearUV ? bottomRight.x : 1.f;
		tuv.uv_[1] = linearUV ? bottomRight.y : 1.f;
		mesh.push_back( tuv );

		if (material.begin())
		{
			for ( uint i=0; i<material.nPasses(); i++ )
			{
				material.beginPass(i);
				Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
				Moo::rc().fogEnabled( false );
				Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
				Moo::rc().setRenderState( D3DRS_ZENABLE, D3DZB_FALSE );
				Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
				mesh.draw();
				material.endPass();
			}
			material.end();
		}
	}
}


void Geometrics::drawUnitSquareOnXZPlane( const Moo::Colour& colour )
{
	if( Moo::rc().device() )
	{
		DX::Device* device_ = Moo::rc().device();

		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		Moo::rc().fogEnabled( true );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

		CustomMesh<Moo::VertexXYZL> mesh( D3DPT_TRIANGLESTRIP );
		Moo::VertexXYZL xyzl;
		xyzl.pos_.y = 0.f;
		xyzl.colour_ = colour;

		std::vector< Moo::VertexXYZL > vertices;

		xyzl.pos_.x = -0.5;
		xyzl.pos_.z = 0.5f;
		mesh.push_back( xyzl );

		xyzl.pos_.x = 0.5f;
		xyzl.pos_.z = 0.5f;
		mesh.push_back( xyzl );

		xyzl.pos_.x = -0.5f;
		xyzl.pos_.z = -0.5f;
		mesh.push_back( xyzl );

		xyzl.pos_.x = 0.5f;
		xyzl.pos_.z = -0.5f;
		mesh.push_back( xyzl );

		device_->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
		device_->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		device_->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

		mesh.draw();
	}
}


void Geometrics::texturedRect( const Vector2& topLeft,
		const Vector2& bottomRight,
		const Moo::Colour& colour,
		bool linearUV )
{
	DX::Device* device_ = Moo::rc().device();

	bool hasAlpha = (colour.a != 0.f && colour.a != 1.f);

	if( device_ )
	{
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, hasAlpha ? TRUE : FALSE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, hasAlpha ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, hasAlpha ? D3DBLEND_SRCALPHA : D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, hasAlpha ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		if ( linearUV )
		{
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
/*			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP );*/
		}

		CustomMesh< Moo::VertexTLUV > mesh( D3DPT_TRIANGLESTRIP );
		Geometrics::createRectMesh(topLeft,bottomRight,colour,linearUV,mesh);
		mesh.draw();
	}
}

void Geometrics::texturedRect( const Vector2& topLeft, 
			const Vector2& bottomRight,
			const Vector2& topLeftUV, 
			const Vector2& bottomRightUV,
			const Moo::Colour& colour,
			bool singleMip )
{
	DX::Device* device_ = Moo::rc().device();

	bool hasAlpha = (colour.a != 0.f && colour.a != 1.f);

	if( device_ )
	{
		Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, hasAlpha ? TRUE : FALSE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, hasAlpha ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, hasAlpha ? D3DBLEND_SRCALPHA : D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
		Moo::rc().fogEnabled( false );
		Moo::rc().setRenderState( D3DRS_ZENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );

		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, hasAlpha ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
		Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

		if ( singleMip )
		{
			Moo::rc().setSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
			Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
			Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP );
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP );
			Moo::rc().setSamplerState( 0, D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP );
/*			Moo::rc().setTextureStageState( 0, D3DTSS_MIPFILTER, D3DTEXF_NONE );
			Moo::rc().setTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
			Moo::rc().setTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
			Moo::rc().setTextureStageState( 0, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP );*/
		}

		CustomMesh< Moo::VertexTLUV > mesh( D3DPT_TRIANGLESTRIP );
		Geometrics::createRectMesh(topLeft,bottomRight,colour,false,mesh);		
		mesh.draw();
	}
}

void Geometrics::texturedUnitWorldRectOnXZPlane( const Moo::Colour& colour )
{
	if( Moo::rc().device() )
	{
		DX::Device* device_ = Moo::rc().device();

		Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

		CustomMesh<Moo::VertexXYZNDUV> mesh( D3DPT_TRIANGLESTRIP );
		Moo::VertexXYZNDUV xyzluv;
		xyzluv.pos_.y = 0.f;
		xyzluv.normal_ = Vector3( 0.f, 1.f, 0.f );
		xyzluv.colour_ = colour;

		xyzluv.pos_.x = -0.5;
		xyzluv.pos_.z = 0.5f;
		xyzluv.uv_[0] = 0.f;
		xyzluv.uv_[1] = 1.f;
		mesh.push_back( xyzluv );

		xyzluv.pos_.x = 0.5f;
		xyzluv.pos_.z = 0.5f;
		xyzluv.uv_[0] = 1.f;
		xyzluv.uv_[1] = 1.f;
		mesh.push_back( xyzluv );

		xyzluv.pos_.x = -0.5f;
		xyzluv.pos_.z = -0.5f;
		xyzluv.uv_[0] = 0.f;
		xyzluv.uv_[1] = 0.f;
		mesh.push_back( xyzluv );

		xyzluv.pos_.x = 0.5f;
		xyzluv.pos_.z = -0.5f;
		xyzluv.uv_[0] = 1.f;
		xyzluv.uv_[1] = 0.f;
		mesh.push_back( xyzluv );

		device_->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
		device_->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
		device_->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

		mesh.draw();
	}
}


/**
 *	This static method draws a line.
 *
 *	@param points		Vector3 array of points
 *	@param count		Number of points in the vector
 *	@param pointSize	Size of the points, in pixels.
 *	@param colour		The colour of the points.
 */
void Geometrics::drawPoints( const Vector3* points, uint32 count, 
		float pointSize, const Moo::Colour& colour )
{
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setTexture( 0, NULL );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&pointSize) );
	std::vector<Moo::VertexXYZL> v( count );
	for ( uint32 i = 0; i < count; ++i )
	{
		v[i].pos_ = points[i];
		v[i].colour_ = colour;
	}
	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	Moo::rc().drawPrimitiveUP( D3DPT_POINTLIST, count, &v.front(), sizeof( Moo::VertexXYZL ) );
}

/**
 *	This static method draws a line.
 *
 *	@param start		The start of the line in world coordinates.
 *	@param end			The end of the line in world coordinates.
 *	@param colour		The colour of the line.
 *	@param alwaysDraw	True if always drawn otherwise uses Z-buffer.
 */
void Geometrics::drawLine( const Vector3 & start,
		const Vector3 & end,
		const Moo::Colour & colour,
		bool alwaysDraw )
{
	DX::Device *device = Moo::rc().device();

	if (device == NULL) 
        return;

	device->SetTransform(D3DTS_WORLD     , &Matrix::identity      );
	device->SetTransform(D3DTS_VIEW      , &Moo::rc().view()      );
	device->SetTransform(D3DTS_PROJECTION, &Moo::rc().projection());

    CustomMesh<Moo::VertexXYZL> mesh(D3DPT_LINELIST);
    Moo::VertexXYZL vertex;
    vertex.colour_ = colour;
    vertex.pos_ = start;
    mesh.push_back(vertex);
    vertex.pos_ = end;
    mesh.push_back(vertex);

	Moo::Material::setVertexColour();
	Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
	if (alwaysDraw)
	{
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
	}
	else
	{
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	}

	mesh.draw();
}


/**
 *	This static method draws a textured line.
 *
 *	@param start			The start of the line in world coordinates.
 *	@param end				The end of the line in world coordinates.
 *	@param thickness		The thickness of the line in clip coordinates.
 *	@param colour			The colour of the line (vertex colour).
 *	@param textureRepeat	The repeats of the texture along the line.
 *	@param useEffect		Use an effect file to draw, rather than the fixed-function pipeline.
 */
void Geometrics::texturedLine( const Vector3 & start,
		const Vector3 & end,
		float thickness,
		const Moo::PackedColour & colour,
		float textureRepeat,
		bool useEffect )
{
	DX::Device* device_ = Moo::rc().device();

	if( !device_ ) return;

	Matrix	objectToClip;
	objectToClip.multiply( Moo::rc().world(), Moo::rc().viewProjection() );

	Vector4	clipStart,	clipEnd;
	objectToClip.applyPoint( clipStart, Vector4(start.x,start.y,start.z,1) );
	objectToClip.applyPoint( clipEnd,	Vector4(end.x,end.y,end.z,1) );

	Moo::VertexTLUV v;
	v.colour_ = colour;
	static CustomMesh< Moo::VertexTLUV > mesh( D3DPT_TRIANGLELIST );
	mesh.clear();

	float endX = 1.f;
	if ( textureRepeat > 0.f )
		endX = (end-start).length() / textureRepeat;

	//triangle 1
	v.pos_ = clipStart;
	v.pos_.y -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,0.f);
	mesh.push_back( v );

	v.pos_ = clipStart;
	v.pos_.y += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,1.f);
	mesh.push_back( v );

	v.pos_ = clipEnd;
	v.pos_.y -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,0.f);
	mesh.push_back( v );

	//triangle 2
	v.pos_ = clipEnd;
	v.pos_.y -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,0.f);
	mesh.push_back( v );

	v.pos_ = clipStart;
	v.pos_.y += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,1.f);
	mesh.push_back( v );

	v.pos_ = clipEnd;
	v.pos_.y += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,1.f);
	mesh.push_back( v );

	//triangle 3
	v.pos_ = clipStart;
	v.pos_.x -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,0.f);
	mesh.push_back( v );

	v.pos_ = clipStart;
	v.pos_.x += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,1.f);
	mesh.push_back( v );

	v.pos_ = clipEnd;
	v.pos_.x -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,0.f);
	mesh.push_back( v );

	//triangle 4
	v.pos_ = clipEnd;
	v.pos_.x -= thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,0.f);
	mesh.push_back( v );

	v.pos_ = clipStart;
	v.pos_.x += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(0.f,1.f);
	mesh.push_back( v );

	v.pos_ = clipEnd;
	v.pos_.x += thickness;
	projectInPlace( v.pos_ );
	v.uv_ = Vector2(endX,1.f);
	mesh.push_back( v );

	if (mesh[0].pos_.w > 0 || mesh[2].pos_.w > 0)
	{
		if ( useEffect )
			mesh.drawEffect();
		else
			mesh.draw();
		
	}
}


static CustomMesh< Moo::VertexXYZDUV > s_worldLineMesh( D3DPT_TRIANGLELIST );

bool Geometrics::beginTexturedWorldLines()
{
	DX::Device* device_ = Moo::rc().device();
	if( !device_ ) return false;
	s_worldLineMesh.clear();
	return true;
}


void Geometrics::endTexturedWorldLines( const Matrix& world )
{
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &world );
	Moo::rc().device()->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	if ( s_worldLineMesh.size() )
		s_worldLineMesh.draw();
	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
}


/**
 *	This static method draws a textured line in world coordinates.
 *
 *	@param start		The start of the line in world coordinates.
 *	@param end			The end of the line in world coordinates.
 *	@param thickness	The thickness of the line in world coordinates.
 *	@param colour		The colour of the line (vertex colour).
 *	@param textureRepeat	The repeats of the texture along the line.
 */
void Geometrics::texturedWorldLine( const Vector3 & start,
		const Vector3 & end,
		float thickness,
		const Moo::PackedColour & colour,
		float textureRepeat )
{
	float endX = 1.f;
	if ( textureRepeat > 0.f )
		endX = (end-start).length() / textureRepeat;

	Geometrics::texturedWorldLine( start, end, thickness, colour, endX, 0.f );
}


/**
 *	This static method draws a textured line in world coordinates.
 *
 *	@param start		The start of the line in world coordinates.
 *	@param end			The end of the line in world coordinates.
 *	@param thickness	The thickness of the line in world coordinates.
 *	@param colour		The colour of the line (vertex colour).
 *	@param stepX		The amount of uv to step by
 *	@param startX		The starting value for the uv
 */
void Geometrics::texturedWorldLine( const Vector3 & start,
		const Vector3 & end,
		float thickness,
		const Moo::PackedColour & colour,
		float stepX,
		float startX )
{
	//note - this is the pc ( triist ) version
	int numVertices = s_worldLineMesh.size();
	s_worldLineMesh.resize( numVertices + 18 );
	Moo::VertexXYZDUV* v = &s_worldLineMesh[ numVertices ];

	float endX = startX + stepX;

	Vector2 startX0(startX, 0.f);
	Vector2 startX1(startX, 1.f);
	Vector2 endX0(endX, 0.f);
	Vector2 endX1(endX, 1.f);

	//triangle 1
	v->pos_ = start;
	v->pos_.y -= thickness;
	v->colour_ = colour;
	v->uv_ = startX0;
	v++;

	v->pos_ = start;
	v->pos_.y += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
	v++;

	v->pos_ = end;
	v->pos_.y -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	//triangle 2
	v->pos_ = end;
	v->pos_.y -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	v->pos_ = start;
	v->pos_.y += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
	v++;

	v->pos_ = end;
	v->pos_.y += thickness;
	v->colour_ = colour;
	v->uv_ = endX1;
	v++;

	//triangle 3
	v->pos_ = start;
	v->pos_.x += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
	v++;

	v->pos_ = start;
	v->pos_.x -= thickness;
	v->colour_ = colour;
	v->uv_ = startX0;
	v++;

	v->pos_ = end;
	v->pos_.x -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	//triangle 4
	v->pos_ = end;
	v->pos_.x -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	v->pos_ = end;
	v->pos_.x += thickness;
	v->colour_ = colour;
	v->uv_ = endX1;
	v++;

	v->pos_ = start;
	v->pos_.x += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
	v++;

	//triangle 5
	v->pos_ = start;
	v->pos_.z += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
	v++;

	v->pos_ = start;
	v->pos_.z -= thickness;
	v->colour_ = colour;
	v->uv_ = startX0;
	v++;

	v->pos_ = end;
	v->pos_.z -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	//triangle 6
	v->pos_ = end;
	v->pos_.z -= thickness;
	v->colour_ = colour;
	v->uv_ = endX0;
	v++;

	v->pos_ = end;
	v->pos_.z += thickness;
	v->colour_ = colour;
	v->uv_ = endX1;
	v++;

	v->pos_ = start;
	v->pos_.z += thickness;
	v->colour_ = colour;
	v->uv_ = startX1;
}

/**
 *	This method draws lines in clip space. The points are connected together.
 *	The first point is connected to the last. There must be at least 3 points.
 */
void Geometrics::drawLinesInClip( Vector2 * points, uint count,
	const Moo::Colour& colour )
{
	if (count < 3 || !Moo::rc().device())
		return;

/*	Material mat;
	TextureStage ts;
	ts.setAlphaOperation( TextureStage::SELECTARG2, TextureStage::TEXTURE, TextureStage::DIFFUSE );
	ts.setColourOperation( TextureStage::SELECTARG2, TextureStage::TEXTURE, TextureStage::DIFFUSE );
	mat.addTextureStage( ts );
	ts.setColourOperation( TextureStage::DISABLE );
	ts.setAlphaOperation( TextureStage::DISABLE );
	mat.addTextureStage( ts );
	rc->setMaterial( &mat );*/

	CustomMesh< Moo::VertexTL > mesh( D3DPT_LINESTRIP );
//	TLVertex *tlvs = new TLVertex[ points_.size()];

	float hx = Moo::rc().halfScreenWidth();
	float hy = Moo::rc().halfScreenHeight();

	Moo::VertexTL tlv;
	tlv.colour_ = colour;
	tlv.pos_.w = 1;
	tlv.pos_.z = 0;

	for( uint i = 0; i < count; i++ )
	{
		tlv.pos_.x = ( points[i][0] * hx ) + hx;
		tlv.pos_.y = ( points[i][1] *-hy ) + hy;
		mesh.push_back( tlv );
	}

	mesh.push_back( mesh.front() );

	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	mesh.draw();
}


/**
 *	This method draws lines in world space. The points are connected together.
 *	The first point is connected to the last. There must be at least 3 points.
 */
void Geometrics::drawLinesInWorld( Vector3 * points, uint count,
	const Moo::Colour& colour )
{
	if (count < 3 || !Moo::rc().device())
		return;

	CustomMesh< Moo::VertexXYZL > mesh( D3DPT_LINESTRIP );	

	Moo::VertexXYZL tlv;
	tlv.colour_ = colour;		

	for( uint i = 0; i < count; i++ )
	{
		tlv.pos_ = points[i];		
		mesh.push_back( tlv );
	}

	mesh.push_back( mesh.front() );

	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	mesh.draw();
}


/**
 *	This method draws lines in world space. The points are connected together.
 *	The first point is connected to the last. There must be at least 3 points.
 *	This version of the method takes an array of Vector2's, and sets the z
 *	component of each to 0.
 */
void Geometrics::drawLinesInWorld( Vector2 * points, uint count,
	const Moo::Colour& colour )
{
	if (count < 3 || !Moo::rc().device())
		return;

	CustomMesh< Moo::VertexXYZL > mesh( D3DPT_LINESTRIP );	

	Moo::VertexXYZL tlv;
	tlv.colour_ = colour;		
	tlv.pos_.z = 0;

	for( uint i = 0; i < count; i++ )
	{
		tlv.pos_[0] = points[i][0];
		tlv.pos_[1] = points[i][1];
		mesh.push_back( tlv );
	}

	mesh.push_back( mesh.front() );

	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ZERO );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	mesh.draw();
}


void Geometrics::wireBox( const BoundingBox& bb,
		const Moo::Colour& colour, bool alwaysDraw )
{
	Moo::Material::setVertexColour();
	texturedWireBox( bb, colour, 0.0f, 0.0f, alwaysDraw );
}


void Geometrics::texturedWireBox( const BoundingBox& bb,
		const Moo::Colour& colour, float offset, float tile, bool alwaysDraw )
{
	DX::Device* device_ = Moo::rc().device();

	if (bb.insideOut() || !device_)
	{
		return;
	}

	Vector3 lines[18];
	lines[0] = Vector3( bb.minBounds() );
	lines[1] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.minBounds().z );
	lines[2] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.minBounds().z );
	lines[3] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.minBounds().z );
	lines[4] = lines[0];

	lines[5] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.maxBounds().z );
	lines[6] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.maxBounds().z );
	lines[7] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.maxBounds().z );
	lines[8] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.maxBounds().z );
	lines[9] = lines[5];

	lines[10] = lines[0];
	lines[11] = lines[5];

	lines[12] = lines[1];
	lines[13] = lines[6];

	lines[14] = lines[2];
	lines[15] = lines[7];

	lines[16] = lines[3];
	lines[17] = lines[8];

	Moo::VertexXYZDUV lv;
	lv.colour_ = colour;
	std::vector< Moo::VertexXYZDUV > vertices;

	for ( int i = 0; i < 18; i++ )
	{
		lv.pos_ = lines[i];
		float u = offset + i * tile;
		lv.uv_ = Vector2( u, u );
		vertices.push_back( lv );
	}

	if (alwaysDraw)
	{
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );
	}
	else
	{
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
	}

	device_->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	device_->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	device_->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setPixelShader( NULL );
	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( Moo::VertexXYZDUV::fvf() );

	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 4, &vertices[0], sizeof( Moo::VertexXYZDUV ) );
	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 4, &vertices[5], sizeof( Moo::VertexXYZDUV ) );
	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 1, &vertices[10], sizeof( Moo::VertexXYZDUV ) );
	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 1, &vertices[12], sizeof( Moo::VertexXYZDUV ) );
	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 1, &vertices[14], sizeof( Moo::VertexXYZDUV ) );
	Moo::rc().drawPrimitiveUP( D3DPT_LINESTRIP, 1, &vertices[16], sizeof( Moo::VertexXYZDUV ) );
}


void Geometrics::rgbBox( const BoundingBox& bb )
{
	DX::Device* device_ = Moo::rc().device();

	if (bb.insideOut() || !device_)
	{
		return;
	}

	Vector3 vertices[6][4];
	
	vertices[0][0] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[0][1] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.maxBounds().z );
	vertices[0][2] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.minBounds().z );
	vertices[0][3] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.maxBounds().z );

	vertices[1][0] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[1][1] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.minBounds().z );
	vertices[1][2] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.maxBounds().z );
	vertices[1][3] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.maxBounds().z );

	vertices[2][0] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[2][1] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[2][2] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.maxBounds().z );
	vertices[2][3] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.maxBounds().z );

	vertices[3][0] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.minBounds().z );
	vertices[3][1] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.maxBounds().z );
	vertices[3][2] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.minBounds().z );
	vertices[3][3] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.maxBounds().z );

	vertices[4][0] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[4][1] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.minBounds().z );
	vertices[4][2] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.minBounds().z );
	vertices[4][3] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.minBounds().z );

	vertices[5][0] = Vector3( bb.minBounds().x, bb.minBounds().y, bb.maxBounds().z );
	vertices[5][1] = Vector3( bb.maxBounds().x, bb.minBounds().y, bb.maxBounds().z );
	vertices[5][2] = Vector3( bb.minBounds().x, bb.maxBounds().y, bb.maxBounds().z );
	vertices[5][3] = Vector3( bb.maxBounds().x, bb.maxBounds().y, bb.maxBounds().z );

	typedef std::vector< Moo::VertexXYZL > VerticesVec;
	typedef std::vector< VerticesVec > VerticesVecVec;
	VerticesVecVec vertexBuffer;

	Moo::Colour s_colour[] = {
		Moo::Colour(1.0, 0.0, 0.0, 1.0),
		Moo::Colour(0.0, 1.0, 0.0, 1.0),
		Moo::Colour(0.0, 0.0, 1.0, 1.0),
		};

	for (int i=0; i<6; i++)
	{
		static Moo::VertexXYZL s_lv;
		s_lv.colour_ = s_colour[i/2];
		vertexBuffer.push_back(VerticesVec());
		for (int j=0; j<4; j++)
		{
			s_lv.pos_ = vertices[i][j];
			vertexBuffer.back().push_back(s_lv);
		}
	}

	Moo::Material::setVertexColour();
	Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

	device_->SetTransform( D3DTS_WORLD, &Moo::rc().world() );
	device_->SetTransform( D3DTS_VIEW, &Moo::rc().view() );
	device_->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );
	
	Moo::rc().setFVF( D3DFVF_XYZ | D3DFVF_DIFFUSE );

	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[0][0], sizeof( Moo::VertexXYZL ) );
	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[1][0], sizeof( Moo::VertexXYZL ) );
	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[2][0], sizeof( Moo::VertexXYZL ) );
	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[3][0], sizeof( Moo::VertexXYZL ) );
	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[4][0], sizeof( Moo::VertexXYZL ) );
	Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, &vertexBuffer[5][0], sizeof( Moo::VertexXYZL ) );
}


/**
 *	This method draws a sphere, at world position 'center',
 *	radius 'radius' and colour 'colour'
 *
 *	You are responsible for making sure the viewProjection matrix
 *	is set up, as well as the textured material.
 *
 *	The colour sets the texture factor
 */
void Geometrics::drawSphere( const Vector3 & center,
		float radius,
		const Moo::Colour& colour )
{
	sphere_.draw( center, radius, colour );
}


/**
 *	This method draws a wireframe sphere, at world position 'center',
 *	radius 'radius' and colour 'colour'
 *
 *	You are responsible for making sure the viewProjection matrix
 *	is set up, as well as the textured material.
 *
 *	The wireframe sphere has less rows and points, and does not
 *	draw the mesh of the sphere - only the circumnavigational lines.
 */
void Geometrics::wireSphere( const Vector3 & center,
		float radius,
		const Moo::Colour& colour )
{
	sphere_.draw( center, radius, colour, true );
}


/**
 *	This method is the sphere's implementation of draw
 */
void Geometrics::Sphere::draw( const Vector3 & center,
		float radius,
		const Moo::Colour& colour,
		bool wire )
{
	if ( !inited_ )
	{
		this->create( wire );
		inited_ = true;
	}


	Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	//fill the dynamic vertex buffer's vertices
	Moo::DynamicVertexBufferBase2<Moo::VertexXYZL>& vb = Moo::DynamicVertexBufferBase2<Moo::VertexXYZL>::instance();
	Moo::VertexXYZL* buffer = vb.lock( nVerts_ );
	
	memcpy( buffer, verts_, nVerts_ * sizeof( Moo::VertexXYZL ) );

	vb.unlock();

	uint32 vertexBase = vb.lockIndex();


	//fill the dynamic index buffer's vertices
	Moo::DynamicIndexBufferBase& dynamicIndexBuffer = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = dynamicIndexBuffer.lock( nIndices_ );
	ind.fill( indices_, nIndices_ );
	dynamicIndexBuffer.unlock();

	//set the transforms
	Matrix transform;
	transform.setScale( Vector3( radius, radius, radius ) );
	transform.translation( center );

	Moo::rc().push();
	Moo::rc().world( transform );

	//set the world view projection matrix
	WorldViewProjSetter::set();
	setColourConstant( *material_, "diffuseColour", colour );

	vb.set( 0 );
	dynamicIndexBuffer.indexBuffer().set();

	uint32 firstIndex = dynamicIndexBuffer.lockIndex();

	if (material_->begin())
	{
		for (uint32 i = 0; i < material_->nPasses(); i++)
		{
			material_->beginPass( i );
			if ( wire )
			{
				Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
				Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
				int numStrips = (nRows_ - 1) * 2;
				int s;
				// Draw the Rows
				for ( s = 0; s < nRows_- 1; s++ )
				{
					Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, nPts_ * s, nPts_, 
						firstIndex + nPts_ * s, nPts_ - 1 );
				}
				// Draw the verticle stripes
				for ( s = nRows_- 1; s < numStrips- 1; s++ )
				{
					Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, nPts_ * s, nPts_, 
						firstIndex + nPts_ * s, nPts_ );
				}
				// Avoid buggy line by not fully drawing the last verticle stripe
				Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, nPts_ * s, nPts_, 
						firstIndex + nPts_ * s, nPts_ - 1 );
			}
			else
			{
				int numStrips = nRows_ - 1;
				for ( int s = 0; s < numStrips; s++ )
				{
					Moo::rc().drawIndexedPrimitive( D3DPT_TRIANGLESTRIP, vertexBase, nPts_ * s, nPts_ * 2, 
						firstIndex + nPts_ * 2 * s, ( nPts_ * 2 ) - 2 );
				}
			}

			material_->endPass();
		}
		material_->end();
	}

	Moo::rc().pop();
	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
}


static AutoConfigString s_materialName( "system/geometricsMaterial" );

void Geometrics::Sphere::create( bool wire )
{
	//procedurally generate the sphere

	if ( wire )
	{
		nRows_ = 16;
		nPts_ = 16;
	}
	else
	{
		nRows_ = 20;
		nPts_ = 20;
	}

	nVerts_ = nRows_ * nPts_;

	float deltaMu = 360.f / ((float)nPts_-1.f);
	float deltaTheta = 180.f / ((float)nRows_-1.f);

	verts_ = new Moo::VertexXYZL[nVerts_];


	int idx = 0;
	for ( int row = 0; row < nRows_; row++ )
	{
		float theta = DEG_TO_RAD( deltaTheta * (float)row );

		for ( int pt = 0; pt < nPts_; pt++ )
		{
			float mu = DEG_TO_RAD( deltaMu * (float)pt );

			verts_[idx].pos_.x = sinf( theta ) * cosf( mu );
			verts_[idx].pos_.y = cosf( theta );
			verts_[idx].pos_.z = sinf( theta ) * sinf( mu );

			idx++;
		}
	}

	if ( wire )
	{
		createWireIndices();
	}
	else
	{
		createSolidIndices();
	}

	//create our material
	material_ = new Moo::EffectMaterial;	
	DataSectionPtr pSect = BWResource::openSection( s_materialName );
	material_ ->load( pSect );
}


void Geometrics::Sphere::createWireIndices()
{
	int nStrips = ( nRows_ - 1 );
	nIndices_ = nStrips * nPts_ * 2;

	indices_ = new uint16[nIndices_];
	int currIdx = 0;

	//( drawn as line strips )
	int i = 0;
	//the horizontal lines are easy.  the mesh was created this way.
	while ( i < nIndices_ / 2 )
	{
		indices_[currIdx++] = i++;
	}
	//the vertical lines are not so easy.
	//for half of the horizontal points, we stitch up
	//the opposing sides of the sphere.
	for ( i = 0; i < nPts_/2; i++ )
	{
		int j;
		for ( j=0; j<nStrips; j++ )
		{
			indices_[currIdx++] = i + (j*nPts_);
		}
		for ( j=0; j<nStrips; j++ )
		{
			indices_[currIdx++] = i + nPts_/2 + ((nStrips-j)*nPts_);
		}
	}
}


void Geometrics::Sphere::createSolidIndices()
{
	int nStrips = ( nRows_ - 1 );
	nIndices_ = nStrips * nPts_ * 2;

	indices_ = new uint16[nIndices_];
	int currIdx = 0;

	//( drawn as triangle strips )
	int i = 0;
	while ( i < nIndices_ )
	{
		indices_[i++] = currIdx;
		indices_[i++] = currIdx + nPts_;
		currIdx++;
	}
}


/**
 *	This method draws a cylinder from world position 'start' to world
 *  position 'end', with color 'colour'
 *
 *	You are responsible for making sure the viewProjection matrix
 *	is set up, as well as the textured material.
 *
 *	The colour sets the texture factor
 */
void Geometrics::drawCylinder( const Vector3 & start,
		const Vector3 & end,
		float radius,
		const Moo::Colour& colour )
{
	MF_ASSERT(0);	// this is unfinished
	//cylinder_.draw( start, end, radius, colour );
}


/**
 *	This method draws a cylinder from world position 'start' to world
 *  position 'end', with color 'colour'
 *
 *	You are responsible for making sure the viewProjection matrix
 *	is set up, as well as the textured material.
 *
 *	The wireframe sphere has less rows and points, and does not
 *	draw the mesh of the sphere - only the circumnavigational lines.
 */
void Geometrics::wireCylinder( const Vector3 & start,
		const Vector3 & end,
		float radius,
		const Moo::Colour& colour )
{
	cylinder_.draw( start, end, radius, colour, true );
}


/**
 *	This method is the sphere's implementation of draw
 */
void Geometrics::Cylinder::draw( const Vector3 & start,
		const Vector3 & end,
		float radius,
		const Moo::Colour& colour,
		bool wire )
{
	if ( !inited_ )
	{
		this->create( wire );
		inited_ = true;
	}

	//fill the dynamic vertex buffer's vertices
	Moo::DynamicVertexBufferBase2<Moo::VertexXYZL>& vb = Moo::DynamicVertexBufferBase2< Moo::VertexXYZL >::instance();
	Moo::VertexXYZL * buffer = vb.lock( nVerts_ );
	memcpy( buffer, verts_, nVerts_ * sizeof( Moo::VertexXYZL ) );
	vb.unlock();
	uint32 vertexBase = vb.lockIndex();

	//fill the dynamic index buffer's vertices
	Moo::DynamicIndexBufferBase& indices = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
	Moo::IndicesReference ind = indices.lock( nIndices_ );
	ind.fill( indices_, nIndices_ );
	indices.unlock();

	// the cylinder is created along the z axis, so rotate this to the right direction
	Vector3 cylinderAxis = end - start;
	cylinderAxis.normalise();
	Vector3 rightAngleVector = Vector3(cylinderAxis.y, -cylinderAxis.x, cylinderAxis.z);
	if (cylinderAxis.z == 1.f)
		rightAngleVector = Vector3(1.f, 0.f, 0.f);

	rightAngleVector.normalise();
	Matrix m;
	m.row(3, Vector4(start, 1.f) );
	m.row(2, Vector4(cylinderAxis, 0.f) );
	m.row(1, Vector4(rightAngleVector.crossProduct(cylinderAxis), 0.f) );
	Vector4 row1 = m.row(1); row1.normalise();
	m.row(1, row1 );
	m.row(0, Vector4(m[1].crossProduct(cylinderAxis), 0.f) );
	Vector4 row0 = m.row(0); row0.normalise();
	m.row(0, row0 );

	float longScale = (end - start).length();
	Matrix scaleMatrix;
	scaleMatrix.setScale(radius, radius, longScale);
	m.preMultiply(scaleMatrix);

	Moo::rc().push();
	Moo::rc().world( m );

	WorldViewProjSetter::set();
	setColourConstant( *material_, "diffuseColour", colour );

	//draw the sphere
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	vb.set( 0 );
    Moo::rc().setFVF( Moo::VertexXYZL::fvf() );
	indices.indexBuffer().set();

	uint32 firstIndex = indices.lockIndex();

	if (material_->begin())
	{
		for (uint32 i = 0; i < material_->nPasses(); i++)
		{
			material_->beginPass( i );
			if ( wire )
			{
				Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
				Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );

				// draw the rings
				int s;
				for ( s = 0; s < nRows_; s++ )
				{
					Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, nPoints_ * s, nPoints_, 
						firstIndex + circleStripLength_ * s, circleStripLength_ - 1 );
				}

				// draw the connecting lines and the top and bottom
				int stripStart = circleStripLength_ * nRows_;
				for ( s = 0; s < nPoints_/2-1; s++ )
				{
					Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, 0, nVerts_, 
						firstIndex + stripStart + (wallStripLength_ * s), wallStripLength_ );
				}
				Moo::rc().drawIndexedPrimitive( D3DPT_LINESTRIP, vertexBase, 0, nVerts_, 
						firstIndex + stripStart + (wallStripLength_ * s), wallStripLength_ - (s + 2) );
			}
			else
			{
				MF_ASSERT(0);	// not yet written
			}
			material_->endPass();
		}
		material_->end();
	}

	Moo::rc().pop();
	Moo::rc().setRenderState( D3DRS_LIGHTING, TRUE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );

}


void Geometrics::Cylinder::create( bool wire )
{
	//procedurally generate the cylinder
	if ( wire )
	{
		nRows_ = 16;
		nPoints_ = 16;
		//NOTE: only works for even values
	}
	else
	{
		MF_ASSERT(0);	// not yet written
	}

	nVerts_ = (nRows_ * nPoints_);

	float deltaTheta = 360.f / ((float)nPoints_);

	verts_ = new Moo::VertexXYZL[nVerts_];

	int idx = 0;
	float heightStep = 1.f / (nRows_ - 1.f);
	float height = 0.f;
	for ( int row = 0; row < nRows_; row++ )
	{
		for ( int pt = 0; pt < nPoints_; pt++ )
		{
			float theta = DEG_TO_RAD( deltaTheta * (float)pt );

			verts_[idx].pos_.x = sinf( theta );
			verts_[idx].pos_.y = cosf( theta );
			verts_[idx].pos_.z = height;

			idx++;
		}

		height += heightStep;
	}

	if ( wire )
	{
		createWireIndices();
	}
	else
	{
		createSolidIndices();
	}

	material_ = new Moo::EffectMaterial;	
	DataSectionPtr pSect = BWResource::openSection( s_materialName );
	material_ ->load( pSect );
}


void Geometrics::Cylinder::createWireIndices()
{
	int nStrips = nRows_;
	circleStripLength_ = nPoints_ + 1;
	wallStripLength_ = (nRows_ * 2) + 2;
	nIndices_ = (nRows_ * circleStripLength_);		// circle indices
	nIndices_ += ((nPoints_/2) * wallStripLength_);		// wall indices

	indices_ = new uint16[nIndices_];
	int currIdx = 0;

	//( drawn as line strips )
	for (int j = 0; j < nRows_; j++ )
	{
		int base = j * nPoints_;

		for (int i = 0; i < nPoints_; i++ )
		{
			indices_[currIdx++] = base + i;
		}

		// complete the circle
		indices_[currIdx++] = (j * nPoints_);
	}


	//the vertical lines are not so easy.
	//for half of the horizontal points, we stitch up
	//the opposing sides of the sphere.
	for (int i = 0; i < nPoints_/2; i++ )
	{
		int j;
		for ( j = 0; j < nRows_; j++ )
		{
			indices_[currIdx++] = i + (j*nPoints_);
		}

		for ( j = 0; j < nRows_; j++ )
		{
			indices_[currIdx++] = i + nPoints_/2 + ((nRows_ - j - 1)*nPoints_);
		}

		indices_[currIdx++] = i;
	}
}


void Geometrics::Cylinder::createSolidIndices()
{
	MF_ASSERT(0);	// not yet written
}


std::ostream& operator<<(std::ostream& o, const Geometrics& t)
{
	o << "Geometrics\n";
	return o;
}

/*geometrics.cpp*/
