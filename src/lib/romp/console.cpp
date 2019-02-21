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

#pragma warning (disable:4503)

#include "console.hpp"
#include "romp/console_manager.hpp"
#include "romp/histogram_provider.hpp"

#include "cstdmf/config.hpp"
#include "cstdmf/dogwatch.hpp"
#include "cstdmf/watcher.hpp"

#include "math/colour.hpp"

#include "pyscript/script.hpp"
#include "pyscript/py_output_writer.hpp"

#include "ashes/simple_gui.hpp"

#ifndef CODE_INLINE
#include "console.ipp"
#endif

DECLARE_DEBUG_COMPONENT2( "UI", 0 )


namespace { // anonymous

// Named constants
const int			AUTO_INDEX_RESET	= 0x7FFFFFFF;
const std::string	SEPARATORS			= " ~!%^&*+-={}\\|;:'\",/<>?";

// Helper types
typedef std::vector<std::string> StringVector;
typedef std::pair< std::string, std::string > StringPair;

// Helper functions
StringPair splitHeadAndExpression( const std::string & line );
StringPair splitObjAndAttr( const std::string & expression );
StringVector getAttributes( const std::string & object );

StringVector filterAttributes(
	const StringVector & attributes,
	const std::string  & startsWith );

std::string buildCompletedLine(
	const std::string & head,
	const std::string & object,
	const std::string & attribute );

std::string getCommonPrefix(
	const StringVector & filtered,
	const std::string  & knownStart );

void printAttributes(
	XConsole           & console,
	const StringVector & attributes );

} // namespace anonymous


// -----------------------------------------------------------------------------
// Section: StatisticsConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
StatisticsConsole::StatisticsConsole( StatisticsConsole::Handler * pHandler ) :
	pHandler_( pHandler )
{
}


/**
 *	This method initialises this object.
 */
bool StatisticsConsole::init()
{
	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( "Statistics Console\n" );
	}

	return succeeded;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void StatisticsConsole::update()
{
	this->clear();

	DogWatchManager::iterator	selection = this->followPath();
	selection.flags( selection.flags() | 4 );

	pHandler_->displayStatistics( *this );

	selection.flags( selection.flags() & ~4 );
}


/**
 *	This method overrides the InputHandler method.
 */
bool StatisticsConsole::handleKeyEvent( const KeyEvent & event )
{
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}

	bool handled = true;

	DogWatchManager::iterator iter = this->followPath();

	switch (event.key())
	{
	case KeyEvent::KEY_JOY3:
	case KeyEvent::KEY_RETURN:
		if (event.isAltDown())
		{
			handled = false;
			break;
		}

		if (iter.begin() != iter.end())
		{
			iter.flags( iter.flags() ^ 1 );
		}
		break;

	case KeyEvent::KEY_G:
	case KeyEvent::KEY_JOY2:
		iter.flags( iter.flags() ^ 2 );
		break;

	case KeyEvent::KEY_RBRACKET:
	case KeyEvent::KEY_PGDN:
	case KeyEvent::KEY_JOY1:
	{
		// ok, we want to move down.
		int psize = path_.size();

		// are we visible and we have children?
		if ((iter.flags() & 1) && iter.begin() != iter.end())
		{
			path_.push_back( 0 );
			break;
		}

		// ok, add one to the last path element until we succeed then
		bool movedDown = false;

		while (!movedDown && psize > 0)
		{
			path_.back()++;
			iter = this->followPath();
			movedDown = path_.size() == psize;
			psize = path_.size();
		}

		break;
	}

	case KeyEvent::KEY_LBRACKET:
	case KeyEvent::KEY_PGUP:
	case KeyEvent::KEY_JOY0:
	{
		// we're moving up in the world...
		if (path_.size() > 0)
		{
			// are we at the start of our group?
			if (path_.back() == 0)
			{		// yep, go up a level, and that's it
				path_.erase( path_.end() - 1 );
				break;
			}

			// nope, so take a step back
			path_.back()--;
			iter = this->followPath();
		}

		// and follow down the chain of last children
		while ((iter.flags() & 1) && iter.begin() != iter.end())
		{
			int	idx = 0;

			// find the last child
			DogWatchManager::iterator post = iter.begin();
			DogWatchManager::iterator end = iter.end();
			do
			{
				iter = post;
				++post;
				idx++;
			}
			while (post != end);

			// and repeat!
			path_.push_back( idx - 1 );
			// (iter already set)
		}

		break;
	}

	case KeyEvent::KEY_NUMPADMINUS:
		this->scrollUp();
		break;

	case KeyEvent::KEY_ADD:
		this->scrollDown();
		break;

	case KeyEvent::KEY_HOME:
		this->scrollOffset( 0 );
		break;

	default:
		handled = false;
		break;
	}

	return handled;
}


/**
 *	This method turns path_ into an iterator.
 */
DogWatchManager::iterator StatisticsConsole::followPath()
{
	DogWatchManager::iterator	iter = DogWatchManager::instance().begin();

	for (uint i = 0; i < path_.size(); i++)
	{
		DogWatchManager::iterator sub = iter.begin();

		// iterate until we get to path_[i] down
		for (int tidx = path_[i]; tidx > 0 && sub != iter.end(); tidx--)
		{
			++sub;
		}

		// did we run out of elements (not sure if this can happen)
		if (sub == iter.end())
		{
			path_.erase( path_.begin() + i, path_.end() );
			return iter;
		}

		// ok, this one looks good then
		iter = sub;
	}

	return iter;
}


// -----------------------------------------------------------------------------
// Section: ResourceUsageConsole
// -----------------------------------------------------------------------------
/**
 *	Constructor.
 */
ResourceUsageConsole::ResourceUsageConsole( Handler * pHandler ) :
	pHandler_( pHandler )
{
}


