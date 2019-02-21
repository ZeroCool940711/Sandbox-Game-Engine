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
#include "minimap.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/mouse_cursor.hpp"
#include "cstdmf/debug.hpp"
#include "resmgr/auto_config.hpp"
#include "moo/render_context.hpp"
#include "moo/dynamic_vertex_buffer.hpp"
#include "moo/dynamic_index_buffer.hpp"
#include "romp/geometrics.hpp"

#pragma warning(disable: 4355)	// 'this' used in initialiser, for cell bounds holder

DECLARE_DEBUG_COMPONENT2( "Client", 0 )

#ifndef CODE_INLINE
#include "minimap.ipp"
#endif

PY_TYPEOBJECT( Minimap )


PY_BEGIN_METHODS( Minimap )
	/*~ function Minimap.add
	 *	Adds an entry to the minimap.  This function takes a MatrixProvider and
	 *	a GUI component.  The GUI component is rotated when its matrix rotates,
	 *	so these minimap icons can represent the facing of the matrix.	 
	 *	@param pMatrix MatrixProvider. Matrix Provider for the entry on the minimap.
	 *	@param component GUI Component.  A GUI Component to draw on the minimap.
	 */
	PY_METHOD( add )
	/*~ function Minimap.addSimple
	 *	Adds an entry to the minimap.  This function takes a MatrixProvider and
	 *	a colour.  The entry is represented via the minimap's simple entry map,
	 *	and all simple entries are drawn in a batch, i.e. with a single draw
	 *	call.  This is a much faster way to draw a lot of entries in the minimap
	 *	than by using the Minimap.add function.
	 *
	 *	@param MatrixProvider pMatrix. Matrix Provider for the entry on the minimap.
	 *	@param colour PyColour.  4-tuple (r,g,b,a) representing the colour to
	 *	display for the entry.
	 *	@return Integer A handle used to identify this entry on the minimap, which
	 *	can be used to remove the entry from the minimap.
	 */
	PY_METHOD( addSimple )
	/*~ function Minimap.remove
	 *	Removes an entry from the minimap.  You must pass the minimap handle
	 *	returned from either Minimap.add or Minimap.addSimple in order to remove
	 *	an entry from the minimap.	 
	 *
	 *	@param Intengeer Handle. Minimap Handle returned from add or addSimple.	 
	 */
	PY_METHOD( remove )
PY_END_METHODS()


PY_BEGIN_ATTRIBUTES( Minimap )
	/*~ attribute Minimap.viewpoint
	 *	Viewpoint from which the minimap sees the world.  It only
	 *	uses the position if rotate is set to False, or it uses both the
	 *	position and the forward vector if rotate is set to True.  If
	 *	no viewpoint is provided, the camera matrix is used instead.
	 *	@type MatrixProvider
	 */
	PY_ATTRIBUTE( viewpoint )
	/*~ attribute Minimap.range
	 *	Range in metres the minimap shows around the camera, or
	 *	the visible radius of the minimap.
	 *	@type Float
	 */
	PY_ATTRIBUTE( range )
	/*~ attribute Minimap.zoomSpeed
	 *	Time in seconds it takes for the minimap to zoom between
	 *	one range and another.
	 *	@type Float
	 */
	PY_ATTRIBUTE( zoomSpeed )
	/*~ attribute Minimap.maskName
	 *	Resource name of the texture that is used to mask out the
	 *	minimap.  The alpha in the mask texture is multiplied over
	 *	the map itself to stamp out an appropriate shaped border.
	 *	@type String
	 */
	PY_ATTRIBUTE( maskName )
	/*~ attribute Minimap.simpleEntryMap
	*	Resource name of the texture that is used to display
	*	simple entry, or, entries added with the addSimpleEntry
	*	function.
	*	Simple Entries are all drawn with a single
	*	texture map, and are batched for maximum drawing speed / 
	*	minimum number of draw calls.
	 *	@type String
	 */
	PY_ATTRIBUTE( simpleEntryMap )
	/*~ attribute Minimap.worldMapWidth
	 *	Width in metres the minimap texture map represents, for example,
	 *	this would be the width of the entire space, for an outdoor minimap.
	 *	@type Float
	 */
	PY_ATTRIBUTE( worldMapWidth )
	/*~ attribute Minimap.worldMapHeight
	 *	Height in metres the minimap texture map represents, for example,
	 *	this would be the depth of the entire space, for an outdoor minimap.
	 *	You may set this to a negative value; in some cases the texture map
	 *	may be upside-down.
	 *	@type Float
	 */
	PY_ATTRIBUTE( worldMapHeight )
	/*~ attribute Minimap.rotate
	 *	Whether the minimap should rotate so that the camera directio is always
	 *	facing up on the map.  False (default) means North is always up.  True
	 *	means the camera direction is always up.
	 *	@type Boolean
	 */
	PY_ATTRIBUTE( rotate )
	/*~ attribute Minimap.simpleEntrySize
	 *	The size in pixels that simple entries should be drawn.
	 *	Simple Entries are all drawn with a single
	 *	texture map, and are batched for maximum drawing speed / 
	 *	minimum number of draw calls.
	 *	This attribute is only used if pointSizeScaling == (0,0)
	 *	@type Float
	 */
	PY_ATTRIBUTE( simpleEntrySize )
	/*~ attribute Minimap.simpleEntriesVisible
	 *	Turn on or off the display of the simple entries.
	 *	@type Boolean
	 */	
	PY_ATTRIBUTE( simpleEntriesVisible )
	/*~ attribute Minimap.pointSizeScaling
	 *	Linear equation to determine the size of the point sprites used
	 *	to draw the simple entries.  The linear equation is of the form
	 *	y = a + bx, and this attribute is a Vector2 describing (a,b)
	 *	Set to (0,0) to disable scaling, and enable use of the simpleEntrySize
	 *	attribute.
	 *	@type Vector2
	 */	
	PY_ATTRIBUTE( pointSizeScaling )
	/*~ attribute Minimap.worldMapAnchor
	 *	World XZ position that represents the centre of the minimap texture
	 *	map.  For example many outdoor minimap maps would have a world map
	 *	anchor of (0,0) or perhaps (50,50) depending on the number of chunks
	 *	in the space.
	 *	@type Vector2
	 */
	PY_ATTRIBUTE( worldMapAnchor )
	/*~ attribute Minimap.mouseEntryPicking
	 *	The minimap can check whether the mouse is hovering over entries,
	 *	and give the minimap's gui script a callback when the selected
	 *	entry has changed. If you enable mouseEntryPicking, you will receive
	 *	one of two callback functions :
	 *	
	 *	@{
	 *	#Mouse is over a new entry
	 *	def onEntryFocus( self, handle )
	 *	#Mouse is no longer over an entry
	 *	def onEntryBlur( self, handle )
	 *	@}
	 */
	PY_ATTRIBUTE( mouseEntryPicking )
