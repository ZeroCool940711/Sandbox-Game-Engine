/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#pragma once
#include "resource.h"

#include "cstdmf/concurrency.hpp"

#include "controls/auto_tooltip.hpp"

#include "guitabs/guitabs_content.hpp"

#include "bw_message_info.hpp"

class PythonAdapter;

class MsgsImpl
{
public:
	virtual afx_msg void OnNMClickMsgList(NMHDR *pNMHDR, LRESULT *pResult) = 0;
	virtual afx_msg void OnNMCustomdrawMsgList(NMHDR *pNMHDR, LRESULT *pResult) = 0;
	virtual ~MsgsImpl(){};
};

// PageMessages

class PageMessages: public CFormView, public GUITABS::Content
{
	DECLARE_DYNCREATE(PageMessages)

	IMPLEMENT_BASIC_CONTENT( L("WORLDEDITOR/GUI/PAGE_MESSAGES/SHORT_NAME"),
		L("WORLDEDITOR/GUI/PAGE_MESSAGES/LONG_NAME"), 285, 448, NULL )

	DECLARE_AUTO_TOOLTIP( PageMessages, CFormView )

public:
	PageMessages();
	virtual ~PageMessages();

	void mainFrame( CFrameWnd* mf );
	void pythonAdapter( PythonAdapter* pa );

	bool showFilter( int priority );
	bool setPriority( int priority, bool enabled );
	bool addItem( BWMessageInfoPtr message );
	void redrawList();

	static PageMessages* currPage();

	void msgsImpl( MsgsImpl* msgsImpl );
	MsgsImpl* msgsImpl();

// Dialog Data
	enum { IDD = IDD_ERRORS };

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
	
	virtual BOOL OnInitDialog();
	
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);

private:

	SmartPointer< struct PageMessagesImpl > pImpl_;

	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );

	void showDateTime( bool show );
	void showPriority( bool show );

	int autoSizeColumn( int i );
	void updateColumnWidths( bool all = false );

	static const int TREE_COL = 0;
	static const int DATE_COL = 1;
	static const int TIME_COL = 2;
	static const int PRIORITY_COL = 3;
	static const int MSG_COL = 4;

public:
	CListCtrl msgList_;

	std::map< int, bool > msgFilter_;
	std::map< std::string, int > index_;
	std::map< std::string, bool > expanded_;

	std::string selected_;

	afx_msg void OnBnClickedErrorsShowDate();
	afx_msg void OnBnClickedErrorsShowTime();
	afx_msg void OnBnClickedErrorsShowPriority();
	
	afx_msg void OnBnClickedErrorsError();
	afx_msg void OnBnClickedErrorsWarning();
	afx_msg void OnBnClickedErrorsNotice();
	afx_msg void OnBnClickedErrorsInfo();
	afx_msg void OnBnClickedMsgAssets();

	afx_msg void OnNMClickMsgList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMCustomdrawMsgList(NMHDR *pNMHDR, LRESULT *pResult);
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageMessages )
