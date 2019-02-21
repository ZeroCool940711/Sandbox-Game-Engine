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

#include "xconsole.hpp"
#include "console_manager.hpp"

#include <cstdmf/dprintf.hpp>
#include <cstdmf/watcher.hpp>
#include "resmgr/auto_config.hpp"

#ifndef CODE_INLINE
#include "xconsole.ipp"
#endif

#include "font.hpp"
#include "moo/render_context.hpp"

namespace { // anonymous

AutoConfigStrings s_consoleFonts("system/consoleFonts", "font");

const int IDEAL_CONSOLE_WIDTH  = 95;
const int IDEAL_CONSOLE_HEIGHT = 45;

} // namespace anonymous

DECLARE_DEBUG_COMPONENT2( "Romp", 0 )

#define EMPTY_SPACE ' '


// -----------------------------------------------------------------------------
// Section: XConsole
// -----------------------------------------------------------------------------

/**
 *	Constructor
 */
XConsole::XConsole() :
	font_(NULL),
	visibleWidth_(0),
	visibleHeight_(0),
	fitToScreen_(false),
	fonts_(),
	consoleColour_( 0xFFFFFFFF ),
	scrollable_( false ),
	showCursor_( false ),
	shouldWrap_( false ),
	scrollOffset_( 0 ),
	cursorX_( 0 ),
	cursorY_( 0 ),
	offset_( 0, 0 )
{
	this->clear();

	typedef AutoConfigStrings::Vector StringVector;
	StringVector fonts = s_consoleFonts.value();
	StringVector::const_iterator fontIt  = fonts.begin();
	StringVector::const_iterator fontEnd = fonts.end();
	while (fontIt != fontEnd)
	{
		FontPtr font;
		font = FontManager::instance().get(*fontIt);
		if (!font.exists())
		{
			ERROR_MSG( "XConsole(): Couldn't load font %s\n", fontIt->c_str() );
		}
		else
		{
			fonts_.push_back(font);
		}
		++fontIt;
	}
	this->selectFontBestMatch();
}


/**
 *	Destructor
 */
XConsole::~XConsole()
{
}

/**
 *	This method indicates if the current console allows 
 *	the screen to be darkened before it is drawn
 *
 *	@return		Returns true if background can be darknened.
 */
bool XConsole::allowDarkBackground() const
{
	return true;
}

/**
 *	This method initialises this object.
 *
 *	@return		True on success, false otherwise.
 */
bool XConsole::init()
{
	return true;
}


/**
 *	This method clears this console.
 */
void XConsole::clear()
{
	memset( buffer_, EMPTY_SPACE, sizeof( buffer_ ) );

	for (int i=0; i<MAX_CONSOLE_HEIGHT; i++)
	{
		lineColours_[i].inUse = false;
	}

	this->setCursor( 0, 0 );

	if (scrollOffset_ != 0)
	{
		bw_snprintf( (char *)buffer_[0], sizeof(buffer_[0]), "[%d More]", scrollOffset_ );
	}
}


/**
 *	Set the colour for the console's body text
 */
void XConsole::setConsoleColour( uint32 colour )
{
	consoleColour_ = colour;
}


/**
 *	This method draws this console.
 *
 *	@param dTime	The elapsed time since the last time the console was drawn.
 */
void XConsole::draw( float dTime )
{
	if ( !font_ )
		return;

	this->update();

	Moo::rc().setPixelShader( NULL );

	bool lastWasOverride = false;

	font_->colour( consoleColour_ );
	FontManager::instance().setMaterialActive( font_ );

	float mainAlpha = (consoleColour_ >> 24) / 255.f;

	for ( int row = 0; row < this->visibleHeight(); row++ )
	{
		if (lineColours_[row].inUse)
		{
			uint32 lcol = lineColours_[row].colour;
			font_->colour( (lcol & 0x00ffffffu) |
				(uint32( (lcol >> 24) * mainAlpha ) << 24) );
			FontManager::instance().setMaterialActive( font_ );
			lastWasOverride = true;
		}
		else if (lastWasOverride)
		{
			font_->colour( consoleColour_ );
			FontManager::instance().setMaterialActive( font_ );
		}

		std::string line = "";
		line.assign( (char*)( buffer_[row] ), this->visibleWidth_ );
		font_->drawConsoleString( line, 0, row, (int) offset_.x, (int) offset_.y );
	}

	if ( showCursor_ )
		drawCursor( dTime );
}


