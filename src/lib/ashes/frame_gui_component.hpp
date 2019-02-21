/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef FRAME_GUI_COMPONENT_HPP
#define FRAME_GUI_COMPONENT_HPP

#include "simple_gui_component.hpp"


/*~ class GUI.FrameGUIComponent
 *
 *	This class inherits from SimpleGUIComponent.  In addition to the basic
 *  (tiled) image displayed by the SimpleGUIComponent, it adds extra images on
 *	 top to form a frame around the image.
 *
 *	It uses two extra textures.  The first is the corner texture, which is
 *  split into four quarters, which are rendered in the corresponding corners
 *  of the overall component.  The width and height of the corners is based on
 *  the tiledWidth and tiledHeight attributes of the component.
 *
 *	The second extra texture is the edge texture.  This is assumed to be
 *  oriented correctly to display along the bottom edge of the
 *  FrameGUIComponent.  It is then mirrored to display along the top, and
 *  rotated to display along either side edge.
 *
 *	The height of the top and bottom edges is specified by the tiledHeight
 *  attribute, and the texture is tiled along the top and bottom using the
 *  tiledWidth attribute. 
 *	
 *	The width of the side edges is specified by the tiledWidth attribute, and
 *  the texture is tiled long the side edges using the tiledHeight attribute.
 *
 *	The background image is always displayed tiled, using the tiledWidth and
 *  tiledHeight attributes.  It is rendered across the entire surface of the
 *  FrameGUIComponent, with the corners and edges rendered on top of it.
 *
 *	A new FrameGUIComponent is created using GUI.Frame function.
 */
/**
 * FrameGUIComponent is a SimpleGUIComponent.
 * The only addition in the frame GUI component, is that it
 * updates the texture coordinates of the base mesh to tile
 * the background frame texture.
 */
class FrameGUIComponent : public SimpleGUIComponent
{
	Py_Header( FrameGUIComponent, SimpleGUIComponent )

public:
	FrameGUIComponent( const std::string& backgroundTextureName,
					const std::string& frameTextureName,
					const std::string& edgeTextureName,
					int tileWidth, int tileHeight,
					PyTypePlus * pType = &s_type_ );
	~FrameGUIComponent();

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		std::string, edgeTextureName, edgeTextureName )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE(
		std::string, cornerTextureName, cornerTextureName )

	PY_FACTORY_DECLARE()

	void		update( float dTime, float relParentWidth, float relParentHeight );
	void		applyShaders( float dTime );
	void		applyShader( GUIShader& shader, float dTime );
	void		draw( bool overlay = true );

	const std::string & edgeTextureName() const;
	void		edgeTextureName( const std::string & name );

	const std::string & cornerTextureName() const;
	void		cornerTextureName( const std::string & name );

private:
	FrameGUIComponent(const FrameGUIComponent&);
	FrameGUIComponent& operator=(const FrameGUIComponent&);

	SimpleGUIComponent* corners_[4];
	SimpleGUIComponent* edges_[4];

	virtual bool		load( DataSectionPtr pSect, LoadBindings & bindings );
	virtual void		save( DataSectionPtr pSect, SaveBindings & bindings );

	COMPONENT_FACTORY_DECLARE( FrameGUIComponent("","","",16,16) )
};

#ifdef CODE_INLINE
#include "frame_gui_component.ipp"
#endif


#endif // FRAME_GUI_COMPONENT_HPP
