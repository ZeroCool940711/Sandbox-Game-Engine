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
#include "gui_toolbar.hpp"
#include "gui_bitmap.hpp"
#include "resmgr/string_provider.hpp"
#include <malloc.h>
#include <algorithm>
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT( 0 );


// NOTE: the original function is got from MFC : AfxGetGrayBitmap
// I have modified it into WIN32
BEGIN_GUI_NAMESPACE

static const UINT WM_CGUITOOLBAR_REGISTER = RegisterWindowMessage( "WM_CGUITOOLBAR_REGISTER_APP" );
static const UINT WM_CGUITOOLBAR_UNREGISTER = RegisterWindowMessage( "WM_CGUITOOLBAR_UNREGISTER" );
static const UINT WM_CGUITOOLBAR_USE_WRAP = RegisterWindowMessage( "WM_CGUITOOLBAR_USE_WRAP" );


////////////////////////////////////////////////////////
// CGUIToolBar
////////////////////////////////////////////////////////

CGUIToolBar::CGUIToolBar() :
	vertical_( false ),
	guiToolbar_( NULL ),
	refreshOnUpdateCmd_( false )
{
}

CGUIToolBar::~CGUIToolBar()
{
}

void CGUIToolBar::OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler )
{
	if ( !refreshOnUpdateCmd_ )
		return;

	refreshOnUpdateCmd_ = false;

	if ( guiToolbar_ )
	{
		guiToolbar_->forceChanged();
		RedrawWindow( NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ERASE );
	}
}

BEGIN_MESSAGE_MAP( CGUIToolBar, CToolBar )
END_MESSAGE_MAP()

BOOL CGUIToolBar::OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult )
{
	if ( message == WM_CGUITOOLBAR_USE_WRAP )
	{
		*pResult = IsFloating() || vertical_;
		return TRUE;
	}
	else if ( message == WM_CGUITOOLBAR_REGISTER )
	{
		guiToolbar_ = (Toolbar*)lParam;
	}
	else if ( message == WM_CGUITOOLBAR_UNREGISTER )
	{
		guiToolbar_ = NULL;
	}
	else if( message == WM_SYSCOLORCHANGE )
	{
		if ( guiToolbar_ )
			guiToolbar_->onSysColorChange();
	}

	return CToolBar::OnWndMsg( message, wParam, lParam, pResult );
}

CSize CGUIToolBar::CalcDynamicLayout( int nLength, DWORD nMode )
{
	CSize size = CToolBar::CalcDynamicLayout( nLength, nMode );

	if ( nMode & LM_HORZ )
		vertical_ = false;
	else
		vertical_ = true;

	refreshOnUpdateCmd_ = true;

	if ( guiToolbar_ )
		guiToolbar_->restoreText();	// for some reason the toolbar name gets cleared

	return size;
}

////////////////////////////////////////////////////////
// Toolbar
////////////////////////////////////////////////////////

Toolbar::Toolbar( const std::string& root, HWND toolbar, unsigned int iconSize /* = 16 */ ) :
	Subscriber( root ),
	toolbar_( toolbar ),
	forceChanged_( false )
{
	sendMessage( WM_CGUITOOLBAR_REGISTER, 0, (LPARAM)this );
	sendMessage( TB_AUTOSIZE , 0, 0 );
	sendMessage( TB_SETEXTENDEDSTYLE, 0,
		sendMessage( TB_GETEXTENDEDSTYLE, 0, 0 ) | TBSTYLE_EX_MIXEDBUTTONS );
	SetWindowLong( toolbar, GWL_STYLE,
		GetWindowLong( toolbar, GWL_STYLE ) | TBSTYLE_LIST );

	disabledImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );
	hotImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );
	normalImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );

	// destroy it
	while( sendMessage( TB_BUTTONCOUNT, 0, 0 ) )
		sendMessage( TB_DELETEBUTTON , 0, 0 );

	HIMAGELIST imglist;
	imglist = (HIMAGELIST)sendMessage( TB_SETIMAGELIST, 0, (LPARAM)normalImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );
	imglist = (HIMAGELIST)sendMessage( TB_SETHOTIMAGELIST, 0, (LPARAM)hotImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );
	imglist = (HIMAGELIST)sendMessage( TB_SETDISABLEDIMAGELIST, 0, (LPARAM)disabledImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );

	prevWndProc_ = (WNDPROC)GetWindowLong( toolbar_, GWL_WNDPROC );
	subclassMap()[ toolbar_ ] = this;
	SetWindowLong( toolbar_, GWL_WNDPROC, (LONG)subclassProc );
}

