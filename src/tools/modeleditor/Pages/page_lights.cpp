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

#include "windowsx.h"
#include "resmgr/auto_config.hpp"
#include "appmgr/options.hpp"
#include "ual/ual_manager.hpp"

#include "romp/time_of_day.hpp"

#include "python_adapter.hpp"

#include "model_editor.h"
#include "main_frm.h"
#include "me_shell.hpp"
#include "me_app.hpp"
#include "mru.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "ual/ual_manager.hpp"

#include "common/editor_views.hpp"
#include "common/user_messages.hpp"
#include "common/file_dialog.hpp"

#include "delay_redraw.hpp"

#include "controls/slider.hpp"

#include "utilities.hpp"

#include "page_lights.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "moo/directional_light.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

static AutoConfigString s_default_lights( "system/defaultLightsPath" );

struct PageLightsImpl: public SafeReferenceCount
{
	static PageLights* s_currPage;	
	
	bool ready;

	bool inited;

	bool been_visible;
	std::string last_time;
	
	CButton useGameLighting;
	CButton useCustomLighting;
		
	CComboBox lightSetups;
		
	LightList lightList;
	
	CToolBarCtrl toolbar;

	CButton showLightModels;
	
	CButton lockToCamera;
	bool lockedToCamera;

	controls::Slider timeOfDaySlider;
	CStatic timeOfDayTitle;
	CEdit timeOfDayEdit;

	bool updating;

	GeneralLight* currLight;

	HTREEITEM ambient;
	std::vector< HTREEITEM > omni;
	std::vector< HTREEITEM > dir;
	std::vector< HTREEITEM > spot;
};

PageLights* PageLightsImpl::s_currPage = NULL;

BEGIN_MESSAGE_MAP(LightList, CTreeCtrl)
	ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
END_MESSAGE_MAP()

void LightList::OnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	// retrieve mouse cursor position when msg was sent
	DWORD dw = GetMessagePos();							
	CPoint point(GET_X_LPARAM(dw), GET_Y_LPARAM(dw));
	ScreenToClient(&point);

	UINT htFlags = 0;
	HTREEITEM item = HitTest(point, &htFlags);

	bool checked = this->GetCheck( item ) == BST_UNCHECKED;
	
	if (item && ( htFlags & TVHT_ONITEMSTATEICON ) )
	{
		if (checked) // Select the light if we have just enabled it
		{
			Select(item, TVGN_CARET);
		}
	}
	else
	{
		return;
	}

	GeneralLight* light = (GeneralLight*)(this->GetItemData( item ));

	light->enabled( checked );
}

BOOL LightList::PreTranslateMessage( MSG* pMsg )
{
    if(pMsg->message == WM_KEYDOWN)
    {
        if (pMsg->wParam == VK_SPACE)
		{
			bool checked = this->GetCheck( GetSelectedItem() ) == BST_UNCHECKED;

			GeneralLight* light = (GeneralLight*)(this->GetItemData( GetSelectedItem() ));

			light->enabled( checked );
		}
    }

	return CTreeCtrl::PreTranslateMessage( pMsg );
}

// PageLights

//ID string required for the tearoff tab manager
const std::string PageLights::contentID = "PageLightsID";

PageLights::PageLights():
	PropertyTable( PageLights::IDD ),
	filter_(0)
{
	pImpl_ = new PageLightsImpl;

	pImpl_->updating = false;
	pImpl_->ready = false;
	pImpl_->inited = false;
	pImpl_->currLight = NULL;

	pImpl_->been_visible = false;
	pImpl_->last_time = "";

	pImpl_->lockedToCamera = false;

	pImpl_->s_currPage = this;
}

PageLights::~PageLights()
{
	if ( PageLightsImpl::s_currPage )
		PageLightsImpl::s_currPage->fini();
	PageLightsImpl::s_currPage = NULL;
}

void PageLights::fini()
{
	// Make sure any elected gizmos are removed so that the effect materials
	// are deleted prior to application exit.
	if ((pImpl_) && (pImpl_->currLight))
		pImpl_->currLight->expel();
}

/*static*/ PageLights* PageLights::currPage()
{
	return PageLightsImpl::s_currPage;
}

