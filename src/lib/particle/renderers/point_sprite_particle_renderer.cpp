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
#include "point_sprite_particle_renderer.hpp"
#include "particle_system_draw_item.hpp"
#include "particle/particle_system_manager.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/visual_channels.hpp"
#include "moo/fog_helper.hpp" 


DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

PROFILER_DECLARE( PointSpriteParticleRenderer_realDraw, "PointSpriteParticleRenderer RealDraw" );
PROFILER_DECLARE( PointSpriteParticleRenderer_realDraw_Fill, "PointSpriteParticleRenderer RealDraw Fill" );

const std::string PointSpriteParticleRenderer::nameID_ = "PointSpriteParticleRenderer";


// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PointSpriteParticleRenderer.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PointSpriteParticleRenderer.
 *
 *	@param newTextureName	String containing the file name of the sprite.
 */
PointSpriteParticleRenderer::PointSpriteParticleRenderer(
		const std::string newTextureName ) :
	SpriteParticleRenderer( newTextureName )
{
}

/**
 *	This is the destructor for PointSpriteParticleRenderer.
 */
PointSpriteParticleRenderer::~PointSpriteParticleRenderer()
{
}



// -----------------------------------------------------------------------------
// Section: Renderer Overrides for the PointSpriteParticleRenderer.
// -----------------------------------------------------------------------------

/**
 *	This is the draw method that does the drawing for the frame. In theory,
 *	a sprite particle renderer could draw multiple particle systems provided
 *	they all use the same texture and effects.
 *
 *	@param	worldTransform	Transformation matrix to use when rendering.
 *	@param	beg				Starting particle to draw.
 *	@param	end				Final particle to draw.
 *  @param	bb				Bounding box of the particles to be drawn.
 */
void PointSpriteParticleRenderer::draw( const Matrix& worldTransform,
	Particles::iterator beg,
	Particles::iterator end,
	const BoundingBox &bb )
{
	BW_GUARD;
	float particleDistConvert = 65535.f / Moo::rc().camera().farPlane();
	
	// Make sure the texture is ready.
	if ( materialSettingsChanged_ )
	{
		updateMaterial();
	}

	//calculate sorting information if necessary
	if (beg != end)
	{
		const Matrix& view = Moo::rc().view();
		float distance = 0.f;

		if (material_.destBlend() == Moo::Material::ONE)
		{
			BoundingBox bounds = bb;
			bounds.transformBy( view );
			distance = (bounds.maxBounds().z + bounds.minBounds().z) * 0.5f;
		}
		else if (!viewDependent())
		{
			Particles::iterator it = beg;
			const Vector3& pos = it->position();
			float maxDist = pos.x * view[0][2] +
				pos.y * view[1][2] +
				pos.z * view[2][2] +
				view[3][2];

			float minDist = maxDist;
			it++;

			while (it != end)
			{
				const Vector3& p = it->position();
				float dist = p.x * view[0][2] +
					p.y * view[1][2] +
					p.z * view[2][2] +
					view[3][2];

				if (dist <= 0)
					it->distance( 0 );
				else if (dist >= Moo::rc().camera().farPlane())
					it->distance( 65535 );
				else
					it->distance( uint16(particleDistConvert * dist) );

				maxDist = max( maxDist, dist );
				minDist = min( minDist, dist );

				it++;
			}
			distance = (maxDist + minDist) * 0.5f;
		}
		Moo::SortedChannel::addDrawItem( new ParticleSystemDrawItem( this, worldTransform, beg, end, distance ) );		
	}
}


