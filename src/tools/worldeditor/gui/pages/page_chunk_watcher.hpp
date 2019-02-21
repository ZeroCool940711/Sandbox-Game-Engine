/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PAGE_CHUNK_WATCHER_HPP
#define PAGE_CHUNK_WATCHER_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "worldeditor/gui/controls/chunk_watch_control.hpp"
#include "guitabs/guitabs_content.hpp"
#include "controls/layout_manager.hpp"
#include "controls/color_static.hpp"
#include "controls/auto_tooltip.hpp"


class PageChunkWatcher : public CDialog, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT
    ( 
        L("WORLDEDITOR/GUI/PAGE_CHUNK_WATCHER/SHORT_NAME"),					// short name
        L("WORLDEDITOR/GUI/PAGE_CHUNK_WATCHER/LONG_NAME"),					// long name
        290, 380,                           // width, height
        NULL                                // icon
    )

public:
	enum { IDD = IDD_PAGE_CHUNK_WATCHER };

	PageChunkWatcher();
	~PageChunkWatcher();

	/*virtual*/ BOOL OnInitDialog();

	/*virtual*/ void DoDataExchange(CDataExchange* pDX);

	afx_msg LRESULT OnNewSpace       (WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnChangedChunk   (WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnNewWorkingChunk(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnUpdateControls (WPARAM wParam, LPARAM lParam);
	afx_msg void OnUpdateOptions();

	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO *mmi);

	afx_msg LRESULT OnMouseMoveChunkWatchControl(WPARAM wparam, LPARAM lparam); 

protected:
	void setDisplayOption(UINT id, std::string const &optionText, uint32 value);
	void getDisplayOption(UINT id, std::string const &optionText, uint32 value);

	DECLARE_MESSAGE_MAP()
	DECLARE_AUTO_TOOLTIP(PageChunkWatcher, CDialog);

private:
	controls::SizerPtr			rootSizer_;
	ChunkWatchControl			chunkWatchCtrl_;
	controls::ColorStatic		unloadClrStatic_;
	controls::ColorStatic		loadClrStatic_;	
	controls::ColorStatic		dirtyClrStatic_;
	controls::ColorStatic		shadowedClrStatic_;
	size_t						numLoaded_;
	size_t						numUnloaded_;
	size_t						numDirty_;
	size_t						numCalced_;
};


IMPLEMENT_CDIALOG_CONTENT_FACTORY(PageChunkWatcher, PageChunkWatcher::IDD)


#endif // PAGE_CHUNK_WATCHER_HPP
