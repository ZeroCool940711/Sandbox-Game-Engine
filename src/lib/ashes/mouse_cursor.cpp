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

#include "mouse_cursor.hpp"
#include "simple_gui.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#include "resmgr/auto_config.hpp"
#include "romp/py_texture_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "MouseCursor", 0 );

namespace { // anonymous

// Helper functions
DX::Surface * getTextureSurface( Moo::BaseTexturePtr texture );

// Named constants
// Note: changes made to the AutoConfig parameters below must be also made in
// bigworld/src/tools/res_packer/packer_helper.cpp.
const AutoConfigString s_cursorDefFile( 
		"gui/cursorsDefinitions", 
		"gui/mouse_cursors.xml" );

} // namespace anonymous

// -----------------------------------------------------------------------------
// section: MouseCursor
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( MouseCursor )

PY_BEGIN_METHODS( MouseCursor )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MouseCursor )
	/*~ attribute MouseCursor.position
	 *
	 *	Current position of the mouse cursor (in clip space). 
	 *	Read and write property.
	 *
	 *	@type	Vector2
	 */
	PY_ATTRIBUTE( position )
	/*~ attribute MouseCursor.shape
	 *
	 *	Currently active cursor shape. Read and write property. 
	 *
	 *	Possible values are defined in the mouse cursor definition file.
	 *
	 *	@type	str
	 */
	PY_ATTRIBUTE( shape )
	/*~ attribute MouseCursor.visible
	 *
	 *	Mouse cursor current visibility status. Read and write property.
	 *
	 *	@type	boolean
	 */
	PY_ATTRIBUTE( visible )
	/*~ attribute MouseCursor.dynamic
	 *
	 *	Mouse cursor current dynamic flag value. Read and write property.
	 *
	 *	@type	boolean
	 */
	PY_ATTRIBUTE( dynamic )
	/*~ attribute MouseCursor.active
	 *
	 *	Mouse cursor current activity status. Will be true if the 
	 *	mouse cursor is the current active cursor (see BigWorld.setCursor).
	 *	Read and write property.
	 *
	 *	@type	boolean
	 */
	PY_ATTRIBUTE( active )
	/*~ attribute MouseCursor.texture
	 *
	 *	Current texture used by the mouse cursor if any, wrapped in a 
	 *	PyTextureProvider. Read and write property.
	 
	 *	Will only return a valid texture if the current cursor was set 
	 *	using the texture attribute. Otherwise, returns an empty 
	 *	PyTextureProvider.
	 *
	 *	When using the texture attribute to set the cursos shape, the
	 *	current hotSpot will be preserved. You may reset it to the desired
	 *	new value, either before or after setting the texture, using the
	 *	hotSpot property (see BigWorld.hotSpot).
	 *
	 *	See below an example of how to use the texture property to 
	 *	dynamically generate a mouse cursor from a model in the game:
	 *
	 *	render = BigWorld.PyModelRenderer(32,32)
	 *	model  = BigWorld.Model('sets/items/grail.model')
	 *	model.zoomExtents(True)
	 *	render.models = [model]
	 *	render.render()
	 *	import GUI
	 *	mouse = GUI.mcursor()
	 *	mouse.hotSpot = (0, 0)
	 *	mouse.visible = True
	 *	mouse.texture = render.texture
	 *	BigWorld.setCursor(mouse)
	 *
	 *	@type	PyTextureProvider
	 */
	PY_ATTRIBUTE( texture )
	/*~ attribute MouseCursor.active
	 *
	 *	Mouse cursor's current hot-spot. Read and write property.
	 *
	 *	@type	Vector2
	 */
	PY_ATTRIBUTE( hotSpot )
PY_END_ATTRIBUTES()


/**
 *	Constructor.
 */