///deferred renderer
void PointSpriteParticleRenderer::realDraw( const Matrix& worldTransform, Particles::iterator beg, Particles::iterator end )
{
	BW_GUARD_PROFILER( PointSpriteParticleRenderer_realDraw );

	if ( beg == end )
		return;

	if (material_.numTextureStages() == 0 ||
		(SpriteParticleRenderer::FX_SHIMMER == materialFX() &&
		!Moo::Material::shimmerMaterials))
	{
		return;
	}

	// The particles may be either in view space or world space.

	Matrix view(Matrix::identity);

	if (!viewDependent())
	{
		view = Moo::rc().view();
	}

	//////setup vertex shader here.

	if (material_.fogged())
	{
		Moo::FogHelper::setFog( Moo::rc().fogNear(), Moo::rc().fogFar(), 
 			D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR );
	}

	//render states
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );

	//streams + shaders
	Moo::rc().setIndices( NULL );
	Moo::rc().setVertexShader( ParticleSystemManager::instance().
		pPointSpriteVertexShader() );
	Moo::rc().setVertexDeclaration( ParticleSystemManager::instance().
		pPointSpriteVertexDeclaration()->declaration() );
	
	Moo::rc().setPixelShader( NULL );

	//vertex shader constants (transforms and lighting)
	Vector4 constants( 0.f, 0.5f, 1.f, 2.f );
	Moo::rc().device()->SetVertexShaderConstantF( 0, static_cast<const float*>( constants ), 1 );

	//1..4 projection matrix
	Matrix proj( local() ? worldTransform : Matrix::identity );
	proj.postMultiply( view );
	proj.postMultiply( Moo::rc().projection() );
	Matrix vp;
	XPMatrixTranspose( &vp, &proj );
	Moo::rc().device()->SetVertexShaderConstantF( 1, (float*)&vp, 4 );

	//5 - fog
	Moo::rc().device()->SetVertexShaderConstantF(  5,
		(const float*)Vector4( - ( 1.f / ( Moo::rc().fogFar() - Moo::rc().fogNear() ) ),
		Moo::rc().fogFar() / ( Moo::rc().fogFar() - Moo::rc().fogNear() ), 0, 0 ),  1 );

	//6 - PC only - screen height
	Moo::rc().device()->SetVertexShaderConstantF(  6, (const float*)Vector4(Moo::rc().screenHeight(),0.f,0.f,0.f ), 1  );

	Moo::DynamicVertexBufferBase2<Moo::VertexXYZDP>& vb = Moo::DynamicVertexBufferBase2<Moo::VertexXYZDP>::instance();
	Moo::VertexXYZDP* pVertex = vb.lock2( end - beg );
	if (!pVertex)
		return;

	Moo::rc().setRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
	Moo::rc().setRenderState( D3DRS_POINTSCALEENABLE,  TRUE );

	// store colorwriteenable in case we are shimmering
	uint32 state = Moo::rc().getRenderState( D3DRS_COLORWRITEENABLE );

	material_.set();

	if (material_.destBlend() != Moo::Material::ONE)
	{
		std::sort( beg, end, Particle::sortParticlesReverse );
	}

	uint32 nPoints = 0;
	Particles::iterator it = beg;

#ifdef SSE_MATH
	MF_ASSERT( sizeof( Moo::VertexXYZDP ) == 20 );
	MF_ASSERT( offsetof( Moo::VertexXYZDP, pos_ ) == 0 );
	MF_ASSERT( offsetof( Moo::VertexXYZDP, colour_ ) == 12 );
	MF_ASSERT( offsetof( Moo::VertexXYZDP, size_ ) == 16 );
#endif

	PROFILER_BEGIN( PointSpriteParticleRenderer_realDraw_Fill );

#ifdef SSE_MATH
	// Optimised assembly version for SSE. We can hide a lot of latency by
	// doing four particles at a time. Intrinsics don't work because the
	// compiler can't figure out how to allocate the registers in a way
	// that prevents needless stalls.

	Particle* p = &(*beg);
	nPoints = &(*end) - p;

	// Do particles in blocks of four
	uint nLoops = nPoints >> 2;
	float unpackScale = Particle::sizeRawToFloat( 1 ) * 2.f;

#define PARTICLE_OFFSET( x )		x * TYPE Particle

	if ( nLoops )
	{
		__asm
		{
			mov				esi,p				// Array of particles
			mov				edi,pVertex			// Array of vertices of type VertexXYZDP
			mov				ecx,nLoops			// Number of loops ( nPoints / 4 )
			movss			xmm7,unpackScale	// UNPACK_SIZE from particle.ipp

ploop:
			// Load sizes for particles 0 and 1 as uint16s
			movzx			eax,[esi + PARTICLE_OFFSET( 0 )]Particle.extraData_.nonMeshData_.size_
			movzx			edx,[esi + PARTICLE_OFFSET( 1 )]Particle.extraData_.nonMeshData_.size_

			// Load the position and colour of each particle into its own SSE register
			movaps			xmm0,[esi + PARTICLE_OFFSET( 0 )]Particle.positionColour_
			movaps			xmm1,[esi + PARTICLE_OFFSET( 1 )]Particle.positionColour_
			movaps			xmm2,[esi + PARTICLE_OFFSET( 2 )]Particle.positionColour_
			movaps			xmm3,[esi + PARTICLE_OFFSET( 3 )]Particle.positionColour_

			// Convert size from uint16 to float for particles 0 and 1
			cvtsi2ss		xmm4,eax
			cvtsi2ss		xmm5,edx

			// Load sizes for particles 2 and 3 as uint16s
			movzx			eax,[esi + PARTICLE_OFFSET( 2 )]Particle.extraData_.nonMeshData_.size_
			movzx			edx,[esi + PARTICLE_OFFSET( 3 )]Particle.extraData_.nonMeshData_.size_

			// Store the position and colour of each particle into the vertex buffer
			movups			[edi],xmm0
			movups			[edi+20],xmm1
			movups			[edi+40],xmm2
			movups			[edi+60],xmm3

			// Convert size from uint16 to float for particles 2 and 3
			cvtsi2ss		xmm2,eax
			cvtsi2ss		xmm3,edx

			// Scale the float sizes by UNPACK_SCALE
			mulss			xmm4,xmm7
			mulss			xmm5,xmm7
			mulss			xmm2,xmm7
			mulss			xmm3,xmm7

			// Store the sizes for each particle into the vertex buffer
			movss			[edi+16],xmm4
			movss			[edi+36],xmm5
			movss			[edi+56],xmm2
			movss			[edi+76],xmm3

			// Advance to the next particle, decrement the loop counter and branch if there's more
			add				esi,PARTICLE_OFFSET( 4 )
			add				edi,80
			sub				ecx,1

			jnz				ploop
		}
	}