Toolbar::~Toolbar()
{
	SetWindowLong( toolbar_, GWL_WNDPROC, (LONG)prevWndProc_ );
	subclassMap().erase( subclassMap().find( toolbar_ ) );
	sendMessage( WM_CGUITOOLBAR_REGISTER, 0, 0 );
	ImageList_Destroy( (HIMAGELIST)sendMessage( TB_GETIMAGELIST, 0, 0 ) );
	ImageList_Destroy( (HIMAGELIST)sendMessage( TB_GETHOTIMAGELIST, 0, 0 ) );
	ImageList_Destroy( (HIMAGELIST)sendMessage( TB_GETDISABLEDIMAGELIST, 0, 0 ) );
}

LRESULT Toolbar::sendMessage( UINT msg, WPARAM wparam, LPARAM lparam )
{
	return ( result_ = SendMessage( toolbar_, msg, wparam, lparam ) );
}

int Toolbar::getImageIndex( ItemPtr item )
{
	if( imageIndices_.find( item.getObject() ) == imageIndices_.end() )
	{
		COLORREF transparency = RGB( 192, 192, 192 );
		if( item->exist( "transparency" ) )
		{
			std::string color = (*item)[ "transparency" ];
			if( std::count( color.begin(), color.end(), ',' ) == 2 )
			{
				BYTE r = atoi( color.c_str() );
				BYTE g = atoi( color.c_str() + color.find( ',' ) + 1 );
				std::string::size_type next = color.find( ',' ) + 1;
				BYTE b = atoi( color.c_str() + color.find( ',', next ) + 1 );
				transparency = RGB( r, g, b );
			}
		}
		BitmapPtr bitmap = Manager::instance().bitmaps().get( (*item)["imageNormal"], transparency );
		result_ = ImageList_Add( normalImageList_, (*bitmap), NULL );
		if( item->exist( "imageHot" ) )
			ImageList_Add( hotImageList_,
				*Manager::instance().bitmaps().get( (*item)["imageHot"], transparency ), NULL );
		else
			ImageList_Add( hotImageList_,
				*Manager::instance().bitmaps().get( (*item)["imageNormal"], transparency, "HOVER" ), NULL );

		if( item->exist( "imageDisabled" ) )
			ImageList_Add( disabledImageList_,
				*Manager::instance().bitmaps().get( (*item)["imageDisabled"], transparency ), NULL );
		else
			ImageList_Add( disabledImageList_,
				*Manager::instance().bitmaps().get( (*item)["imageNormal"], transparency, "DISABLED" ), NULL );
		imageIndices_[ item.getObject() ] = result_;
	}
	return imageIndices_[ item.getObject() ];
}

void Toolbar::changed( ItemPtr item )
{// normally a toolbar only has several buttons ( less than 30 ), always do a full refresh
	unsigned int index = 0;
	changed( index, rootItem() );
	++index;
	while( index < (unsigned int)sendMessage( TB_BUTTONCOUNT, 0, 0 ) )
	{
		static const unsigned int MAX_MENU_TEXT = 1024;
		char txtBuf[ MAX_MENU_TEXT + 1 ];
		TBBUTTONINFO info;
		memset( &info, 0, sizeof( info ) );
		info.cbSize = sizeof( info );
		info.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_IMAGE | TBIF_LPARAM
			| TBIF_SIZE | TBIF_STATE | TBIF_STYLE | TBIF_TEXT;
		info.pszText = txtBuf;
		txtBuf[0] = 0;
		info.cchText = MAX_MENU_TEXT;
		sendMessage( TB_GETBUTTONINFO, index, (LPARAM)&info );
		if( ( info.fsStyle & BTNS_SEP ) && info.lParam )
			DestroyWindow( (HWND)info.lParam );
		sendMessage( TB_DELETEBUTTON , index, 0 );
	}
	restoreText();
	UpdateWindow( toolbar_ );
}