/**
 *	This method initialises this object.
 */
bool ResourceUsageConsole::init()
{
	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( "Resource Usage Console\n" );
	}

	return succeeded;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void ResourceUsageConsole::update()
{
	this->clear();

	pHandler_->displayResourceStatistics( *this );
}


/**
 *	This method overrides the InputHandler method.
 */
bool ResourceUsageConsole::handleKeyEvent( const KeyEvent & event )
{
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}

	bool handled = true;

	switch (event.key())
	{
	// Cycle through the memory type groupings
	case KeyEvent::KEY_SPACE:
	case KeyEvent::KEY_JOY2:
		pHandler_->cycleGranularity();
		break;

	// Scroll up
	case KeyEvent::KEY_NUMPADMINUS:
	case KeyEvent::KEY_PGUP:
		this->scrollUp();
		break;

	// Scroll down
	case KeyEvent::KEY_ADD:
	case KeyEvent::KEY_PGDN:
		this->scrollDown();
		break;

	// Jump to top of page
	case KeyEvent::KEY_HOME:
		this->scrollOffset( 0 );
		break;

	// Save to CSV
	case KeyEvent::KEY_C:
		pHandler_->dumpToCSV(*this);
		break;

	// Key not handled
	default:
		handled = false;
		break;
	}

	return handled;
}


// -----------------------------------------------------------------------------
// Section: HistogramConsole
// -----------------------------------------------------------------------------

bool HistogramConsole::allowDarkBackground() const
{
	return !Luminance_ && !R_ && !G_ && !B_;
}

/**
 *	This method initialises this object.
 */
bool HistogramConsole::init()
{
	Luminance_ = true;
	R_ = true;
	G_ = true;
	B_ = true;
	Background_ = false;
	topRatio_ = 1;

	bool succeeded = XConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( "Histogram Console\n" );
	}

	return succeeded;
}

void HistogramConsole::activate( bool isReactivate )
{
	InputConsole::activate( isReactivate );
	if( !isReactivate )
		HistogramProvider::instance().addUser();
}

void HistogramConsole::deactivate()
{
	HistogramProvider::instance().removeUser();
}

class GraphTrack
{
	float left_;// all dimensions are from 0.0 to 1.0
	float top_;
	float width_;
	float height_;
public:
	enum DUMMY
	{
		ENTRY_SIZE = 256,
		MARGIN_SIZE = 1
	};
	GraphTrack( float left, float top, float width, float height )
		: left_( left ), top_( top ), width_( width ), height_( height )
	{}

	void clear()
	{
		static std::vector< Moo::VertexTL > tlvv;

		// draw the background
		tlvv.erase( tlvv.begin(), tlvv.end() );

		float left = left_ * Moo::rc().screenWidth();
		float right = ( left_ + width_ ) * Moo::rc().screenWidth();
		float top = top_ * Moo::rc().screenHeight();
		float bottom = ( top_ + height_ ) * Moo::rc().screenHeight();

		Moo::VertexTL tlv;
		tlv.pos_.z = 0;
		tlv.pos_.w = 1;
		tlv.colour_ = 0x00000000;

		tlv.pos_.x = left;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );

		tlv.pos_.x = left;				tlv.pos_.y = top;			tlvv.push_back( tlv );
		tlv.pos_.x = right;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );
		tlv.pos_.x = left;				tlv.pos_.y = bottom;		tlvv.push_back( tlv );

		Moo::Material::setVertexColour();

		Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
		Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
		Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
		Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

		Moo::rc().setVertexShader( NULL );
		Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

		Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLELIST, tlvv.size() / 3,
			&tlvv.front(), sizeof( Moo::VertexTL ) );
	}
	void draw( uint32 color, const unsigned int* histogram, unsigned int topRatio ) const
	{
		static std::vector< Moo::VertexTL > tlvv;

		unsigned int max = 0;
		for( int i = 0; i < ENTRY_SIZE; ++i )
			if( histogram[ i ] > max )
				max = histogram[ i ];

		max /= topRatio;

		tlvv.erase( tlvv.begin(), tlvv.end() );

		// draw the border
		{
			float margin = MARGIN_SIZE;//float( MARGIN_SIZE ) / ( ENTRY_SIZE + MARGIN_SIZE * 2 ) * Moo::rc().screenWidth();
			float left = left_ * Moo::rc().screenWidth();
			float right = ( left_ + width_ ) * Moo::rc().screenWidth();
			float top = top_ * Moo::rc().screenHeight();
			float bottom = ( top_ + height_ ) * Moo::rc().screenHeight();

			Moo::VertexTL tlv;
			tlv.pos_.z = 0;
			tlv.pos_.w = 1;
			tlv.colour_ = 0xffffffff;

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = top + margin;		tlvv.push_back( tlv );

			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = right;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = right - margin;	tlv.pos_.y = top;				tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = top;				tlvv.push_back( tlv );
			tlv.pos_.x = left + margin;		tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );

			tlv.pos_.x = left;				tlv.pos_.y = bottom - margin;	tlvv.push_back( tlv );
			tlv.pos_.x = right;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
			tlv.pos_.x = left;				tlv.pos_.y = bottom;			tlvv.push_back( tlv );
		}

		Moo::VertexTL tlv;
		tlv.pos_.z = 0;
		tlv.pos_.w = 1;
		tlv.colour_ = color;

		float lastX = 0, lastY = height_;
		float currentX, currentY;

		for( int i = 0; i < ENTRY_SIZE; ++i )
		{
			currentX = float( i + MARGIN_SIZE ) / ( ENTRY_SIZE + MARGIN_SIZE * 2 ) * width_;
			currentY = height_ - ( histogram[ i ] <= max ? histogram[ i ] : max ) / float( max ) * height_;

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( lastY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( currentX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( currentY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( lastX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.x = ( currentX + left_ ) * Moo::rc().screenWidth();
			tlv.pos_.y = ( currentY + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			tlv.pos_.y = ( height_ + top_ ) * Moo::rc().screenHeight();
			tlvv.push_back( tlv );

			lastX = currentX;
			lastY = currentY;
		}

		if( tlvv.size() )
		{
			Moo::Material::setVertexColour();

			Moo::rc().setRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
			Moo::rc().setRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
			Moo::rc().setRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
			Moo::rc().setRenderState( D3DRS_ZENABLE, TRUE );
			Moo::rc().setRenderState( D3DRS_ZFUNC, D3DCMP_ALWAYS );

			Moo::rc().setVertexShader( NULL );
			Moo::rc().setFVF( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );

			Moo::rc().drawPrimitiveUP( D3DPT_TRIANGLELIST, tlvv.size() / 3,
				&tlvv.front(), sizeof( Moo::VertexTL ) );
		}

	}
};

/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void HistogramConsole::update()
{
	this->clear();
	this->setCursor( 0, 0 );
	std::string title = "Histogram : ";
	if( Luminance_ )
		title += "Luminance";
	if( R_ )
		title += *title.rbegin() == ' ' ? "Red" : " + Red";
	if( G_ )
		title += *title.rbegin() == ' ' ? "Green" : " + Green";
	if( B_ )
		title += *title.rbegin() == ' ' ? "Blue" : " + Blue";
	if( *title.rbegin() == ' ' )
	{
		title += "None\n\n";
		title += "Press Numpad 0 to toggle Luminance\n";
		title += "Press Numpad 1 to toggle Red\n";
		title += "Press Numpad 2 to toggle Green\n";
		title += "Press Numpad 3 to toggle Blue\n";
		title += "Press Numpad 4 to toggle Background\n";
		title += "Press Numpad 5 to increase range\n";
		title += "Press Numpad 6 to decrease range\n";
	}
	this->print( title.c_str() );

	if( Luminance_ )
	{
		GraphTrack GraphTrack( 0.0f, 0.025f, 0.25f, 0.25f );
		if( Background_ )
			GraphTrack.clear();
		GraphTrack.draw( 0xffffffff, HistogramProvider::instance().get( HistogramProvider::HT_LUMINANCE ).value_, topRatio_ );
	}
	if( R_ || G_ || B_ )
	{
		GraphTrack GraphTrack( !Luminance_ ? 0.0f : 0.27f, 0.025f, 0.25f, 0.25f );
		if( Background_ )
			GraphTrack.clear();

		if( R_ )
			GraphTrack.draw( 0xffff0000, HistogramProvider::instance().get( HistogramProvider::HT_R ).value_, topRatio_ );
		if( G_ )
			GraphTrack.draw( 0xff00ff00, HistogramProvider::instance().get( HistogramProvider::HT_G ).value_, topRatio_ );
		if( B_ )
			GraphTrack.draw( 0xff0000ff, HistogramProvider::instance().get( HistogramProvider::HT_B ).value_, topRatio_ );
	}
}

/**
 *	This method overrides the InputHandler method.
 */
bool HistogramConsole::handleKeyEvent( const KeyEvent & event )
{
	if (!event.isKeyDown())
	{
		// required behaviour for consoles
		return false;
	}

	bool handled = true;

	switch (event.key())
	{
	case KeyEvent::KEY_NUMPAD0:
		Luminance_ = !Luminance_;
		break;
	case KeyEvent::KEY_NUMPAD1:
		R_ = !R_;
		break;
	case KeyEvent::KEY_NUMPAD2:
		G_ = !G_;
		break;
	case KeyEvent::KEY_NUMPAD3:
		B_ = !B_;
		break;
	case KeyEvent::KEY_NUMPAD4:
		Background_ = !Background_;
		break;
	case KeyEvent::KEY_NUMPAD5:
		if( topRatio_ > 1 )
			--topRatio_;
		break;
	case KeyEvent::KEY_NUMPAD6:
		++topRatio_;
		break;
	default:
		handled = false;
		break;
	}

	return handled;
}


// -----------------------------------------------------------------------------
// Section: EditConsole
// -----------------------------------------------------------------------------

// 'this' : used in base member initializer list
#pragma warning(disable: 4355)

/**
 *	Constructor.
 */
EditConsole::EditConsole() :
	lineEditor_(this),
	editLine_( 0 ),
	editCol_( 0 ),
	editColour_( 0xFFFFFFFF )
{
	this->editLine_ = this->visibleHeight()-1;
}


/**
 *	This method overrides the InputHandler method to handle key presses.
 */
bool EditConsole::handleKeyEvent( const KeyEvent & event )
{
	bool handled = false;

	if (this->isCursorShowing())
	{
		std::string resultLine;

		LineEditor::ProcessState state =
			lineEditor_.processKeyEvent( event, resultLine );

		switch (state)
		{
			case LineEditor::NOT_HANDLED:
				handled = false;
				break;

			case LineEditor::PROCESSED:
				this->update();
				handled = true;
				break;

			case LineEditor::RESULT_SET:
				this->processLine( resultLine );
				handled = true;
				break;

			default:
				MF_ASSERT( 0 );
				break;
		}
	}

	return handled;
}


/**
 *	This method overrides the InputHandler method to handle axis positions.
 */
bool EditConsole::handleAxisEvent( const AxisEvent & event )
{
	bool handled = false;

	if (this->isCursorShowing())
	{
		// we consume all axis events
		handled = true;
		// but we only use the associated key down events
	}

	return handled;
}


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void EditConsole::update()
{
	this->XConsole::update();

	if (this->isCursorShowing())
	{
		// now do normal update stuff
		this->lineColourOverride( editLine_, editColour_ );

		this->setCursor( editCol_, editLine_ );
		this->print( this->prompt() );
		this->setCursor( editCol_ + this->prompt().size(), editLine_ );
		this->print( lineEditor_.editString() );

		this->setCursor(
			editCol_ + this->prompt().size() + lineEditor_.cursorPosition(), editLine_ );
	}
}


/// Deactivates this console
void EditConsole::deactivate()
{
	this->lineEditor_.deactivate();
}


/// Draw this console
void EditConsole::draw( float dTime )
{
	this->lineEditor_.tick( dTime );
	this->XConsole::draw( dTime );
}


/**
 * This method reset editLine_ according to visibleHeight()
 */
void EditConsole::visibleAreaChanged( int oldVisibleWidth, int oldVisibleHeight )
{
	this->editLine_ = this->visibleHeight() - 1;
}


/**
 *	This method hides the cursor.
 */
void EditConsole::hideCursor()
{
	XConsole::hideCursor();
	this->lineEditor_.deactivate();
	this->setCursor( editCol_, editLine_ );
	this->print( " " );
}

void EditConsole::createUnmanagedObjects()
{
	XConsole::createUnmanagedObjects();
}


#if ENABLE_WATCHERS
// -----------------------------------------------------------------------------
// Section: DebugConsole
// -----------------------------------------------------------------------------

int DebugConsole::highlightItem_ = 0;

DebugConsole::~DebugConsole()
{
}
/**
 *	This method initialises this object.
 */
bool DebugConsole::init()
{
	bool succeeded = EditConsole::init();

	this->hideCursor();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFF00 );
		this->setCursor( 0, 0 );
		this->print( "Debug Console\n" );
	}

	return succeeded;
}

/**
 *	This method is called when the debug console is activated
 *
 */
void DebugConsole::activate( bool isReactivate )
{
	InputConsole::activate( isReactivate );
}

void DebugConsole::deactivate()
{
	this->lineEditor_.deactivate();
}

/**
 * TODO: to be documented.
 */
class DebugConsoleWatcherVisitor : public WatcherVisitor
{
public:
	DebugConsoleWatcherVisitor( DebugConsole *pDebugConsole ) :
		pDebugConsole_( pDebugConsole ),
		count_( 0 )
	{
	}

	~DebugConsoleWatcherVisitor()
	{
		if (DebugConsole::highlightItem_ < 0)
		{
			DebugConsole::highlightItem_ = count_ - 1;
		}
		if (DebugConsole::highlightItem_ >= count_)
		{
			DebugConsole::highlightItem_ = 0;
		}
	}

protected:
	virtual bool visit( Watcher::Mode mode,
		const std::string & label,
		const std::string & desc,
		const std::string & value )
	{
		char buf[256];

		if (mode == Watcher::WT_DIRECTORY)
		{
			bw_snprintf( buf, sizeof(buf), "%2d [%s]\n",
				count_, label.c_str() );
		}
		else
		{
			bw_snprintf( buf, sizeof(buf), "%2d %-14s = %8s    \n",
				count_, label.c_str(), value.c_str() );
		}

		pDebugConsole_->lineColourOverride( pDebugConsole_->cursorY(),
			(count_ == DebugConsole::highlightItem_)	? 0xffff0000 :
			(mode == Watcher::WT_READ_WRITE)			? 0xffffffff :
			(mode == Watcher::WT_DIRECTORY)				? 0xffffff20 :
														  0xff9f9f9f );

		if (count_ == DebugConsole::highlightItem_)
		{
			int8 x, y;
			x = pDebugConsole_->cursorX();
			y = pDebugConsole_->cursorY();

			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 4, 0xffffffff );
			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 3, 0xffffffff );
			pDebugConsole_->lineColourOverride( pDebugConsole_->visibleHeight() - 2, 0xffffffff );

			uint iChar = 0;

			// print desription with word wrap at the bottom of the consol
			int bufferSize = pDebugConsole_->visibleWidth() * 3;
			char * descriptionBuffer = new char[bufferSize+1];
			int copySize = std::min(bufferSize, int(desc.length()));
			strncpy(descriptionBuffer, desc.c_str(), copySize);
			descriptionBuffer[copySize] = '\0';

			pDebugConsole_->setCursor( 0, pDebugConsole_->visibleHeight() - 4 );

            char * iToken = strtok( descriptionBuffer, " \n" );
			while( iToken )
			{
				if( (int)strlen( iToken ) > pDebugConsole_->visibleWidth() - pDebugConsole_->cursorX() )
					pDebugConsole_->print( "\n" );

				if( pDebugConsole_->cursorX() != 0 )
					pDebugConsole_->print( " " );
				pDebugConsole_->print( iToken );

				iToken = strtok( NULL, " \n" );
			}
			delete [] descriptionBuffer;
			pDebugConsole_->setCursor( x, y );
		}

		pDebugConsole_->print( buf );

		count_++;

		return true;
	}