#if INCLUDE_CELL_BOUNDARY_VIZ
	/*~ attribute Minimap.cellBounds
	 *	Vector of floats, in groups of 4, representing all cell boundaries.
	 *	@type List of Floats
	 */
	PY_ATTRIBUTE( cellBounds )
	/*~ attribute Minimap.cellBoundsVisible
	 *	Turn on or off the display of the cell bounds
	 *	@type Boolean
	 */
	PY_ATTRIBUTE( cellBoundsVisible )
#endif
PY_END_ATTRIBUTES()

/*~ function GUI.Minimap
 *	Factory function to create and return a new Minimap object. Minimap is a
 *  GUI component that can display a scrolling map, with entries displayed
 *	on the map. 
 *	@return A new Minimap object.
 */
PY_FACTORY_NAMED( Minimap, "Minimap", GUI )

COMPONENT_FACTORY( Minimap )

static AutoConfigString s_mfmName( "system/minimapMaterial" );


// -----------------------------------------------------------------------------
// Section: Minimap
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Minimap::Minimap( PyTypePlus * pType )
:SimpleGUIComponent("", pType ), 
 range_( 500.f ),
 currentRange_( 500.f ),
 zoomSpeed_( 0.25f ),
 currHandle_( 1 ), 
 rotate_( true ),
 simpleEntrySize_( 10 ),
 simpleEntryMap_( NULL ),
 simpleEntriesVisible_( true ),
 worldMapAnchor_( 0.f, 0.f ),
 margin_( 0.05f ),
 viewpoint_( NULL ),
 mouseEntryPicking_( false ),
 pickedEntry_( 0 ),
 pointSizeScaling_( 0.f, 0.f ),
#if INCLUDE_CELL_BOUNDARY_VIZ
 cellBoundariesVB_( D3DPT_LINELIST ),
 cellBoundsHolder_( cellBounds_, this, true ),
 cellBoundsVisible_( true ),
#endif
 simpleEntriesVB_( D3DPT_POINTLIST )
{	
	material_ = NULL;	
	buildMaterial();
}


/**
 *	Destructor.
 */
Minimap::~Minimap()
{
	material_ = NULL;	
}


/**
 *	This method overrides SimpleGUIComponent's method and makes sure the
 *	mask texture is set into the second texture stage.
 *
 *	@returns	true if successful
 */