void PageLights::DoDataExchange(CDataExchange* pDX)
{
	GeneralLight* light;
	
	PropertyTable::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIGHTS_USE_GAME, pImpl_->useGameLighting );
	DDX_Control(pDX, IDC_LIGHTS_USE_CUSTOM, pImpl_->useCustomLighting );

	if (CModelEditorApp::instance().pythonAdapter())
	{
		updateCheck( pImpl_->useGameLighting, "actUseCustomLighting", false );
		updateCheck( pImpl_->useCustomLighting, "actUseCustomLighting" );
	}

	DDX_Control(pDX, IDC_LIGHTS_SETUPS, pImpl_->lightSetups );

	CRect setupsRect;
	pImpl_->lightSetups.GetWindowRect(setupsRect);
    ScreenToClient (&setupsRect);
	setupsRect.bottom += 256;	// Extend the dropdown box to show a reasonable selection
	pImpl_->lightSetups.MoveWindow(setupsRect);
	pImpl_->lightSetups.SelectString(-1, "");

	DDX_Control(pDX, IDC_LIGHTS_LIST, pImpl_->lightList);

	pImpl_->ambient = pImpl_->lightList.InsertItem( L("MODELEDITOR/PAGES/PAGE_LIGHTS/AMBIENT"), NULL );
	light = MeApp::instance().lights()->ambient();
	pImpl_->lightList.SetItemData( pImpl_->ambient, (DWORD)light );

	for (int i=0; i<MeApp::instance().lights()->numOmni(); i++)
	{
		pImpl_->omni.push_back( pImpl_->lightList.InsertItem( L("MODELEDITOR/PAGES/PAGE_LIGHTS/OMNI", i+1), NULL ));
		light = MeApp::instance().lights()->omni(i);
		pImpl_->lightList.SetItemData( pImpl_->omni[i], (DWORD)light );
	}
	for (int i=0; i<MeApp::instance().lights()->numDir(); i++)
	{
		pImpl_->dir.push_back( pImpl_->lightList.InsertItem( L("MODELEDITOR/PAGES/PAGE_LIGHTS/DIRECTIONAL", i+1), NULL ));
		light = MeApp::instance().lights()->dir(i);
		pImpl_->lightList.SetItemData( pImpl_->dir[i], (DWORD)light );
	}
	for (int i=0; i<MeApp::instance().lights()->numSpot(); i++)
	{
		pImpl_->spot.push_back( pImpl_->lightList.InsertItem( L("MODELEDITOR/PAGES/PAGE_LIGHTS/SPOT", i+1), NULL ));
		light = MeApp::instance().lights()->spot(i);
		pImpl_->lightList.SetItemData( pImpl_->spot[i], (DWORD)light );
	}

	DDX_Control(pDX, IDC_LIGHTS_CAMERA, pImpl_->lockToCamera);

	DDX_Control(pDX, IDC_LIGHTS_MODELS, pImpl_->showLightModels );

	DDX_Control(pDX, IDC_TIMEOFDAY_TITLE, pImpl_->timeOfDayTitle);
	DDX_Control(pDX, IDC_TIMEOFDAY_TEXT, pImpl_->timeOfDayEdit);
	DDX_Control(pDX, IDC_TIMEOFDAY_SLIDER, pImpl_->timeOfDaySlider);
		
	pImpl_->toolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	GUI::Manager::instance().add( new GUI::Toolbar( "LightsToolbar", pImpl_->toolbar ) );

	CWnd toolbarPos;
	DDX_Control(pDX, IDC_LIGHTS_TOOLBAR, toolbarPos);
	
	CRect toolbarRect;
    toolbarPos.GetWindowRect (&toolbarRect);
    ScreenToClient (&toolbarRect);

	pImpl_->toolbar.MoveWindow(toolbarRect);

	pImpl_->ready = true;
}