private:
	DebugConsole	*pDebugConsole_;
	int				count_;
};


/**
 *	This method overrides the XConsole method. It is called to update this
 *	console.
 */
void DebugConsole::update()
{
	this->clear();

	this->setCursor( 0, 0 );
	this->print( "Debug Console\n" );
	this->print( path_.c_str() );
	this->print( "\n\n" );
	int headerLines = this->cursorY();

	DebugConsoleWatcherVisitor	dcwv( this );

#if ENABLE_WATCHERS
	Watcher::rootWatcher().visitChildren( NULL, path_.c_str(), dcwv );
#endif

	EditConsole::update();

	const int scro = this->scrollOffset();
	if (highlightItem_ <= scro && scro != 0)
		this->scrollUp();
	if (highlightItem_ >= scro + (this->visibleHeight() - headerLines - 1))
		this->scrollDown();
}


/**
 *	This method processes the current line.
 *
 *	@param line		The current line to process.
 */
void DebugConsole::processLine( const std::string & line )
{
	if (line.substr( 0, 2 ) == "cd")
	{
		if (line.length() == 2)
		{
			path_ = "";
		}
		else
		{
			path_ += line.substr( 3, line.length() - 3 ) + "/";
		}

		highlightItem_ = 0;
	}
	else
	{
		int assignmentLocation = line.find_first_of( "=" );

		if (assignmentLocation > 0)
		{
			// Strip
			int startPos = line.find_first_not_of( " " );
			int endPos = line.find_last_not_of( " ", assignmentLocation - 1 );
			std::string id	 = line.substr( startPos, endPos - startPos + 1 );

			std::string value = line.substr( assignmentLocation + 1, line.size() );

			Watcher::rootWatcher().setFromString( NULL, (path_ + id).c_str(), value.c_str() );
		}
	}

	this->hideCursor();
}


