/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 *	GUI Tearoff panel framework - Tab Control widget class implementation
 */

#include "pch.hpp"
#include "guitabs.hpp"

namespace GUITABS
{


TabCtrl::TabCtrl( CWnd* parent, Alignment align ) :
	eventHandler_( 0 ),
	btnHilight_( PS_SOLID, 1, GetSysColor(COLOR_BTNHILIGHT) ),
	btnShadow_( PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW) ),
	btnSeparator_( PS_SOLID, 1,	// tab separator color is 1/4 the shadow color
		RGB(
			(GetRValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetRValue( GetSysColor(COLOR_BTNFACE) ))/4,
			(GetGValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetGValue( GetSysColor(COLOR_BTNFACE) ))/4,
			(GetBValue( GetSysColor(COLOR_BTNSHADOW) ) + 3*GetBValue( GetSysColor(COLOR_BTNFACE) ))/4 )
		),
	curItem_( 0 ),
	multiline_( true ),
	numLines_( 1 ),
	align_( align )
{
	CreateEx(
		0,
		AfxRegisterWndClass( CS_DBLCLKS|CS_OWNDC, ::LoadCursor(NULL, IDC_ARROW), 0 ),
		"TabCtrl",
		WS_CHILD,
		CRect( 0, 0, 100, TABBAR_HEIGHT ),
		parent,
		0,
		0);

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof( metrics );
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, metrics.cbSize, (void*)&metrics, 0 );
	metrics.lfSmCaptionFont.lfWeight = FW_NORMAL;
	font_.CreateFontIndirect( &metrics.lfSmCaptionFont );
}

TabCtrl::~TabCtrl()
{

}

TabCtrl::Alignment TabCtrl::getAlignment()
{
	return align_;
}

void TabCtrl::setEventHandler( TabCtrlEventHandler* handler )
{
	eventHandler_ = handler;
}

void TabCtrl::insertItem( const std::string& caption, HICON icon, void* itemData )
{
	if ( getItem( itemData ) )
		return;
	itemList_.push_back( TabCtrlItem( caption, icon, itemData ) );
	paint();
}

void TabCtrl::removeItem( void* itemData )
{
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			itemList_.erase( i );
			break;
		}
	}

	if ( curItem_ >= (int)itemList_.size() )
		curItem_ = 0;
	paint();
}

bool TabCtrl::contains( void* itemData )
{
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		if ( (*i).itemData == itemData )
			return true;

	return false;
}

int TabCtrl::itemCount()
{
	return (int)itemList_.size();
}

void* TabCtrl::getItemData( int index )
{
	if ( index < 0 || index >= (int)itemList_.size() )
		return 0;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, --index )
		if ( !index )
			return (*i).itemData;

	return 0;
}



int TabCtrl::getHeight()
{
	return TabCtrl::TABBAR_HEIGHT * numLines_;
}

int TabCtrl::recalcHeight()
{
	int ret = recalcHeightOnly();
	recalcLineSizes();
	return ret;
}

int TabCtrl::recalcHeightOnly()
{
	// startup
	numLines_ = 1;

	if ( itemList_.size() == 0 )
		return 0;

	CClientDC dc(this);

	CRect rect;
	GetClientRect( &rect );

	CFont* oldFont = dc.SelectObject( &font_ );

	// Calc the total size to see if it overflows the actual window size
	int totalSize = TABBAR_MARGIN;
	int lineSize = TABBAR_MARGIN;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		CString caption( (*i).caption.c_str() );
		int itemSize = 0;
		if ( (*i).icon )
			itemSize += ICON_SIZE + TABBAR_MARGIN;
		itemSize += dc.GetTextExtent( caption ).cx + TABBAR_MARGIN*2;
		totalSize += itemSize;
		if ( multiline_ )
		{
			lineSize += itemSize;
			if ( lineSize + TABBAR_MARGIN > rect.Width() )
			{
				numLines_++;
				lineSize = TABBAR_MARGIN + itemSize;
			}
		}
	}

	totalSize += TABBAR_MARGIN;
	if ( totalSize < rect.Width() )
		totalSize = rect.Width();

	if ( numLines_ > (int)itemList_.size() )
		numLines_ = (int)itemList_.size();

	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	numLines_ = (int)ceil( (float)itemList_.size() / (float)itemsPerLine );

	dc.SelectObject( oldFont );

	return totalSize;
}