void Toolbar::changed( unsigned int& index, ItemPtr item )
{
	static const unsigned int MAX_MENU_TEXT = 1024;
	char txtBuf[ MAX_MENU_TEXT + 1 ];
	for( unsigned int i = 0; i < item->num(); ++i, ++index )
	{
		TBBUTTONINFO info;
		memset( &info, 0, sizeof( info ) );
		info.cbSize = sizeof( info );
		info.dwMask = TBIF_BYINDEX | TBIF_COMMAND | TBIF_IMAGE | TBIF_LPARAM
			| TBIF_SIZE | TBIF_STATE | TBIF_STYLE | TBIF_TEXT;
		info.pszText = txtBuf;
		txtBuf[0] = 0;
		info.cchText = MAX_MENU_TEXT;

		sendMessage( TB_GETBUTTONINFO, index, (LPARAM)&info );

		ItemPtr subItem = ( *item )[ i ];
		if( subItem->type() == "SEPARATOR" )
			updateSeparator( index, subItem, info );
		else if( subItem->type() == "GROUP" )
			updateGroup( index, subItem, info );
		else if( subItem->type() == "ACTION" )
			updateAction( index, subItem, info );
		else if( subItem->type() == "TOGGLE" )
			updateToggle( index, subItem, info );
		else if( subItem->type() == "CHOICE" )
			updateChoice( index, subItem, info );
		else if( subItem->type() == "EXPANDED_CHOICE" )
			updateExpandedChoice( index, subItem, info );
	}
	--index;
}

void Toolbar::updateSeparator( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	int width = item->exist( "width" )	?	atoi( (*item)[ "width" ].c_str() )	:
											6;
	if( ( info.fsStyle & BTNS_SEP ) == 0 //  not a separator
		|| info.lParam != 0 // a place-holder separator
		|| info.iImage != width ) // width, will be configurable in future
	{
		TBBUTTON button;
		memset(	&button, 0, sizeof(button) );
		button.iBitmap = width; // width of the separatar, could be configurable in future
		button.idCommand = 0;
		button.fsState = 0;
		button.fsStyle = BTNS_SEP;
		button.dwData = 0;
		button.iString = -1;

		sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );
	}
	TBBUTTONINFO buttonInfo = { sizeof( buttonInfo ), TBIF_BYINDEX | TBIF_STATE };
	buttonInfo.fsState = 0;
	if ( sendMessage( WM_CGUITOOLBAR_USE_WRAP, 0, 0 ) &&
		( info.fsState & TBSTATE_WRAP ) )
		buttonInfo.fsState |= TBSTATE_WRAP;

	if( forceChanged_ || info.fsState != buttonInfo.fsState )
		sendMessage( TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo );
}

void Toolbar::updateGroup( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	changed( index, item );
}

void Toolbar::updateAction( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	unsigned int mask = BTNS_SEP | BTNS_CHECK | BTNS_GROUP | BTNS_CHECKGROUP | BTNS_DROPDOWN;
	if( ( info.fsStyle & mask ) != 0 || // not a push button
		info.idCommand != item->commandID() ) // not for me
	{
		
		TBBUTTON button;
		memset(	&button, 0, sizeof(button) );
		button.iBitmap = getImageIndex( item );
		button.idCommand = item->commandID();
		button.fsState = TBSTATE_ENABLED;
		button.fsStyle = BTNS_BUTTON;
		button.dwData = 0;
		button.iString = -1;

		sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );
	}
	TBBUTTONINFO buttonInfo = { sizeof( buttonInfo ), TBIF_TEXT | TBIF_BYINDEX | TBIF_STATE };
	std::string tooltip = item->description();	// auto tooltip
	if( item->shortcutKey().size() )
	{
		tooltip += " (";
		tooltip += item->shortcutKey();
		tooltip += ')';
	}
	buttonInfo.pszText = &tooltip[0];
	buttonInfo.fsState = item->update()	&& isEnabled() ?	TBSTATE_ENABLED : 0;
	if ( sendMessage( WM_CGUITOOLBAR_USE_WRAP, 0, 0 ) &&
		( info.fsState & TBSTATE_WRAP ) )
		buttonInfo.fsState |= TBSTATE_WRAP;

	if( forceChanged_ || info.pszText != tooltip || info.fsState != buttonInfo.fsState )
		sendMessage( TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo );
}