/**
 *	This method overrides the EditConsole method to process key events.
 *
 *	@param event	An object representing the key event.
 *
 *	@return			True if it was handled, false otherwise.
 */
bool DebugConsole::handleKeyEvent( const KeyEvent& event )
{
	bool handled = false;

	// Do some checking before we send it to the EditConsole.

	if (this->isCursorShowing() && event.isKeyDown())
	{
		handled = true;

		switch (event.key())
		{
			case KeyEvent::KEY_PGDN:
			case KeyEvent::KEY_PGUP:
			case KeyEvent::KEY_JOY0:
			case KeyEvent::KEY_JOY1:
			{
				printf("Page up or down\n");

				float increment =
					event.isAltDown()	? 1000.f :
					event.isCtrlDown()	?  100.f :
					event.isShiftDown()	?   10.f : 1.f;

				if (event.key() == KeyEvent::KEY_PGDN ||
					event.key() == KeyEvent::KEY_JOY1)
				{
					increment = -increment;
				}

				this->incrementLine( increment );
			}
			break;

			case KeyEvent::KEY_ESCAPE:
			case KeyEvent::KEY_JOY9:
//			case KeyEvent::KEY_JOY2:
				lineEditor_.editString( "" );
				lineEditor_.cursorPosition( 0 );
				this->hideCursor();
				//watcherDesc_->visible( false );
			break;

			default:
				handled = false;
			break;
		}
	}

	// Give the EditConsole a go.

	if (!handled)
	{
		handled = EditConsole::handleKeyEvent( event );
	}

	// Do some processing if it wasn't handled by the EditConsole.

	if (!handled && event.isKeyDown())
	{
		handled = true;

		switch (event.key())
		{
			case KeyEvent::KEY_0:
			case KeyEvent::KEY_1:
			case KeyEvent::KEY_2:
			case KeyEvent::KEY_3:
			case KeyEvent::KEY_4:
			case KeyEvent::KEY_5:
			case KeyEvent::KEY_6:
			case KeyEvent::KEY_7:
			case KeyEvent::KEY_8:
			case KeyEvent::KEY_9:
			{
				int itemNum = 0;

				if (event.key() == KeyEvent::KEY_0)
				{
					itemNum = 0;
				}
				else
				{
					// HACK: Assumes KEY_1 to KEY_9 are sequential.
					itemNum = event.key() - KeyEvent::KEY_1 + 1;
				}

				itemNum +=
					(event.isShiftDown()? 10 : 0) +
					(event.isCtrlDown()	? 20 : 0) +
					(event.isAltDown()	? 40 : 0);

				this->selectItem( itemNum );
				break;
			}

			case KeyEvent::KEY_NUMPADENTER:
			case KeyEvent::KEY_RETURN:
//			case KeyEvent::KEY_JOY3:
			case KeyEvent::KEY_JOY8:
				if (event.isAltDown())
				{
					handled = false;
					break;
				}

				if (event.isShiftDown())
				{
					this->showCursor();
				}
				else
				{
					this->selectItem( highlightItem_ );
				}
				break;

			case KeyEvent::KEY_HOME:
				path_ = "";
				highlightItem_ = 0;
				break;

			case KeyEvent::KEY_BACKSPACE:
//			case KeyEvent::KEY_JOY2:
			case KeyEvent::KEY_JOY9:
			{
				if (path_.size() > 0)
				{
					int pos = path_.find_last_of( "/", path_.size() - 2 );

					if (pos >= 0)
					{
						path_ = path_.substr( 0, pos + 1 );
					}
					else
					{
						path_ = "";
					}

					highlightItem_ = 0;
				}
				break;
			}

			case KeyEvent::KEY_PGUP:
			case KeyEvent::KEY_JOY0:
				--highlightItem_;
				break;

			case KeyEvent::KEY_PGDN:
			case KeyEvent::KEY_JOY1:
				++highlightItem_;
				break;

			case KeyEvent::KEY_ADD:
				this->scrollDown();
				break;

			case KeyEvent::KEY_NUMPADMINUS:
				this->scrollUp();
				break;

			default:
				handled = false;
				break;
		}
	}

	return handled;
}


/**
 *	This method increments the current value on the edit line.
 *
 *	@param increment		The amount to increment by.
 */