BOOL PageLights::OnInitDialog()
{
	filter_++;
	//Add some drop acceptance functors
	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageLights >( this, "mvl", this, &PageLights::applyLights ) );

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageLights >( this, "", this, &PageLights::applyLights ) );
	
	if (Options::getOptionInt( "startup/loadLastLights", 1 ))
	{
		std::string lights = Options::getOptionString( "lights/file0", "" );
		if (lights != "")
		{
			if (Options::getOptionInt( "settings/useCustomLighting", 0 ))
			{
				openLightFile( lights );
			}
			else
			{
				redrawLightList( false );
				enableCustomLighting( false );
			}
		}
		else
		{
			redrawLightList( false );
			enableCustomLighting( false );
		}
	}
	else
	{
		redrawLightList( false );
		enableCustomLighting( false );
	}

	pImpl_->timeOfDaySlider.SetRangeMin(0); 
	pImpl_->timeOfDaySlider.SetRangeMax(60*24 - 1);

	MeShell::instance().timeOfDay()->setTimeOfDayAsString(
		Options::getOptionString( "settings/gameTime",
			MeShell::instance().timeOfDay()->getTimeOfDayAsString() ));	

	//if there is no sun light info, we disable game lighting
	if (!ChunkManager::instance().cameraSpace()->sunLight())
	{
		pImpl_->timeOfDaySlider.EnableWindow( false );
		pImpl_->useGameLighting.EnableWindow( false );
	}

	INIT_AUTO_TOOLTIP();

	--filter_;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageLights, PropertyTable)
	
	ON_WM_SIZE()

	ON_WM_HSCROLL()
	ON_WM_SETFOCUS()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_MESSAGE(WM_CHANGE_PROPERTYITEM, OnChangePropertyItem)
	ON_MESSAGE(WM_DBLCLK_PROPERTYITEM, OnDblClkPropertyItem)

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_NOTIFY(TVN_SELCHANGED, IDC_LIGHTS_LIST, OnTvnSelchangedLightList)

	ON_CBN_SELCHANGE(IDC_LIGHTS_SETUPS, OnCbnSelchangeLightsSetups)
	ON_BN_CLICKED(IDC_LIGHTS_USE_CUSTOM, OnBnClickedLightsUseCustom)
	ON_BN_CLICKED(IDC_LIGHTS_CAMERA, OnBnClickedLightsCamera)
	ON_BN_CLICKED(IDC_LIGHTS_MODELS, OnBnClickedLightsModels)
	ON_BN_CLICKED(IDC_LIGHTS_USE_GAME, OnBnClickedLightsUseGame)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

END_MESSAGE_MAP()

void PageLights::OnGUIManagerCommand(UINT nID)
{
	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageLights::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )                                                   
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageLights::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	LPSTR* msg = (LPSTR*)wParam;
	CMainFrame::instance().SetMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageLights::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	CMainFrame::instance().SetMessageText( "" );
	return 0;
}

// PageLights message handlers

void PageLights::OnSize(UINT nType, int cx, int cy)
{
	if (!pImpl_->ready) return;
	
	Utilities::stretchToRight( this, pImpl_->lightSetups, cx, 12 );
	Utilities::stretchToRight( this, pImpl_->lightList, cx, 12 );

	Utilities::moveToRight( this, pImpl_->timeOfDayTitle, cx, 52 );
	Utilities::moveToRight( this, pImpl_->timeOfDayEdit, cx, 12 );

	Utilities::stretchToRight( this, pImpl_->timeOfDaySlider, cx, 12 );

	PropertyTable::OnSize( nType, cx, cy );

	pImpl_->timeOfDayTitle.RedrawWindow();
	pImpl_->timeOfDayEdit.RedrawWindow();
}

void PageLights::redrawLightList( bool sel )
{
	std::vector<std::string> lights;
	MRU::instance().read( "lights", lights );
	pImpl_->lightSetups.ResetContent();
	for (unsigned i=0; i<lights.size(); i++)
	{
		std::string lightingName = BWResource::getFilename( lights[i] );
		lightingName = BWResource::removeExtension( lightingName );
		pImpl_->lightSetups.InsertString( i, lightingName.c_str() );
	}
	pImpl_->lightSetups.InsertString( lights.size(), L("MODELEDITOR/OTHER") );
	pImpl_->lightSetups.SetCurSel( sel ? 0 : -1 );

	//Update the file menu too
	CModelEditorApp::instance().updateRecentList( "lights" );
}

void PageLights::updateCheck( CButton& button, const std::string& actionName, int test /* = true */ )
{
	int enabled = 0;
	int checked = 0;
	CModelEditorApp::instance().pythonAdapter()->ActionScriptUpdate( actionName, enabled, checked );
	button.SetCheck( checked == test ? BST_CHECKED : BST_UNCHECKED );
}

