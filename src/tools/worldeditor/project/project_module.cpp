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
#include "worldeditor/project/project_module.hpp"
#include "worldeditor/project/space_map_debug.hpp"
#include "worldeditor/project/space_information.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/world/editor_chunk.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/misc/cvswrapper.hpp"
#include "worldeditor/import/terrain_utils.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/cvs_info_dialog.hpp"
#include "appmgr/app.hpp"
#include "appmgr/module_manager.hpp"
#include "appmgr/options.hpp"
#include "appmgr/closed_captions.hpp"
#include "romp/custom_mesh.hpp"
#include "romp/time_of_day.hpp"
#include "resmgr/bwresource.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "common/base_camera.hpp"
#include "ashes/simple_gui.hpp"
#include "chunk/chunk_format.hpp"
#include "resmgr/string_provider.hpp"
#include "moo/camera.hpp"
#include "math/colour.hpp"


DECLARE_DEBUG_COMPONENT2( "ProjectModule", 2 );



typedef ModuleManager ModuleFactory;

IMPLEMENT_CREATOR(ProjectModule, Module);

ProjectModule* ProjectModule::currentInstance_ = 0;

ProjectModule::ProjectModule() : gridWidth_(0), gridHeight_(0),minX_(0),minY_(0),
	selectionStart_( Vector2::zero() ),
	currentSelection_( GridRect::zero() ),
	currentSelectedCoord_( GridCoord::invalid() ),
	spaceAlpha_( 0.5f ),
	spaceColour_( 0x00ffffff ),
	localToWorld_( GridCoord::zero() ),
	mouseDownOn3DWindow_( false )
{
	lastCursorPosition_.x = -1;
	lastCursorPosition_.y = -1;

	currentInstance_ = this;

	shadow_ = new TextGUIComponent( TextGUIComponent::s_defFont() );
	shadow_->filterType( SimpleGUIComponent::FT_LINEAR );
	shadow_->colour( 0xff000000 );
	shadow_->slimLabel( "" );
	SimpleGUI::instance().addSimpleComponent( *shadow_ );

	text_ = new TextGUIComponent( TextGUIComponent::s_defFont() );
	text_->filterType( SimpleGUIComponent::FT_LINEAR );
	text_->colour( 0xffffffff );
	text_->slimLabel( "" );
	SimpleGUI::instance().addSimpleComponent( *text_ );

	font_ = FontManager::instance().get( Options::getOptionString( "project/normalFont", "system_medium.font" ) );
	boldFont_ = FontManager::instance().get(  Options::getOptionString( "project/boldFont", "verdana_medium.font" ) );
	if( font_ && boldFont_ )
	{
		font_->colour( 0xff000000 );
		boldFont_->colour( 0xff000000 );
	}
}

ProjectModule::~ProjectModule()
{
	currentInstance_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *text_ );
	Py_XDECREF( text_ );
	text_ = NULL;

	SimpleGUI::instance().removeSimpleComponent( *shadow_ );
	Py_XDECREF( shadow_ );
	shadow_ = NULL;
}

bool ProjectModule::init( DataSectionPtr pSection )
{
	if ( pSection )
	{
		lockMap_.init( pSection->openSection( "lockMap" ) );
		SpaceMap::instance().init( pSection->openSection( "spaceMap" ) );
	}
	return true;
}


void ProjectModule::onStart()
{
	// needed, otherwise the mouse cursor is hidden when we start?!
	::ShowCursor(1);

	// read some options in
	spaceAlpha_ = Options::getOptionFloat(
			"render/project/spaceMapAlpha", 0.5f );
	spaceAlpha_ = Math::clamp( 0.f, spaceAlpha_, 1.f );

	// get the space colour from 12:00 time of day for the space.
	EnviroMinder& em = ChunkManager::instance().cameraSpace()->enviro();
	Vector3 sunColour = em.timeOfDay()->sunAnimation().animate( 12.f );
	Vector3 ambColour = em.timeOfDay()->ambientAnimation().animate( 12.f );
	spaceColour_=Colour::getUint32(sunColour+ambColour);
	//DEBUG_MSG( "sun %0.2f, %0.2f, %0.2f ....... amb %0.2f, %0.2f, %0.2f ........ space %lx\n",
	//	sunColour.x, sunColour.y, sunColour.z, ambColour.x, ambColour.y, ambColour.z, spaceColour_ );

	// Get the handdrawn map for the current space
	std::string space = WorldManager::instance().getCurrentSpace();

	handDrawnMap_ = Moo::TextureManager::instance()->get(
		space + "/space.map.bmp",
		false,						// allowAnimation
		false						// mustExist
		);

	if (!handDrawnMap_)
	{
		handDrawnMap_ = Moo::TextureManager::instance()->get(
			"resources/maps/default.map.bmp",
			false,						// allowAnimation
			false						// mustExist
			);
	}

	// work out grid size
	DataSectionPtr	pDS = BWResource::openSection( space + "/" + SPACE_SETTING_FILE_NAME );	
	if (pDS)
	{
		int minX = pDS->readInt( "bounds/minX", 1 );
		int minY = pDS->readInt( "bounds/minY", 1 );
		int maxX = pDS->readInt( "bounds/maxX", -1 );
		int maxY = pDS->readInt( "bounds/maxY", -1 );

		minX_ = minX;
		minY_ = minY;
		gridWidth_ = maxX - minX + 1;
		gridHeight_ = maxY - minY + 1;

		localToWorld_ = GridCoord( minX, minY );
	}

	viewPosition_ = Vector3( gridWidth_ / 2.f, gridHeight_ / 2.f, -1.f );

	// set the zoom to the extents of the grid
	float angle =  Moo::rc().camera().fov() / 2.f;
	float yopp = gridHeight_ / 2.f;
	float xopp = gridWidth_ / 2.f;

	// Get the distance away we have to be to see the x points and the y points
	float yheight = yopp / tanf( angle );
	float xheight = xopp / tanf( angle * Moo::rc().camera().aspectRatio() );
	
	// Go back the furthest amount between the two of them
	viewPosition_.z = min( -xheight, -yheight );
	minViewZ_ = viewPosition_.z * 1.1f;
	savedFarPlane_ = Moo::rc().camera().farPlane();
	Moo::Camera camera = Moo::rc().camera();
	camera.farPlane( minViewZ_ * -1.1f );
	Moo::rc().camera( camera );

	// Create an automatic space map, to display the current progress
	SpaceMap::instance().spaceInformation(
		SpaceInformation(space, localToWorld_, gridWidth_, gridHeight_) );

	// Create us a lock texture, to display the locks with
	lockMap_.gridSize( gridWidth_, gridHeight_ );

	updateLockData();

	cc_ = SmartPointer<ClosedCaptions>(new ClosedCaptions(), true);
	Commentary::instance().addView( &*cc_ );
	cc_->visible( true );
}