MouseCursor::MouseCursor( PyTypePlus * pType ) :
	InputCursor( pType ),
	inactivePos_( 0, 0 ),
	isActive_( false ),
	isVisible_( false ),
	isDirty_( false ),
	isDynamic_( false ),
	cursorDefs_(),
	curCursor_(),
	curTexture_(NULL),
	curProvider_(NULL),
	curHotSpot_(0, 0)
{
	this->loadCursorsDefinitions();
	curCursor_ = cursorDefs_.begin();
}


/**
 *	Destructor.
 */
MouseCursor::~MouseCursor()
{
	this->curTexture_ = NULL;
}


/**
 *	Returns mouse cursor's current position.
 */
Vector2 MouseCursor::position() const
{
	if (!this->isActive_)
	{
		return this->inactivePos_;
	}
	
	// Get the mouse cursor position
	POINT pt;
	::GetCursorPos( &pt );

	// If we are running in windowed mode, get the position in the 
	// client area of the window
	// If we are in fullscreen mode, use the position relative to the 
	// top left corner of the window, this is done as occasionally the 
	// runtime gets a bit confused when running in fullscreen mode and
	// thinks the fullscreen window has a border
	if (Moo::rc().windowed())
	{
		::ScreenToClient( (HWND)SimpleGUI::instance().hwnd(), &pt );
	}
	else
	{
		RECT windowRect;
		if (GetWindowRect((HWND)SimpleGUI::instance().hwnd(), &windowRect))
		{
			pt.x -= windowRect.left;
			pt.y -= windowRect.top;
		}
	}
	
	Vector2 resolutionOverride = SimpleGUI::instance().resolutionOverride();
	if (resolutionOverride.lengthSquared() > 0)
	{
		// Work out where the equivalent position is in the override resolution.
		float nx = ((float)pt.x) / Moo::rc().screenWidth();
		float ny = ((float)pt.y) / Moo::rc().screenHeight();

		pt.x = (LONG)(resolutionOverride.x*nx);
		pt.y = (LONG)(resolutionOverride.y*ny);
	}

	float x, y;
	SimpleGUI::instance().pixelRangesToClip( float(pt.x), float(pt.y), &x, &y );
		
	return Vector2( x - 1.0f, 1.0f - y );
}

/**
 *	Sets a new position for the mouse cursor.
 */
void MouseCursor::position( const Vector2 & pos )
{
	float x, y;
	SimpleGUI::instance().clipRangesToPixel( 
		float(pos.x + 1.0f), 
		float(1.0f - pos.y), 
		&x, &y );

	POINT pt;
	Vector2 resolutionOverride = SimpleGUI::instance().resolutionOverride();
	if (resolutionOverride.lengthSquared() > 0)
	{
		// Work out where the equivalent position is in real-screen coordinates.
		float nx = ((float)x) / resolutionOverride.x;
		float ny = ((float)y) / resolutionOverride.y;

		pt.x = (LONG)(Moo::rc().screenWidth()*nx);
		pt.y = (LONG)(Moo::rc().screenHeight()*ny);
	}
	else
	{
		pt.x = LONG( x );
		pt.y = LONG( y );
	}
	

	// If we are running in windowed mode, use the position relative to the 
	// client area of the window
	// If we are in fullscreen mode, use the position relative to the 
	// top left corner of the window, this is done as occasionally the 
	// runtime gets a bit confused when running in fullscreen mode and
	// thinks the fullscreen window has a border
	if (Moo::rc().windowed())
	{
		::ClientToScreen( (HWND)SimpleGUI::instance().hwnd(), &pt );
	}
	else
	{
		RECT windowRect;
		if (GetWindowRect((HWND)SimpleGUI::instance().hwnd(), &windowRect))
		{
			pt.x += windowRect.left;
			pt.y += windowRect.top;
		}

	}
	::SetCursorPos( pt.x, pt.y );
	
	if (Moo::rc().device()) 
	{
		Moo::rc().device()->SetCursorPosition( pt.x, pt.y, 0 );
	}
	
	this->inactivePos_ = pos;
}

/**
 *	Returns current mouse cursor shape.
 */
