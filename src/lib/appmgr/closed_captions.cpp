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
#include "closed_captions.hpp"
#include "ashes/simple_gui.hpp"
#include "ashes/text_gui_component.hpp"
#include <strstream>

#ifndef CODE_INLINE
#include "closed_captions.ipp"
#endif


namespace
{
	SimpleMutex s_pendingMessagesMutex;
} // anonymous namespace


// -----------------------------------------------------------------------------
// Section: Construction/Destruction
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
ClosedCaptions::ClosedCaptions( int bufferSize, PyTypePlus * pType ) :
	PyObjectPlus( pType ),
	head_( 0 ),
	timeout_( 6.f ),
	fadeout_( 2.f ),
	clipHeight_( 0.05f )
{
	msgs_.resize( bufferSize );

	margin_.x = 0.05f;
	margin_.y = 0.05f;

	root_ = new SimpleGUIComponent( "" );
	root_->visible( false );
	root_->width( 0.f );
	root_->height( 0.f );
	SimpleGUI::instance().addSimpleComponent( *root_ );
	font_ = FontManager::instance().get( "default_small.font" );

	for ( int i = 0; i < bufferSize; i++ )
	{
		msgs_[i].age_ = timeout_;
		msgs_[i].component_ = new TextGUIComponent( font_ );
		msgs_[i].backing_ = new SimpleGUIComponent( "resources/maps/gui/background.dds" );

		//set up backing component properties
		SimpleGUIComponent * b = msgs_[i].backing_;
		b->position( Vector3( -1.f + margin_.x, -1.f + margin_.y, 1.f ) );
		b->widthMode( SimpleGUIComponent::SIZE_MODE_CLIP ); 
		b->heightMode( SimpleGUIComponent::SIZE_MODE_CLIP ); 
		b->width( 2.f - 2*margin_.x );
		b->height( clipHeight_ );
		b->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
				SimpleGUIComponent::ANCHOR_V_BOTTOM );
		b->colour( 0x40000020 );
		b->materialFX( SimpleGUIComponent::FX_BLEND );
		b->visible( false );
		b->tiled( true );
		b->tileWidth( 16 );
		b->tileHeight( 16 );

		std::ostringstream childNameStream;
		childNameStream << "CaptionBackground_" << i;
		root_->addChild( childNameStream.str(), b );

		//set up text component properties
		TextGUIComponent * c = msgs_[i].component_;

		c->filterType( SimpleGUIComponent::FT_LINEAR );
		c->slimLabel( "***" );
		c->visible( false );
		c->colour( 0xffffffff );
		c->anchor( SimpleGUIComponent::ANCHOR_H_LEFT,
				SimpleGUIComponent::ANCHOR_V_BOTTOM );

		std::ostringstream captionNameStream;
		captionNameStream << "CaptionText_" << i;
		b->addChild( captionNameStream.str(), c );
	}
}


/**
 *	Destructor.
 */
ClosedCaptions::~ClosedCaptions()
{
	for ( uint i = 0; i < msgs_.size(); i++ )
	{
		TextGUIComponent * c = msgs_[i].component_;
		root_->removeChild( c );
		Py_DECREF( c );
		msgs_[i].component_ = NULL;

		SimpleGUIComponent * b = msgs_[i].backing_;
		root_->removeChild( b );
		Py_DECREF( b );
		msgs_[i].backing_ = NULL;
	}

	SimpleGUI::instance().removeSimpleComponent( *root_ );

	Py_DecRef(root_);
}


/**
 *	This method is called by the global commentary system, when
 *	a commentary msg was added.
 *
 *	Note that we push the message onto a pending message queue.
 *	This is because we may have messages added when the windowing
 *	system doesn't know how large an area it has to draw in.
 *	If this is the case, then we can't successfully add a message
 *	because we don't know whether or not to clip it.
 *
 *
 *	@param msg	the new commentary message
 *	@param id	the id of the type of commentary message
 */
void ClosedCaptions::onAddMsg( const std::string & msg, int id )
{
	SimpleMutexHolder mutex( s_pendingMessagesMutex );

	pendingMessages_.push_back( PendingMessage( msg, id ) );
}


/**
 *	This method adds all pending messages to the gui system.
 */
void ClosedCaptions::parseEventQueue()
{
	SimpleMutexHolder mutex( s_pendingMessagesMutex );

	PendingMessages::iterator it = pendingMessages_.begin();
	PendingMessages::iterator end = pendingMessages_.end();

	while ( it != end )
	{
		PendingMessage& pm = *it++;

		addMsg( pm.msg_, pm.id_ );
	}

	pendingMessages_.clear();
}


/**
 *	This method adds a message to the gui system.
 */
