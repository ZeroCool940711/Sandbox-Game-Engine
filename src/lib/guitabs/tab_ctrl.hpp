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
 *	GUI Tearoff panel framework - Tab Control widget class
 */

#ifndef GUITABS_TAB_CTRL_HPP
#define GUITABS_TAB_CTRL_HPP

namespace GUITABS
{

/**
 *	This struct keeps info about an item/tab shown in the tab control. It
 *  contains the caption string and icon to be displayed on the corresponding
 *  tab, and a pointer to some item data useful to the user of the control.
 */
struct TabCtrlItem
{
	std::string caption;
	HICON icon;
	void* itemData;
	int curLeft;
	int curRight;
	int curTop;

	TabCtrlItem( const std::string& c, HICON i, void* d ) :
		caption( c ),
		icon( i ),
		itemData( d ),
		curLeft( 0 ),
		curRight( 0 ),
		curTop( 0 )
	{};
};


/**
 *	This class/interface must be inherited by classes that use the tabctrl
 *	class in order to receive events related to this class.
 */
class TabCtrlEventHandler
{
public:
	virtual void clickedTab( void* itemData, int x, int y ) = 0;
	virtual void doubleClickedTab( void* itemData, int x, int y ) = 0;
	virtual void rightClickedTab( void* itemData, int x, int y ) = 0;
};

/**
 *	This class implements a tab bar control class similar in look and
 *	behaviour to the tabs used in Visual Studio.
 */
class TabCtrl : public CWnd, public ReferenceCount
{
public:
	enum Alignment
	{
		TOP,
		BOTTOM
	};

	TabCtrl( CWnd* parent, Alignment align = TOP );
	~TabCtrl();

	int recalcHeight();

	Alignment getAlignment();

	void setEventHandler( TabCtrlEventHandler* handler );

	void insertItem( const std::string& caption, HICON icon, void* itemData );

	void removeItem( void* itemData );

	bool contains( void* itemData );

	int itemCount();
	void* getItemData( int index );

	int getHeight();
	void setCurItem( void* itemData );

	int getItemIndex( void* itemData );
	int getItemIndex( void* itemData, int x, int y );
	void updateItemPosition( void* itemData, int x, int y );
	void updateItemPosition( void* itemData, int index );

	void updateItemData( void* itemData, const std::string& caption, HICON icon );

private:
	static const int CAPTION_FONT_SIZE = 13;
	static const int BACK_COLOR_ADDR = 31;
	static const int BACK_COLOR_ADDG = 31;
	static const int BACK_COLOR_ADDB = 29;
	static const int TABBAR_HEIGHT = 22;
	static const int TABBAR_MARGIN = 4;
	static const int ICON_SIZE = 16;

	TabCtrlEventHandler* eventHandler_;
	CFont font_;
	CPen btnHilight_;
	CPen btnShadow_;
	CPen btnSeparator_;
	std::list<TabCtrlItem> itemList_;
	typedef std::list<TabCtrlItem>::iterator TabCtrlItemItr;
	int curItem_;
	bool multiline_;
	int numLines_;
	std::vector<int> lineSizes_;
	std::vector<int> lineYPos_;
	Alignment align_;

	int recalcHeightOnly();
	void recalcLineSizes();
	void paint( bool multilineSwap = true );
	TabCtrlItem* getItem( int idx );
	TabCtrlItem* getItem( void* itemData );
	TabCtrlItem* getItem( int x, int y );
	void setCurItem( int x, int y );

	// MFC stuff
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP();
};


}	// namespace

#endif //GUITABS_TAB_CTRL_HPP