const std::string & MouseCursor::shape() const
{
	if (curCursor_ != this->cursorDefs_.end())
	{
		return curCursor_->first;
	}
	else 
	{
		static std::string emptyStr( "" );
		return emptyStr;
	}
}


/**
 *	Sets a new shape for the mouse cursor.
 *
 *	@param	cursorName	name of the cursor to use (as defined in mouse_cursors.xml)
 */
void MouseCursor::shape( const std::string & cursorName )
{
	CursorDefMap::const_iterator cursor = this->cursorDefs_.find( cursorName );
	if (this->setShape( cursor ))
	{
		this->curCursor_   = cursor;
		this->curTexture_  = NULL;
		this->curProvider_ = NULL;
		this->visible( this->isVisible_ );
	}
	else
	{
		PyErr_Format( 
			PyExc_ValueError,
			"Could not change cursor shape: %s", cursorName.c_str() );
	}
}

/** 
 *	Retrieves the texture being used as pointer 
 *	(only if set from a TextureProvider).
 */
PyObject * MouseCursor::pyGet_texture()
{
	return this->curProvider_.exists() ? this->curProvider_.getObject() : Py_None;
}

/**
 *	Set texture to be used as mouse pointer from 
 *	the given TextureProvider.
 *
 *	@param	value	the TextureProvider to be used as mouse pointer.
 */
int MouseCursor::pySet_texture( PyObject * value )
{
	PyTextureProviderPtr textureProvider;
	if (Script::setData(value, textureProvider, "MouseCursor.texture") != 0)
	{
		return -1;
	}
	else
	{
		if (textureProvider.exists())
		{
			if (!this->setTexture(textureProvider, this->curHotSpot_))
			{
				PyErr_Format( 
					PyExc_ValueError,
					"Could not get texture from given TextureProvider");
					
				return -1;
			}

			this->curCursor_   = this->cursorDefs_.end();
			this->visible( this->isVisible_ );
		}
	}
	return 0;
}


/**
 *	Returns current visibility status of the mouse cursor.
 */
bool MouseCursor::visible() const
{
	return this->isVisible_;
}


/**
 *	Sets the visibility status of the mouse cursor.
 */
void MouseCursor::visible( bool state )
{
	if ( Moo::rc().device() )
	{
		Moo::rc().device()->ShowCursor( state );
		if (state) {
			// increase cursor visibility 
			// counter until cursor is visible
			while (::ShowCursor( TRUE ) <	0) {}
		}
		else {
			// decrease cursor visibility 
			// counter until cursor is invisible
			while (::ShowCursor( FALSE ) > -1) {}
		}
		this->isVisible_ = state;
	}
}


/**
 *	Retrieves the current dynamicity status of the mouse cursor.
 */
bool MouseCursor::dynamic() const
{
	return this->isDynamic_;
}


/**
 *	Sets the dynamicity status of the mouse cursor (only meaningful if
 *	a dynamic TextureProvider is being used as mouse pointer. Setting it 
 *	to true will force the mouse pointer texture to be updated every frame.
 */
void MouseCursor::dynamic( bool dynamic )
{
	this->isDynamic_ = dynamic;
}

/**
 *	Returns current mouse's hot-spot position.
 */
const Vector2 & MouseCursor::hotSpot() const
{
	return this->curHotSpot_;
}

/**
 *	Sets mouse's hot-spot position.
 *
 *	@param	pos		new hot-spot position.
 */
void MouseCursor::hotSpot( const Vector2 & pos )
{
	this->curHotSpot_ = pos;
	this->refresh();
}

/**
 *	Handle key events. The mouse cursor does nothing with them.
 */
bool MouseCursor::handleKeyEvent( const KeyEvent & /*event*/ )
{ 
	return false;
}

/**
 *	Handle mouse events. The mouse cursor does nothing with them.
 */
bool MouseCursor::handleMouseEvent( const MouseEvent & /*event*/ )
{
	if (Moo::rc().device()) 
	{
		POINT pt;
		::GetCursorPos( &pt );
		Moo::rc().device()->SetCursorPosition( pt.x, pt.y, 0 );
	}
	return false;
}

/**
 *	Handle axis events. The mouse cursor does nothing with them.
 */
bool MouseCursor::handleAxisEvent( const AxisEvent & /*event*/ )
{ 
	return false;
}

/**
 *	Ticks the mouse cursor.
 *
 *	@param	dTime	time elapsed in between frames.
 */
void MouseCursor::tick( float dTime )
{
	if (this->isDirty_)
	{
		this->refresh();
	}
	else if (this->curProvider_.exists() && this->isDynamic_)
	{
		if (!this->setTexture(this->curProvider_, this->curHotSpot_ ))
		{
			this->curProvider_ = NULL;
		}
	}
	else if (this->isActive_ &&
			 this->curCursor_ != this->cursorDefs_.end() &&
			 this->curCursor_->second.texture->isAnimated())
	{
		// Updating cursor if it's an animated texture
		this->setTexture(
			this->curCursor_->second.texture->pTexture(), this->curHotSpot_ );
	}
}

/**
 *	Informs the MouseCursor class about changes in the 
 *	application focus state.
 *
 *	@param	focus	new application focus state.
 */
void MouseCursor::focus( bool focus )
{
	this->isDirty_ = focus;
}

/**
 *	Returns true if MouseCursor requires refreshing.
 */
bool MouseCursor::isDirty() const
{
	return this->isDirty_;
}

/**
 *	Refreshes the MouseCursor, recreating the pointer object.
 */	
void MouseCursor::refresh()
{
	//Only execute if we are using this cursor
	if (this->isActive_ == false)
	{
		return;
	}

	if (this->curCursor_ != this->cursorDefs_.end())
	{
		this->setShape( this->curCursor_ );
	}
	else if (this->curTexture_.hasComObject())
	{
		this->setTexture(this->curTexture_.pComObject(), this->curHotSpot_);
	}
	this->visible( this->isVisible_ );
	this->isDirty_ = false;
}

/**
 *	Activates the mouse cursor. This method is called automaticaly
 *	by App when the scripts sets the mouse cursor as the active cursor.
 *	Normally, there should be no need to call it directly.
 */
void MouseCursor::activate()
{
	this->position( this->inactivePos_ );
	this->isActive_ = true;
}

/**
 *	Activates the mouse cursor. This method is called automaticaly
 *	by App when the scripts unsets the mouse cursor as the active 
 *	cursor. Normally, there should be no need to call it directly.
 */
void MouseCursor::deactivate()
{
	this->inactivePos_ = this->position();
	this->isActive_ = false;
}

/**
 *	Returns mouse cursor's current activity status 
 */
bool MouseCursor::isActive() const 
{ 
	return this->isActive_;
}

/**
 *	Loads cursor definitions based on the information in the 
 *	cursor defition xml file. The name of the cursor definition xml 
 *	resource file can be set in the resources.xml file, under the field
 *	gui/cursorsDefinitions field. If defaults to "gui/mouse_cursors.xml"
 */
void MouseCursor::loadCursorsDefinitions()
{
	DataSectionPtr pTop = BWResource::openSection( s_cursorDefFile );
	this->cursorDefs_.erase( this->cursorDefs_.begin(), this->cursorDefs_.end() );
	if (pTop)
	{
		for (int i=0; i<pTop->countChildren(); ++i) 
		{
			CursorDefinition cursorDef;
			DataSectionPtr pSect = pTop->openChild( i );
			std::string name     = pSect->sectionName();
			std::string fileName = pSect->readString( "texture" );
			cursorDef.hotSpot    = pSect->readVector2( "hotSpot", Vector2( 0, 0 ) );
			Moo::TextureManager::instance()->setFormat(fileName, D3DFMT_A8R8G8B8);
			cursorDef.texture = Moo::TextureManager::instance()->get(fileName);
			if (cursorDef.texture->format() != D3DFMT_A8R8G8B8)
			{
				ERROR_MSG(
					"Cursor texture maps need to be of D3DFMT_A8R8G8B8 format to work.\n"
					"Make sure the DDS file is saved as D3DFMT_A8R8G8B8 or that the\n"
					"original TGA is available for conversion to D3DFMT_A8R8G8B8.");
			}

			this->cursorDefs_.insert( std::make_pair( name, cursorDef ) );
		}
	}
}