void Toolbar::updateToggle( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	if( ( info.fsStyle & BTNS_CHECK ) == 0 || // not a toggle button
		info.idCommand != item->commandID() ) // not for me
	{
		TBBUTTON button;
		memset(	&button, 0, sizeof(button) );
		button.iBitmap = getImageIndex( item );
		button.idCommand = item->commandID();
		button.fsState = TBSTATE_ENABLED;
		button.fsStyle = BTNS_CHECK;
		button.dwData = (DWORD)item.getObject();
		button.iString = -1;
		sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );
	}
	TBBUTTONINFO buttonInfo = { sizeof( buttonInfo ), TBIF_TEXT | TBIF_BYINDEX | TBIF_STATE };
	std::string tooltip = item->description();	// auto tooltip
	if( item->shortcutKey().size() )
	{
		tooltip += " (";
		tooltip += item->shortcutKey();
		tooltip += ')';
	}
	buttonInfo.pszText = &tooltip[0];
	LPARAM enabled = item->update()	&& isEnabled() ?	TBSTATE_ENABLED : 0;
	LPARAM checked = ( *item )[ 0 ]->update()	?	0 : TBSTATE_CHECKED;
	if( !enabled )
		checked = 0;
	buttonInfo.fsState = (BYTE)( enabled | checked );
	if ( sendMessage( WM_CGUITOOLBAR_USE_WRAP, 0, 0 ) &&
		( info.fsState & TBSTATE_WRAP ) )
		buttonInfo.fsState |= TBSTATE_WRAP;

	if( forceChanged_ || info.pszText != tooltip || info.fsState != buttonInfo.fsState )
		sendMessage( TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo );
}

