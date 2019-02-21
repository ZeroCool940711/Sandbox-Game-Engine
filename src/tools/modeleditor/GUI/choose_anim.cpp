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
#include "choose_anim.hpp"

#include "me_app.hpp"
#include "utilities.hpp"

BEGIN_MESSAGE_MAP(CChooseAnim, TreeListDlg)
	ON_EN_CHANGE(IDC_ACT_NAME, OnEnChangeActName)
	ON_NOTIFY(NM_DBLCLK, IDC_SEARCH_TREE, OnNMDblclkSearchTree)
END_MESSAGE_MAP()

CChooseAnim::CChooseAnim( int dialogID, bool withName ):
	TreeListDlg( dialogID, MeApp::instance().mutant()->animTree(), "animations" ),
	withName_(withName),
	defaultName_(true)
{
}

void CChooseAnim::DoDataExchange(CDataExchange* pDX)
{
	TreeListDlg::DoDataExchange(pDX);

	if (withName_)
	{
		DDX_Control(pDX, IDC_ACT_NAME, name_);
	}

	DDX_Control(pDX, IDOK, ok_);
}

BOOL CChooseAnim::OnInitDialog()
{
	CDialog::OnInitDialog();

	OnUpdateTreeList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChooseAnim::selChange( const StringPair& animId )
{
	defaultNameChange_ = true;
	
	if (( tree().GetParentItem(selItem()) == NULL ) || ( !MeApp::instance().mutant()->hasAnims( animId.second ) ))
	{
		ok_.ModifyStyle( 0, WS_DISABLED );
		if ((withName_) && (defaultName_))
		{
			name_.SetWindowText( "" );
		}
	}
	else
	{
		ok_.ModifyStyle( WS_DISABLED, 0 );
		if ((withName_) && (defaultName_))
		{
			name_.SetWindowText( selID().first.c_str() );
		}
	}

	defaultNameChange_ = false;
	
	animName_ = selID().first;
	
	ok_.RedrawWindow();
}



void CChooseAnim::OnEnChangeActName()
{
	if (!withName_) return;

	static bool s_ignoreCallback = false;
	if (s_ignoreCallback) return;
	s_ignoreCallback = true;

	if (!defaultNameChange_)
	{
		defaultName_ = false;
	}

	int first, last;
	name_.GetSel(first,last);
	CString actName_cstr;
	name_.GetWindowText( actName_cstr );
	actName_ = std::string( actName_cstr );
	actName_ = Utilities::pythonSafeName( actName_ );
	name_.SetWindowText( actName_.c_str() );
	name_.SetSel(first,last);
		
	if (( tree().GetParentItem(selItem()) ) && (actName_.length()) )
	{
		ok_.ModifyStyle( WS_DISABLED, 0 );
	}
	else
	{
		ok_.ModifyStyle( 0, WS_DISABLED );
	}

	if (!actName_.length()) // If there is no text entered...
	{
		defaultName_ = true; // Return to using the default names
	}

	ok_.RedrawWindow();

	s_ignoreCallback = false;
}

void CChooseAnim::OnNMDblclkSearchTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnOK();
	*pResult = 0;
}