void PageLights::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (CModelEditorApp::instance().pythonAdapter())
	{
		CModelEditorApp::instance().pythonAdapter()->onSliderAdjust("slrCurrentTime", 
												pImpl_->timeOfDaySlider.GetPos(), 
												pImpl_->timeOfDaySlider.GetRangeMin(), 
												pImpl_->timeOfDaySlider.GetRangeMax());

		std::string currentTime = MeShell::instance().timeOfDay()->getTimeOfDayAsString();
		pImpl_->timeOfDayEdit.SetWindowText( currentTime.c_str() );
	}

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

afx_msg LRESULT PageLights::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	if (!pImpl_->inited)
	{
		OnInitDialog();

		pImpl_->inited = true;
	}

	if (CModelEditorApp::instance().pythonAdapter())
	{
		updateCheck( pImpl_->useGameLighting, "actUseCustomLighting", false );
		updateCheck( pImpl_->useCustomLighting, "actUseCustomLighting" );
		updateCheck( pImpl_->showLightModels, "actShowLightModels" );
	}

	//This hack is required since CListCtrls with checks are not
	//updated if they have not been visible... Sad but true.
	if ((!pImpl_->been_visible) && IsWindowVisible() )
	{
		updateChecks();
		pImpl_->been_visible = true;
	}

	if ((pImpl_->lockedToCamera) &&  (pImpl_->currLight))
	{
		Matrix view = MeApp::instance().camera()->view();
		view.invert();
		static Matrix s_last_view = view;
		view[2] = Vector3(0,0,0) - view[2];
		if (view != s_last_view)
		{
			pImpl_->currLight->setMatrix( view );
			s_last_view = view;
		}
	}

	std::string time = MeShell::instance().timeOfDay()->getTimeOfDayAsString();
	if (time != pImpl_->last_time)
	{
		pImpl_->timeOfDayEdit.SetWindowText(time.c_str());
		pImpl_->timeOfDaySlider.SetPos( (int)(
			pImpl_->timeOfDaySlider.GetRangeMin() +
			(pImpl_->timeOfDaySlider.GetRangeMax() - pImpl_->timeOfDaySlider.GetRangeMin())
			* MeShell::instance().timeOfDay()->gameTime() / 24.f ));
		Options::setOptionString( "settings/gameTime", time );
		pImpl_->last_time = time;
	}

	PropertyTable::update();

	return 0;
}

void PageLights::OnSetFocus( CWnd* pOldWnd )
{
	// The reason for this OnSetFocus is to make that no erroneous 
	// messages messages sent to this page are processed during
	// the OnSetFocus
	filter_++;

	PropertyTable::OnSetFocus( pOldWnd );

	--filter_;
}

void PageLights::updateChecks()
{
	pImpl_->lightList.SetCheck( pImpl_->ambient, 
		MeApp::instance().lights()->ambient()->enabled() ? BST_CHECKED : BST_UNCHECKED );
	for (int i=0; i<MeApp::instance().lights()->numOmni(); i++)
	{
		pImpl_->lightList.SetCheck( pImpl_->omni[i], 
			MeApp::instance().lights()->omni(i)->enabled() ? BST_CHECKED : BST_UNCHECKED );
	}
	for (int i=0; i<MeApp::instance().lights()->numDir(); i++)
	{
		pImpl_->lightList.SetCheck( pImpl_->dir[i], 
			MeApp::instance().lights()->dir(i)->enabled() ? BST_CHECKED : BST_UNCHECKED );
	}
	for (int i=0; i<MeApp::instance().lights()->numSpot(); i++)
	{
		pImpl_->lightList.SetCheck( pImpl_->spot[i], 
			MeApp::instance().lights()->spot(i)->enabled() ? BST_CHECKED : BST_UNCHECKED );
	}
	pImpl_->lightList.RedrawWindow();
}

void PageLights::lightsNew()
{
	if (!MeApp::instance().lights()->newSetup()) return;

	redrawLightList( false );

	updateChecks();

	pImpl_->lockedToCamera = false;
	pImpl_->lockToCamera.SetCheck( BST_UNCHECKED );
}

/*~ function ModelEditor.newLights
 *	@components{ modeleditor }
 *
 *	This function disables all custom lighting and allows for a new light's display settings to be created.
 */
static PyObject * py_newLights( PyObject * args )
{
	if (PageLights::currPage())
		PageLights::currPage()->lightsNew();

	Py_Return;
}
PY_MODULE_FUNCTION( newLights, ModelEditor )