void DebugConsole::incrementLine( float increment )
{
	std::string line = lineEditor_.editString();

	int assignmentLocation = line.find_first_of( "=" );

	if (assignmentLocation > 0)
	{
		std::string valueStr = line.substr( assignmentLocation + 1, line.size() );
		int startPos	= line.find_first_not_of( " " );
		int endPos		= line.find_last_not_of( " ", assignmentLocation - 1 );
		std::string label	= line.substr( startPos, endPos - startPos + 1 );

		std::string path		= path_ + label;

		float oldValue = 0.f;

		if (sscanf( valueStr.c_str(), "%f", &oldValue ) == 1)
		{
			float newValue = oldValue + increment;

			char buf[128];
			bw_snprintf( buf, sizeof(buf), "%f", newValue );

			Watcher::rootWatcher().setFromString( NULL, path.c_str(), buf );

			std::string	result;
			std::string	desc;
			Watcher::Mode mode;

			if (Watcher::rootWatcher().getAsString( NULL, path.c_str(), result, desc, mode ))
			{
				this->edit( label, result );
			}
		}
	}
}

/**
 * This is a simple helper class used by DebugConsole::selectItem
 */
class SelectWatcherVisitor : public WatcherVisitor
{
public:
	SelectWatcherVisitor( int itemNum ) : found_( false ),
		count_( 0 ),
		itemNum_( itemNum )
	{}

	virtual bool visit( Watcher::Mode mode,
		const std::string & label,
		const std::string & desc,
		const std::string & value )
	{
		if (count_ == itemNum_)
		{
			mode_ = mode;
			label_ = label;
			value_ = value;
			desc_ = desc;
			found_ = true;
		}

		count_++;

		return true;
	}

	bool found_;
	Watcher::Mode mode_;
	std::string label_;
	std::string value_;
	std::string desc_;

	int count_;
	int itemNum_;
};


/**
 *	This method is used to select an item from the current list.
 *
 *	@param itemNum		The item number starting from 0.
 */
void DebugConsole::selectItem( int itemNum )
{
	if (this->isCursorShowing())
	{
		return;
	}

	SelectWatcherVisitor visitor( itemNum );
	Watcher::rootWatcher().visitChildren( NULL,path_.c_str(), visitor );

	if (visitor.found_)
	{
		switch (visitor.mode_)
		{
			case Watcher::WT_DIRECTORY:
				path_ += visitor.label_;
				path_ += "/";
				highlightItem_ = 0;
				break;

			case Watcher::WT_READ_WRITE:
				this->edit( visitor.label_, visitor.value_ );
				break;

			case Watcher::WT_READ_ONLY:
				// No action.
				break;

			default:
				WARNING_MSG( "DebugConsole::selectItem: Unhandled case\n" );
				break;
		}
	}
}


/**
 *	This method starts editing a value.
 */
void DebugConsole::edit( const std::string & label, const std::string & value )
{
	std::string line = label + "=" + value;

	lineEditor_.editString( line.c_str() );
	lineEditor_.cursorPosition( lineEditor_.editString().size() );

	this->showCursor();
}
#endif /* ENABLE_WATCHERS */


// -----------------------------------------------------------------------------
// Section: PythonConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
PythonConsole::PythonConsole() :
	isMultiline_( false ),
	autoCompIndex_( 0 )
{
}


/// Initialise this console
bool PythonConsole::init()
{
	bool succeeded = EditConsole::init();

	if (succeeded)
	{
		this->setConsoleColour( 0xFFFFFFFF );
		this->lineEditor_.advancedEditing( true );
		this->setScrolling( true );

		this->prompt( ">>> " );

		char	initString[512];
		bw_snprintf( initString, sizeof(initString), "Python %s on %s\n",
			Py_GetVersion(), Py_GetPlatform() );
		this->setCursor( 0, 0 );
		this->print( initString );
	}

	return succeeded;
}


/// Draw this console
void PythonConsole::draw( float dTime )
{
	this->editLine( this->cursorY() );
	this->showCursor();

	this->EditConsole::draw( dTime );
}

/**
 *	Retrieves the current command history list.
 */
const LineEditor::StringVector PythonConsole::history() const
{
	return this->EditConsole::lineEditor_.history();
}

/**
 *	Replaces the command history list.
 */
void PythonConsole::setHistory(const LineEditor::StringVector & history)
{
	this->EditConsole::lineEditor_.setHistory(history);
}


/// Handle a key event
bool PythonConsole::handleKeyEvent( const KeyEvent& event )
{
	bool handled = false;

	switch (event.key())
	{
	// deactivate console
	case KeyEvent::KEY_Z:
		if (event.isCtrlDown() && event.isKeyDown())
		{
			ConsoleManager::instance().deactivate();
			handled = true;
		}
		break;

	// try auto completion
	case KeyEvent::KEY_TAB:
		if (event.isKeyDown() && !event.isAltDown())
		{
			if (this->autoCompIndex_ == AUTO_INDEX_RESET)
			{
				this->completeLine( AUTO_INDEX_RESET );
				this->autoCompIndex_ = 0;
			}
			else
			{
				// cycle auto completion index
				this->autoCompIndex_ += event.isShiftDown() ? -1 : +1;
				this->completeLine( this->autoCompIndex_ );
			}
			handled = true;
		}
		break;
	}

	if (!handled)
	{
		handled = this->EditConsole::handleKeyEvent( event );
		if (handled)
		{
			this->autoCompIndex_ = AUTO_INDEX_RESET;
		}
	}

	return handled;
}


/// Process a line
void PythonConsole::processLine( const std::string & line )
{
	if (line.length() == 0)
	{
		if (!isMultiline_)
		{
			ConsoleManager::instance().deactivate();
			return;
		}

		isMultiline_ = false;	// no more multiline => run existing string
		this->prompt( ">>> " );
	}
	else
	{
		this->print( "\n" );

		std::string subbed = PyInputSubstituter::substitute( line );
		if (subbed.size() == 0)
			return;

		if (subbed.length() != 0 && subbed[ subbed.length() - 1] == ':')
		{
			isMultiline_ = true;
			this->prompt( "... " );
		}

		multiline_ += subbed + "\n";
	}

	if (!isMultiline_)
	{
		PyErr_Clear();
		PyObject * res = Script::runString( const_cast<char *>( multiline_.c_str() ), true );

		if (PyErr_Occurred())
		{
			PyErr_PrintEx(0);
			PyErr_Clear();
		}

		if (Py_FlushLine())
			PyErr_Clear();

		multiline_ = "";
	}
}