int ProjectModule::onStop()
{
	::ShowCursor( 0 );

	Options::setOptionFloat( "render/project/spaceMapAlpha", spaceAlpha_ );

	cc_->visible( false );
	Commentary::instance().delView( &*cc_ );
	//delete cc_;
	cc_ = NULL;

	Moo::Camera camera = Moo::rc().camera();
	camera.farPlane( savedFarPlane_ );
	Moo::rc().camera( camera );

	return 0;
}

void ProjectModule::onPause()
{
	cc_->visible( false );
	Commentary::instance().delView( &*cc_ );
}

void ProjectModule::onResume( int exitCode )
{
	Commentary::instance().addView( &*cc_ );
	cc_->visible( true );
}

bool ProjectModule::updateState( float dTime )
{
	cc_->update( dTime );

	bool userInteracting =
				InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE ) ||
				InputDevices::isKeyDown( KeyEvent::KEY_MIDDLEMOUSE ) ||
				InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE );

	SpaceMap::instance().update( dTime, !userInteracting );

	// set input focus as appropriate
	bool acceptInput = WorldManager::instance().cursorOverGraphicsWnd();
	InputDevices::setFocus( acceptInput );

	SimpleGUI::instance().update( dTime );

	//Update the locked regions once a second
	static float s_lockDataUpdate = 0.f;
	s_lockDataUpdate -= dTime;

	if (s_lockDataUpdate < 0.f)
	{
		WorldManager::instance().connection().tick();
		updateLockData();
		s_lockDataUpdate = 1.f;
	}

	if ( InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ) )
	{
		// Keep cursor's click position
		// Hide the cursor when the right mouse is held down
		if ( lastCursorPosition_.x == -1 && lastCursorPosition_.y == -1 )
		{
			::ShowCursor(0);
			::GetCursorPos( &lastCursorPosition_ );
		}
		::SetCursorPos( lastCursorPosition_.x, lastCursorPosition_.y );
	}
	else
	{
		if ( lastCursorPosition_.x != -1 || lastCursorPosition_.y != -1 )
		{
			::ShowCursor(1);
			lastCursorPosition_.x = -1;
			lastCursorPosition_.y = -1;
		}
	}

	return true;
}

/**
 * Generate a mesh for a quad, given the starting locations in
 * world and texture space, and the extents for both
 */
namespace 
{

template<class V>
void quad( CustomMesh<V>& mesh,
		  Vector3 bottomLeft, float xextent, float yextent,
		  Vector2 bottomLeftUV, float xuvextent, float yuvextent )
{
	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.uv_ = bottomLeftUV;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.uv_ = bottomLeftUV + Vector2( 0.f, yuvextent );

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.uv_ = bottomLeftUV + Vector2( xuvextent, 0.f );

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.uv_ = bottomLeftUV + Vector2( xuvextent, yuvextent );


	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void quad( CustomMesh<V>& mesh,
		  Vector3 bottomLeft, float xextent, float yextent,
		  Vector2 bottomLeftUV, float xuvextent, float yuvextent, uint32 colour )
{
	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.uv_ = bottomLeftUV;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.uv_ = bottomLeftUV + Vector2( 0.f, yuvextent );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.uv_ = bottomLeftUV + Vector2( xuvextent, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.uv_ = bottomLeftUV + Vector2( xuvextent, yuvextent );
	tr.colour_ = colour;


	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void quad( CustomMesh<V>& mesh, Vector3 bottomLeft, float xextent, float yextent, DWORD colour )
{
	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.colour_ = colour;

	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );

	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

template<class V>
void wiredQuad( CustomMesh<V>& mesh, Vector3 bottomLeft, float xextent, float yextent, DWORD colour )
{
	// bottom left
	V bl;
	bl.pos_ = bottomLeft;
	bl.colour_ = colour;

	// top left
	V tl;
	tl.pos_ = bottomLeft + Vector3( 0.f, yextent, 0.f );
	tl.colour_ = colour;

	// bottom right
	V br;
	br.pos_ = bottomLeft + Vector3( xextent, 0.f, 0.f );
	br.colour_ = colour;

	// top right
	V tr;
	tr.pos_ = bottomLeft + Vector3( xextent, yextent, 0.f );
	tr.colour_ = colour;

	mesh.push_back( tl );
	mesh.push_back( tr );
	mesh.push_back( br );
	mesh.push_back( bl );
	mesh.push_back( tl );
}

}

void ProjectModule::render( float dTime )
{
    if (!Moo::rc().device())
		return;

	if (!handDrawnMap_)
		return;

	DX::Device* device_ = Moo::rc().device();

	device_->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
		0xff004e98, 1, 0 );

	Moo::rc().setPixelShader( NULL );

	Moo::rc().reset();
	Moo::rc().updateViewTransforms();
	Moo::rc().updateProjectionMatrix();


	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().setRenderState( D3DRS_CLIPPING, TRUE );
	Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	Moo::rc().setRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
	Moo::rc().setRenderState( D3DRS_LIGHTING, FALSE );
	Moo::rc().fogEnabled( false );

	Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	Moo::rc().setTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );
	Moo::rc().setTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );


	Moo::rc().device()->SetTransform( D3DTS_WORLD, &Moo::rc().world() );

	Matrix view;
	view.setTranslate( viewPosition_ );
	view.invert();

	Moo::rc().device()->SetTransform( D3DTS_VIEW, &view );
	Moo::rc().device()->SetTransform( D3DTS_PROJECTION, &Moo::rc().projection() );

	// Draw the overhead view
	Moo::rc().setTexture( 0, handDrawnMap_->pTexture() );

	CustomMesh<Moo::VertexXYZUV> mesh;
	// The bitmap is apparently upside down, so we draw the texture upside down too
	quad( mesh, Vector3( 0.f, 0.f, 0.f ),
		static_cast<float>(gridWidth_),
		static_cast<float>(gridHeight_),
		Vector2( 0.f, 1.f ), 1.f, -1.f );