void Toolbar::updateChoice( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	unsigned int i = 0;
	while( index < (unsigned int)sendMessage( TB_BUTTONCOUNT, 0, 0 ) &&
		i < item->num() )
	{
		static const unsigned int MAX_MENU_TEXT = 1024;
		char txtBuf[ MAX_MENU_TEXT + 1 ];

		TBBUTTONINFO info = { sizeof( info ), TBIF_BYINDEX | TBIF_COMMAND | TBIF_IMAGE
			| TBIF_LPARAM | TBIF_SIZE | TBIF_STATE | TBIF_STYLE | TBIF_TEXT };
		info.pszText = txtBuf;
		info.cchText = MAX_MENU_TEXT;

		sendMessage( TB_GETBUTTONINFO, index, (LPARAM)&info );

		ItemPtr subItem = (*item)[ i ];

		if( ( info.fsStyle & BTNS_CHECK ) == 0 || // not a toggle button
			info.idCommand != subItem->commandID() ) // not for me
		{
			TBBUTTON button;
			memset(	&button, 0, sizeof(button) );
			button.iBitmap = getImageIndex( subItem );
			button.idCommand = subItem->commandID();
			button.fsState = TBSTATE_ENABLED;
			button.fsStyle = BTNS_CHECK;
			button.dwData = 0;
			button.iString = -1;
			sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );
		}
		TBBUTTONINFO buttonInfo = { sizeof( buttonInfo ), TBIF_TEXT | TBIF_BYINDEX | TBIF_STATE };
		std::string tooltip = subItem->description();	// auto tooltip
		if( subItem->shortcutKey().size() || item->shortcutKey().size() )
		{
			if (subItem->shortcutKey().size() && item->shortcutKey().size())
			{
				tooltip = 
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM0", 
						subItem->description(),
						subItem->shortcutKey(),
						item->shortcutKey()
					);
			}
			else if (subItem->shortcutKey().size())
			{
				tooltip =
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM1",
						subItem->description(),
						subItem->shortcutKey()
					);
			}
			else if (item->shortcutKey().size())
			{
				tooltip =
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM2",
						subItem->description(),
						item->shortcutKey()
					);
			}
		}
		buttonInfo.pszText = &tooltip[0];
		LPARAM enabled = item->update()	&& isEnabled() ?	TBSTATE_ENABLED : 0;
		LPARAM checked = subItem->update()	?	TBSTATE_CHECKED : 0;
		if( !enabled )
			checked = 0;
		buttonInfo.fsState = (BYTE)( enabled | checked );
		if ( sendMessage( WM_CGUITOOLBAR_USE_WRAP, 0, 0 ) &&
			( info.fsState & TBSTATE_WRAP ) )
			buttonInfo.fsState |= TBSTATE_WRAP;

		if( forceChanged_ || info.pszText != tooltip || info.fsState != buttonInfo.fsState )
			sendMessage( TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo );
		++i;
		++index;
	}
	while( i < item->num() )
	{
		ItemPtr subItem = (*item)[ i ];

		TBBUTTON button;
		memset(	&button, 0, sizeof(button) );
		button.iBitmap = getImageIndex( subItem );
		button.idCommand = subItem->commandID();
		button.fsState = TBSTATE_ENABLED;
		button.fsStyle = BTNS_CHECK;
		button.dwData = 0;
		button.iString = -1;
		sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );

		TBBUTTONINFO buttonInfo = { sizeof( buttonInfo ), TBIF_TEXT | TBIF_BYINDEX | TBIF_STATE };
		std::string tooltip = subItem->description();	// auto tooltip
		if( subItem->shortcutKey().size() || item->shortcutKey().size() )
		{
			if (subItem->shortcutKey().size() && item->shortcutKey().size())
			{
				tooltip = 
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM0", 
						subItem->description(),
						subItem->shortcutKey(),
						item->shortcutKey()
					);
			}
			else if (subItem->shortcutKey().size())
			{
				tooltip =
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM1",
						subItem->description(),
						subItem->shortcutKey()
					);
			}
			else if (item->shortcutKey().size())
			{
				tooltip =
					L
					(
						"GUIMANAGER/GUI_TOOLBAR/TOOLTIP_FORM2",
						subItem->description(),
						item->shortcutKey()
					);
			}
		}
		buttonInfo.pszText = &tooltip[0];
		LPARAM enabled = item->update()	&& isEnabled() ?	TBSTATE_ENABLED : 0;
		LPARAM checked = subItem->update()	?	TBSTATE_CHECKED : 0;
		if( !enabled )
			checked = 0;
		buttonInfo.fsState = (BYTE)( enabled | checked );
		if ( sendMessage( WM_CGUITOOLBAR_USE_WRAP, 0, 0 ) &&
			( info.fsState & TBSTATE_WRAP ) )
			buttonInfo.fsState |= TBSTATE_WRAP;

		if( forceChanged_ || info.pszText != tooltip || info.fsState != buttonInfo.fsState )
			sendMessage( TB_SETBUTTONINFO, index, (LPARAM)&buttonInfo );
		++i;
		++index;
	}
	--index;
}