bool PageLights::openLightFile( const std::string& lightFile )
{
	DataSectionPtr lighting = BWResource::openSection( lightFile );

	if (!lighting)
	{
		::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
			L("MODELEDITOR/PAGES/PAGE_LIGHTS/LIGHTS_FILE_MISSING", lightFile),
			L("MODELEDITOR/PAGES/PAGE_LIGHTS/UNABLE_LIGHTS"), MB_OK | MB_ICONERROR );
		
		MRU::instance().update( "lights", lightFile, false );

		redrawLightList( false );

		return false;
	}
		
	if (!MeApp::instance().lights()->open( lightFile, lighting )) return false;
	
	MRU::instance().update( "lights", lightFile, true );

	redrawLightList( true );

	updateChecks();

	//Do this to achieve a refresh of the current light's displayed settings
	if (pImpl_->currLight)
	{
		DelayRedraw temp( propertyList() );
		PropTable::table( this );
		pImpl_->currLight->expel();
		pImpl_->currLight->elect();
	}

	std::string fname = BWResource::getFilename( lightFile );
	std::string longText = BWResource::resolveFilename( lightFile );
	std::replace( longText.begin(), longText.end(), '/', '\\' );
	UalManager::instance().history().add( AssetInfo( "FILE", fname, longText ));

	pImpl_->lockedToCamera = false;
	pImpl_->lockToCamera.SetCheck( BST_UNCHECKED );

	return true;
}

void PageLights::lightsOpen()
{
	static char BASED_CODE szFilter[] =	"Lighting Model (*.mvl)|*.mvl||";
	BWFileDialog fileDlg (TRUE, "", "", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, szFilter);

	std::string lightsDir;
	MRU::instance().getDir("lights", lightsDir, s_default_lights );
	fileDlg.m_ofn.lpstrInitialDir = lightsDir.c_str();

	if ( fileDlg.DoModal() == IDOK )
	{
		std::string lightingModel = BWResource::dissolveFilename( std::string( fileDlg.GetPathName() ));

		if (BWResource::validPath( lightingModel ))
		{
			openLightFile( lightingModel );
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				L("MODELEDITOR/PAGES/PAGE_LIGHTS/BAD_DIR_LOAD"),
				L("MODELEDITOR/PAGES/PAGE_LIGHTS/UNABLE_RESOLVE"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

/*~ function ModelEditor.openLights
 *	@components{ modeleditor }
 *
 *	This function enables the Open File dialog, which allows a light to be loaded.
 */
static PyObject * py_openLights( PyObject * args )
{
	if (PageLights::currPage())
		PageLights::currPage()->lightsOpen();

	Py_Return;
}
PY_MODULE_FUNCTION( openLights, ModelEditor )

void PageLights::lightsSave()
{
	static char BASED_CODE szFilter[] =	"Lighting Model (*.mvl)|*.mvl||";
	BWFileDialog fileDlg (FALSE, "", "", OFN_OVERWRITEPROMPT, szFilter);

	std::string lightsDir;
	MRU::instance().getDir("lights", lightsDir, s_default_lights );
	fileDlg.m_ofn.lpstrInitialDir = lightsDir.c_str();

	if ( fileDlg.DoModal() == IDOK )
	{
		std::string lightingModel = BWResource::dissolveFilename( std::string( fileDlg.GetPathName() ));

		if (BWResource::validPath( lightingModel ))
		{
			MRU::instance().update( "lights", lightingModel, true );
			redrawLightList( true );

			MeApp::instance().lights()->save( lightingModel );
		}
		else
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				L("MODELEDITOR/PAGES/PAGE_LIGHTS/BAD_DIR_SAVE"),
				L("MODELEDITOR/PAGES/PAGE_LIGHTS/UNABLE_RESOLVE"),
				MB_OK | MB_ICONWARNING );
		}
	}
}

/*~ function ModelEditor.saveLights
 *	@components{ modeleditor }
 *
 *	This function opens the Save dialog, for saving the current light's display settings.
 */
static PyObject * py_saveLights( PyObject * args )
{
	if (PageLights::currPage())
		PageLights::currPage()->lightsSave();

	Py_Return;
}
PY_MODULE_FUNCTION( saveLights, ModelEditor )

afx_msg LRESULT PageLights::OnChangePropertyItem(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		BaseView* relevantView = (BaseView*)lParam;
		bool transient = !!wParam;
		relevantView->onChange( transient );
	}

	return 0;
}

afx_msg LRESULT PageLights::OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam)
{
	if (lParam)
	{
		PropertyItem* relevantView = (PropertyItem*)lParam;
		relevantView->onBrowse();
	}
	
	return 0;
}