/// Try to complete current line
void PythonConsole::completeLine( int compIndex )
{
	int useIndex;
	if (compIndex == AUTO_INDEX_RESET)
	{
		// get fresh input from lineEditor
		std::string fullLine = lineEditor_.editString();
		int cPos = lineEditor_.cursorPosition();
		this->lastLineStart_  = fullLine.substr( 0, cPos );
		this->lastLineExcess_ = fullLine.substr( cPos, fullLine.length() - cPos );
		useIndex = 0;
	}
	else
	{
		// use saved state
		useIndex = compIndex;
	}

	std::string subbedLine =
		PyInputSubstituter::substitute( this->lastLineStart_ );
	if (subbedLine.size() == 0)
		return;

	StringPair headAndExp   = splitHeadAndExpression( subbedLine );
	StringPair objAndAttr   = splitObjAndAttr( headAndExp.second );
	StringVector attributes = getAttributes( objAndAttr.first.c_str() );

	std::string partialAttribute = objAndAttr.second.c_str();
	StringVector filtered = filterAttributes( attributes, partialAttribute );
	std::sort( filtered.begin(), filtered.end() );

	std::string completed;

	// ... single match. Return completed line
	if (filtered.size() == 1)
	{
		completed = buildCompletedLine(
				headAndExp.first, objAndAttr.first, filtered[0] );
	}
	// multiple match. Cycle
	else if (filtered.size() > 1)
	{
		// uncomment to print all candidates to console and
		// complete up to a common prefix (bash like behavior)
		/*
		this->print( "\n" );
		printAttributes( *this, filtered );
		std::string commonStart = getCommonPrefix( filtered, partialAttribute );
		*/

		// make sure useIndex is never negative (the
		// mod trick does't work for negative values)
		int index = ( useIndex % filtered.size() + filtered.size() ) % filtered.size();
		completed = buildCompletedLine(
				headAndExp.first, objAndAttr.first,
				filtered[ index ] );
		++useIndex;
	}
	// no match
	else
	{
		completed = this->lastLineStart_;
	}

	// update lineEditor
	lineEditor_.editString( completed + this->lastLineExcess_ );
	lineEditor_.cursorPosition( completed.length() );
}

// -----------------------------------------------------------------------------
// Section: TextConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
TextConsole::TextConsole() : pageNum_( 0 )
{
}


/**
 *	This method initialises this console with the input data section.
 */
bool TextConsole::init( DataSectionPtr pSection )
{
	pSection_ = pSection;

	return true;
}


/**
 *	This method is called when this console is activated.
 */
void TextConsole::activate( bool isReactivate )
{
	if (isReactivate)
	{
		pageNum_++;
	}
	else
	{
		pageNum_ = 0;
	}

	if (pageNum_ < this->numPages())
	{
		this->refresh();
	}
	else
	{
		ConsoleManager::instance().deactivate();
	}
}


/**
 *	This method returns the number of pages associated with this console.
 */
int TextConsole::numPages() const
{
	return pSection_ ? pSection_->countChildren() : 0;
}


/**
 *	This method refreshes the information displayed by this console.
 */
void TextConsole::refresh()
{
	this->clear();

	DataSectionPtr pPage = pSection_->openChild( pageNum_ );

	// Read the colour associated with the page.
	this->setConsoleColour(
		Colour::getUint32( pPage->readVector3( "colour",
				Vector3( 0xff, 0xff, 0xff ) ) ) | 0xff000000 );

	DataSection::iterator iter = pPage->begin();

	while (iter != pPage->end())
	{
		if ((*iter)->sectionName() == "line")
		{
			// Read the colour associated with a line.
			DataSectionPtr pColour = (*iter)->openSection( "colour" );

			if (pColour)
			{
				this->lineColourOverride( this->cursorY(),
					Colour::getUint32( pColour->asVector3() ) | 0xff000000 );
			}
			else
			{
				this->lineColourOverride( this->cursorY() );
			}

			// Read the line's string.
			this->print( (*iter)->asString() );
			this->print( "\n" );
		}

		iter++;
	}
}


/**
 *	This method handles keyboard events for this console.
 */
bool TextConsole::handleKeyEvent( const KeyEvent & event )
{
	bool handled = false;

	if (event.isKeyDown())
	{
		handled = true;

		switch( event.key() )
		{
		case KeyEvent::KEY_ADD:
			this->scrollDown();
			break;

		case KeyEvent::KEY_NUMPADMINUS:
			this->scrollUp();
			break;

		case KeyEvent::KEY_HOME:
			this->scrollOffset( 0 );
			break;

		default:
			handled = false;
			break;
		}
	}

	return handled;
}


/**
 *	This method handles the scroll event that comes from the XConsole.
 */
void TextConsole::onScroll()
{
	this->refresh();
}