void Toolbar::updateExpandedChoice( unsigned int& index, ItemPtr item, TBBUTTONINFO& info )
{
	struct PatchDisplayName
	{
		std::string operator()( std::string name )
		{
			std::string::iterator iter = name.begin();
			while( iter != name.end() )
			{
				if( *iter == '&' )
					name.erase( iter );
				else
					++iter;
			}
			return name;
		}
	}
	PatchDisplayName;
	HWND combobox = (HWND)info.lParam;
	if( ( info.fsStyle & BTNS_SEP ) == 0 || // not a place holder
		info.lParam == 0 ) // do not contain a combo box
	{
		int width = item->exist( "width" )	?	atoi( (*item)[ "width" ].c_str() )	:
												150;
		combobox = CreateWindow( "COMBOBOX", "",
			CBS_AUTOHSCROLL | CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE,
			0, 0, width, 1,
			toolbar_, NULL, GetModuleHandle( NULL ), NULL );
		SendMessage( combobox, WM_SETFONT,
			SendMessage( toolbar_, WM_GETFONT, 0, 0 ),
			FALSE );
		TBBUTTON button;
		memset(	&button, 0, sizeof(button) );
		button.iBitmap = width; // width of the combobox, MUST be configurable in future
		button.idCommand = 0;
		button.fsState = 0;
		button.fsStyle = BTNS_SEP;
		button.dwData = (DWORD)combobox;
		button.iString = -1;

		sendMessage( TB_INSERTBUTTON, index, (LPARAM)&button );
	}
	{// set the position & size
		RECT rectButton, rectWin;
		sendMessage( TB_GETITEMRECT, index, (LPARAM)&rectButton );
		GetWindowRect( combobox, &rectWin );
		ScreenToClient( toolbar_, (POINT*)&rectWin );
		MoveWindow( combobox, rectButton.left, rectButton.top,
			rectButton.right - rectButton.left, ( rectButton.bottom - rectButton.top ) * 20, TRUE );
	}
	int comboIndex = 0;
	unsigned int subitemIndex = 0;
	while( comboIndex < SendMessage( combobox, CB_GETCOUNT, 0, 0 ) &&
		subitemIndex < item->num() )
	{
		ItemPtr subitem = (*item)[ subitemIndex ];
		char* buffer = (char*)_alloca( SendMessage( combobox, CB_GETLBTEXTLEN, comboIndex, 0 ) + 1 );
		SendMessage( combobox, CB_GETLBTEXT, comboIndex, (LPARAM)buffer );
		if( SendMessage( combobox, CB_GETITEMDATA, comboIndex, 0 ) != subitem->commandID() ||
			buffer != subitem->displayName() )
		{
			SendMessage( combobox, CB_INSERTSTRING, comboIndex,
				(LPARAM)PatchDisplayName( subitem->displayName() ).c_str() );
			SendMessage( combobox, CB_SETITEMDATA, comboIndex, subitem->commandID() );
		}
		if( subitem->update() && !SendMessage( combobox, CB_GETDROPPEDSTATE, 0, 0 ) )
			SendMessage( combobox, CB_SETCURSEL, comboIndex, 0 );
		++comboIndex;
		++subitemIndex;
	}
	while( comboIndex < SendMessage( combobox, CB_GETCOUNT, 0, 0 ) )
		SendMessage( combobox, CB_DELETESTRING, comboIndex, 0 );
	while( subitemIndex < item->num() )
	{
		ItemPtr subitem = (*item)[ subitemIndex ];
		SendMessage( combobox, CB_INSERTSTRING, comboIndex,
			(LPARAM)PatchDisplayName( subitem->displayName() ).c_str() );
		SendMessage( combobox, CB_SETITEMDATA, comboIndex, subitem->commandID() );
		if( subitem->update()  && !SendMessage( combobox, CB_GETDROPPEDSTATE, 0, 0 ) )
			SendMessage( combobox, CB_SETCURSEL, comboIndex, 0 );
		++comboIndex;
		++subitemIndex;
	}
	EnableWindow( combobox, item->update() && item->num() && isEnabled() );
}

std::map<HWND,Toolbar*>& Toolbar::subclassMap()
{
	static std::map<HWND,Toolbar*> subclassMap;
	return subclassMap;
}

LRESULT Toolbar::subclassProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
	return subclassMap()[ hwnd ]->wndProc( msg, wparam, lparam );
}

LRESULT Toolbar::wndProc( UINT msg, WPARAM wparam, LPARAM lparam )
{
	if( msg == WM_COMMAND && HIWORD( wparam ) == CBN_SELCHANGE )
	{
		HWND combobox = (HWND)lparam;
		int data = SendMessage( combobox, CB_GETITEMDATA,
			SendMessage( combobox, CB_GETCURSEL, 0, 0 ), 0 );
		int index = SendMessage( combobox, CB_GETCURSEL, 0, 0 );
		if( data != CB_ERR )
			Manager::instance().act( data );
	}
	return CallWindowProc( prevWndProc_, toolbar_, msg, wparam, lparam );
}