void PageLights::OnTvnSelchangedLightList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM item = pImpl_->lightList.GetSelectedItem();

	DelayRedraw temp( propertyList() );
	
	PropTable::table( this );
	
	if (pImpl_->currLight)
		pImpl_->currLight->expel();
	
	pImpl_->currLight = (GeneralLight*)(pImpl_->lightList.GetItemData( item ));

	if (pImpl_->currLight)
		pImpl_->currLight->elect();

	pImpl_->lockedToCamera = false;
	pImpl_->lockToCamera.SetCheck( BST_UNCHECKED );

	*pResult = 0;
}

void PageLights::OnCbnSelchangeLightsSetups()
{
	if (pImpl_->lightSetups.GetCurSel() == pImpl_->lightSetups.GetCount()-1)
	{
		redrawLightList( true );
		lightsOpen();
		return;
	}
	
	std::vector<std::string> lights;
	MRU::instance().read( "lights", lights );
	if ( !openLightFile( lights[ pImpl_->lightSetups.GetCurSel() ] ) )
		redrawLightList( true );
}

void PageLights::OnBnClickedLightsUseGame()
{
	if ( filter_ == 0 )
	{
		filter_++;

		if (Options::getOptionInt( "settings/useCustomLighting", 1 ) ==  1)
		{
			if (CModelEditorApp::instance().pythonAdapter())
			{
				CModelEditorApp::instance().pythonAdapter()->ActionScriptExecute("actUseCustomLighting");
				enableCustomLighting( false );
			}
		}

		--filter_;
	}
}

void PageLights::OnBnClickedLightsUseCustom()
{
	if ( filter_ == 0 )
	{
		filter_++;

		if (Options::getOptionInt( "settings/useCustomLighting", 0 ) ==  0)
		{
			if (CModelEditorApp::instance().pythonAdapter())
			{
				CModelEditorApp::instance().pythonAdapter()->ActionScriptExecute("actUseCustomLighting");
				enableCustomLighting( true );
			}
		}

		--filter_;
	}
}

void PageLights::OnBnClickedLightsCamera()
{
	if ( filter_ == 0 )
	{
		filter_++;

		pImpl_->lockedToCamera = pImpl_->lockToCamera.GetCheck() == BST_CHECKED;

		--filter_;
	}
}

void PageLights::OnBnClickedLightsModels()
{
	if ( filter_ == 0 )
	{
		filter_++;

		if (CModelEditorApp::instance().pythonAdapter())
		{
			CModelEditorApp::instance().pythonAdapter()->ActionScriptExecute("actShowLightModels");
		}

		--filter_;
	}
}

bool PageLights::applyLights( UalItemInfo* ii )
{
	std::string lightsPath = BWResource::dissolveFilename( ii->longText() );

	if (lightsPath == "UseGameLighting")
	{
		enableCustomLighting( false );
	}
	else
	{
		openLightFile( lightsPath );
	}

	return true;
}

void PageLights::enableCustomLighting( bool enable )
{
	Options::setOptionInt( "settings/useCustomLighting", enable ? 1 : 0 );
	// Fix up game lighting edits
	if (!enable && ChunkManager::instance().cameraSpace()->sunLight())
	{
		pImpl_->timeOfDaySlider.EnableWindow( !enable );
	}

	//Fix up custom lighting edits
	pImpl_->lightSetups.EnableWindow( enable );
	pImpl_->toolbar.EnableWindow( enable );
	pImpl_->toolbar.Invalidate();
	GUI::Manager::instance().update();
	pImpl_->lightList.EnableWindow( enable );
	propertyList()->enable( enable );
	pImpl_->lockToCamera.EnableWindow( enable );
	pImpl_->showLightModels.EnableWindow( enable );

	// Fix up the gizmo
	if ( pImpl_->currLight )
	{
		if ( enable )
			pImpl_->currLight->elect();
		else
			pImpl_->currLight->expel();
	}
}