// Helper functions
namespace { // anonymous

/**
 *	Splits given console line into two parts: head and expression.
 *	The expression is the substring that currently matters in the
 *	context of code completion. The head is anything preceeding it.
 *	Return result as a std::pair of strings.
 *
 *	For example, in the line:
 *		"if BigWorld.entities[ BigWorld.player().targetI"
 *	Head would be:
 *		"if BigWorld.entities[ "
 *	And expression:
 *		"BigWorld.player().targetI"
 */
StringPair splitHeadAndExpression( const std::string & line )
{
	std::string::const_iterator sepBegin = SEPARATORS.begin();
	std::string::const_iterator sepEnd   = SEPARATORS.end();

	int parenthesisCount = 0;
	int bracketsCount    = 0;
	std::string::const_reverse_iterator lineIt  = line.rbegin();
	std::string::const_reverse_iterator lineEnd = line.rend();
	while (lineIt != lineEnd)
	{
		if (std::find( sepBegin, sepEnd, *lineIt ) != sepEnd)
		{
			break;
		}
		else if (*lineIt == '(')
		{
			if (parenthesisCount == 0)
			{
				break;
			}
			else
			{
				--parenthesisCount;
			}
		}
		else if (*lineIt == ')')
		{
			++parenthesisCount;
		}
		else if (*lineIt == '[')
		{
			if (bracketsCount == 0)
			{
				break;
			}
			else
			{
				--bracketsCount;
			}
		}
		else if (*lineIt == ']')
		{
			++bracketsCount;
		}
		++lineIt;
	}

	uint pos = std::distance( lineIt, line.rend() );
	return std::make_pair(
			line.substr( 0, pos ),
			line.substr( pos, line.length() - pos ) );
}


/**
 *	Splits given expression line into two parts: object and attribute.
 *	The object is the substring where potential matching attributes are
 *	extracted from. The attribute is anything succeding the last dot in
 *	the expression. Return result as a std::pair of strings.
 *
 *	For example, in the expresttion:
 *		"BigWorld.player().targetI"
 *	Expression would be:
 *		"BigWorld.player()"
 *	And attribute:
 *		"targetI"
 */
StringPair splitObjAndAttr( const std::string & expression )
{
	// find lastest occurent of a point
	int lastDotPos = expression.find_last_of( '.' );

	// return result
	if (lastDotPos != -1)
	{
		// context + partial attribute
		return std::make_pair(
			expression.substr( 0, lastDotPos ),
			expression.substr(
					lastDotPos + 1,
					expression.length() - lastDotPos - 1 ) );
	}
	else
	{
		// partial attribute only
		return std::make_pair(
				"", expression.substr( 0, expression.length() ) );
	}
}

/**
 *	Extracts Python attributes from given object. Attributes are
 *	assumed to be the list of keys in object's dir(). If object
 *	is empty, return globals and all Python 2.4 keywords.
 */
StringVector getAttributes( const std::string & object )
{
	std::string out;
	if (object != "")
	{
		// get attributes from object's dictionary
		out = "dir( "+object+" )";
	}
	else
	{
		// empty object get globals + keywords.
		// See keyword.py in Python's distribution
		out = "globals().keys() + ";
		out += "['and', 'assert', 'break', 'class', 'continue', 'def', ";
		out += " 'del', 'elif', 'else', 'except', 'exec', 'finally', 'for', ";
		out += " 'from', 'global', 'if', 'import', 'in', 'is', 'lambda', ";
		out += " 'not', 'or', 'pass', 'print', 'raise', 'return', 'try', ";
		out += " 'while', 'yield', 'None', 'as' ] ";
	}

	PyObject * list = Script::runString( out.c_str(), false );
	if (PyErr_Occurred())
	{
		PyErr_Clear();
	}

	StringVector attributes;
	if (list != NULL)
	{
		int n = PyList_Size( list );
		if (n > 0) {
			for (int i = 0; i < n; i++) {
				PyObject * entry = PyList_GetItem( list, i );
				attributes.push_back( PyString_AS_STRING( entry ) );
			}
		}
	}
    return attributes;
}


/**
 *	Given a list of attribute names, return the
 *	ones that starts with the given string.
 */
StringVector filterAttributes(
		const StringVector &attributes,
		const std::string &startsWith )
{
	StringVector result;

	StringVector::const_iterator attIt  = attributes.begin();
	StringVector::const_iterator attEnd = attributes.end();
	while (attIt != attEnd)
	{
		if (attIt->find( startsWith ) == 0)
		{
			result.push_back( *attIt );
		}
		++attIt;
	}
	return result;
}


/**
 *	Builds the completed line.
 */
std::string buildCompletedLine(
	const std::string &head,
	const std::string &object,
	const std::string &attribute )
{
	return object != ""
		? head + object + "." + attribute
		: head + attribute;
}


/**
 *	Extracts a prefix common to all attributes
 *	name in the given list, if any.
 */
std::string getCommonPrefix(
		const StringVector & filtered,
		const std::string  & knownStart )
{
	MF_ASSERT( filtered.size() > 1 )

	bool failed = false;
	std::string result = knownStart;
	for (uint i=knownStart.size(); i < filtered[0].length(); ++i)
	{
		const char & testChar = filtered[0][i];
		StringVector::const_iterator filterIt  = filtered.begin();
		StringVector::const_iterator filterEnd = filtered.end();
		while (filterIt != filterEnd)
		{
			if ((*filterIt)[i] != testChar)
			{
				failed = true;
				break;
			}
			++filterIt;
		}
		if (failed)
		{
			break;
		}
		else
		{
			result += testChar;
		}
	}
	return result;
}


/**
 *	Prints the list of attribute names into the given console.
 */
void printAttributes( XConsole &console, const StringVector &attributes )
{
	StringVector::const_iterator filterIt  = attributes.begin();
	StringVector::const_iterator filterEnd = attributes.end();
	std::string attribListLine;
	while (filterIt != filterEnd)
	{
		if (attribListLine.length() + filterIt->length() > 80)
		{
			console.print( attribListLine + "\n" );
			attribListLine = "";
		}
		attribListLine += "  " + *filterIt;
		++filterIt;
	}
	if (attribListLine.length() > 0)
	{
		console.print( attribListLine + "\n" );
	}
}

} // namespace anonymous

// console.cpp