bool Minimap::buildMaterial( void )
{
	bool ret = false;

	if ( !material_ )
	{
		material_ = new Moo::EffectMaterial();		
		material_->load( BWResource::openSection( s_mfmName ) );		
	}

	if ( material_->pEffect() && material_->pEffect()->pEffect() )
	{
		ComObjectWrap<ID3DXEffect> pEffect = material_->pEffect()->pEffect();
		uint32 i = materialFX()-FX_ADD;
		D3DXHANDLE handle = pEffect->GetTechnique( i );
		material_->hTechnique( handle );
		ret = true;
	}
	else
	{
		ERROR_MSG(" Minimap::buildMaterial - material is invalid.");
	}

	return ret;
}


/**
 *	This method overrides SimpleGUIComponent::update, and moves all the minimap
 *	icons around and does all that stuff it needs.
 *
 *	@param	dTime			see SimpleGUIComponent::update
 *	@param	relParentWidth	see SimpleGUIComponent::update
 *	@param	relParentHeight	see SimpleGUIComponent::update
 */
void Minimap::update( float dTime, float relParentWidth, float relParentHeight )
{
	Matrix viewpoint;
	if (viewpoint_)
	{
		viewpoint_->matrix( viewpoint );
	}
	else
	{
		viewpoint = Moo::rc().invView();
	}

	Vector3 camPos = viewpoint.applyToOrigin();
	Vector3 fwd = viewpoint.applyToUnitAxisVector(2);
	float camYaw = atan2f( fwd.x, fwd.z );
	Matrix relRot;
	relRot.setRotateZ( camYaw );

	//because the gui is drawn in clip-space, but the screen is not usually square
	//we must take this into account when rotating, otherwise we end up accidentally
	//squashing the rotation matrix
	float w, h;
	SimpleGUI::instance().pixelRangesToClip( 1.f, 1.f, &w, &h );
	Matrix preScale, postScale;
	preScale.setScale( h/w, 1.f, 1.f );
	postScale.setScale( w/h, 1.f, 1.f );
	relRot.preMultiply( preScale );
	relRot.postMultiply( postScale );

	float rangeDiff = (range_ - currentRange_);
	float amount = Math::clamp( 0.01f, (dTime/zoomSpeed_), 1.f );
	currentRange_ += rangeDiff * amount;

	//move entities around.	
#if INCLUDE_CELL_BOUNDARY_VIZ
	if (cellBoundsVisible_)
	{
		this->updateCellBoundaries( camPos, relParentWidth, relParentHeight );
	}
#endif
	if (simpleEntriesVisible_)
	{
		this->updateSimpleEntries( camPos, relParentWidth, relParentHeight );
	}
	this->updateEntries( camPos, relParentWidth, relParentHeight );

	//calculate the map texture coordinates
	//the worldMapAnchor specifies where the center of the texture map is, in world coordinates.
	///The map coords are shader constants, representing (xOffset (uv), yOffset (uv), xScale (world -> uv), yScale (world -> uv)
	mapCoords_.set(
		(camPos.x - worldMapAnchor_.x) / worldMapWidth_,
		-(camPos.z - worldMapAnchor_.y) / worldMapHeight_,
		2.f*currentRange_/worldMapWidth_,
		2.f*currentRange_/worldMapHeight_ );	

	//Save this matrix, we will need it later, and SGC::update nulls it out.
	Matrix oldRunTimeTransform = this->runTimeTransform();

	SimpleGUIComponent::update( dTime, relParentWidth, relParentHeight );

	//calculate the runTimeTransform for ourselves, or the border if we are not spinning.
	if (this->rotate_)
	{		
		this->runTimeTransform( relRot );
	}
	else
	{
		SmartPointer<SimpleGUIComponent> spBorder = this->child( "border" );
		if ( spBorder )
		{
			spBorder->runTimeTransform( relRot );
		}
	}


	if (mouseEntryPicking_)
	{
		Vector2 topLeft, topRight, botLeft, botRight;
		this->clipBounds( topLeft, topRight, botLeft, botRight );

		//project using last world-view-projection transform
		const float &z = position_.z;
		Vector3 p[] = {
			oldRunTimeTransform.applyPoint( Vector3( topLeft.x, topLeft.y, z ) ),
			oldRunTimeTransform.applyPoint( Vector3( topRight.x, topRight.y, z ) ),
			oldRunTimeTransform.applyPoint( Vector3( botRight.x, botRight.y, z ) ),
			oldRunTimeTransform.applyPoint( Vector3( botLeft.x, botLeft.y, z ) ) };

		Vector3 center( (p[0] + p[1] + p[2] + p[3]) / 4.0 );
		Vector2 size( p[1].x - p[0].x, p[0].y - p[3].y );

		this->doMousePick(camPos, center, size);
	}
}


/**
 *	This method picks the entry underneath the mouse cursor,
 *	and calls onEntryFocus and/or onEntryBlur.
 *
 *	@param	camPos		camera position, in world coordinates.
 *	@param	center		center of minimap, in absolute clip coordinates.
 *	@param	size		size of minimap, in absolute clip coordinates.
 */
