/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef __BASE_MAINFRAME_HPP__
#define __BASE_MAINFRAME_HPP__

#include <afxpriv.h>
#include "guimanager/gui_toolbar.hpp"
#include "resmgr/string_provider.hpp"


/**
 *	This class manages the automated creation of toolbars in a CFrameWnd,
 *  and provides a mechanism to verify bar state data before calling the
 *  LoadBarState method (which can crash/assert if the state is not valid).
 *  It also fills a menu item named ShowToolbarsSubmenu with the loaded
 *  toolbar names, and provides methods to respond to the events the subitems
 *  filled in defined. The subitems define the actions doShowToolbar and
 *  doHideToolbar, and the updater updateToolbar, and the item passed in to
 *  these handlers contain a value toolbarIndex. The application is responsible
 *  for binding these methods to python or C++ through the GUI manager.
 */
class BaseMainFrame : public CFrameWnd
{
public:
	BaseMainFrame() :
		toolbars_( NULL ),
		numToolbars_( 0 ),
		tbId_( AFX_IDW_CONTROLBAR_LAST-1 )
	{
	}

	virtual ~BaseMainFrame()
	{
		delete [] toolbars_;
	}

	/**
	 *	This method should be called before calling LoadBarState, in order to
	 *  verify that the bar state you are about to load is actually valid.
	 *  Otherwise, the brilliant LoadBarState method will assert or crash.
	 */
	virtual BOOL verifyBarState(LPCTSTR lpszProfileName)
	{
		CDockState state;
		state.LoadState(lpszProfileName);

		for (int i = 0; i < state.m_arrBarInfo.GetSize(); i++)
		{
			CControlBarInfo* pInfo = (CControlBarInfo*)state.m_arrBarInfo[i];
			ASSERT(pInfo != NULL);
			int nDockedCount = pInfo->m_arrBarID.GetSize();
			if (nDockedCount > 0)
			{
				// dockbar
				for (int j = 0; j < nDockedCount; j++)
				{
					UINT nID = (UINT) pInfo->m_arrBarID[j];
					if (nID == 0) continue; // row separator
					if (nID > 0xFFFF)
						nID &= 0xFFFF; // placeholder - get the ID
					if (GetControlBar(nID) == NULL)
						return FALSE;
				}
			}
	        
			if (!pInfo->m_bFloating) // floating dockbars can be created later
				if (GetControlBar(pInfo->m_nBarID) == NULL)
					return FALSE; // invalid bar ID
		}

		return TRUE;
	}


	/**
	 *	This method creates all the toolbars specified in the gui xml file
	 *  and links them to GUI::Toolbars.
	 */
	virtual bool createToolbars(
		const std::string appTbsSection,
		DWORD ctrlStyle = TBSTYLE_FLAT,
		DWORD style =
			WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER |
			CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
		CSize btnSize = CSize( 32, 30 ),
		int imgSize = 24 )
	{
		// initialise the toolbar array with the # of toolbars
		numToolbars_ = GUI::Toolbar::getToolbarsCount( appTbsSection );
		if ( !numToolbars_ )
			return false;

		toolbars_ = new GUI::CGUIToolBar[ numToolbars_ ];

		// initialise the windows toolbars
		for (unsigned int i=0; i < numToolbars_; i++)
		{
			if (!toolbars_[i].CreateEx(this,
				ctrlStyle,
				style,
				CRect( 0, 0, 0, 0 ), tbId_-- ) )
			{
				TRACE0(L("COMMON/BASE_MAINFRAME/CREATETOOLBARS"));
				return false;      // fail to create
			}
			toolbars_[i].SetSizes( btnSize, CSize( imgSize, imgSize ) );
			toolbars_[i].EnableDocking( CBRS_ALIGN_ANY );
		}
		defaultToolbarLayout( false );

		// link them to GUI manager
		GUI::Toolbar::HwndVector hwnds;
		fillToolbarsVector( hwnds );
		GUI::Toolbar::createToolbars( appTbsSection, hwnds, imgSize );

		RecalcLayout();

		// create toolbars menu option
		GUI::ItemPtr menu = GUI::Manager::instance().findByName( "ShowToolbarsSubmenu" );
		if( menu )
		{
			while( menu->num() )
				menu->remove( 0 );
			for( unsigned int i = 0; i < numToolbars_; ++i )
			{
				std::stringstream name, displayName;
				name << "showToolbar" << i;
				CString str;
				toolbars_[ i ].GetWindowText( str );
				displayName << (LPCSTR)str;
				GUI::ItemPtr item = new GUI::Item( "TOGGLE", name.str(), displayName.str(),
					"",	"", "", "", "", "" );
				str.Format( "%d", i );
				GUI::ItemPtr subitem1 = 
					new GUI::Item( "CHILD", name.str() + "Up",
						std::string( "show " ) + displayName.str(),
						"",	"", "", "doShowToolbar", "updateToolbar", "" );
				subitem1->set( "toolbarIndex", (LPCSTR)str );
				GUI::ItemPtr subitem2 = 
					new GUI::Item( "CHILD", name.str() + "Down",
						std::string( "show " ) + displayName.str(),
						"",	"", "", "doHideToolbar", "", "" );
				subitem2->set( "toolbarIndex", (LPCSTR)str );
				item->add( subitem1 );
				item->add( subitem2 );
				menu->add( item );
			}
		}

		return true;
	}