#undef PARTICLE_OFFSET

	pVertex += nPoints & ~3;
	p += nPoints & ~3;

	// Do last 0-3 particles
	for ( uint i = 0; i < ( nPoints & 3 ); i++ )
	{
		_mm_storeu_ps( ( float* )&pVertex->pos_, _mm_load_ps( ( float* )p->positionColour() ) );
		pVertex->size_ = p->size() * 2.f;
		pVertex++;
		p++;
	}

#else

	while (it != end)
	{
		Particle& p = *(it++);

		pVertex->pos_ = p.position();
		pVertex->colour_ = p.colour();
		pVertex->size_ = p.size() * 2.f;

		pVertex++;
		nPoints++;
	}

#endif

	PROFILER_END();
	
	vb.unlock();
	uint32 lockIndex = vb.lockIndex();

	if (nPoints > 0)
	{
		vb.set( 0 );
		Moo::rc().drawPrimitive( D3DPT_POINTLIST, lockIndex, nPoints );
		//addToParticleCount( nPoints );
	}

	Moo::rc().setRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_POINTSCALEENABLE,  FALSE );

	// set color write enable in case we are shimmering
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE, state );
}



/**
 *	This method is called whenever a material property for the sprite has
 *	been changed. It prepares any changes for drawing.
 */
void PointSpriteParticleRenderer::updateMaterial( void )
{
	BW_GUARD;
	typedef BGUpdateData<PointSpriteParticleRenderer> PSPRBGUpdateData;
	PSPRBGUpdateData * data = new PSPRBGUpdateData(this);

	PSPRBGUpdateData::loadTexture(data);
	PSPRBGUpdateData::updateMaterial(data);
		
	materialSettingsChanged_ = false;
}


void PointSpriteParticleRenderer::updateMaterial( Moo::BaseTexturePtr texture )
{
	BW_GUARD;
	if ( material_.numTextureStages() == 0 )
	{
		// Set up the texture stages if they have not
		// been created beforehand and load the texture file.

		Moo::TextureStage textureStage;
		textureStage.pTexture( texture );
		textureStage.colourOperation( Moo::TextureStage::MODULATE,
			Moo::TextureStage::TEXTURE,
			Moo::TextureStage::DIFFUSE );
		textureStage.alphaOperation( Moo::TextureStage::MODULATE,
			Moo::TextureStage::TEXTURE,
			Moo::TextureStage::DIFFUSE );
		material_.addTextureStage( textureStage );

		// add a second texture stage that is disabled
		textureStage.colourOperation( Moo::TextureStage::DISABLE );
		textureStage.alphaOperation( Moo::TextureStage::DISABLE );
		textureStage.pTexture( NULL );
		material_.addTextureStage( textureStage );

		material_.alphaBlended( true );
		material_.selfIllum( 255.0f );
		material_.srcBlend( Moo::Material::ONE );
		material_.destBlend( Moo::Material::INV_SRC_ALPHA);
		material_.sorted( true );
		material_.zBufferRead( true );
		material_.zBufferWrite( false );
		material_.fogged( true );
	}
	else
	{
		// If the texture stages have already been set
		// up, then simply load the new texture file.
		material_.textureStage( 0 ).pTexture( texture );
	}

	// Update the Moo::Material with the current setting for materialFX.
	switch ( materialFX() )
	{
	case SpriteParticleRenderer::FX_ADDITIVE:
		material_.srcBlend( Moo::Material::SRC_ALPHA );
		material_.destBlend( Moo::Material::ONE );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
    	break;
	case SpriteParticleRenderer::FX_ADDITIVE_ALPHA:
		material_.srcBlend( Moo::Material::ONE );
		material_.destBlend( Moo::Material::INV_SRC_ALPHA );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_BLENDED:
		material_.srcBlend( Moo::Material::SRC_ALPHA );
		material_.destBlend( Moo::Material::INV_SRC_ALPHA );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_BLENDED_COLOUR:
		material_.srcBlend( Moo::Material::SRC_COLOUR );
		material_.destBlend( Moo::Material::INV_SRC_COLOUR );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_BLENDED_INVERSE_COLOUR:
		//NOTE : this option removed because fogging does not work.
		//changed to do the same as BLENDED_COLOUR.		
		material_.srcBlend( Moo::Material::SRC_COLOUR );
		material_.destBlend( Moo::Material::INV_SRC_COLOUR );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_SOLID:
		material_.srcBlend( Moo::Material::ONE );
		material_.destBlend( Moo::Material::ZERO );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( false );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_SHIMMER:
		material_.solid( false );
		material_.sorted( false );
		material_.shimmer( true );
        material_.alphaBlended( true );
        material_.alphaTestEnable( false );
		break;
	case SpriteParticleRenderer::FX_SOURCE_ALPHA:
		material_.srcBlend( Moo::Material::ONE );
		material_.destBlend( Moo::Material::ZERO );
		material_.fogged( true );
		material_.shimmer( false );
        material_.alphaBlended( false );
        material_.alphaTestEnable( true );
        material_.alphaReference( 0x80 );
		break;
	}

	materialSettingsChanged_ = false;
}