void Minimap::doMousePick(const Vector3& camPos, const Vector3& center, const Vector2& size)
{
	MouseCursor& mc = SimpleGUI::instance().mouseCursor();
	if (!mc.visible())
	{
		this->onEntryBlur();		
		return;
	}

	Vector2 mpos = mc.position();
	mpos.x -= center.x;
	mpos.y -= center.y;
	mpos.x /= (size.x / 2.f);
	mpos.y /= (size.y / 2.f);

	//TRACE_MSG( "mouse pos %0.2f, %0.2f, adjusted mouse pos %0.2f, %0.2f\n", mc.position().x, mc.position().y, mpos.x, mpos.y );

	if ( fabsf(mpos.x) >= (1.f - margin_) || (fabsf(mpos.y) >= (1.f - margin_)) )
	{
		this->onEntryBlur();
		return;
	}

	float best = FLT_MAX;
	MinimapHandle bestEntry = 0;

	SimpleEntryMap::iterator it = simpleEntries_.begin();
	SimpleEntryMap::iterator end = simpleEntries_.end();
	while (it != end)
	{
		SimpleEntry entry = it->second;
		float dist = distanceToEntry( mpos, camPos, entry.pMatrix_.getObject() );

		if (dist < best)
		{
			best = dist;
			bestEntry = it->first;
		}

		++it;
	}


	float entrySize = simpleEntrySize_ / Moo::rc().screenWidth();
	entrySize /= (size.x / 2.0f);

	//TRACE_MSG( "best %0.2f, entry size %0.2f\n", best, entrySize );

	if ( best > entrySize )
	{
		bestEntry = 0;
	}

	if (bestEntry != pickedEntry_)
	{
		this->onEntryBlur();
		pickedEntry_ = bestEntry;
		this->onEntryFocus();
	}
}


/** 
 *	This method returns the distance to the given entry in clip coordinates, given a
 *	mouse pos relative to the minimap, and a camera position in world coordinates.
 *
 *	@param	mpos		normalised, relative mouse position
 *	@param	cpos		world space camera position
 *	@param	pMatrix		matrix provider giving world position of the entry.
 *
 *	@return	float		distance in normalised, relative coordinates of the
 *						mouse cursor from the entry.
 */
float Minimap::distanceToEntry( const Vector2& mpos, const Vector3& cpos, MatrixProvider* pMatrix )
{
	if (pMatrix)
	{			
		Matrix p;
		pMatrix->matrix(p);
		Vector3 pos = p.applyToOrigin() - cpos;
		pos.y = pos.z;
		pos.z = 0.f;
		pos /= currentRange_;

		float dist = pos.length();
		if (dist < (1.f-margin_))
		{
			pos.x -= mpos.x;
			pos.y -= mpos.y;
			return pos.length();
		}
	}

	return FLT_MAX;
}


/**
 *	This method is called when the currently picked entry is no longer picked.
 *	Call into our script and let them know.
 *	pickedEntry_ will be set to 0 by this function.
 */
void Minimap::onEntryBlur()
{
	if (pickedEntry_ != 0)
	{
		if (this->pScriptObject_.hasObject())
		{
			PyObject * args = PyTuple_New( 1 );
			PyTuple_SetItem( args, 0, PyInt_FromLong(pickedEntry_) );

			bool ret = Script::call( 
				PyObject_GetAttrString( pScriptObject_.getObject(), "onEntryBlur" ), 
				args, "Minimap::onEntryBlur", true );			
		}

		pickedEntry_ = 0;
	}
}


/**
 *	This method is called when a new entry is picked.
 *	Call into our script and let them know.
 *	Assumes pickedEntry_ is set.
 */
void Minimap::onEntryFocus()
{
	if (pickedEntry_ != 0)
	{
		if (this->pScriptObject_.hasObject())
		{				
			PyObject * args = PyTuple_New( 1 );
			PyTuple_SetItem( args, 0, PyInt_FromLong(pickedEntry_) );

			bool ret = Script::call( 
				PyObject_GetAttrString( pScriptObject_.getObject(), "onEntryFocus" ), 
				args, "Minimap::onEntryFocus", true );
		}
	}
}


/**
 *	This method updates all the simple entries (that do not
 *	have SimpleGUIComponents)
 *	It builds a vertex vector containing the clip coordinates
 *	of all of the simple entries.
 *
 *	@param	camPos			position of the camera
 *	@param	relParentWidth	see SimpleGUIComponent::update
 *	@param	relParentHeight	see SimpleGUIComponent::update
 */
