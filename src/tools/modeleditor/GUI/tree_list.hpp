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

#include "mutant.hpp"

typedef std::pair < std::string , std::string > StringPair;

// TreeList

class TreeList: public CFormView
{
	DECLARE_DYNCREATE(TreeList)

public:
	//This is the default constructor required for DYNCREATE
	TreeList(): CFormView(IDD_EMPTY) {}
	
	TreeList( UINT dialogID, TreeRoot* tree_root_, const std::string& what );
	virtual ~TreeList();

	static const char LOCKED_TAG[];

private:
	HTREEITEM selItem_ ;
	StringPair selID_;
	std::string search_str_;

	TreeRoot* treeRoot_;
	std::string what_;

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void OnSize(UINT nType, int cx, int cy);

	DECLARE_MESSAGE_MAP()

public:

	void OnUpdateTreeList();
	
	afx_msg void OnEnSetFocusSearch();
	afx_msg void OnEnChangeSearch();
	afx_msg void OnStnClickedCancelSearch();
	afx_msg void OnTvnSelChangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickSearchTree(NMHDR *pNMHDR, LRESULT *pResult);

	virtual void selChange( const StringPair& itemID ) {};
	virtual void selClick( const StringPair& itemID ) {};

	StringPair& selID() { return selID_; };
	HTREEITEM& selItem() { return selItem_; };
	CTreeCtrl& tree() { return tree_; };

	bool locked() { return locked_; }
	void locked( bool locked ) { locked_ = locked; OnUpdateTreeList(); }

	bool sameClicked() const { return sameClicked_; }
	void sameClicked( bool same ) { sameClicked_ = same; }

private:

	bool ignoreSelChange_;
	bool sameClicked_;

	CStatic search_bkg_;
	CEdit search_;
	CWnd search_button_;
	CWnd search_cancel_;
	CTreeCtrl tree_;

	std::vector<std::string*> pathData_;

protected:

	bool locked_;
};