// -----------------------------------------------------------------------------
// Section: The Python Interface to PyPointSpriteParticleRenderer.
// -----------------------------------------------------------------------------

#undef PY_ATTR_SCOPE
#define PY_ATTR_SCOPE PyPointSpriteParticleRenderer::

PY_TYPEOBJECT( PyPointSpriteParticleRenderer )

/*~ function Pixie.PointSpriteRenderer
 *	Factory function to create and return a new PyPointSpriteParticleRenderer object. A
 *	PointSpriteParticleRenderer is a ParticleSystemRenderer which renders each
 *	particle as a point sprite.
 *	@param spritename Name of sprite to use for particles.
 *	@return A new PyPointSpriteParticleRenderer object.
 */
PY_FACTORY_NAMED( PyPointSpriteParticleRenderer, "PointSpriteRenderer", Pixie )

PY_BEGIN_METHODS( PyPointSpriteParticleRenderer )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyPointSpriteParticleRenderer )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
 PyPointSpriteParticleRenderer::PyPointSpriteParticleRenderer( PointSpriteParticleRendererPtr pR, PyTypePlus *pType ):
	PySpriteParticleRenderer( pR, pType )	
{	
}


/**
 *	This is an automatically declared method for any derived classes of
 *	PyObjectPlus. It connects the readable attributes to their corresponding
 *	C++ components and searches the parent class' attributes if not found.
 *
 *	@param attr		The attribute being searched for.
 */
PyObject *PyPointSpriteParticleRenderer::pyGetAttribute( const char *attr )
{
	BW_GUARD;
	PY_GETATTR_STD();

	return PySpriteParticleRenderer::pyGetAttribute( attr );
}


/**
 *	This is an automatically declared method for any derived classes of
 *	PyObjectPlus. It connects the writable attributes to their corresponding
 *	C++ components and searches the parent class' attributes if not found.
 *
 *	@param attr		The attribute being searched for.
 *	@param value	The value to assign to the attribute.
 */
int PyPointSpriteParticleRenderer::pySetAttribute( const char *attr, PyObject *value )
{
	BW_GUARD;
	PY_SETATTR_STD();

	return PySpriteParticleRenderer::pySetAttribute( attr, value );
}


/**
 *	Static Python factory method. This is declared through the factory
 *	declaration in the class definition.
 *
 *	@param	args	The list of parameters passed from Python. This should
 *					just be a string (textureName.)
 */
PyObject *PyPointSpriteParticleRenderer::pyNew( PyObject *args )
{
	BW_GUARD;
	char *nameFromArgs = "None";
	if (!PyArg_ParseTuple( args, "|s", &nameFromArgs ) )
	{
		PyErr_SetString( PyExc_TypeError, "PointSpriteRenderer() expects "
			"a optional texture name string" );
		return NULL;
	}

	PointSpriteParticleRenderer* pspr = new PointSpriteParticleRenderer( nameFromArgs );	

	return new PyPointSpriteParticleRenderer( pspr );
}
