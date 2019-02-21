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
#include "py_scene_renderer.hpp"

#include "ashes/simple_gui_component.hpp"
#include "chunk/chunk_flare.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "chunk/chunk_water.hpp"
#include "duplo/py_splodge.hpp"
#include "terrain/base_terrain_renderer.hpp"
#include "moo/light_container.hpp"
#include "moo/render_context.hpp"
#include "moo/visual_channels.hpp"
#include "moo/visual_compound.hpp"
#include "romp/enviro_minder.hpp"
#include "romp/py_texture_provider.hpp"
#include "romp/rain.hpp"

DECLARE_DEBUG_COMPONENT2( "App", 0 )

#pragma warning (disable:4355)	// this used in base member initialiser list


// -----------------------------------------------------------------------------
// Section: PySceneRenderer
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( PySceneRenderer )
PY_BEGIN_METHODS( PySceneRenderer )
	/*~ function PySceneRenderer.render
	 *
	 *	This function causes the PySceneRenderer to update its texture to the
	 *	view currently visible through the camera when the function is called.
	 *	This function only needs to be called if the dynamic attribute is 
	 *	zero (false), otherwise it is automatically called each tick.
	 */
	PY_METHOD( render )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PySceneRenderer )
	/*~ attribute PySceneRenderer.cameras
	 *
	 *	This attribute is the list of cameras through which the world is rendered.
	 *	The texture is divided vertically between the cameras, with the first
	 *	camera being rendered leftmost, and the last camera being rendered 
	 *	rightmost.
	 *
	 *	For example:
	 *	@{
	 *	psr = BigWorld.PySceneRenderer( 100, 100 )
	 *	psr.cameras = [ BigWorld.camera() ]
	 *	@}
	 *
	 *	@type	List of Cameras
	 */
	PY_ATTRIBUTE( cameras )
	/*~ attribute PySceneRenderer.texture
	 *
	 *	This attribute is the texture that the PySceneRenderer draws to.  It can
	 *	be used by components which require a PyTextureProvider, such as the gui
	 *	components to provide the texture to draw.
	 *
	 *	The texture will contain a render of the world for each camera in the
	 *	cameras list.  If dynamic is true, then the texture will be updated
	 *	each tick, otherwise it will only be updated when render is called.
	 *	If dynamic is not set, then the render function should be called at least
	 *	once before this is assigned, or it will not render.
	 *
	 *	The texture will be of the size specified in the BigWorld.PySceneRenderer
	 *	function that created this object.
	 *
	 *	For example:
	 *	@{
	 *	psr = BigWorld.PySceneRenderer( 100, 100 )
	 *	psr.cameras = [ BigWorld.camera() ]
	 *	psr.render()
	 *	com.texture = psr.texture # where com is a SimpleGUIComponent
	 *	@}
	 *
	 *	@type	Read-Only PyTextureProvider
	 */
	PY_ATTRIBUTE( texture )
	/*~ attribute PySceneRenderer.dynamic
	 *
	 *	If this attribute is zero (false) then the texture will only be updated
	 *	when the render function is called.  If this attribute is non-zero (true)
	 *	then it will be updated every tick.
	 *
	 *	@type	Integer as boolean
	 */
	PY_ATTRIBUTE( dynamic )
	/*~ attribute PySceneRenderer.staggered
	 *
	 * Controls the frequency of dynamic update. Staggered (true) provides more consistent performance while 
	 * un-staggered (false) gives more consistent quality.
	 *
	 *	@type	Integer as boolean
	 */
	PY_ATTRIBUTE( staggered )
	/*~ attribute PySceneRenderer.skipFrames
	 *
	 *	The number of frames to elapse between updates this texture.
	 *
	 *	@type	Non-negative Intager
	 */
	PY_ATTRIBUTE( skipFrames )
	/*~ attribute PySceneRenderer.fov
	 *
	 *	This attribute represents the field-of-view of the camera, in radians
	 *
	 *	@type	radian angle as float
	 */
	PY_ATTRIBUTE( fov )	
PY_END_ATTRIBUTES()

/*~ function BigWorld.PySceneRenderer
 *
 *	This function creates a new PySceneRenderer.  The function takes two
 *	arguments, which are the width and height of the texture to render to.
 *	PySceneRenderers render one or more cameras to a texture, which can then
 *	be applied to anything which can use a PyTextureProvider.
 *
 *	@param	width	an Integer.  The width of the texture to render to
 *					(in Pixels).
 *	@param	height	an Integer.  The height of the texture to render to
 *					(in Pixels).
 *
 *	@return			a new PySceneRenderer.
 */
PY_FACTORY( PySceneRenderer, BigWorld )


/**
 *	Constructor.
 */
PySceneRenderer::PySceneRenderer( int width, int height, PyTypePlus * pType ) :
	PyObjectPlus( pType ),
	TextureRenderer( width, height ),
	camerasHolder_( cameras_, this, true ),
	fovRadians_(DEG_TO_RAD(120.f)),
	idx_( 0 ),
	dynamic_( false ),
	staggered_( true )
{
	//we don't want the texture renderer to clear our render target.
	clearTargetEachFrame_ = false;		
}

/**
 *	Destructor.
 */
PySceneRenderer::~PySceneRenderer()
{
	BW_GUARD;
	for (Cameras::iterator iter = cameras_.begin();
		iter != cameras_.end();
		iter++)
	{
		Py_DECREF( *iter );
	}

	TextureRenderer::delStaggeredDynamic( this );
}


/**
 *	Draw ourselves
 */
