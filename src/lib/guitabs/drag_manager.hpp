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
 *	GUI Tearoff panel framework - DragManager class
 */

#ifndef GUITABS_DRAG_MANAGER_HPP
#define GUITABS_DRAG_MANAGER_HPP

#include "cstdmf/smartpointer.hpp"

namespace GUITABS
{


class TempTab : public Tab
{
public:
	TempTab() : Tab( 0, "" ) {};
	void setLabel( const std::string& label ) { label_ = label; };
	std::string getDisplayString() { return label_; };
	std::string getTabDisplayString() { return label_; };
private:
	std::string label_;
};
typedef SmartPointer<TempTab> TempTabPtr;


/**
 *  This singleton class is responsible for managing user events from the
 *  momentwhen a panel begins to be dragged until the panel is released, be it
 *  on top of another panel, or in a non-dockable area. After determining
 *  where a panel or tab has been dragged, this class will call the appropriate
 *  methods in Dock to actually accomplish the final results (dock a panel into
 *  a panel, float a tab dragging it outside of a panel, etc).
 */
class DragManager : public ReferenceCount
{
public:
	DragManager();

	void startDrag( int x, int y, PanelPtr panel, TabPtr tab );

private:
	PanelPtr panel_;
	bool tabCtrlAtTop_;
	TabPtr tab_;
	PanelPtr destPanel_; // used to show tab position feedback
	TempTabPtr tempTab_; // used to show tab position feedback
	int tabInsertIndex_; // used to properly set a tabs position on end drag
	int startX_;
	int startY_;
	int downX_;
	int downY_;
	bool isDragging_;
	bool isFloating_;
	bool hilightSrc_;

	InsertAt lastIns_;
	CRect lastRect_;
	CSize lastSize_;
	CWnd* wnd_;
	CDC* dc_;

	void prepareDrag();
	void updateDrag( int x, int y );
	void endDrag( int x, int y );
	void cancelDrag( bool leaveTempTab = false );
	void drawDragRect( InsertAt ins, CRect rect, SIZE size );

	void dragLoop();

	InsertAt getInsertLocation( DockNodePtr node, int x, int y, CRect& locationRect );
};


} // namespace

#endif // GUITABS_DRAG_MANAGER_HPP