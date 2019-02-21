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

#include "latency_gui_component.hpp"

#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include "moo/render_context.hpp"
#include "cstdmf/debug.hpp"
#include "romp/geometrics.hpp"
#include "romp/font.hpp"
#include "common/servconn.hpp"

#include "connection_control.hpp"



DECLARE_DEBUG_COMPONENT2( "2DComponents", 0 )

#ifndef CODE_INLINE
#include "latency_gui_component.ipp"
#endif



PY_TYPEOBJECT( LatencyGUIComponent )

PY_BEGIN_METHODS( LatencyGUIComponent )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( LatencyGUIComponent )
PY_END_ATTRIBUTES()

/*~ function GUI.Latency
 *
 *	This function creates a new LatencyGUIComponent.  This will display the
 *  word "Offline" on the screen if the client is not connected to the server,
 *  and nothing otherwise.
 *
 *	@return		a new LatencyGUIComponent.
 */
PY_FACTORY_NAMED( LatencyGUIComponent, "Latency", GUI )



LatencyGUIComponent::LatencyGUIComponent( PyTypePlus* pType )
 	: SimpleGUIComponent( "" )
{
	BW_GUARD;
	this->widthMode( SimpleGUIComponent::SIZE_MODE_LEGACY );
	this->heightMode( SimpleGUIComponent::SIZE_MODE_LEGACY );

	this->position(Vector3(-0.85f, -0.85f, 0.0f));
	this->width(1.7f);
	this->height(0.01f);
	this->anchor( SimpleGUIComponent::ANCHOR_H_LEFT, SimpleGUIComponent::ANCHOR_V_BOTTOM );

	FontPtr tf = FontManager::instance().get( "default_medium.font" );	
	//tf->scale( 0.75f, 0.75f );

	txt_ = new TextGUIComponent(tf);
	txt_->label(L"Offline");
	txt_->colour(0x808080ff);
	txt_->anchor(SimpleGUIComponent::ANCHOR_H_LEFT, SimpleGUIComponent::ANCHOR_V_BOTTOM);
	txt_->position(Vector3(-0.85f, -0.85f, 0));
	this->addChild( "label", txt_ );
}


LatencyGUIComponent::~LatencyGUIComponent()
{
	BW_GUARD;
	Py_DECREF(txt_);
}



/// Get an attribute for python
PyObject* LatencyGUIComponent::pyGetAttribute( const char* attr )
{
	BW_GUARD;
	PY_GETATTR_STD();

	return SimpleGUIComponent::pyGetAttribute( attr );
}


/// Set an attribute for python
int LatencyGUIComponent::pySetAttribute( const char* attr, PyObject* value )
{
	BW_GUARD;
	PY_SETATTR_STD();

	return SimpleGUIComponent::pySetAttribute( attr, value );
}


/**
 *	Static python factory method
 */
PyObject* LatencyGUIComponent::pyNew( PyObject * args )
{
	BW_GUARD;
	return new LatencyGUIComponent();
}




/**
 *	This method draws the meter
 */
void LatencyGUIComponent::draw( bool overlay )
{
	BW_GUARD;
	if (!SimpleGUIComponent::visible())
		return;

	std::vector<int>::iterator it = childOrder_.begin();
	std::vector<int>::iterator end = childOrder_.end();
	while (it != end)
	{
		(children_.begin() + *it)->second->draw( overlay );
		it++;
	}

	ServerConnection& server =
		*ConnectionControl::serverConnection();

	if (server.online())
	{

		txt_->visible(false);
	}
	else
	{
		txt_->visible(true);
	}

	Moo::rc().setVertexShader( NULL );
	Moo::rc().setFVF( GUIVertex::fvf() );
}





std::ostream& operator<<(std::ostream& o, const LatencyGUIComponent& t)
{
	BW_GUARD;
	o << "LatencyGUIComponent\n";
	return o;
}


/*latency_gui_component.cpp*/