void Minimap::updateSimpleEntries( const Vector3& camPos, float relParentWidth, float relParentHeight )
{
	float modX =  this->widthInClip( relParentWidth ) / 2.f;
	float modY =  this->heightInClip( relParentHeight ) / 2.f;

	bool first = false;	

	simpleEntriesVB_.clear();
	simpleEntriesVB_.reserve( simpleEntries_.size() );

	SimpleEntryMap::iterator it = simpleEntries_.begin();
	SimpleEntryMap::iterator end = simpleEntries_.end();
	while (it != end)
	{
		SimpleEntry entry = it->second;

		MatrixProviderPtr pMatrix = entry.pMatrix_;		
		if (pMatrix)
		{
			MinimapVertex mv;
			Matrix p;
			pMatrix->matrix(p);
			mv.pos_ = Vector3(p.applyToOrigin() - camPos);
			mv.pos_.y = mv.pos_.z;
			mv.pos_.z = 0.f;
			mv.pos_ /= currentRange_;

			float dist = mv.pos_.length();
			//TODO : configurable margin here
			if (dist < (1.f-margin_))
			{
				mv.uv_.set( 0.f, 0.f );
				mv.pos_.x *= modX;
				mv.pos_.y *= modY;
				mv.pos_.z = 0.5f;
				mv.colour_ = entry.colour_;				
				simpleEntriesVB_.push_back(mv);
			}
		}

		it++;
	}
}


/**
 *	This method updates all the entries that have SimpleGUIComponents.
 *	It moves them relative to the camera, and turns them on/off as
 *	they go into or out of range.
 *
 *	@param	camPos			position of the camera
 *	@param	relParentWidth	see SimpleGUIComponent::update
 *	@param	relParentHeight	see SimpleGUIComponent::update
 */
void Minimap::updateEntries( const Vector3& camPos, float relParentWidth, float relParentHeight )
{
	float modX =  this->widthInClip( relParentWidth ) / 2.f;
	float modY =  this->heightInClip( relParentHeight ) / 2.f;

	//because the gui is drawn in clip-space, but the screen is not usually square
	//we must take this into account when rotating, otherwise we end up accidentally
	//squashing the rotation matrix
	float w, h;
	SimpleGUI::instance().pixelRangesToClip( 1.f, 1.f, &w, &h );
	Matrix preScale, postScale;
	preScale.setScale( h/w, 1.f, 1.f );
	postScale.setScale( w/h, 1.f, 1.f );

	EntryMap::iterator it = entries_.begin();
	EntryMap::iterator end = entries_.end();
	while ( it != end )
	{
		EntryInfo& ei = it->second;

		MatrixProviderPtr pMatrix = ei.pMatrix_;
		Matrix m;		
		SimpleGUIComponentPtr pComponent = ei.c_;		

		Vector3 relPos;

		if ( pMatrix )
		{
			pMatrix->matrix(m);

			relPos = Vector3( m.applyToOrigin() - camPos );
			relPos.y = relPos.z;
			relPos.z = 0.f;
		
			float worldLength = relPos.length();
			//TODO : configurable margin here
			if ( worldLength <= (currentRange_ * (1.f-margin_)) )
			{
				pComponent->visible( true );

				//normalise relative coordinates (so goes from -1 .. +1)
				relPos /= currentRange_;
				relPos.z = 0.f;
				relPos.x *= modX;
				relPos.y *= modY;
				ei.m_.setTranslate( relPos );

				Matrix relRot;
				relRot.setRotateZ( -m.yaw() );
				relRot.preMultiply( preScale );
				relRot.postMultiply( postScale );
				ei.m_.preMultiply( relRot );
			}
			else
			{
				pComponent->visible( false );
			}
		}
		else
		{
			pComponent->visible( false );			
		}

		it++;
	}
}