	mesh.draw();

	Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE);

	// Draw the autogen map
	SpaceMap::instance().setTexture( 0 );

	Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
	Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );

	uint32 alpha = uint32(spaceAlpha_ * 255.f) << 24;
	CustomMesh<Moo::VertexXYZDUV> cmesh;
	quad( cmesh, Vector3( 0.f, 0.f, 0.f ),
		static_cast<float>(gridWidth_),
		static_cast<float>(gridHeight_),
		Vector2( 0.f, 0.f ), 1.f, 1.f,
		alpha | spaceColour_ );

	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_CURRENT );
	cmesh.draw();
	Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

	Moo::rc().setTexture( 0, 0 );	

	// Draw the lockTexture
	if ( WorldManager::instance().connection().enabled() && Options::getOptionInt( "render/misc/drawOverlayLocks", 1 ) == 1 )
	{
		lockMap_.setTexture( 0 );

		Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );

		mesh.clear();
		quad( mesh, Vector3( 0.f, 0.f, 0.f ),
			static_cast<float>(gridWidth_),
			static_cast<float>(gridHeight_),
			Vector2( 0.f, 0.f ), 1.f, 1.f );

		mesh.draw();

		Moo::rc().setSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		Moo::rc().setSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
	}

	Moo::rc().setTexture( 0, 0 );

	// Draw the current selection
	if (currentSelection_.valid())
	{
		CustomMesh<Moo::VertexXYZL> sel;

		BWLock::WorldEditordConnection& conn = WorldManager::instance().connection();
		if( conn.connected() )
		{
			float x = (float) currentSelection_.bottomLeft.x - conn.xExtent();
			float y = (float) currentSelection_.bottomLeft.y - conn.zExtent();
			float width = (float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x + conn.xExtent() * 2;
			float height = (float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y + conn.zExtent() * 2;
			if( x < 0 )
			{
				width += x;
				x = 0;
			}
			if( y < 0 )
			{
				height += y;
				y = 0;
			}
			if( x + width > gridWidth_ )
				width = (float)gridWidth_ - x;
			if( y +height > gridHeight_ )
				height = (float)gridHeight_ - y;
			quad( sel, Vector3( x, y, 0.f), width, height, 0x40202020 );

			CustomMesh<Moo::VertexXYZL> wired( D3DPT_LINESTRIP );
			wiredQuad( wired, Vector3( x, y, 0.f), width, height, 0xffffffff );
			wired.draw();
		}

		quad( sel, Vector3( (float) currentSelection_.bottomLeft.x, (float) currentSelection_.bottomLeft.y, 0.f),
			(float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x,
			(float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y,
			0x40202020 );

		sel.draw();

		CustomMesh<Moo::VertexXYZL> wired( D3DPT_LINESTRIP );
		wiredQuad( wired, Vector3( (float) currentSelection_.bottomLeft.x, (float) currentSelection_.bottomLeft.y, 0.f),
			(float) currentSelection_.topRight.x - currentSelection_.bottomLeft.x,
			(float) currentSelection_.topRight.y - currentSelection_.bottomLeft.y, 0xffffff7f );
		wired.draw();
	}
	else if( memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0
		&& WorldManager::instance().connection().connected() && WorldManager::instance().connection().isLockedByMe(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		std::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		for( std::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
		{
			CustomMesh<Moo::VertexXYZL> sel;

			int left = max( (int)iter->left_, minX_ );
			int top = max( (int)iter->top_, minY_ );
			int right = min( (int)iter->right_, (int)gridWidth_ + minX_ - 1 );
			int bottom = min( (int)iter->bottom_, (int)gridHeight_ + minY_ - 1 );
			quad( sel, Vector3( (float)left - minX_, (float)top - minY_, 0.f ),
				(float)right - left + 1, (float)bottom - top + 1,
				0x4400ffff );

			sel.draw();
		}
	}

	Vector2 grid = currentGridPos();
	if( WorldManager::instance().connection().connected() &&
		grid.x >= 0 && grid.y >= 0 && grid.x < gridWidth_ && grid.y < gridHeight_ &&
		WorldManager::instance().cursorOverGraphicsWnd() && font_ && boldFont_ )
	{
		Moo::rc().setPixelShader( NULL );

		BWLock::GridInfo gridInfo = WorldManager::instance().connection().getGridInformation(
			int16( floor( grid.x + minX_ ) ), int16( floor( grid.y + minY_ ) ) );

		if( !gridInfo.empty() )
		{
			class ToWSTR
			{
			public:
				std::wstring operator()( const std::string& str )
				{
					std::wstring wstr;
					for( std::string::const_iterator iter = str.begin(); iter != str.end(); ++iter )
						wstr += *iter;
					return wstr;
				}
			}
			ToWSTR;

			Vector2 gridPos = currentGridPos();
			Vector3 world = gridPosToWorldPos( gridPos );
			ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();

			gridInfo.insert( gridInfo.end() - 1, std::make_pair( "Where:", dirMap->outsideChunkIdentifier(world) ) );

			static const int MARGIN = 5;
			POINT cursorPos = WorldManager::instance().currentCursorPosition();

			int width = 512;

			int wb, w, hb, h, W = 0, H = 0;

			for( unsigned int i = 0; i < gridInfo.size(); ++i )
			{
				boldFont_->metrics().stringDimensions( ToWSTR( gridInfo[ i ].first ), wb, hb );
				font_->metrics().stringDimensions( ToWSTR( gridInfo[ i ].second ), w, h );
				if( i == gridInfo.size() - 1 && wb + w > width )
				{
					W = width;
					w = W - wb;
					font_->metrics().breakString( ToWSTR( gridInfo[ i ].second ), w, h );
					H += h;
				}
				else
				{
					if( wb + w > W )
						W = wb + w;
					H += hb;
				}
			}

			int x = cursorPos.x + MARGIN * 2;
			if( x + W + MARGIN * 4 > Moo::rc().screenWidth() )
				x = (int)Moo::rc().screenWidth() - W - MARGIN * 4;
			int y = cursorPos.y + MARGIN * 2;
			if( y + H + MARGIN * 4 > Moo::rc().screenHeight() )
				y = (int)Moo::rc().screenHeight() - H - MARGIN * 4;

			CustomMesh<Moo::VertexXYZL> border;
			Vector2 leftBottom = pixelToGridPos( x - MARGIN - 1, y + H + MARGIN + 1 );
			Vector2 rightTop = pixelToGridPos( x + W + MARGIN + 1, y - MARGIN - 1 );

			quad( border, Vector3( leftBottom.x, leftBottom.y, 0.f ),
				rightTop.x - leftBottom.x, rightTop.y - leftBottom.y, 0xff000000 );

			border.draw();

			CustomMesh<Moo::VertexXYZL> mesh;
			leftBottom = pixelToGridPos( x - MARGIN, y + H + MARGIN );
			rightTop = pixelToGridPos( x + W + MARGIN, y - MARGIN );

			quad( mesh, Vector3( leftBottom.x, leftBottom.y, 0.f ),
				rightTop.x - leftBottom.x, rightTop.y - leftBottom.y, 0xffffffE1 );

			mesh.draw();

			for( unsigned int i = 0; i < gridInfo.size(); ++i )
			{
				FontManager::instance().setMaterialActive( boldFont_ );
				boldFont_->drawString( gridInfo[ i ].first, x, y );
				boldFont_->metrics().stringDimensions( ToWSTR( gridInfo[ i ].first ), wb, hb );
				font_->metrics().stringDimensions( ToWSTR( gridInfo[ i ].second ), w, h );
				FontManager::instance().setMaterialActive( font_ );
				font_->drawString( ToWSTR( gridInfo[ i ].second ), x + wb, y + hb - h, width - wb, 0x7fffffff );
				y += hb;
			}
		}
	}

	// Draw the space map debug overlay
	SpaceMapDebug::instance().draw();

	this->writeStatus();
}


/**
 *	This method writes out some status panel sections that are done every frame.
 *	i.e. FPS and cursor location.
 */
void ProjectModule::writeStatus()
{
    WorldManager::instance().setStatusMessage(1, "");
    WorldManager::instance().setStatusMessage(2, "");
    WorldManager::instance().setStatusMessage(4, "");

	//Panel 0 - memory load
	WorldManager::instance().setStatusMessage( 0,
		L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/MEMORY_LOAD", WorldManager::instance().getMemoryLoad() ) );

	//Panel 3 - locator position
	Vector2 gridPos = currentGridPos();
	Vector3 world = gridPosToWorldPos( gridPos );
	ChunkDirMapping* dirMap = WorldManager::instance().chunkDirMapping();
	std::string name = dirMap->outsideChunkIdentifier(world);
	WorldManager::instance().setStatusMessage( 3, name );

	// Panel 5 - number of chunks loaded
	EditorChunkCache::lock();
	
	unsigned int dirtyTotal = WorldManager::instance().dirtyChunks();
	unsigned int numLodTex  = WorldManager::instance().dirtyLODTextures();
	if ( dirtyTotal != 0 || numLodTex != 0 )
	{
		WorldManager::instance().setStatusMessage( 5,
			L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOADED_WITH_DIRTY",
				EditorChunkCache::chunks_.size(), dirtyTotal, numLodTex ) );
	}
	else
	{
		WorldManager::instance().setStatusMessage( 5,
			L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOADED",
				EditorChunkCache::chunks_.size() ) );
	}
	EditorChunkCache::unlock();

	MainFrame *mainFrame = reinterpret_cast<MainFrame *>(AfxGetMainWnd());
    mainFrame->frameUpdate(true);
}


Vector2 ProjectModule::currentGridPos()
{
	POINT pt = WorldManager::instance().currentCursorPosition();
	return pixelToGridPos( pt.x, pt.y );
}

Vector2 ProjectModule::pixelToGridPos( int x, int y )
{
	Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
			(float(x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(y) / Moo::rc().screenHeight()) * 2.f );

	Matrix view;
	view.setTranslate( viewPosition_ );

	Vector3 worldRay = view.applyVector( cursorPos );
	worldRay.normalise();

	PlaneEq gridPlane( Vector3(0.f, 0.f, 1.f), .0001f );

	Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

	return Vector2( gridPos.x, gridPos.y );
}

Vector3 ProjectModule::gridPosToWorldPos( Vector2 gridPos )
{
	Vector2 w = (gridPos + Vector2( float(localToWorld_.x), float(localToWorld_.y) )) *
		GRID_RESOLUTION;

	return Vector3( w.x, 0, w.y);
}

bool ProjectModule::handleKeyEvent( const KeyEvent & event )
{
	if (event.key() == KeyEvent::KEY_LEFTMOUSE)
	{
		if ( event.isKeyDown() )
		{
			mouseDownOn3DWindow_ = true;
		}
		else
		{
			mouseDownOn3DWindow_ = false;
		}
	}

	// Go to the world location when the middle mouse button is held down
	if (event.key() == KeyEvent::KEY_MIDDLEMOUSE && !event.isKeyDown())
	{
		// Get where we click in grid coords
		Vector2 gridPos = currentGridPos();
		Vector3 world = gridPosToWorldPos( gridPos );
        
        // Find the height at this point
        float height = TerrainUtils::heightAtPos(world.x, world.z, true);
        world.y = height + Options::getOptionInt( "graphics/farclip", 500 )/10.0f;

		// Set the view matrix to the new world coords
		Matrix view = WorldEditorCamera::instance().currentCamera().view();
		view.setTranslate( world );
		view.preRotateX( DEG_TO_RAD( 30.f ) );
		view.invert();
		WorldEditorCamera::instance().currentCamera().view( view );

		if( Options::getOptionInt( "camera/ortho" ) == WorldEditorCamera::CT_Orthographic )
		{
			WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_MouseLook );
			WorldEditorCamera::instance().changeToCamera( WorldEditorCamera::CT_Orthographic );
		}

		// Now, change back to object mode
		PyObject* pModule = PyImport_ImportModule( "WorldEditorDirector" );
		if (pModule != NULL)
		{
			PyObject* pScriptObject = PyObject_GetAttr( pModule, Py_BuildValue( "s", "bd" ) );

			if (pScriptObject != NULL)
			{
				Script::call(
					PyObject_GetAttrString( pScriptObject, "changeToMode" ),
					Py_BuildValue( "(s)", "Object" ),
					"ProjectModule");
					
				Py_DECREF( pScriptObject );
			}

			PanelManager::instance().setDefaultToolMode();
			Py_DECREF( pModule );
		}

		return true;
	}

	// Drag a rectangle snapped to chunk boundaries with the left mouse button
	if (event.key() == KeyEvent::KEY_LEFTMOUSE && event.isKeyDown())
	{
		// Start the selection
		selectionStart_ = currentGridPos();
		currentSelection_ = GridRect::fromCoords( selectionStart_, selectionStart_ );


		currentSelectedCoord_ = GridCoord( currentGridPos() );
		if( currentSelectedCoord_.x < 0 || currentSelectedCoord_.x >= (int)gridWidth_ ||
			currentSelectedCoord_.y < 0 || currentSelectedCoord_.y >= (int)gridHeight_ )
			currentSelectedCoord_ = GridCoord::invalid();
	}

	if (event.key() == KeyEvent::KEY_1 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "Objects" );
	}
	if (event.key() == KeyEvent::KEY_2 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "TerrainTexture" );
	}
	if (event.key() == KeyEvent::KEY_3 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "TerrainHeight" );
	}
	if (event.key() == KeyEvent::KEY_4 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "TerrainFilter" );
	}
	if (event.key() == KeyEvent::KEY_5 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "TerrainMesh" );
	}
	if (event.key() == KeyEvent::KEY_6 && event.isKeyDown() && !event.modifiers())
	{
		PanelManager::instance().setToolMode( "TerrainImpExp" );
	}

	return false;
}

