/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef GUI_TOOLBAR_HPP__
#define GUI_TOOLBAR_HPP__

#include "gui_manager.hpp"
#include <map>

BEGIN_GUI_NAMESPACE

class Toolbar;

// Base class to enable GUI Manager toolbars. Use instead of CToolBar in
// the applications main frame window, or inherit from it.
class CGUIToolBar : public CToolBar
{
public:
	CGUIToolBar();
	virtual ~CGUIToolBar();

protected:
	virtual void OnUpdateCmdUI( CFrameWnd* pTarget, BOOL bDisableIfNoHndler );
	virtual BOOL OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	virtual CSize CalcDynamicLayout( int nLength, DWORD nMode );
	DECLARE_MESSAGE_MAP()
private:
	bool vertical_;
	Toolbar* guiToolbar_;
	bool refreshOnUpdateCmd_;
};


// basically, we doesn't manage the toolbar resource, the application should do that
// we just fill it with different buttons
class Toolbar : public Subscriber
{
	LRESULT result_;
	WNDPROC prevWndProc_;
	std::map<Item*, int> imageIndices_;
	static std::map<HWND,Toolbar*>& subclassMap();
	static LRESULT subclassProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );
protected:
	HIMAGELIST disabledImageList_;
	HIMAGELIST hotImageList_;
	HIMAGELIST normalImageList_;
	HWND toolbar_;
	bool forceChanged_;
	LRESULT sendMessage( UINT msg, WPARAM wparam, LPARAM lparam );
	int getImageIndex( ItemPtr item );
	virtual void changed( unsigned int& index, ItemPtr item );
	virtual void updateSeparator( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual void updateGroup( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual void updateAction( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual void updateToggle( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual void updateChoice( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual void updateExpandedChoice( unsigned int& index, ItemPtr item, TBBUTTONINFO& info );
	virtual LRESULT wndProc( UINT msg, WPARAM wparam, LPARAM lparam );
	virtual void forceChanged();
	bool isEnabled();
	void onSysColorChange();
	void restoreText();

	friend CGUIToolBar;

public:
	Toolbar( const std::string& root, HWND toolbar, unsigned int iconSize = 16 );
	virtual ~Toolbar();
	virtual void changed( ItemPtr item );
	static void staticInit();
	static ItemPtr getToolbarsItem( const std::string appTbsSection );
	static int getToolbarsCount( const std::string appTbsSection );
	typedef std::vector<HWND> HwndVector;
	static bool createToolbars(
		const std::string appTbsSection, const HwndVector& appTbsData, int iconSize =16 );
	operator HWND();
	SIZE minimumSize();
};

END_GUI_NAMESPACE

#endif//GUI_TOOLBAR_HPP__