#if INCLUDE_CELL_BOUNDARY_VIZ
void Minimap::updateCellBoundaries( const Vector3& camPos, float relParentWidth, float relParentHeight )
{
	static bool s_watched = false;
	static bool s_debugMinimap = false;
	if (!s_watched)
	{
		s_watched = true;
		MF_WATCH( "debug Minimap", s_debugMinimap );
	}

	float modX =  this->widthInClip( relParentWidth ) / 2.f;
	float modY =  this->heightInClip( relParentHeight ) / 2.f;
	const float CELL_MAX_VALUE = 100000.f;

	//bounding box of minimap to clip the lines to.
	Vector3 bbmin( -currentRange_, -5.f, -currentRange_ );
	Vector3 bbmax( +currentRange_, +5.f, +currentRange_ );
	BoundingBox bounds( bbmin, bbmax );

	if (s_debugMinimap)
	{
		DEBUG_MSG( "Clipping Bounds min : %0.2f, %0.2f, %0.2f \tmax: %0.2f, %0.2f, %0.2f\n", bbmin.x, bbmin.y, bbmin.z, bbmax.x, bbmax.y, bbmax.z );
	}
	
	std::vector< Vector4 >	cellBoundaries;	
	for (size_t i=3; i<cellBounds_.size(); i+=4)
	{
		float a = Math::clamp( -CELL_MAX_VALUE, cellBounds_[i-3], CELL_MAX_VALUE );
		float b = Math::clamp( -CELL_MAX_VALUE, cellBounds_[i-2], CELL_MAX_VALUE );
		float c = Math::clamp( -CELL_MAX_VALUE, cellBounds_[i-1], CELL_MAX_VALUE );
		float d = Math::clamp( -CELL_MAX_VALUE, cellBounds_[i], CELL_MAX_VALUE );

		cellBoundaries.push_back( Vector4(a,b,c,d) );
	}

	cellBoundariesVB_.clear();
	
	for (size_t i=0; i<cellBoundaries.size(); i++)
	{
		Vector2 bl = Vector2( cellBoundaries[i].x, cellBoundaries[i].y );
		Vector2 tr = Vector2( cellBoundaries[i].z, cellBoundaries[i].w );

		MinimapVertex mv;
		mv.colour_ = 0x80808080;

		for (uint8 j=0; j<4; j++)
		{
			Vector3 lineA, lineB;

			switch (j)
			{
			case 0:
				lineA.set( bl.x - camPos.x, 0.f, bl.y - camPos.z );
				lineB.set( tr.x - camPos.x, 0.f, bl.y - camPos.z );
				break;
			case 1:
				lineA.set( bl.x - camPos.x, 0.f, bl.y - camPos.z );
				lineB.set( bl.x - camPos.x, 0.f, tr.y - camPos.z );
				break;
			case 2:
				lineA.set( bl.x - camPos.x, 0.f, tr.y - camPos.z );
				lineB.set( tr.x - camPos.x, 0.f, tr.y - camPos.z );
				break;
			case 3:
				lineA.set( tr.x - camPos.x, 0.f, bl.y - camPos.z );
				lineB.set( tr.x - camPos.x, 0.f, tr.y - camPos.z );
				break;
			}						

			if (s_debugMinimap)
			{
				DEBUG_MSG( "Line %d %d : A : %0.2f, %0.2f, %0.2f \tB : %0.2f, %0.2f, %0.2f\t", i, j, lineA.x, lineA.y, lineA.z, lineB.x, lineB.y, lineB.z );
			}

			if (bounds.clip( lineA, lineB ))
			{
				if (s_debugMinimap)
				{
					DEBUG_MSG( "Clipped! : A : %0.2f, %0.2f, %0.2f \tB : %0.2f, %0.2f, %0.2f\n", lineA.x, lineA.y, lineA.z, lineB.x, lineB.y, lineB.z );
				}

				//line 1, endpoint 1
				mv.pos_.x = lineA.x;
				mv.pos_.y = lineA.z;
				mv.pos_.z = 0.f;
				mv.pos_ /= currentRange_;
				mv.uv_.set( mv.pos_.x/2.f + 0.5f, mv.pos_.y/2.f + 0.5f );
				mv.pos_.x *= modX;
				mv.pos_.y *= modY;
				cellBoundariesVB_.push_back(mv);

				//line 1, endpoint 2
				mv.pos_.x = lineB.x;
				mv.pos_.y = lineB.z;
				mv.pos_.z = 0.f;
				mv.pos_ /= currentRange_;
				mv.uv_.set( mv.pos_.x/2.f + 0.5f, mv.pos_.y/2.f + 0.5f );
				mv.pos_.x *= modX;
				mv.pos_.y *= modY;
				cellBoundariesVB_.push_back(mv);
			}
			else
			{
				if (s_debugMinimap)
				{
					DEBUG_MSG( "Fully outside area\n" );
				}
			}
		}
	}
}
#endif


/**
 *	This method implements a standard draw of self and
 *	is used internally and by derived classes.
 *
 *	@param	overlay	Boolean representing whether we are drawing as a GUI overlay
 *	across the screen (the default), and False if we are drawing in the 3D scene.
 */