void ClosedCaptions::addMsg( const std::string & msg, int id )
{
	TextGUIComponent * c = msgs_[head_].component_;
	SimpleGUIComponent * b = msgs_[head_].backing_;

	float w,h;
	SimpleGUI::instance().clipRangesToPixel( b->width(), b->height(), &w, &h );

	if ( !font_ )
		return;

	if ( font_->metrics().maxWidth_ <= 0 )
		return;

	int maxCharacters = (int) (w / font_->metrics().maxWidth_);

	if ( maxCharacters < 0 )
		return;

	if ( msg.length() == 0 )
		return;

	bool clipped = false;

	if ( ((int) msg.length()) > maxCharacters )
		clipped = true;

	c->visible( true );
	b->visible( true );
	c->slimLabel( clipped ? msg.substr(0,maxCharacters) : msg );
	msgs_[head_].age_ = timeout_;

	switch ( id )
	{
	case Commentary::COMMENT:
		c->colour( 0xffffffff );
		break;
	case Commentary::CRITICAL:
		c->colour( 0xffff0000 );
		break;
	case Commentary::ERROR_LEVEL:
		c->colour( 0xffff0000 );
		break;
	case Commentary::WARNING:
		c->colour( 0xffff8000 );
		break;
	case Commentary::HACK:
		c->colour( 0xff4040ff );
		break;
	default:
		c->colour( 0xffffffff );
	}

	head_++;
	head_ %= msgs_.size();

	if ( clipped )
	{
		addMsg( msg.substr( maxCharacters, msg.length() ), id );
	}
}


/**
 *	This method updates our messages.
 *
 *	@param dTime the change in time since the last frame.
 */
void ClosedCaptions::update( float dTime )
{
	if ( !SimpleGUI::instance().hasResolutionChanged() )
		this->parseEventQueue();

	int idx = head_;

	//update all ages
	for ( uint i = 0; i < msgs_.size(); i++ )
	{
		Caption & c = msgs_[ idx ];

		if ( c.age_ > 0.f )
		{
			c.age_ -= dTime;

			if ( c.age_ <= 0.f )
			{
				c.component_->visible( false );
				c.backing_->visible( false );
				c.age_ = timeout_;
			}
			else 
			{
				unsigned int alpha = 255;

				if ( c.age_ < fadeout_ )
				{
					alpha = (unsigned int)( ( c.age_ / fadeout_ ) * 255.f );
				}

				alpha <<= 24;

				unsigned int col = c.component_->colour() & 0x00ffffff;
				c.component_->colour( col | alpha );

				alpha >>= 24;
				alpha /= 4;
				alpha <<= 24;
				unsigned int bcol = c.backing_->colour() & 0x00ffffff;
				c.backing_->colour( bcol | alpha );
			}
		}

		idx++;
		idx %= msgs_.size();
	}

	//Now go through and lay them out
	idx = head_;

	for ( uint i = 0; i < msgs_.size(); i++ )
	{
		idx--;
		if ( idx < 0 )
			idx = msgs_.size() - 1;

		TextGUIComponent & c = *msgs_[ idx ].component_;
		c.position( Vector3( -1.f + margin_.x, -1.f + margin_.y + ( (float)i * clipHeight_ ), 0.9f ) );

		SimpleGUIComponent & b = *msgs_[ idx ].backing_;
		b.position( Vector3( -1.f + margin_.x, -1.f + margin_.y + ( (float)i * clipHeight_ ), 1.f ) );
	}
}

// -----------------------------------------------------------------------------
// Section: Python stuff
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( ClosedCaptions )

PY_BEGIN_METHODS( ClosedCaptions )
	PY_METHOD( update )
	PY_METHOD( addAsView )
	PY_METHOD( delAsView )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ClosedCaptions )
	PY_ATTRIBUTE( visible )
PY_END_ATTRIBUTES()

PY_FACTORY( ClosedCaptions, GUI )


/**
 *	Get an attribute for python
 */
PyObject * ClosedCaptions::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return PyObjectPlus::pyGetAttribute( attr );
}

/**
 *	Set an attribute for python
 */
int ClosedCaptions::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return PyObjectPlus::pySetAttribute( attr, value );
}

/**
 *	This method gets the captions to update
 */
PyObject * ClosedCaptions::py_update( PyObject * args )
{
	float dtime;
	if (!PyArg_ParseTuple( args, "f", &dtime ))
	{
		PyErr_SetString( PyExc_TypeError, "ClosedCaptions.update: "
			"Argument parsing error: Expected float dtime" );
		return NULL;
	}

	this->update( dtime );

	Py_Return;
}

/**
 *	Put this one in the central mi...er.. commentary position
 */
PyObject * ClosedCaptions::py_addAsView( PyObject * args )
{
	Commentary::instance().addView( this );
	Py_Return;
}

/**
 *	Now it's walking a view fine line
 */
PyObject * ClosedCaptions::py_delAsView( PyObject * args )
{
	Commentary::instance().delView( this );
	Py_Return;
}


/**
 *	Factory method
 */
PyObject * ClosedCaptions::pyNew( PyObject * args )
{
	int nlines = 5;
	if (!PyArg_ParseTuple( args, "|i", &nlines ))
	{
		PyErr_SetString( PyExc_TypeError, "ClosedCaptions() "
			"expects an optional integer number of lines to display" );
		return NULL;
	}

	return new ClosedCaptions( nlines );
}

// closed_captions.cpp
