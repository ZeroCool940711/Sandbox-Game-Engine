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
#include "py_model_renderer.hpp"

#include "pymodel.hpp"

#include "moo/light_container.hpp"
#include "moo/render_context.hpp"

#include "romp/geometrics.hpp"

#include "chunk/chunk_space.hpp"
#include "chunk/chunk_manager.hpp"

#include "romp/py_texture_provider.hpp"
#include "moo/visual_channels.hpp"
#include "moo/visual_compound.hpp"

extern void drawSortedVisuals();

static bool s_debugBB = false;


DECLARE_DEBUG_COMPONENT2( "Model", 0 )

#pragma warning (disable:4355)	// this used in base member initialiser list


// -----------------------------------------------------------------------------
// Section: PyModelRenderer
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PyModelRenderer )
PY_BEGIN_METHODS( PyModelRenderer )
	/*~ function PyModelRenderer.render
	 *
	 *	This function causes the PyModelRender to redraw all of its models with
	 *	all their current fashions (Animations, dyes, etc).  This function call
	 *	is not necessary if the dynamic attribute is set to non-zero, because
	 *	it will automatically render every tick.
	 */
	PY_METHOD( render )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyModelRenderer )
	/*~ attribute PyModelRenderer.models
	 *
	 *	This attribute is a list of all the models that will be rendered.  
	 *	
	 *	@type	List of PyModels
	 */
	PY_ATTRIBUTE( models )
	/*~ attribute PyModelRenderer.texture
	 *
	 *	This attribute is the read-only PyTextureProvider which can be used by
	 *	something like the SimpleGUIComponent to provide a dynamic texture to
	 *	render.
	 *	
	 *	@type	Read-Only PyTextureProvider
	 */
	PY_ATTRIBUTE( texture )
	/*~ attribute PyModelRenderer.dynamic
	 *
	 *	If this attribute is zero (its default), then it only updates its
	 *	texture when the render() function is called.  If it non-zero, then
	 *	every tick it updates the texture to reflect any changes in the models.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( dynamic )
PY_END_ATTRIBUTES()

/*~ function BigWorld.PyModelRenderer
 *
 *	This function creates a new PyModelRenderer, which can be used to render
 *  models into a PyTextureProvider which can supply a texture to other things,
 *  such as a SimpleGUIComponent, which need a texture to render.
 *
 *	@param	width	the width of the texture to render to 
 *	@param	height	the height of the texture to render to
 */
PY_FACTORY( PyModelRenderer, BigWorld )

/**
 *	Custom insert method to avoid placing the subject
 *	of this renderer into the space.
 *  (which would mean it gets tick() called twice)
 */
int PyModelRenderer::ModelHolder::insert( PyObject * pItem )
{
	BW_GUARD;
	int err = PySTLSequenceHolder<ChunkEmbodiments>::insert(pItem);
	if (err == 0)
	{
		ChunkEmbodimentPtr& item = lastInsert();
		if (PyModelRenderer::s_pSpace_)	
			PyModelRenderer::s_pSpace_->delHomelessItem( item );
	}
	return err;
}


/**
 *	Constructor.
 */
PyModelRenderer::PyModelRenderer( int width, int height, PyTypePlus * pType ) :
	PyObjectPlusWithVD( pType ),
	TextureRenderer( width, height ),
	modelsHolder_( models_, this, true ),
	dynamic_( false )
{
	BW_GUARD;
	if (!s_pSpace_)
	{
		s_pSpace_ = ChunkManager::instance().space( -3 );
		MF_WATCH( "Render/Debug PMR bounds", s_debugBB, Watcher::WT_READ_WRITE, "Display the bounding box of the model on the rendered texture." );
		s_pSpace_->incRef();	// never dispose so no destructor order probs
	}

	if ( !s_lighting_ )
	{
		//MOo::Colour takes either DWORD(argb) or floats (rgba)
		s_lighting_ = new Moo::LightContainer;
		s_lighting_->ambientColour( Moo::Colour(0xff101820) );
		s_lighting_->addDirectional( new Moo::DirectionalLight(
			Moo::Colour(0.75f,0.75f,0.75f,1.f), Vector3(0.5f,0.5f,-0.5f)));
		s_lighting_->addOmni( new Moo::OmniLight(
			Moo::Colour(0xff603000), Vector3(-1.f,-3.f,-3.f), 0.f, 50.f ) );
		s_lighting_->addOmni( new Moo::OmniLight(
			Moo::Colour(0xff181860), Vector3(-0.5f,2.f,5.f), 0.f, 50.f ) );
	}
}