void TabCtrl::recalcLineSizes()
{
	CClientDC dc(this);

	CFont* oldFont = dc.SelectObject( &font_ );

	// Calc the total size to see if it overflows the actual window size
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	int lineSize = TABBAR_MARGIN;
	// calc line sizes, in order to proportionally calc item sizes in multiline
	if ( multiline_ )
	{
		int count = 0;
		int lineSize = TABBAR_MARGIN;
		lineSizes_.clear();
		for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		{
			CString caption( (*i).caption.c_str() );
			int itemSize = 0;
			if ( (*i).icon )
				itemSize += ICON_SIZE + TABBAR_MARGIN;
			itemSize += dc.GetTextExtent( caption ).cx + TABBAR_MARGIN*2;
			lineSize += itemSize;
			count++;
			if ( count == itemsPerLine )
			{
				lineSizes_.push_back( lineSize + TABBAR_MARGIN );
				lineSize = TABBAR_MARGIN;
				count = 0;
			}
		}
		lineSizes_.push_back( lineSize + TABBAR_MARGIN );
	}

	dc.SelectObject( oldFont );
}

void TabCtrl::paint( bool multilineSwap )
{
	int totalSize = 0;
	if ( multilineSwap || numLines_ == 1 )
		totalSize = recalcHeight();
	else
		recalcLineSizes();

	// startup
	CClientDC dc(this);

	CRect rect;
	GetClientRect( &rect );

	// background
	COLORREF backColor = GetSysColor(COLOR_BTNFACE);
	int r = GetRValue( backColor ) + BACK_COLOR_ADDR;
	int g = GetGValue( backColor ) + BACK_COLOR_ADDG;
	int b = GetBValue( backColor ) + BACK_COLOR_ADDB;
	int maxComp = max( r, max( g, b ) );
	if ( maxComp > 255 )
	{
		// normalize to 255
		r = r * 255 / maxComp;
		g = g * 255 / maxComp;
		b = b * 255 / maxComp;
	}
	dc.FillSolidRect( rect, RGB( min( 255, r ), min( 255, g ), min( 255, b ) ) );

	if ( itemList_.size() == 0 )
		return;

	// Tab buttons + text
	COLORREF textActive = GetSysColor(COLOR_BTNTEXT);
	COLORREF textInactive = GetSysColor(COLOR_GRAYTEXT);

	int oldBkMode = dc.SetBkMode(TRANSPARENT);
	CFont* oldFont = dc.SelectObject( &font_ );
	COLORREF oldColor = dc.SetTextColor( textActive );
	CPen* oldPen = 0;

	if ( align_ == TOP )
	{
		oldPen = dc.SelectObject( &btnHilight_ );
		dc.MoveTo( 0, rect.bottom - 1 );
		dc.LineTo( rect.right, rect.bottom - 1 );
	}
	else
	{
		oldPen = dc.SelectObject( &btnShadow_ );
		dc.MoveTo( 0, rect.top );
		dc.LineTo( rect.right, rect.top );
	}

	// items per line is used in multiline calculations
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );

	if ( multilineSwap && multiline_ )
	{
		// prepare line ypos array puting the current item's line at the bottom in a multiline tab control
		lineYPos_.clear();
		for( int i = 0; i < numLines_; ++i )
			lineYPos_.push_back( TABBAR_HEIGHT * i );

		int count = 0;
		for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		{
			if ( curItem_ == count )
			{
				int icur = min( count / itemsPerLine, (int)lineYPos_.size() - 1 );
				int isel = (align_ == TOP) ? (int)lineYPos_.size()-1 : 0;
				
				int tmp = lineYPos_.at( icur );
				lineYPos_.at( icur ) = lineYPos_.at( isel );
				lineYPos_.at( isel ) = tmp;
				break;
			}

			count++;
		}
	}

	// do the draw loop
	int xpos = TABBAR_MARGIN;
	int ypos;
	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		// calc desired item size
		CString caption( (*i).caption.c_str() );
		int actualSize = dc.GetTextExtent( caption ).cx + TABBAR_MARGIN * 2;
		int iconSize = 0;
		if ( (*i).icon )
		{
			iconSize = ICON_SIZE + TABBAR_MARGIN;
			actualSize += iconSize;
		}

		// adjust item pos and size depending if it's single or multi line 
		int itemSize;
		if ( numLines_ == 1 )
			itemSize = actualSize * rect.Width() / totalSize;
		else
			itemSize = actualSize * rect.Width() / lineSizes_.at(
				min( count / itemsPerLine, (int)lineSizes_.size() - 1 ) );

		if ( (*i).icon && itemSize < iconSize + TABBAR_MARGIN )
			iconSize = 0;

		if ( multiline_ && count && (count % itemsPerLine) == 0 )
			xpos = TABBAR_MARGIN;	// new line, reset pos

		ypos = lineYPos_.at( min( count / itemsPerLine, (int)lineYPos_.size() - 1 ) );

		if ( curItem_ == count )
		{
			// draw tab arround item
			CRect tabRect( xpos, ypos, xpos + itemSize, ypos + TABBAR_HEIGHT - 2 );
			if ( align_ == TOP )
				tabRect.OffsetRect( 0, 2 );

			dc.FillSolidRect( tabRect, GetSysColor(COLOR_BTNFACE) );

			dc.SelectObject( &btnHilight_ );
			if ( align_ == TOP )
			{
				dc.MoveTo( tabRect.left, tabRect.bottom );
				dc.LineTo( tabRect.left, tabRect.top );
				dc.LineTo( tabRect.right, tabRect.top );

				dc.SelectObject( &btnShadow_ );
				dc.LineTo( tabRect.right, tabRect.bottom );
				if ( !multilineSwap && numLines_ > 1 )
					dc.LineTo( tabRect.left, tabRect.bottom );
			}
			else
			{
				dc.MoveTo( tabRect.left, tabRect.top );
				dc.LineTo( tabRect.left, tabRect.bottom );

				dc.SelectObject( &btnShadow_ );
				dc.LineTo( tabRect.right, tabRect.bottom );
				dc.LineTo( tabRect.right, tabRect.top );
				if ( !multilineSwap && numLines_ > 1 )
				{
					dc.SelectObject( &btnHilight_ );
					dc.LineTo( tabRect.left, tabRect.top );
				}
			}

			dc.SetTextColor( textActive );
		}
		else
		{
			// if it's not the last to the right and not the last, draw the separator
			if ( ((count + 1) % itemsPerLine ) != 0 && count < (int)itemList_.size()-1 )
			{
				dc.SelectObject( &btnSeparator_ );
				dc.MoveTo( xpos + itemSize, ypos + 4 );
				dc.LineTo( xpos + itemSize, ypos + TABBAR_HEIGHT - 4 );
			}
			dc.SetTextColor( textInactive );
		}

		// draw icon
		if ( iconSize && itemSize > iconSize )
			DrawIconEx( dc.GetSafeHdc(),
				xpos + TABBAR_MARGIN, ypos + TABBAR_MARGIN,
				(*i).icon, ICON_SIZE, ICON_SIZE, 0, NULL, DI_NORMAL );

		// draw text
		int centerOff = 0;
		if ( numLines_ != 1 )
			centerOff = max( 0, (int)( itemSize - dc.GetTextExtent( caption ).cx - iconSize - TABBAR_MARGIN*2 ) / 2 );

		CRect txtRect(
			xpos + centerOff + TABBAR_MARGIN + iconSize, ypos + TABBAR_MARGIN + 1,
			xpos + centerOff + itemSize - TABBAR_MARGIN + 1, ypos + TABBAR_HEIGHT - TABBAR_MARGIN );
		dc.DrawText( CString( (*i).caption.c_str() ) , &txtRect, DT_SINGLELINE | DT_LEFT /*| DT_END_ELLIPSIS*/ );

		// move current item position
		(*i).curLeft = xpos;
		xpos += itemSize;
		(*i).curRight = xpos;
		(*i).curTop = ypos;
		++count;
	}

	// Restore old DC objects
	dc.SelectObject( oldPen );
	dc.SelectObject( oldFont );
	dc.SetBkMode( oldBkMode );
	dc.SetTextColor( oldColor );
}

TabCtrlItem* TabCtrl::getItem( int idx )
{
	TabCtrlItem* ret = 0;
	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( idx == count++ )
		{
			ret = &(*i);
			break;
		}
	}

	return ret;
}

TabCtrlItem* TabCtrl::getItem( void* itemData )
{
	TabCtrlItem* ret = 0;

	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			ret = &(*i);
			break;
		}
	}

	return ret;
}

TabCtrlItem* TabCtrl::getItem( int x, int y )
{
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).curLeft <= x && (*i).curRight >= x &&
			(*i).curTop <= y && (*i).curTop + TABBAR_HEIGHT >= y )
			return &(*i);
	}
	return 0;
}

void TabCtrl::setCurItem( int x, int y )
{
	void* itemData = 0;
	TabCtrlItem* item = getItem( x, y );
	if ( item )
		itemData = item->itemData;
	setCurItem( itemData );
}

void TabCtrl::setCurItem( void* itemData )
{
	int count = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
	{
		if ( (*i).itemData == itemData )
		{
			curItem_ = count;
			paint();
			break;
		}
		++count;
	}
}

int TabCtrl::getItemIndex( void* itemData )
{
	int cnt = 0;
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, ++cnt )
		if ( (*i).itemData == itemData )
			return cnt;

	return -1;
}

