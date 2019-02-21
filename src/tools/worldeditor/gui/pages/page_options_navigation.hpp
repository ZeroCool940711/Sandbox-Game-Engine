/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PAGE_OPTIONS_NAVIGATION_HPP
#define PAGE_OPTIONS_NAVIGATION_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/resource.h"
#include "controls/auto_tooltip.hpp"
#include "controls/slider.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/edit_commit.hpp"
#include "guitabs/guitabs_content.hpp"
#include <afxwin.h>
#include <afxcmn.h>
#include <vector>
#include <set>


class OptionsLocationsTree : public CTreeCtrl
{
	DECLARE_DYNAMIC(OptionsLocationsTree)

public:
	OptionsLocationsTree();
	virtual ~OptionsLocationsTree() {};

private:
	DataSectionPtr locationData_;
	std::set< std::string > locations_;
	std::string search_str_;

protected:
	DECLARE_MESSAGE_MAP()

public:
	void updateLocationsList( const std::string& spaceName );
	void redrawLocationsList();
	void setSearchString( const std::string& searchString );

	void doAdd();
	void doRename();
	void doUpdate();
	void doRemove();
	void doMove();

	afx_msg void OnTvnEndlabeleditOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnKeydownOptionsLocationList(NMHDR *pNMHDR, LRESULT *pResult);
};


class PageOptionsNavigation : public CFormView, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( L("WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/SHORT_NAME"),
		L("WORLDEDITOR/GUI/PAGE_OPTIONS_NAVIGATION/LONG_NAME"), 263, 442, NULL )

public:
	PageOptionsNavigation();
	virtual ~PageOptionsNavigation();

// Dialog Data
	enum { IDD = IDD_PAGE_OPTIONS_NAVIGATION };

private:
	bool pageReady_;
	void updateSliderEdits();

	bool dontUpdateHeightEdit_;
	bool cameraHeightEdited_;

	CEdit search_;
	CWnd search_cancel_;

	virtual BOOL PreTranslateMessage( MSG* pMsg );

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_AUTO_TOOLTIP_EX( PageOptionsNavigation )
	DECLARE_MESSAGE_MAP()

public:
	void InitPage();

	controls::EditNumeric posXEdit_;
	controls::EditNumeric posYEdit_;
	controls::EditNumeric posZEdit_;
	controls::EditCommit  posChunkEdit_;
	OptionsLocationsTree locationList_;
	controls::Slider farPlaneSlider_;
	controls::EditNumeric farPlaneEdit_;
	controls::Slider cameraHeightSlider_;
	controls::EditNumeric cameraHeightEdit_;

	CButton locationRename_;
	CButton locationUpdate_;
	CButton locationRemove_;
	CButton locationMoveTo_;

	Vector3 lastPos_;

	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnChangeEditNumeric(WPARAM mParam, LPARAM lParam);
	afx_msg void OnEnChangeOptionsCameraHeightEdit();
	CButton isPlayerPreviewModeEnabled_;
	afx_msg void OnBnClickedPlayerPreviewMode();
	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnBnClickedOptionsPosMove();
	afx_msg void OnBnClickedOptionsChunkMove();
	afx_msg void OnBnClickedOptionsLocationAdd();
	afx_msg void OnBnClickedOptionsLocationRename();
	afx_msg void OnBnClickedOptionsLocationRemove();
	afx_msg void OnBnClickedOptionsLocationMove();
	afx_msg void OnStnClickedOptionsLocationSearchButton();
	afx_msg void OnEnChangeOptionsLocationSearchText();
	afx_msg void OnStnClickedOptionsLocationSearchCancel();
	afx_msg void OnBnClickedOptionsLocationUpdate();
	afx_msg HBRUSH OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor );

};


IMPLEMENT_BASIC_CONTENT_FACTORY( PageOptionsNavigation )


#endif // PAGE_OPTIONS_NAVIGATION_HPP