/**
 * Sets shape of mouse cursor.
 *
 * @param	curIt iterator to entry into the cursor definitions map.
 *
 * @return	true if cursor was successfully set. False on error.
 */
bool MouseCursor::setShape( const CursorDefMap::const_iterator & curIt )
{
	if ( curIt != this->cursorDefs_.end() )
	{
		this->curHotSpot_ = curIt->second.hotSpot;
		if (this->setTexture(curIt->second.texture->pTexture(), this->curHotSpot_))
		{
			return true;
		}

		ERROR_MSG( 
				"MouseCursor::setShape: "
				"Could not set cursor properties: %s\n", 
				curIt->first.c_str() );
	}
	return false;
}

/**
 *	Sets MouseCursor pointer from a TextureProvider.
 *
 *	@param	provider	the TextureProvider to use for the pointer texture.
 *	@param	hotSpot		the hot-spot position for this cursor shape.
 *	@param	heldTexture	(out) will hold a snapshot of the texture provider
 *						(useful for refreshing).
 *
 *	@return				true on success, false on error.
 */
bool MouseCursor::setTexture(PyTextureProviderPtr provider, const Vector2 & hotSpot )
{
	Moo::BaseTexturePtr srcTexture = provider->texture();
	
	if (!srcTexture.exists())
	{
		ERROR_MSG("MouseCursor.texture: Texture provider is empty.");		
		return false;
	}
	
	if (srcTexture->format() != D3DFMT_A8R8G8B8)
	{
		ERROR_MSG("MouseCursor.texture: "
			"Cursor texture maps need to be of D3DFMT_A8R8G8B8 format to work.\n"
			"Make sure the DDS file is saved as D3DFMT_A8R8G8B8 or that the\n"
			"original TGA is available for conversion to D3DFMT_A8R8G8B8.");
		
		return false;
	}

	bool			res			= false;
	DX::Surface*	srcSurface	= getTextureSurface(srcTexture);
	D3DSURFACE_DESC srcSurfaceDesc;
	
	if ( srcSurface && SUCCEEDED( srcSurface->GetDesc( &srcSurfaceDesc )) )
	{
		if ( srcSurfaceDesc.Usage & D3DUSAGE_RENDERTARGET )
		{
			// copy out of src render target to a suitable temporary texture
			ComObjectWrap<DX::Texture> dstTexture;
			dstTexture = Moo::rc().createTexture(
				srcTexture->width(), srcTexture->height(), 1, 0, 
				D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM );

			if ( dstTexture )
			{
				DX::Surface * dstSurface = NULL;
				HRESULT hr = dstTexture->GetSurfaceLevel( 0, &dstSurface );
				
				if ( SUCCEEDED( hr ) )
				{	
					hr = Moo::rc().device()->GetRenderTargetData( srcSurface, 
																	dstSurface );
					dstSurface->Release();
					
					if ( SUCCEEDED( hr ) )
					{
						res = this->setTexture( dstTexture.pComObject(), hotSpot );
					}
				}
			}
		}
		else
		{
			// use texture as is
			res = this->setTexture( srcTexture->pTexture(), hotSpot );
		}

		srcSurface->Release();
	}

	if ( res )
	{
		this->curProvider_ = provider;
	}

	return res;
}

/**
 *	Sets MouseCursor pointer from a texture.
 *
 *	@param	texture		the texture to use for the pointer texture.
 *	@param	hotSpot		the hot-spot position for this cursor shape.
 *
 *	@return				true on success, false on error.
 */
bool MouseCursor::setTexture( DX::BaseTexture * texture, const Vector2 & hotSpot)
{
	HRESULT hr = -1;

	DX::Surface * surface = NULL;
	D3DRESOURCETYPE resType = texture->GetType();
	if ( resType == D3DRTYPE_TEXTURE )
	{
		DX::Texture* srcTexture = (DX::Texture*)texture;
		hr = srcTexture->GetSurfaceLevel( 0, &surface );
	}
	
	if (SUCCEEDED(hr))
	{	
		hr = Moo::rc().device()->SetCursorProperties( 
			UINT(hotSpot.x), UINT(hotSpot.y), surface );
		surface->Release();

		if ( SUCCEEDED( hr ) )
		{
			this->curTexture_ = texture;
			return true;
		}
		else
		{
			ERROR_MSG( 
					"MouseCursor::setTexture: "
					"Could not set cursor texture (HRESULT: %d)\n", hr);
		}
	}
	return false;
}

/**
 *	Callback triggered by Moo when the device is reset. Flag cursor as dirty.
 */
void MouseCursor::createUnmanagedObjects()
{
	this->isDirty_ = true;
}

	
/**
 *	Callback triggered by Moo when the device is recreated. Flag cursor as dirty.
 */
void MouseCursor::createManagedObjects()
{
	this->isDirty_ = true;
}


/**
 *	Get an attribute for python
 */
PyObject * MouseCursor::pyGetAttribute( const char * attr )
{
	// try our normal attributes
	PY_GETATTR_STD();

	// ask our base class
	return PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Set an attribute for python
 */
int MouseCursor::pySetAttribute( const char * attr, PyObject * value )
{
	// try our normal attributes
	PY_SETATTR_STD();

	// ask our base class
	return PyObjectPlus::pySetAttribute( attr, value );
}

/*~ function GUI.mcursor
 *
 *	This function returns the MouseCursor object. 
 *
 *	Use it, in conjunction with BigWorld.setCursor function to activate the 
 *	mouse cursor. The mouse cursor object should be activated whenever is 
 *	desirable to have the user interacting with GUI components by using the 
 *	mouse. No mouse events will be propagated to the components if the mouse 
 *	cursor is not active. To activate the mouse cursor, use:
 *
 *	@{
 *		BigWorld.setCursor( GUI.mcursor() )
 *	@}
 *
 *	Also, use it to access and modify all mouse cursor properties, like 
 *	position, visible, shape and active:
 *	
 *	@{
 *		mouseStatus = GUI.mcursor().active
 *		mousePosition = GUI.mcursor().position
 *		GUI.mcursor().visible = True
 *		GUI.mcursor().shape = 'arrow'
 *	@}
 */
/**
 * Returns the mouse cursor singleton as a PyObject.
 */
PyObject * MouseCursor::py_mcursor( PyObject * args )
{
	MouseCursor * pMC = &(SimpleGUI::instance().mouseCursor());
	Py_INCREF( pMC );
	return pMC;
}
PY_MODULE_STATIC_METHOD( MouseCursor, mcursor, GUI )


// Helper functions
namespace { // anonymous

/**
 * Return level 0 surface from given texture.
 *
 * @param	texture		texture from where to extract surface.
 * @return	Pointer to surface on success. NULL on error.
 */
DX::Surface * getTextureSurface( Moo::BaseTexturePtr texture )
{
	DX::Surface * surface = NULL;
	if ( texture.getObject() != NULL && texture->pTexture() != NULL )
	{
		DX::BaseTexture * baseTexture = texture->pTexture();
		D3DRESOURCETYPE resType = baseTexture->GetType();
		if ( resType == D3DRTYPE_TEXTURE )
		{
			DX::Texture* tSrc = (DX::Texture*)baseTexture;
			tSrc->GetSurfaceLevel( 0, &surface );
		}
	}
	else 
	{
		ERROR_MSG( "getTextureSurface: Null or invalid texture object" );
	}
	return surface;
}

} // namespace anonymous

// mouse_cursor.cpp