int TabCtrl::getItemIndex( void* itemData, int x, int y )
{
	// try to find the item that contains the x.y coords
	int count = 0;
	bool passedCurItem = false;
	int itemsPerLine = (int)ceil( (float)itemList_.size() / (float)numLines_ );
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i, ++count )
	{
		if ( x >= (*i).curLeft && x <= (*i).curRight &&
			y >= (*i).curTop && y <= (*i).curTop + TABBAR_HEIGHT )
		{
			// found
			if ( ( x >= ( (*i).curRight + (*i).curLeft ) /2 &&				// if its at right...
				!( !passedCurItem && ( count + 1 ) % itemsPerLine == 0 ) ) ||		// and NOT (it's before the cur item and it's end of line)...
				( passedCurItem && count % itemsPerLine == 0 ) )			// or it's past the cur item and its at the begining of the line
			{
				// if it's in the right half of the item, it means "insert after" (add 1)
				++count;
			}
			break;
		}
		if ( (*i).itemData == itemData )
			passedCurItem = true;
	}

	return count;
}

void TabCtrl::updateItemPosition( void* itemData, int x, int y )
{
	if ( !itemData || !getItem( itemData ) )
		return;

	// save item's data
	TabCtrlItem item = *getItem( itemData );

	updateItemPosition( itemData, getItemIndex( itemData, x, y ) );
}

void TabCtrl::updateItemPosition( void* itemData, int index )
{
	if ( !itemData || !getItem( itemData ) )
		return;

	// get an iterator to it in order to erase from the list later
	TabCtrlItemItr j = itemList_.begin();
	for( ; j != itemList_.end(); ++j )
		if ( (*j).itemData == itemData )
			break;

	// get the iterator for the insert index corresponding to the coordinates
	int count = 0;
	TabCtrlItemItr i = itemList_.begin();
	for( ; i != itemList_.end(); ++i, ++count )
		if ( count == index )
			break;

	// if over the same item that's being moved, the leave it there
	if ( i == j )
		return;

	// do the move:
	// save old's item data to later find position in array
	TabCtrlItem item = *getItem( itemData );
	if ( i != itemList_.end() )
	{
		// save old item's itemdata as an id, and erase the item to be moved
		void* indexItemData = (*i).itemData;
		itemList_.erase( j );

		// found the 'index' item, so insert the itemData item before it
		// find the position of the item to be inserted into
		count = 0;
		// iterators might have been invalidated by erase, so iterate again
		for( j = itemList_.begin(); j != itemList_.end(); ++j, ++count )
			if ( (*j).itemData == indexItemData )
				break;
		// insert it into the new position
		curItem_ = count;
		itemList_.insert( j, item );
	}
	else
	{
		// erase item from the old position
		itemList_.erase( j );
		// 'index' out of range, so assume positioning as the last tab
		curItem_ = (int)itemList_.size();
		itemList_.push_back( item );
	}
	// redraw with the paint multilineSwap flag to false, so it doesn't
	// try to put the current tab down while the user is reordering tabs
	paint( false );
}

void TabCtrl::updateItemData( void* itemData, const std::string& caption, HICON icon )
{
	for( TabCtrlItemItr i = itemList_.begin(); i != itemList_.end(); ++i )
		if ( (*i).itemData == itemData )
		{
			(*i).caption = caption;
			(*i).icon = icon;
			break;
		}
}

BEGIN_MESSAGE_MAP(TabCtrl,CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_SIZE()
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()



void TabCtrl::OnPaint()
{
	CWnd::OnPaint();

	paint();
}

void TabCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	setCurItem( point.x, point.y );
	CWnd::OnLButtonDown( nFlags, point );
	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( curItem_ );
		if ( item )
			eventHandler_->clickedTab( item->itemData, point.x, point.y );
	}
}

void TabCtrl::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize( nType, cx, cy );
	paint();
}

void TabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( curItem_ );
		eventHandler_->doubleClickedTab( item->itemData, point.x, point.y );
	}
}

void TabCtrl::OnRButtonDown(UINT nFlags, CPoint point)
{
	CWnd::OnRButtonDown( nFlags, point );
	if ( eventHandler_ )
	{
		TabCtrlItem* item = getItem( point.x, point.y );
		if ( item )
			eventHandler_->rightClickedTab( item->itemData, point.x, point.y );
	}
}



}	// namespace