/**
 *	This method draws this console's cursor.
 *
 *	@param dTime	The elapsed time since the last time the console was drawn.
 */
void XConsole::drawCursor( float dTime )
{
	if ( !font_ )
		return;

	static float flashTime = 0;
	static bool  drawCursor = true;

	flashTime += dTime;

	if ( flashTime > 0.5f )
	{
		flashTime = 0;
		drawCursor = !drawCursor;
	}

	if (drawCursor)
	{
		//font_->drawCursor( cursorX_, cursorY_ );
		font_->drawConsoleString( "_", cursorX_, cursorY_, (int) offset_.x, (int) offset_.y );
	}
}


/**
 *	This method prints the input string to the console at the current cursor
 *	position.
 */
void XConsole::print( const std::string &string )
{
	for ( uint i = 0; i < string.length(); i++ )
	{
		if (cursorX_ < this->visibleWidth())
		{
			if (string[i] == '\n')
			{
				this->newline();
			}
			else
			{
				this->setChar( cursorY_, cursorX_, string[i] );

				cursorX_++;

				if ( cursorX_ == this->visibleWidth() )
				{
					this->newline();
				}
			}
		}
	}

	for (int i = cursorX_; i < this->visibleWidth(); i++)
	{
		this->setChar( cursorY_, i, ' ' );
	}
}


/**
 *	This method prints text to this console. The arguments the same as for
 *	printf.
 */
void XConsole::formatPrint( const char * format, ... )
{
	va_list argPtr;
	va_start( argPtr, format );

	char buf[ 2048 ];
	bw_vsnprintf( buf, sizeof(buf), format, argPtr );
	buf[sizeof(buf)-1] = '\0';

	va_end( argPtr );

	this->print( buf );
}


/**
 *	This method toggles the scale of the font used by this console.
 */
void XConsole::toggleFontScale()
{
	WARNING_MSG(
		"The use of console's font scaling in deprecated.\n"
		"Use font size matching for better visual results.");

	this->fitToScreen_ = !this->fitToScreen_;
	this->updateVisibleArea();
}


/**
 *	This method sets whether or not this console scrolls.
 */
void XConsole::setScrolling( bool state )
{
	scrollable_ = state;
}


int XConsole::visibleWidth() const
{
	return this->visibleWidth_;
}


int XConsole::visibleHeight() const
{
	return this->visibleHeight_;
}


/**
 *	This method hides the cursor of this console.
 */
void XConsole::hideCursor()
{
	showCursor_ = false;
}


/**
 *	This method shows the cursor of this console.
 */
void XConsole::showCursor()
{
	showCursor_ = true;
}


/**
 *	This method sets the colour of a specific line.
 */
void XConsole::lineColourOverride( int line, uint32 colour )
{
	line -= scrollOffset_;

	if (line < 0 || line >= MAX_CONSOLE_HEIGHT)
		return;

	lineColours_[line].inUse = true;
	lineColours_[line].colour = colour;
}


/**
 *	This method clears the colour override of the input line.
 */
void XConsole::lineColourOverride( int line )
{
	line -= scrollOffset_;

	if (line < 0 || line >= MAX_CONSOLE_HEIGHT)
		return;

	lineColours_[line].inUse = false;
}


/**
 *	This method is called when the ConsoleManager activates this console. The
 *	default behaviour is to no longer be the active console is this call is a
 *	reactivate.
 *
 *	@param isReactivate	True if the console is currently active, otherwise
 *		false.
 */
void XConsole::activate( bool isReactivate )
{
	MF_ASSERT( ConsoleManager::instance().pActiveConsole() == this );

	if (isReactivate)
	{
		ConsoleManager::instance().deactivate();
	}
}


//
//	PRIVATE METHODS
//

/**
 *	This method moves the cursor to the next line and handles scrolling.
 */