void Minimap::drawSelf( bool overlay )
{
	if( momentarilyInvisible() )
		return;

	if( !(vertices_ && nVertices_) )
		return;

	SimpleGUI::instance().setConstants(runTimeColour(), pixelSnap());
	material_->pEffect()->pEffect()->SetInt( "filterType", filterType() );
	material_->pEffect()->pEffect()->SetVector( "mapCoords", &mapCoords_ );
	if ( material_->begin() ) 
	{
		bool valid = true;
		for ( uint32 i=0; i<material_->nPasses() && valid; i++ )
		{
			material_->beginPass(i);				
			Moo::rc().setTexture( 0, texture_ ? texture_->pTexture() : NULL );
			Moo::rc().setTexture( 1, mask_ ? mask_->pTexture() : NULL );			
			if (tiled_)
			{
				Moo::rc().setSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);
				Moo::rc().setSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
			}

			if ( !overlay )
			{
				Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
				Moo::rc().setRenderState( D3DRS_ZWRITEENABLE, FALSE );
				Moo::rc().setRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
				Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_LESS );
			}


			uint32 vertexBase = 0, lockIndex = 0;
			bool indexed = nIndices_ && indices_;

			//DynamicVertexBuffer
			Moo::DynamicVertexBufferBase2<GUIVertex>& vb = Moo::DynamicVertexBufferBase2< GUIVertex >::instance();
			if ( vb.lockAndLoad( vertices_, nVertices_, vertexBase ) &&
				 SUCCEEDED(vb.set( 0 )) )
			{
				if ( indexed )
				{
					//DynamicIndexBuffer
					Moo::DynamicIndexBufferBase& dynamicIndexBuffer = Moo::rc().dynamicIndexBufferInterface().get( D3DFMT_INDEX16 );
					Moo::IndicesReference ind = dynamicIndexBuffer.lock2( nIndices_ );
					if ( ind.valid() )
					{
						ind.fill( indices_, nIndices_ );
						dynamicIndexBuffer.unlock();

						valid = SUCCEEDED(dynamicIndexBuffer.indexBuffer().set());
						lockIndex = dynamicIndexBuffer.lockIndex();
					}
				}

				if (valid)
				{
					SimpleGUI::instance().countDrawCall();
					if ( indexed )
					{
						Moo::rc().drawIndexedPrimitive(D3DPT_TRIANGLELIST, vertexBase, 0, nVertices_, lockIndex, nIndices_/3); 
					}
					else
					{
						Moo::rc().drawPrimitive(D3DPT_TRIANGLELIST, vertexBase, nVertices_/3);
					}
				}
			}
			material_->endPass();
		}

		material_->end();
	}


	//Draw the Simple Entities
	if (simpleEntriesVisible_ || cellBoundsVisible_)
	{
		Geometrics::setVertexColour( 0xffffffff );	
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
		Moo::rc().setTexture( 1, NULL );
	}

	if (simpleEntriesVisible_)
	{
		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );			
		Moo::rc().setRenderState( D3DRS_POINTSPRITEENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_POINTSCALEENABLE,  FALSE );	
		if ( almostZero(pointSizeScaling_.x) && almostZero(pointSizeScaling_.y) )
		{
			Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&simpleEntrySize_) );
		}
		else
		{
			float pSize = pointSizeScaling_.x + pointSizeScaling_.y * currentRange_;
			Moo::rc().setRenderState( D3DRS_POINTSIZE, *((DWORD*)&pSize) );
		}
		Moo::rc().setTexture( 0, simpleEntryMap_.hasObject() ? simpleEntryMap_->pTexture() : NULL );
		simpleEntriesVB_.draw();
		SimpleGUI::instance().countDrawCall();
		Moo::rc().setRenderState( D3DRS_POINTSPRITEENABLE, FALSE );
	}

#if INCLUDE_CELL_BOUNDARY_VIZ
	//Draw the Cell Boundaries
	if (cellBoundsVisible_)
	{
		Moo::rc().setTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		Moo::rc().setTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
		Moo::rc().setRenderState( D3DRS_ANTIALIASEDLINEENABLE, TRUE );
		Moo::rc().setTexture( 0, mask_ ? mask_->pTexture() : NULL );
		cellBoundariesVB_.draw();
		SimpleGUI::instance().countDrawCall();
		Moo::rc().setRenderState( D3DRS_ANTIALIASEDLINEENABLE, FALSE );
	}
#endif

	//Go through all our entries and set their run time transforms up.
	//We can't do this at tick time because SimpleGUIComponent::update
	//resets the runTimeTransform.
	EntryMap::iterator it = entries_.begin();
	EntryMap::iterator end = entries_.end();
	while ( it != end )
	{
		EntryInfo& ei = it->second;
		ei.c_->runTimeTransform( ei.m_ );
		++it;
	}
}


/**
 *	This method adds a minimap entry, given an Entry ID and a colour.
 *	This method uses a simple and optimal method for drawing the Entry
 *	on the map.
 *	@param	pMatrix		MatrixProvider to use for position, rotation information.
 *	@param	colour		colour to draw with.
 *	@return	MinimapHandle	Handle that used to remove the entry later on.
 */
MinimapHandle Minimap::addSimple( MatrixProviderPtr pMatrix, const Vector4& colour )
{
	MinimapHandle h = this->nextHandle();
	SimpleEntry se( pMatrix, colour );
	simpleEntries_.insert( std::make_pair(h,se) );
	return h;
}


/**
 *	This method adds a minimap entry, given an Entry ID and a gui component.
 *	
 *	@param	EntryID	Entry to use for position, rotation information.
 *	@param	pComponent	GUI component to draw with.
 *	@return	MinimapHandle	Handle that used to remove the entry later on.
 */