void PySceneRenderer::renderSelf( float dTime )
{
	BW_GUARD;
	if ( !cameras_.size() || 
		ChunkManager::instance().cameraSpace() == NULL )
		return;

	EnviroMinder * pEnviro = NULL;
	pEnviro = &ChunkManager::instance().cameraSpace()->enviro();

	//first, setup some rendering options.
	//we need to draw as cheaply as possible, and
	//not clobber the main pipeline.
	ChunkWater::simpleDraw( true );
	ChunkFlare::ignore( true );
	PySplodge::s_ignoreSplodge_ = true;
	Rain::disable( true );
	Moo::Visual::disableBatching( true );
	Moo::VisualCompound::disable( true );

	//save some stuff before we stuff around ( haha )
	Matrix invView = Moo::rc().invView();
	Moo::LightContainerPtr pLC = Moo::rc().lightContainer();

	idx_ = (++idx_)%cameras_.size();
	BaseCameraPtr pCamera = cameras_[idx_];

	pCamera->update(dTime);
	pCamera->render();	//sets the view matrix
	Moo::rc().world( Matrix::identity );

	//render target is already set.

	//set the appropriate viewport into our render target
	int widthPerCamera = width_ / cameras_.size();

	DX::Viewport oldViewport;
	Moo::rc().getViewport( &oldViewport );

	float oldFOV = Moo::rc().camera().fov();
	Moo::rc().camera().fov( fovRadians_ );

	DX::Viewport newViewport = oldViewport;
	newViewport.Width = width_ / cameras_.size();
	newViewport.X = widthPerCamera * idx_;

	Moo::rc().setViewport( &newViewport );
	Moo::rc().screenWidth( widthPerCamera );
	Moo::rc().screenHeight( height_ );
	Moo::rc().updateProjectionMatrix();
	Moo::rc().updateViewTransforms();

	//draw the scene
	Moo::rc().device()->Clear(
		0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x80800080, 1.f, 0 );

	if (pEnviro)
	{
		pEnviro->allowUpdate(false);
		pEnviro->drawHind( dTime );
	}

	ChunkManager::instance().camera( Moo::rc().invView(),
		ChunkManager::instance().cameraSpace() );
			
	ChunkManager::instance().draw();
	
	Moo::rc().lightContainer(
		ChunkManager::instance().cameraSpace()->lights() );

	// Note, we won't update the terrain lod controller camera position here, 
	// the py scene renderer can re-use current lod information.
	Terrain::BaseTerrainRenderer::instance()->drawAll();

	if (pEnviro)
	{
		pEnviro->drawHindDelayed( dTime );
		pEnviro->drawFore( dTime );
		pEnviro->allowUpdate(true);
	}
	
	Moo::SortedChannel::draw();

	//set the stuff back
	Moo::rc().lightContainer( pLC );
	Moo::rc().setViewport( &oldViewport );
	Moo::rc().screenWidth( oldViewport.Width );
	Moo::rc().screenHeight( oldViewport.Height );

	ChunkManager::instance().camera( invView,
		ChunkManager::instance().cameraSpace() );

	//restore drawing states!
	ChunkWater::simpleDraw( false );
	ChunkFlare::ignore( false );
	PySplodge::s_ignoreSplodge_ = false;
	Rain::disable( false );
	Moo::VisualCompound::disable( false );
	Moo::Visual::disableBatching( false );

	Moo::rc().camera().fov( oldFOV );
	Moo::rc().updateProjectionMatrix();
}


/**
 *	Get python attribute
 */
PyObject * PySceneRenderer::pyGetAttribute( const char * attr )
{
	BW_GUARD;
	PY_GETATTR_STD();
	return PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int PySceneRenderer::pySetAttribute( const char * attr, PyObject * value )
{
	BW_GUARD;
	PY_SETATTR_STD();
	return PyObjectPlus::pySetAttribute( attr, value );
}


/**
 *	Get our texture as a texture provider
 */
PyObject * PySceneRenderer::pyGet_texture()
{
	BW_GUARD;
	return new PyTextureProvider( this, pRT_ );
}


/**
 *	Static python factory method
 */
PyObject * PySceneRenderer::pyNew( PyObject * args )
{
	BW_GUARD;
	int w, h;
	if (!PyArg_ParseTuple( args, "ii", &w, &h ))
	{
		PyErr_SetString( PyExc_TypeError, "BigWorld.PySceneRenderer() "
			"expects width and height integer arguments" );
		return NULL;
	}
	if (w <= 0 || w > 4096 || h <= 0 || h > 4096)
	{
		PyErr_SetString( PyExc_ValueError, "BigWorld.PySceneRenderer() "
			"width and heigth must be > 0 and <= 4096" );
		return NULL;
	}

	return new PySceneRenderer( w, h );
}


/**
 *	This is the release method for cameras.
 *
 *	@param pCamera		The camera to be released.
 */
void PySceneRendererCamera_release( BaseCamera *&pCamera )
{
	BW_GUARD;
	Py_XDECREF( pCamera );
}


void PySceneRenderer::dynamic( bool state )
{
	BW_GUARD;
	this->dynamic_ = state;
	if ( this->dynamic_ )
	{
		if( this->staggered_ )
			TextureRenderer::addStaggeredDynamic( this );
		else
			TextureRenderer::addDynamic( this );
	}
	else
	{
		if( this->staggered_ )
			TextureRenderer::delStaggeredDynamic( this );
		else
			TextureRenderer::delDynamic( this );
	}
}


void PySceneRenderer::staggered( bool newState )
{
	BW_GUARD;
	if( this->staggered_ == newState )
		return;

	if ( this->dynamic_ )
	{
		if( this->staggered_ )
		{
			TextureRenderer::delStaggeredDynamic( this );
			TextureRenderer::addDynamic( this );
		}
		else
		{
			TextureRenderer::delDynamic( this );
			TextureRenderer::addStaggeredDynamic( this );
		}
	}

	this->staggered_ = newState;
}

// py_scene_renderer.cpp