void XConsole::newline()
{
	cursorX_ = 0;
	cursorY_++;
	
	if (cursorY_ - scrollOffset_ == this->visibleHeight_)
	{
		if (scrollable_)
		{
			// scroll the console contents
			cursorY_--;
			memcpy( &buffer_[0][0], &buffer_[1][0], MAX_CONSOLE_WIDTH * ( this->visibleHeight_ - 1 ) );
			memset( &buffer_[this->visibleHeight_-1][0], EMPTY_SPACE, MAX_CONSOLE_WIDTH );
		}
		else if (shouldWrap_)
		{
			cursorY_ = 0;
		}
		else
		{
			memset( buffer_[this->visibleHeight_-1], EMPTY_SPACE, MAX_CONSOLE_WIDTH );
			memcpy( buffer_[this->visibleHeight_-1], "[More]", 6 );
		}
	}
}


/**
 *	This methods sets the character at the input coordinate.
 */
void XConsole::setChar( int row, int column, char value )
{
	row -= scrollOffset_;

	if (0 < row && row < MAX_CONSOLE_HEIGHT ||
		row == 0 && scrollOffset_ == 0)
	{
		buffer_[row][column] = value;
	}
}


void XConsole::createUnmanagedObjects()
{
	this->selectFontBestMatch();
}


void XConsole::selectFontBestMatch()
{
	this->font_ = NULL;

	float desiredWidth = Moo::rc().screenWidth() / float(IDEAL_CONSOLE_WIDTH);
	float smalledDistanceSoFar = 1e10;

	FontVector::const_iterator fontIt  = this->fonts_.begin();
	FontVector::const_iterator fontEnd = this->fonts_.end();
	while (fontIt != fontEnd)
	{
		int fontWidth  = 0;
		int fontHeight = 0;
		(*fontIt)->metrics().stringDimensions(L"@", fontWidth, fontHeight);
		float distance = fabsf(desiredWidth - fontWidth);
		if (distance < smalledDistanceSoFar)
		{
			smalledDistanceSoFar = distance;
			this->font_ = *fontIt;
		}
		++fontIt;
	}	
	
	this->updateVisibleArea();
}


void XConsole::updateVisibleArea()
{
	int oldVisibleWidth = this->visibleWidth_;
	int oldVisibleHeight = this->visibleHeight_;
	if (this->font_.exists())
	{
		if (!this->fitToScreen_)
		{
			int fontWidth  = 0;
			int fontHeight = 0;
			this->font_->metrics().stringDimensions(L"@", fontWidth, fontHeight);
			this->visibleWidth_ = std::min(
				int(Moo::rc().screenWidth() / fontWidth),
				MAX_CONSOLE_WIDTH);
			MF_ASSERT(this->visibleWidth_ * fontWidth <= Moo::rc().screenWidth());

			// a bottom margin of a single pixel is used
			// just to make sure the under-score cursor
			// is not rendered in the very last line of
			// the screen (makes it really hard to see).
			const int BOTTOM_MARGIN = 1;
			float screenHeight   = Moo::rc().screenHeight()-BOTTOM_MARGIN;
			this->visibleHeight_ = std::min(
				int(screenHeight/fontHeight), 
				MAX_CONSOLE_HEIGHT);
			MF_ASSERT(this->visibleHeight_ * fontHeight <= screenHeight);
			
			int difference = cursorY_ - (this->visibleHeight_-1);
			if (this->scrollable_ && difference > 0 && this->visibleHeight_ > 0)
			{
				cursorY_ -= difference;
				memcpy( 
					&buffer_[0][0], &buffer_[difference][0], 
					MAX_CONSOLE_WIDTH*(cursorY_));
					
				memset( 
					&buffer_[cursorY_][0], EMPTY_SPACE, 
					MAX_CONSOLE_WIDTH*(MAX_CONSOLE_HEIGHT-cursorY_) );
			}
		}
		else
		{
			this->visibleWidth_ = IDEAL_CONSOLE_WIDTH;
			this->visibleHeight_ = IDEAL_CONSOLE_HEIGHT;
		
			// fix scale-to-fit
			font_->fitToScreen( 
				this->fitToScreen_,
				Vector2(
				float(IDEAL_CONSOLE_WIDTH), 
				float(IDEAL_CONSOLE_HEIGHT)));
		}
	}
	if (oldVisibleWidth != this->visibleWidth_ || oldVisibleHeight != this->visibleHeight_)
	{
		this->visibleAreaChanged( oldVisibleWidth, oldVisibleHeight );
	}
}


// xconsole.cpp