bool ProjectModule::handleMouseEvent( const MouseEvent & event )
{
	bool handled = false;

	// Adjust zoom
	if (event.dz() != 0)
	{
		viewPosition_.z += (event.dz() > 0) ? -minViewZ_/25.f : minViewZ_/25.f;

		if (viewPosition_.z > -1.f)
			viewPosition_.z = -1.f;

		if (viewPosition_.z < minViewZ_)
			viewPosition_.z = minViewZ_;

		handled = true;
	}

	// Pan around
	if ((event.dx() != 0 || event.dy() != 0) && InputDevices::isKeyDown( KeyEvent::KEY_RIGHTMOUSE ))
	{
        // The current zoom scale:
        CPoint pt(0, 0);
        Vector2 worldPos = gridPos(pt);
        ++pt.x;
        Vector2 xWorldPos = gridPos(pt);
        --pt.x; ++pt.y;
        Vector2 yWorldPos = gridPos(pt);
        float scaleX_ = fabsf(xWorldPos.x - worldPos.x);
        float scaleY_ = fabsf(yWorldPos.y - worldPos.y);

	    viewPosition_.x -= scaleX_*event.dx();
	    viewPosition_.y += scaleY_*event.dy();

		handled = true;
	}

	// Update the selection
	if( InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE ) && mouseDownOn3DWindow_ )
	{
		if( event.dx() != 0 || event.dy() != 0 )
		{
			Vector2 curPos = currentGridPos();
			Vector2 initPos = selectionStart_;
			if (curPos.x > selectionStart_.x) 
				curPos.x = ceilf( curPos.x );
			else
				initPos.x = ceilf( initPos.x );

			if (curPos.y > selectionStart_.y)
				curPos.y = ceilf( curPos.y );
			else
				initPos.y = ceilf( initPos.y );

			currentSelection_ = GridRect::fromCoords( initPos, curPos );
			currentSelection_.clamp( 0,0,gridWidth_, gridHeight_ );
		}
	}

	return handled;
}
bool readLine( SOCKET sock, std::string& line )
{
	char buf[1024];

	char* p = buf;

	for (uint i = 0; i < 1023; i++)
	{
		if ( recv( sock, p, 1, 0) != 1 )
			break;

		if (*p == '\n')
		{
			*p = '\0';
			line = buf;
			return true;
		}

		p++;
	}

	line = "";
	return false;
}