/**
 *	Destructor.
 */
PyModelRenderer::~PyModelRenderer()
{
	modelsHolder_.erase( 0, models_.size() );
	modelsHolder_.commit();
}


/**
 *	Draw ourselves
 */
void PyModelRenderer::renderSelf( float dTime )
{
	BW_GUARD;
	Matrix oldProj = Moo::rc().projection();
	Matrix newProj;
	newProj.perspectiveProjection(
		MATH_PI / 3.f, float(width_)/height_, 0.25f, 500.f );

	for (uint i = 0; i < models_.size(); i++)
		models_[i]->move( dTime );

	for (uint i = 0; i < models_.size(); i++)
		models_[i]->tick( dTime );

	Matrix m;
	m.lookAt( Vector3( 0, 1, -1 ), Vector3( 0, -1, 1 ), Vector3( 0, 1, 0 ) );
	Moo::rc().world( Matrix::identity );
	Moo::rc().view( m );
	Moo::rc().projection( newProj );
	Moo::rc().updateViewTransforms();

	Moo::LightContainerPtr pLC = Moo::rc().lightContainer();
	Moo::rc().lightContainer( s_lighting_ );

	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );

	Moo::Visual::disableBatching( true );
	Moo::VisualCompound::disable( true );

	for (uint i = 0; i < models_.size(); i++)
	{
		models_[i]->draw();
		if ( s_debugBB )
		{
			BoundingBox bb;
			models_[i]->boundingBox( bb );
			Moo::rc().world( models_[i]->worldTransform() );
			Geometrics::wireBox( bb, 0xff0000ff );
			Moo::rc().world( Matrix::identity );
		}
	}

	if ( s_debugBB )
	{
		BoundingBox bb;
		bb.setBounds(	Vector3( -0.5f, -0.5f, -0.5f ),
						Vector3(  0.5f,  0.5f,  0.5f ) );
		Geometrics::wireBox( bb, 0xff00ff00 );
	}

	Moo::SortedChannel::draw();	
	Moo::rc().lightContainer( pLC );
	Moo::rc().setRenderState( D3DRS_COLORWRITEENABLE,
		D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN );

	Moo::VisualCompound::disable( false );
	Moo::Visual::disableBatching( false );

	Moo::rc().projection( oldProj );
}

/**
 *	Set the dynamism of this PyModelRenderer
 */
void PyModelRenderer::dynamic( bool b )
{
	BW_GUARD;
	if (dynamic_ != b)
	{
		dynamic_ = b;
		if (dynamic_)
			this->addDynamic( this );
		else
			this->delDynamic( this );
	}
}


/**
 *	Get python attribute
 */
PyObject * PyModelRenderer::pyGetAttribute( const char * attr )
{
	BW_GUARD;
	PY_GETATTR_STD();

	if (!strcmp( attr, ".chunkSpace" ))
	{
		return Script::getData( (int)&*s_pSpace_ );
	}

	return PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int PyModelRenderer::pySetAttribute( const char * attr, PyObject * value )
{
	BW_GUARD;
	PY_SETATTR_STD();
	return PyObjectPlus::pySetAttribute( attr, value );
}


/**
 *	Get our texture as a texture provider
 */
PyObject * PyModelRenderer::pyGet_texture()
{
	BW_GUARD;
	return new PyTextureProvider( this, pRT_ );
}


/**
 *	Get the model holder.
 */
PyObject * PyModelRenderer::pyGet_models()
{
	BW_GUARD;
	return Script::getData( modelsHolder_ );
}


/**
 *	Set the model holder.
 */
int PyModelRenderer::pySet_models( PyObject * value )
{
	BW_GUARD;
	return Script::setData( value, modelsHolder_, "models" );
}


/**
 *	Static python factory method
 */
PyObject * PyModelRenderer::pyNew( PyObject * args )
{
	BW_GUARD;
	int w, h;
	if (!PyArg_ParseTuple( args, "ii", &w, &h ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.PyModelRenderer() "
			"expects width and height integer arguments" );
		return NULL;
	}
	if (w <= 0 || w > 1024 || h <= 0 || h > 1024)
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.PyModelRenderer() "
			"width and heigth must be > 0 and <= 1024" );
		return NULL;
	}

	return new PyModelRenderer( w, h );
}


// static initialiser
ChunkSpacePtr PyModelRenderer::s_pSpace_ = NULL;
Moo::LightContainerPtr PyModelRenderer::s_lighting_ = NULL;

// py_model_renderer.cpp
