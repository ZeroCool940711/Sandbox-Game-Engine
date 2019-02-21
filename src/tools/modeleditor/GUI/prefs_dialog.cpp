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

#include "main_frm.h"
#include "me_app.hpp"

#include "appmgr/options.hpp"

#include "prefs_dialog.hpp"

CPrefsDlg::CPrefsDlg() : CDialog(CPrefsDlg::IDD)
{}

BEGIN_MESSAGE_MAP(CPrefsDlg, CDialog)
	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)
END_MESSAGE_MAP()

afx_msg LRESULT CPrefsDlg::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	char* msg = (char*)lParam;
	CMainFrame::instance().SetMessageText( msg );
	return 0;
}

afx_msg LRESULT CPrefsDlg::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	CMainFrame::instance().SetMessageText( "" );
	return 0;
}

void CPrefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PREFS_SHOW_SPLASH_SCREEN, showSplashScreen_);
	DDX_Control(pDX, IDC_PREFS_LOAD_LAST_MODEL, loadLastModel_);
	DDX_Control(pDX, IDC_PREFS_LOAD_LAST_LIGHTS, loadLastLights_);

	DDX_Control(pDX, IDC_PREFS_ZOOM_ON_LOAD, zoomOnLoad_);
	DDX_Control(pDX, IDC_PREFS_REGEN_BB, regenBBOnLoad_);

	DDX_Control(pDX, IDC_PREFS_ANIMATE_ZOOM, animateZoom_);
	DDX_Control(pDX, IDC_PREFS_LOCK_LOD, lockLOD_);
	DDX_Control(pDX, IDC_PREFS_INVERT_MOUSE, invertMouse_);
}

BOOL CPrefsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	showSplashScreen_.SetCheck(
		Options::getOptionInt( "startup/showSplashScreen", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );
	
	loadLastModel_.SetCheck(
		Options::getOptionInt( "startup/loadLastModel", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );
	
	loadLastLights_.SetCheck(
		Options::getOptionInt( "startup/loadLastLights", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );

	regenBBOnLoad_.SetCheck(
		Options::getOptionInt( "settings/regenBBOnLoad", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );
	
	zoomOnLoad_.SetCheck(
		Options::getOptionInt( "settings/zoomOnLoad", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );

	animateZoom_.SetCheck(
		Options::getOptionInt( "settings/animateZoom", 1 ) ?
			BST_CHECKED : BST_UNCHECKED );

	lockLOD_.SetCheck(
		Options::getOptionInt( "settings/lockLodParents", 0 ) ?
			BST_CHECKED : BST_UNCHECKED );
		
	invertMouse_.SetCheck(
		Options::getOptionInt( "camera/invert", 0 ) ?
			BST_CHECKED : BST_UNCHECKED );

	INIT_AUTO_TOOLTIP();

	return TRUE;
}

void CPrefsDlg::OnOK()
{
	Options::setOptionInt( "startup/showSplashScreen",
		showSplashScreen_.GetCheck() == BST_CHECKED );
	
	Options::setOptionInt( "startup/loadLastModel",
		loadLastModel_.GetCheck() == BST_CHECKED );

	Options::setOptionInt( "startup/loadLastLights",
		loadLastLights_.GetCheck() == BST_CHECKED );

	Options::setOptionInt( "settings/regenBBOnLoad",
		regenBBOnLoad_.GetCheck() == BST_CHECKED );

	Options::setOptionInt( "settings/zoomOnLoad",
		zoomOnLoad_.GetCheck() == BST_CHECKED );

	bool animateZoom = animateZoom_.GetCheck() == BST_CHECKED;
	Options::setOptionInt( "settings/animateZoom", animateZoom);
	MeApp::instance().camera()->setAnimateZoom( animateZoom );

	Options::setOptionInt( "settings/lockLodParents",
		lockLOD_.GetCheck() == BST_CHECKED );

	bool invCamera = invertMouse_.GetCheck() == BST_CHECKED;
	Options::setOptionInt( "camera/invert", invCamera );
	MeApp::instance().camera()->invert( invCamera );

	CDialog::OnOK();
}