bool ProjectModule::isReadyToLock() const
{
	BWLock::WorldEditordConnection& conn = WorldManager::instance().connection();
	if( !currentSelection_.valid() || !conn.connected() )
		return false;

	// if all chunks are locked, we shouldn't lock them again
	bool allWritable = true;
	GridRect region = currentSelection_ + localToWorld_;
	for( int16 gridX = region.bottomLeft.x; gridX < region.topRight.x && allWritable; ++gridX )
	{
		for( int16 gridZ = region.bottomLeft.y; gridZ < region.topRight.y && allWritable; ++gridZ )
		{
			if( !conn.isWritableByMe( gridX, gridZ ) )
			{
				allWritable = false;
				break;
			}
		}
	}

	if( allWritable )
		return false;

	// if one of the chunks are locked by others, we cannot lock them
	for( int16 gridX = region.bottomLeft.x - conn.xExtent(); gridX < region.topRight.x + conn.xExtent(); ++gridX )
	{
		for( int16 gridZ = region.bottomLeft.y - conn.zExtent(); gridZ < region.topRight.y + conn.zExtent(); ++gridZ )
		{
			if( conn.isLockedByOthers( gridX, gridZ ) )
			{
				return false;
			}
		}
	}

	return true;
}

bool ProjectModule::isReadyToCommitOrDiscard() const
{
	return memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().connected() && WorldManager::instance().connection().isLockedByMe(
		currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );
}

std::set<std::string> ProjectModule::lockedChunksFromSelection( bool insideOnly )
{
	std::set<std::string> result;
	if( memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		std::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );
		for( std::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
			getChunksFromRect( result, *iter, insideOnly );
	}
	return result;
}