void Toolbar::forceChanged()
{
	forceChanged_ = true;
	changed( rootItem() );
	forceChanged_ = false;
}


bool Toolbar::isEnabled()
{
	return !( GetWindowLong( toolbar_, GWL_STYLE ) & WS_DISABLED );
}

void Toolbar::staticInit()
{
	subclassMap();
}

/*static*/
ItemPtr Toolbar::getToolbarsItem( const std::string appTbsSection )
{
	ItemPtr appTbsItem;
	for( int i = 0; i < (int)Manager::instance().num(); ++i )
	{
		if ( Manager::instance()[ i ]->name() == appTbsSection )
		{
			appTbsItem = Manager::instance()[ i ];
			break;
		}
	}
	return appTbsItem;
}

/*static*/
int Toolbar::getToolbarsCount( const std::string appTbsSection )
{
	ItemPtr appTbsItem = getToolbarsItem( appTbsSection );
	if ( !appTbsItem )
		return 0;

	return appTbsItem->num();
}

/*static*/
bool Toolbar::createToolbars(
	const std::string appTbsSection, const HwndVector& appTbsData, int iconSize /*=16*/ )
{
	ItemPtr appTbsItem = getToolbarsItem( appTbsSection );
	if ( !appTbsItem )
		return false;

	HwndVector::const_iterator hwndIt = appTbsData.begin();
	for( int i = 0; i < (int)appTbsItem->num() && hwndIt != appTbsData.end(); ++i, ++hwndIt )
	{
		Manager::instance().add(
			new Toolbar( (*appTbsItem)[ i ]->name(), *hwndIt, iconSize ) );
	}

	return true;
}

Toolbar::operator HWND()
{
	return toolbar_;
}

SIZE Toolbar::minimumSize()
{
	RECT start, end, sum = { 0 };
	unsigned int totalNum = sendMessage( TB_BUTTONCOUNT, 0, 0 );
	if( totalNum != 0 )
	{
		if( totalNum == 1 )
			sendMessage( TB_GETITEMRECT, 0, (LPARAM)&sum );
		else
		{
			sendMessage( TB_GETITEMRECT, 0, (LPARAM)&start );
			sendMessage( TB_GETITEMRECT, totalNum - 1, (LPARAM)&end );
			UnionRect( &sum, &start, &end );
		}
	}
	SIZE result = { sum.right - sum.left, sum.bottom - sum.top };
	return result;
}

void Toolbar::onSysColorChange()
{
	// the following line is used to hack the toolbar background color for tradeshow
	return;
	int iconSize;
	ImageList_GetIconSize( disabledImageList_, &iconSize, &iconSize );
	disabledImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );
	hotImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );
	normalImageList_ = ImageList_Create( iconSize, iconSize, ILC_COLOR32, 1, 256 );

	// destroy it
	while( sendMessage( TB_BUTTONCOUNT, 0, 0 ) )
		sendMessage( TB_DELETEBUTTON , 0, 0 );

	HIMAGELIST imglist;
	imglist = (HIMAGELIST)sendMessage( TB_SETIMAGELIST, 0, (LPARAM)normalImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );
	imglist = (HIMAGELIST)sendMessage( TB_SETHOTIMAGELIST, 0, (LPARAM)hotImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );
	imglist = (HIMAGELIST)sendMessage( TB_SETDISABLEDIMAGELIST, 0, (LPARAM)disabledImageList_ );
	if( imglist )
		ImageList_Destroy( imglist );

	imageIndices_.clear();
	Manager::instance().bitmaps().clear();

	unsigned int index = 0;
	changed( index, rootItem() );
}

void Toolbar::restoreText()
{
	SetWindowText( toolbar_, rootItem()->displayName().c_str() );
}

END_GUI_NAMESPACE