	/**
	 *	This method layouts toolbars in a default, left to right docking setup.
	 */
	virtual void defaultToolbarLayout( bool fullLayout = true )
	{
		if ( fullLayout )
		{
			// First undock and show all toolbars
			for (unsigned i=0; i<numToolbars_; i++)
			{
				ShowControlBar( &toolbars_[i], TRUE, FALSE );
				FloatControlBar( &toolbars_[i], CPoint( 200, 100 ) );
			}
		}
		for (unsigned i=0; i<numToolbars_; i++)
		{
			if ( i == 0 )
			{
				// First toolbar, just dock on top
				DockControlBar( &toolbars_[i], AFX_IDW_DOCKBAR_TOP );
			}
			else
			{
				// try to dock the second toolbar to the left of the first
				RecalcLayout();
				CRect rect;
				toolbars_[i-1].GetWindowRect( &rect );
				rect.OffsetRect( 1, -1 );
				DockControlBar( &toolbars_[i], AFX_IDW_DOCKBAR_TOP, &rect );
			}
		}
	}


	/**
	 *	Toolbar array info methods
	 */
	unsigned int numToolbars()
	{
		return numToolbars_;
	}

	GUI::CGUIToolBar* toolbar( unsigned int i )
	{
		if ( i < 0 || i >= numToolbars_ )
			return 0;
		return &toolbars_[ i ];
	}


	/**
	 *	Shows / hide toolbar methods by item
	 */
	virtual bool showToolbar( GUI::ItemPtr item )
	{
		return showToolbarIndex( atoi( (*item)[ "toolbarIndex" ].c_str() ) );
	}

	virtual bool hideToolbar( GUI::ItemPtr item )
	{
		return hideToolbarIndex( atoi( (*item)[ "toolbarIndex" ].c_str() ) );
	}

	virtual unsigned int updateToolbar( GUI::ItemPtr item )
	{
		return updateToolbarIndex( atoi( (*item)[ "toolbarIndex" ].c_str() ) );
	}


	/**
	 *	Shows / hide toolbar methods by index
	 */
	virtual bool showToolbarIndex( int i )
	{
		if ( i >= 0 && i < (int)numToolbars_ )
			ShowControlBar( &toolbars_[i], TRUE, FALSE );
		return true;
	}

	virtual bool hideToolbarIndex( int i )
	{
		if ( i >= 0 && i < (int)numToolbars_ )
			ShowControlBar( &toolbars_[i], FALSE, FALSE );
		return true;
	}

	virtual unsigned int updateToolbarIndex( int i )
	{
		if ( i >= 0 && i < (int)numToolbars_ )
			return ~toolbars_[i].GetStyle() & WS_VISIBLE;
		return 0;
	}

protected:
	/**
	 *	Fills a HwndVector with the HWNDs of the BaseMainFrame's toolbars
	 */
	virtual void fillToolbarsVector( GUI::Toolbar::HwndVector& hwnds )
	{
		for (unsigned int i=0; i < numToolbars_; i++)
			hwnds.push_back( toolbars_[ i ].GetSafeHwnd() );
	}

	// stores the number of toolbars loaded
	unsigned int numToolbars_;

	// Toolbars must inherit from CGUIToolBar for OnIdle and docking to work
	GUI::CGUIToolBar* toolbars_;

	// counter used to assign proper IDs to toolbars to allow proper load/save
	int tbId_;
};

#endif // __BASE_MAINFRAME_HPP__