std::set<std::string> ProjectModule::graphFilesFromSelection()
{
	std::set<std::string> result;
	if( memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		std::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		WIN32_FIND_DATA wfd;
		std::string toFind = BWResolver::resolveFilename( currentSpaceDir() ) + "\\*.graph";
		HANDLE find = FindFirstFile( toFind.c_str(), &wfd );
		if( find != INVALID_HANDLE_VALUE )
		{
			do
			{
				if( !( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					DataSectionPtr graph = BWResource::openSection( currentSpaceDir() + '/' + wfd.cFileName );
					if( graph )
					{
						bool added = false;
						for( int i = 0; i < graph->countChildren() && !added; ++i )
						{
							DataSectionPtr wp = graph->openChild( i )->openSection( "worldPosition" );
							if( wp )
							{
								Vector3 worldPosition = wp->asVector3();
								int cx = ChunkSpace::pointToGrid( worldPosition.x );
								int cz = ChunkSpace::pointToGrid( worldPosition.z );
								for( std::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
									if( iter->in( cx, cz ) )
									{
										result.insert( wfd.cFileName );
										added = true;
										break;
									}
							}
						}
					}
				}
			}
			while( FindNextFile( find, &wfd ) );
			FindClose( find );
		}
	}
	return result;
}

std::set<std::string> ProjectModule::vloFilesFromSelection()
{
	std::set<std::string> result;
	if( memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0 &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		std::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		WIN32_FIND_DATA wfd;
		std::string toFind = BWResolver::resolveFilename( currentSpaceDir() ) + "\\*.vlo";
		HANDLE find = FindFirstFile( toFind.c_str(), &wfd );
		if( find != INVALID_HANDLE_VALUE )
		{
			do
			{
				if( !( wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
				{
					DataSectionPtr vlo = BWResource::openSection( currentSpaceDir() + '/' + wfd.cFileName );
					if( vlo && vlo->countChildren() )
					{
						if( vlo->openSection( "water" ) )
						{
							Vector3 position = vlo->readVector3( "water/position" );
							Vector3 size = vlo->readVector3( "water/size" );

							int cxMin = ChunkSpace::pointToGrid( position.x );
							int czMin = ChunkSpace::pointToGrid( position.z );
							int cxMax = ChunkSpace::pointToGrid( position.x + size.x );
							int czMax = ChunkSpace::pointToGrid( position.z + size.z );

							BWLock::Rect rect( cxMin, czMin, cxMax, czMax );

							for( std::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
								if( iter->intersect( rect ) )
								{
									result.insert( wfd.cFileName );
									std::string odata = BWResource::changeExtension( wfd.cFileName, ".odata" );
									if( BWResource::fileExists( ProjectModule::currentSpaceDir() + '/' + odata ) )
										result.insert( odata );
									break;
								}
						}
						else
						{
							ERROR_MSG( "ProjectModule::vloFilesFromSelection: WE has encountered an unknown type of VLO: %s",
								vlo->openChild( 0 )->sectionName().c_str() );
						}
					}
				}
			}
			while( FindNextFile( find, &wfd ) );
			FindClose( find );
		}
	}
	return result;
}

void ProjectModule::getChunksFromRect( std::set<std::string>& chunks, BWLock::Rect rect, bool insideOnly )
{
	for( int x = rect.left_; x <= rect.right_; ++x )
		for( int z = rect.top_; z <= rect.bottom_; ++z )
			if( x >= minX_ && z >= minY_ && ( x < minX_ + (int)gridWidth_ ) && ( z < minY_ + (int)gridHeight_ ) )
				chunks.insert( ChunkFormat::outsideChunkIdentifier( x, z ) );

	std::string spaceDir = BWResource::resolveFilename( Options::getOptionString( "space/mru0" ) ) + "/";

	WIN32_FIND_DATA findData;
	HANDLE find = FindFirstFile( ( spaceDir + "*i.chunk" ).c_str(), &findData );
	if( find != INVALID_HANDLE_VALUE )
	{
		do
		{
			std::string chunkFileName = BWResource::dissolveFilename( spaceDir + findData.cFileName );
			DataSectionPtr pDS = BWResource::openSection( chunkFileName );
			BoundingBox bb( pDS->readVector3( "boundingBox/min" ), pDS->readVector3( "boundingBox/max" ) );
			int left = ChunkSpace::pointToGrid( bb.minBounds().x );
			int top = ChunkSpace::pointToGrid( bb.minBounds().z );
			int right = ChunkSpace::pointToGrid( bb.maxBounds().x );
			int bottom = ChunkSpace::pointToGrid( bb.maxBounds().z );
			*strrchr( findData.cFileName, '.' ) = 0;
			if( insideOnly )
			{
				if( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ &&
					right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ )
					chunks.insert( findData.cFileName );
			}
			else
			{
				if( ( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ ) ||
					( right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ ) )
					chunks.insert( findData.cFileName );
			}
		}
		while( FindNextFile( find, &findData ) );
		FindClose( find );
	}
	find = FindFirstFile( ( spaceDir + "*i.~chunk~" ).c_str(), &findData );
	if( find != INVALID_HANDLE_VALUE )
	{
		do
		{
			std::string chunkFileName = BWResource::dissolveFilename( spaceDir + findData.cFileName );
			DataSectionPtr pDS = BWResource::openSection( chunkFileName );
			BoundingBox bb( pDS->readVector3( "boundingBox/min" ), pDS->readVector3( "boundingBox/max" ) );
			int left = ChunkSpace::pointToGrid( bb.minBounds().x );
			int top = ChunkSpace::pointToGrid( bb.minBounds().z );
			int right = ChunkSpace::pointToGrid( bb.maxBounds().x );
			int bottom = ChunkSpace::pointToGrid( bb.maxBounds().z );
			*strrchr( findData.cFileName, '.' ) = 0;
			if( insideOnly )
			{
				if( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ &&
					right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ )
					chunks.insert( findData.cFileName );
			}
			else
			{
				if( ( left >= rect.left_ && left <= rect.right_ &&
					top >= rect.top_ && top <= rect.bottom_ ) ||
					( right >= rect.left_ && right <= rect.right_ && 
					bottom >= rect.top_ && bottom <= rect.bottom_ ) )
					chunks.insert( findData.cFileName );
			}
		}
		while( FindNextFile( find, &findData ) );
		FindClose( find );
	}
}

/**
 * Don't forget to call updateLockData() after this
 */
bool ProjectModule::lockSelection( const std::string& description )
{
	if (!currentSelection_.valid())
		return false;

	if ( !WorldManager::instance().connection().connected() )
	{
		INFO_MSG( "Unable to connect to bwlockd\n" );
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UNABLE_TO_CONNECT" ) );
		return false;
	}

	if( CVSWrapper::enabled() && !CVSWrapper( currentSpaceResDir() ).isInCVS( currentSpaceDir() ) )
	{
		currentSelection_ = GridRect::fromCoords( GridCoord( 0, 0 ),
			GridCoord( gridWidth_, gridHeight_ ) );
	}
	currentSelectedCoord_.x = min( currentSelection_.bottomLeft.x, currentSelection_.topRight.x );
	currentSelectedCoord_.y = min( currentSelection_.bottomLeft.y, currentSelection_.topRight.y );
	GridRect region = currentSelection_ + localToWorld_;
	// clear the selection
	currentSelection_ = GridRect::zero();

	CWaitCursor wait;

	bool result = WorldManager::instance().connection().lock( region, description );
	if( result )
		updateLockData();
	return result;
}

bool ProjectModule::discardLocks( const std::string& description )
{

	if( memcmp( &currentSelectedCoord_, &GridCoord::invalid(), sizeof( GridCoord ) ) != 0
		&& WorldManager::instance().connection().connected() &&
		WorldManager::instance().connection().isLockedByMe( currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ ) )
	{
		std::set<BWLock::Rect> rects = WorldManager::instance().connection().getLockRects(
			currentSelectedCoord_.x + minX_, currentSelectedCoord_.y + minY_ );

		CWaitCursor wait;
		for( std::set<BWLock::Rect>::iterator iter = rects.begin(); iter != rects.end(); ++iter )
			WorldManager::instance().connection().unlock( *iter, description );

		updateLockData();
		return true;
	}
	return false;
}

void ProjectModule::commitDone()
{
	if( !WorldManager::instance().connection().connected() )
	{
		ERROR_MSG( "Unable to connect to bwlockd\n" );
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UNABLE_TO_CONNECT" ) );
		return;
	}
	// TODO: possibly do sth. here?
}

void ProjectModule::updateLockData()
{
	std::vector<unsigned char> lockData = WorldManager::instance().connection().getLockData(
		minX_, minY_, gridWidth_, gridHeight_ );
	lockMap_.updateLockData( gridWidth_, gridHeight_, &lockData[0] );
}


ProjectModule* ProjectModule::currentInstance()
{
	return currentInstance_;
}

std::string ProjectModule::currentSpaceDir()
{
	return Options::getOptionString( "space/mru0" );
}

std::string ProjectModule::currentSpaceResDir()
{
	std::string spacePath = currentSpaceDir();
	std::string fullPath = BWResource::resolveFilename( spacePath );

	if (!fullPath.empty())
	{
		if (*spacePath.rbegin() != '/' && *fullPath.rbegin() == '/')
		{
			fullPath.resize( fullPath.size() - 1 );
		}
		else if (*spacePath.rbegin() == '/' && *fullPath.rbegin() != '/')
		{
			fullPath += '/';
		}

		fullPath.resize( fullPath.size() - spacePath.size() );
	}

	return fullPath;
}

std::string ProjectModule::currentSpaceAbsoluteDir()
{
	return BWResource::resolveFilename( Options::getOptionString( "space/mru0" ) );
}

void ProjectModule::projectMapAlpha( float a )
{
	spaceAlpha_ = a;
}


float ProjectModule::projectMapAlpha()
{
	return spaceAlpha_;
}


void ProjectModule::regenerateAllDirty()
{
    ProjectModule *projectModule = ProjectModule::currentInstance();
    bool haveProjectModule = projectModule != NULL;
    if (!haveProjectModule)
    {
        projectModule = new ProjectModule();
        projectModule->onStart();
    }
    SpaceMap::instance().regenerateAllDirty(true);
    if (!haveProjectModule)
    {
        projectModule->onStop();
        delete projectModule; projectModule = NULL;
    }

}


Vector2 ProjectModule::gridPos(POINT pt) const
{
	Vector3 cursorPos = Moo::rc().camera().nearPlanePoint(
			(float(pt.x) / Moo::rc().screenWidth()) * 2.f - 1.f,
			1.f - (float(pt.y) / Moo::rc().screenHeight()) * 2.f );

	Matrix view;
	view.setTranslate( viewPosition_ );

	Vector3 worldRay = view.applyVector( cursorPos );
	worldRay.normalise();

	PlaneEq gridPlane( Vector3(0.f, 0.f, 1.f), .0001f );

	Vector3 gridPos = gridPlane.intersectRay( viewPosition_, worldRay );

	return Vector2( gridPos.x, gridPos.y );
}

/*~ function WorldEditor.projectLock
 *	@components{ worldeditor }
 *
 *	This function locks the project for editing the locked chunks.
 *	Locked chunks can only be edited with the editor that locked them.
 *
 *	@param commitMsg A lock message.
 */
static PyObject* py_projectLock( PyObject* args )
{
	char* commitMsg;
	if (!PyArg_ParseTuple( args, "s", &commitMsg ))	{
		PyErr_SetString( PyExc_TypeError, "projectLock() "
			"expects a string argument" );
		return NULL;
	}
	if( !WorldManager::instance().connection().connected() )
	{
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CANNOT_LOCK" ) );
		Py_Return;
	}

	ProjectModule* instance = ProjectModule::currentInstance();

	if (instance)
	{
		if (instance->lockSelection( commitMsg ))
		{
			CVSInfoDialog dlg(
				 L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/LOCK_SELECTION_TITLE" ) );

			CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

			std::set<std::string> chunks = instance->lockedChunksFromSelection( false );

			std::vector<std::string> filesToUpdate;
			for( std::set<std::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
			{
				filesToUpdate.push_back( *iter + ".chunk" );
				filesToUpdate.push_back( *iter + ".cdata" );
			}

			std::set<std::string> graphs = instance->graphFilesFromSelection();
			for( std::set<std::string>::iterator iter = graphs.begin();
				iter != graphs.end(); ++iter )
			{
				if( cvs.isInCVS( *iter ) )
					filesToUpdate.push_back( *iter );
			}

			std::set<std::string> vlos = instance->vloFilesFromSelection();
			for( std::set<std::string>::iterator iter = vlos.begin();
				iter != vlos.end(); ++iter )
			{
				if( cvs.isInCVS( *iter ) )
					filesToUpdate.push_back( *iter );
			}

			if (!cvs.updateFolder( "" )||!cvs.editFiles( filesToUpdate ))
			{
				dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UPDATE_FAILED" ) );
			}
			else
			{
				INFO_MSG( "cvs update done\n" );

				EditorChunkCache::forwardReadOnlyMark();

				// Clear WorldEditors's lists of what's been changed
				WorldManager::instance().resetChangedLists();

				WorldManager::instance().reloadAllChunks( false );
			}
			instance->updateLockData();
			dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/REGION_LOCKED" ) );
		}
	}

	Py_Return;
}
PY_MODULE_FUNCTION( projectLock, WorldEditor )

static PyObject* py_projectProgress( PyObject* args )
{
	Py_Return;
}
PY_MODULE_FUNCTION( projectProgress, WorldEditor )

/*~ function WorldEditor.invalidateSpaceMap
 *	@components{ worldeditor }
 *
 *	This function invalidates the space map, forcing a full recalculation.
 */
static PyObject* py_invalidateSpaceMap( PyObject* args )
{
	SpaceMap::instance().invalidateAllChunks();	
	Py_Return;
}
PY_MODULE_FUNCTION( invalidateSpaceMap, WorldEditor )

/*~ function WorldEditor.projectCommit
 *	@components{ worldeditor }
 *
 *	This function commits the changes to the locked chunks in the project 
 *	to the repository.
 *
 *	@param commitMsg	A commit message.
 *	@param keepLocks	An int to specify whether to keep (1) the project lock
 *						or unlock (0) the project after committing.
 */
static PyObject* py_projectCommit( PyObject* args )
{
	if( !WorldManager::instance().connection().connected() )
	{
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CANNOT_COMMIT" ) );
		Py_Return;
	}

	// parse arguments
	char* commitMsg;
	int keepLocks;
	if (!PyArg_ParseTuple( args, "si", &commitMsg, &keepLocks ))	{
		PyErr_SetString( PyExc_TypeError, "projectCommitChanges() "
			"expects a string and an int argument" );
		return NULL;
	}

	WorldManager::instance().quickSave();

	ProjectModule* instance = ProjectModule::currentInstance();
	if (instance)
	{
		CVSInfoDialog dlg(
			L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/COMMIT_CHANGES_TITLE" ) );

		dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/COMMITTING" ) );

		std::set<std::string> chunks = instance->lockedChunksFromSelection( false );

		std::set<std::string> filesToCommit;
		std::set<std::string> foldersToCommit;

		bool inCVS = true;
		if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
		{
			foldersToCommit = CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).addFolder( ProjectModule::currentSpaceDir(), commitMsg );
			inCVS = false;
		}

		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		if( !inCVS || !cvs.isInCVS( SPACE_SETTING_FILE_NAME ) )
		{
			cvs.addFile( SPACE_SETTING_FILE_NAME, false, false );
			cvs.addFile( "*.chunk", false, true );
			cvs.addFile( "*.cdata", true, true );
			filesToCommit.insert( SPACE_SETTING_FILE_NAME );
		}

		std::set<std::string> graphs = instance->graphFilesFromSelection();
		for( std::set<std::string>::iterator iter = graphs.begin();
			iter != graphs.end(); ++iter )
		{
			if( !inCVS || !cvs.isInCVS( *iter ) )
				cvs.addFile( *iter, false, false );
		}
		filesToCommit.insert( graphs.begin(), graphs.end() );

		std::set<std::string> vlos = instance->vloFilesFromSelection();
		for( std::set<std::string>::iterator iter = vlos.begin();
			iter != vlos.end(); ++iter )
		{
			if( !inCVS || !cvs.isInCVS( *iter ) )
			{
				if( strstr( iter->c_str(), ".odata" ) )
					cvs.addFile( *iter, true, false );
				else
					cvs.addFile( *iter, false, false );
			}
		}
		filesToCommit.insert( vlos.begin(), vlos.end() );

		for( std::set<std::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
		{
			filesToCommit.insert( *iter + ".chunk" );
			if( ( BWResource::fileExists( ProjectModule::currentSpaceDir() + '/' + *iter + ".cdata" ) ||
				cvs.isInCVS( *iter + ".cdata" ) ) )
				filesToCommit.insert( *iter + ".cdata" );
		}
		std::string path = ProjectModule::currentSpaceDir() + '/';
		for( std::set<std::string>::iterator iter = filesToCommit.begin(); iter != filesToCommit.end(); ++iter )
			*iter = path + *iter;

		if( CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).commitFiles( filesToCommit, foldersToCommit, commitMsg ) )
		{
			CVSWrapper( ProjectModule::currentSpaceDir(), &dlg ).refreshFolder( "" );

			for( std::set<std::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
				::DeleteFile( ( ProjectModule::currentSpaceAbsoluteDir() + '/' + *iter + ".~chunk~" ).c_str() );

			instance->commitDone();

			if (keepLocks)
				CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).editFiles( std::vector<std::string>( filesToCommit.begin(), filesToCommit.end() ) );
			else
				instance->discardLocks( commitMsg );

			EditorChunkCache::forwardReadOnlyMark();

			dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_COMMITED" ) );
		}
	}

	Py_Return;
}
PY_MODULE_FUNCTION( projectCommit, WorldEditor )

/*~ function WorldEditor.projectDiscard
 *	@components{ worldeditor }
 *
 *	This function discards changes made to the locked chunks while the project was locked.
 *
 *	@param commitMsg	A commit message.
 *	@param keepLocks	An int to specify whether to keep (1) the project lock
 *						or unlock (0) the project after discarding.
 */
static PyObject* py_projectDiscard( PyObject* args )
{
	if( !WorldManager::instance().connection().connected() )
		Py_Return;

	// parse arguments
	char* commitMsg;
	int keepLocks;
	if (!PyArg_ParseTuple( args, "si", &commitMsg, &keepLocks ))	{
		PyErr_SetString( PyExc_TypeError, "projectDiscardChanges() "
			"expects a string and an int argument" );
		return NULL;
	}

	CVSInfoDialog dlg(
		L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/DISCARD_LOCKS_TITLE" ) );

	ProjectModule* instance = ProjectModule::currentInstance();

	if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
	{
		dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_DISCARDED" ) );
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHUNK_LOCKS_DISCARDED" ) );
		if( !keepLocks && instance )
			instance->discardLocks( commitMsg );
		Py_Return;
	}

	if (instance)
	{
		std::set<std::string> chunks = instance->lockedChunksFromSelection( false );

		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		std::vector<std::string> filesToUpdate;
		for( std::set<std::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
		{
			filesToUpdate.push_back( *iter + ".chunk" );
			filesToUpdate.push_back( *iter + ".cdata" );
		}

		std::set<std::string> graphs = instance->graphFilesFromSelection();
		for( std::set<std::string>::iterator iter = graphs.begin();
			iter != graphs.end(); ++iter )
		{
			if( cvs.isInCVS( *iter ) )
				filesToUpdate.push_back( *iter );
		}

		std::set<std::string> vlos = instance->vloFilesFromSelection();
		for( std::set<std::string>::iterator iter = vlos.begin();
			iter != vlos.end(); ++iter )
		{
			if( cvs.isInCVS( *iter ) )
				filesToUpdate.push_back( *iter );
		}

		if( cvs.revertFiles( filesToUpdate ) )
		{
			for( std::set<std::string>::iterator iter = chunks.begin(); iter != chunks.end(); ++iter )
				::DeleteFile( ( ProjectModule::currentSpaceAbsoluteDir() + '/' + *iter + ".~chunk~" ).c_str() );

			if (keepLocks)
				cvs.editFiles( filesToUpdate );
			else
				instance->discardLocks( commitMsg );

			EditorChunkCache::forwardReadOnlyMark();

			// Clear the undo buffer
			UndoRedo::instance().clear();

			// Clear WorldEditors's lists of what's been changed
			WorldManager::instance().resetChangedLists();

			WorldManager::instance().reloadAllChunks( false );

			INFO_MSG( "Changes discarded\n" );
			dlg.add( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/CHANGES_DISCARDED" ) );
		}
	}

	Py_Return;
}
PY_MODULE_FUNCTION( projectDiscard, WorldEditor )

/*~ function WorldEditor.projectUpdateSpace
 *	@components{ worldeditor }
 *
 *	This function retrieves updates made to the space from the repository.
 */
static PyObject* py_projectUpdateSpace( PyObject* args )
{
	CVSInfoDialog dlg(
		L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/UPDATE_SPACE_TITLE" ) );

	ProjectModule* instance = ProjectModule::currentInstance();

	if( !CVSWrapper( ProjectModule::currentSpaceResDir(), &dlg ).isInCVS( ProjectModule::currentSpaceDir() ) )
	{
		INFO_MSG( "Space not under version control\n" );
		WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/SPACE_NOT_CONTROLLED" ) );
		Py_Return;
	}

	if (instance)
	{
		CVSWrapper cvs( ProjectModule::currentSpaceDir(), &dlg );

		if( cvs.updateFolder( "" ) )
		{
			// Clear WorldEditors's lists of what's been changed
			WorldManager::instance().resetChangedLists();

			WorldManager::instance().reloadAllChunks( false );

			INFO_MSG( "Space updated\n" );
			WorldManager::instance().addCommentaryMsg( L( "WORLDEDITOR/WORLDEDITOR/PROJECT/PROJECT_MODULE/SPACE_UPDATED" ) );
		}
	}


	Py_Return;
}
PY_MODULE_FUNCTION( projectUpdateSpace, WorldEditor )

/*~ function WorldEditor.projectMapAlpha
 *	@components{ worldeditor }
 *
 *	This function gets the current project map's alpha value if no alpha is passed in.
 *	If an alpha is passed as a parameter then the current project map's alpha
 *	will be set to that value.
 *
 *	@param alpha	Optional Float value. If this is provided then the current
 *					project's map alpha will be set to this value.
 *
 *	@return Returns the current project's map alpha if no alpha was provided, otherwise
 *			returns 0.
 */
static PyObject* py_projectMapAlpha( PyObject* args )
{
	ProjectModule *pinstance = ProjectModule::currentInstance();
    HeightModule  *hinstance = HeightModule ::currentInstance();
	if (pinstance != NULL || hinstance != NULL)
	{
		float alpha;
		if (PyArg_ParseTuple( args, "f", &alpha ))
		{
			if (pinstance != NULL)
                pinstance->projectMapAlpha(alpha);
            if (hinstance != NULL)
                hinstance->projectMapAlpha(alpha);
			return PyFloat_FromDouble( 0 );
		}
		else
		{
			PyErr_Clear();
            if (hinstance != NULL)
			    return PyFloat_FromDouble( hinstance->projectMapAlpha() );
            else
                return PyFloat_FromDouble( pinstance->projectMapAlpha() );
		}
	}

	return PyFloat_FromDouble( 0 );
}
PY_MODULE_FUNCTION( projectMapAlpha, WorldEditor )