MinimapHandle Minimap::add( MatrixProviderPtr pMatrix, SimpleGUIComponentPtr pComponent )
{
	MinimapHandle h = this->nextHandle();

	if (pComponent.hasObject())
	{
		EntryInfo ei( pMatrix, pComponent );
		entries_.insert( std::make_pair(h, ei) );

		char buf[256];
		sprintf( buf, "hc%d\0", h );
		this->addChild( buf, &*pComponent );

	}

	return h;
}


/**
 *	This method removes a minimap entry, given a minimap handle
 *	
 *	@param	h			MinimapHandle returned earlier via add or addSimple. 
 */
void Minimap::remove( MinimapHandle h )
{
	EntryMap::iterator it = entries_.find( h );

	if ( it != entries_.end() )
	{
		entries_.erase( it );
		char buf[256];
		sprintf( buf, "hc%d\0", h );
		this->removeChild( buf );
	}
	else
	{
		SimpleEntryMap::iterator it = simpleEntries_.find( h );
		if (it != simpleEntries_.end())
		{
			simpleEntries_.erase(it);
		}
	}
}


/**
 *	This method is a private method that gets a new minimap handle.
 *	
 *	@return	MinimapHandle	A new unique minimap handle.
 */
MinimapHandle Minimap::nextHandle()
{
	//This will wrap if there are 2^32 minimap entries.  NFL
	currHandle_++;
	return currHandle_;
}


/**
 *	This method sets the mask name for the component.
 *	@param	name	resource ID for a texture map that will be used to mask
 *	out the minimap texture map.
 */
void Minimap::maskName( const std::string& name )
{
	if ( maskName() != name )
	{
		mask_ = Moo::TextureManager::instance()->get( name );		
	}
}


/**
 *	This method sets the simple Entry map name for the component.
 *	@param	name	resource ID for a texture map that will be used to draw
 *	entities added with the addSimple function. 
 */
void Minimap::simpleEntryMap( const std::string& name )
{
	if ( simpleEntryMap() != name )
	{
		simpleEntryMap_ = Moo::TextureManager::instance()->get( name );		
	}
}


/**
 *	This method overrides SimpleGUIComponent::load.  It loads the minimap
 *	specific data from a GUI datasection.
 *	@param	pSect		data section containing the data to load from.
 *	@para	bindings	see SimpleGUIComponent::load
 *	@return	bool		success or failure
 */
bool Minimap::load( DataSectionPtr pSect, LoadBindings & bindings )
{
	bool ret = SimpleGUIComponent::load( pSect, bindings );
	this->maskName( pSect->readString( "maskName", this->maskName() ));
	this->simpleEntryMap( pSect->readString( "simpleEntryMap", this->simpleEntryMap() ));
	this->simpleEntrySize_ = pSect->readFloat( "simpleEntrySize", this->simpleEntrySize_ );
	this->worldMapWidth_ = pSect->readFloat( "worldMapWidth", this->worldMapWidth_ );
	this->range_ = pSect->readFloat( "range", this->range_ );
	this->worldMapHeight_ = pSect->readFloat( "worldMapHeight", this->worldMapHeight_ );
	this->rotate_ = pSect->readBool( "rotate", this->rotate_ );
	this->zoomSpeed_ = pSect->readFloat( "zoomSpeed", this->zoomSpeed_ );
	return ret;
}


/**
 *	This method overrides SimpleGUIComponent::save.  It saves the minimap
 *	specific data to a GUI datasection.
 *	@param	pSect		data section containing the data to save to.
 *	@para	bindings	see SimpleGUIComponent::save 
 */
void Minimap::save( DataSectionPtr pSect, SaveBindings & bindings )
{
	SimpleGUIComponent::save( pSect, bindings );
	pSect->writeString( "maskName", this->maskName() );
	pSect->writeString( "simpleEntryMap", this->simpleEntryMap() );
	pSect->writeFloat( "worldMapWidth", this->worldMapWidth_ );
	pSect->writeFloat( "range", this->range_ );
	pSect->writeFloat( "worldMapHeight", this->worldMapHeight_ );
	pSect->writeFloat( "simpleEntrySize", this->simpleEntrySize_ );
	pSect->writeBool( "rotate", this->rotate_ );
	pSect->writeFloat( "zoomSpeed", this->zoomSpeed_ );
}



/// Get an attribute for python
PyObject * Minimap::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	
	return SimpleGUIComponent::pyGetAttribute( attr );
}


/// Set an attribute for python
int Minimap::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return SimpleGUIComponent::pySetAttribute( attr, value );
}

/**
 *	Static python factory method
 */
PyObject * Minimap::pyNew( PyObject * args )
{
	return new Minimap;
}
