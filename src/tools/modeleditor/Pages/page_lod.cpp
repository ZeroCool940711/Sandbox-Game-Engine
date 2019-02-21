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

#include "python_adapter.hpp"

#include "model_editor.h"
#include "main_frm.h"
#include "me_shell.hpp"
#include "me_app.hpp"
#include "me_consts.hpp"

#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"

#include "ual/ual_manager.hpp"

#include "common/user_messages.hpp"
#include "common/file_dialog.hpp"

#include "controls/user_messages.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"

#include "delay_redraw.hpp"
#include "utilities.hpp"
#include "mru.hpp"

#include "lod_bar.hpp"


#include "page_lod.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

/**
 * The LOD page implementation class
 */
struct PageLODImpl: public SafeReferenceCount
{
	static PageLOD* s_currPage;

	bool inited;
	bool ready;
	bool updating;

	int updateCount;

	int lastLockedParents;
	bool locked;
	
	CWnd lodBarRect;
	CLodBar* lodBar;
	
	controls::Slider distSlider;
	CWnd dist_title;
	controls::EditNumeric dist;
	CWnd dist_unit;
	float lodDist;
	float lastLodDist;

	CButton virtualDist;
	bool useVirtualDist;
	
	CListCtrl list;
	CToolBarCtrl toolbar;

	controls::EditNumeric min;
	controls::EditNumeric max;
	CButton hidden;

	bool needsUpdate;
	bool ignoreSelChange;
};

PageLOD* PageLODImpl::s_currPage = NULL;

// PageLOD

//ID string required for the tearoff tab manager
const std::string PageLOD::contentID = "PageLodID";

IMPLEMENT_DYNCREATE(PageLOD, CFormView)

PageLOD::PageLOD():
	CFormView( PageLOD::IDD )
{
	pImpl_ = new PageLODImpl;

	pImpl_->inited = false;
	pImpl_->ready = false;
	pImpl_->updating = false;

	pImpl_->locked = false;

	pImpl_->updateCount = -1;
	pImpl_->lastLockedParents = -1;

	pImpl_->lodBar = NULL;
	pImpl_->useVirtualDist = false;
	pImpl_->needsUpdate = true;
	pImpl_->ignoreSelChange = false;
	pImpl_->updating = false;
	
	pImpl_->dist.SetNumericType( controls::EditNumeric::ENT_FLOAT );
	pImpl_->dist.SetAllowNegative( false );
	pImpl_->dist.SetAllowEmpty( false );
	pImpl_->dist.SetNumDecimals( 1 );
		
	pImpl_->min.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	pImpl_->min.SetAllowNegative( false );
	pImpl_->min.SetAllowEmpty( true );

	pImpl_->max.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	pImpl_->max.SetAllowNegative( false );
	pImpl_->max.SetAllowEmpty( true );

	pImpl_->s_currPage = this;
}

PageLOD::~PageLOD()
{
	clearData();
	delete pImpl_->lodBar;
}

bool PageLOD::locked() const
{
	return pImpl_->locked;
}

void PageLOD::locked( bool locked )
{
	pImpl_->locked = locked;
	pImpl_->lodBar->locked( locked );
	OnUpdateLODList();
}

/*static*/ PageLOD* PageLOD::currPage()
{
	return PageLODImpl::s_currPage;
}

void PageLOD::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LOD_BAR, pImpl_->lodBarRect);
	if (pImpl_->lodBar == NULL)
		pImpl_->lodBar = new CLodBar( &pImpl_->lodBarRect );
	
	DDX_Control(pDX, IDC_LOD_DIST_SLIDER, pImpl_->distSlider);
	pImpl_->distSlider.SetRangeMin(0); 
	pImpl_->distSlider.SetRangeMax(1024);
		
	DDX_Control(pDX, IDC_LOD_VIRTUAL_DIST, pImpl_->virtualDist);
	
	DDX_Control(pDX, IDC_LOD_DIST_TITLE, pImpl_->dist_title);
	DDX_Control(pDX, IDC_LOD_DIST, pImpl_->dist);
	DDX_Control(pDX, IDC_LOD_DIST_UNIT, pImpl_->dist_unit);

	DDX_Control(pDX, IDC_LOD_LIST, pImpl_->list);

	DDX_Control(pDX, IDC_LOD_MIN, pImpl_->min);
	DDX_Control(pDX, IDC_LOD_MAX, pImpl_->max);
	DDX_Control(pDX, IDC_LOD_HIDDEN, pImpl_->hidden);

	pImpl_->list.SetExtendedStyle( LVS_EX_FULLROWSELECT );

	pImpl_->list.InsertColumn(0,"#",     LVCFMT_LEFT, 16);
	pImpl_->list.InsertColumn(1, L("MODELEDITOR/PAGES/PAGE_LOD/MODEL"), LVCFMT_LEFT, 115);
	pImpl_->list.InsertColumn(2, L("MODELEDITOR/PAGES/PAGE_LOD/MIN"),   LVCFMT_LEFT, 32);
	pImpl_->list.InsertColumn(3, L("MODELEDITOR/PAGES/PAGE_LOD/MAX") ,   LVCFMT_LEFT, 32);
	pImpl_->list.InsertColumn(4, L("MODELEDITOR/PAGES/PAGE_LOD/STATE"),   LVCFMT_LEFT, 52);

	pImpl_->toolbar.Create( CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN |
		TBSTYLE_FLAT | WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | CBRS_TOOLTIPS,
		CRect(0,0,0,0), this, 0 );

	GUI::Manager::instance().add( new GUI::Toolbar( "LodToolbar", pImpl_->toolbar ) );

	CWnd toolbarPos;
	DDX_Control(pDX, IDC_LOD_TOOLBAR, toolbarPos);
	
	CRect toolbarRect;
    toolbarPos.GetWindowRect (&toolbarRect);
    ScreenToClient (&toolbarRect);

	pImpl_->toolbar.MoveWindow(toolbarRect);

	pImpl_->inited = true;
}

BOOL PageLOD::OnInitDialog()
{
	pImpl_->lodBar->redraw();

	UalManager::instance().dropManager().add(
		new UalDropFunctor< PageLOD >( &(pImpl_->list), "model", this, &PageLOD::modelDrop ) );
	
	INIT_AUTO_TOOLTIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(PageLOD, CFormView)

	ON_WM_CREATE()

	ON_WM_SIZE()

	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)

	ON_WM_HSCROLL()

	ON_COMMAND_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommand)
	ON_UPDATE_COMMAND_UI_RANGE(GUI_COMMAND_START, GUI_COMMAND_END, OnGUIManagerCommandUpdate)

	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOD_LIST, OnLvnItemchangedLodList)

	ON_BN_CLICKED(IDC_LOD_HIDDEN, OnBnClickedLodHidden)
	ON_BN_CLICKED(IDC_LOD_VIRTUAL_DIST, OnBnClickedLodVirtualDist)

	ON_MESSAGE(WM_SHOW_TOOLTIP, OnShowTooltip)
	ON_MESSAGE(WM_HIDE_TOOLTIP, OnHideTooltip)

END_MESSAGE_MAP()

void PageLOD::clearData()
{
	for ( uint32 i = 0; i < data_.size(); i++ )
	{
		std::string* data = data_[i];
		delete data;
	}
	data_.clear();
}

void PageLOD::OnGUIManagerCommand(UINT nID)
{
	pImpl_->s_currPage = this;
	GUI::Manager::instance().act( nID );
}

void PageLOD::OnGUIManagerCommandUpdate(CCmdUI * cmdUI)
{
	pImpl_->s_currPage = this;
	if( !cmdUI->m_pMenu )                                                   
		GUI::Manager::instance().update( cmdUI->m_nID );
}

afx_msg LRESULT PageLOD::OnShowTooltip(WPARAM wParam, LPARAM lParam)
{
	LPSTR* msg = (LPSTR*)wParam;
	CMainFrame::instance().SetMessageText( *msg );
	return 0;
}

afx_msg LRESULT PageLOD::OnHideTooltip(WPARAM wParam, LPARAM lParam)
{
	CMainFrame::instance().SetMessageText( "" );
	return 0;
}

// PageLOD message handlers

int PageLOD::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

void PageLOD::OnSize(UINT nType, int cx, int cy)
{
	if (!pImpl_->inited) return;
	
	pImpl_->lodBar->setWidth( cx - 36 );

	Utilities::stretchToRight( this, pImpl_->distSlider, cx, 14 );
	Utilities::stretchToRight( this, pImpl_->list, cx, 12 );
}

/*
 * This method moves the camera to the requested LOD distance.
 */
void PageLOD::moveToLodDist( float dist )
{
		
	pImpl_->lodDist = getLodDist();
	
	Matrix view = MeApp::instance().camera()->view();
	view.invert();

	Vector3 forward = view.applyToUnitAxisVector( 2 );

	view._41 -= (dist - pImpl_->lodDist) * forward.x;
	view._42 -= (dist - pImpl_->lodDist) * forward.y;
	view._43 -= (dist - pImpl_->lodDist) * forward.z;

	view.invert();

	MeApp::instance().camera()->view( view );
}

/* 
 * The following method finds the lod distance in the same way as supermodel.
 * It uses the planar distance so it may appear a bit strange.
 */
float PageLOD::getLodDist()
{
	const Matrix & mooWorld = Moo::rc().world();
	const Matrix & mooView = MeApp::instance().camera()->view();

	float realDist = mooWorld.applyToOrigin().dotProduct(
	Vector3( mooView._13, mooView._23, mooView._33 ) ) + mooView._43;
	float yscale = mooWorld.applyToUnitAxisVector(1).length();
	return (realDist / std::max( 1.f, yscale )) * Moo::rc().zoomFactor();
}

void PageLOD::disableField( CEdit& field )
{
	field.SetWindowText( "" );
	field.SetReadOnly( true );
	field.ModifyStyle( 0, WS_DISABLED );
}

void PageLOD::enableField( CEdit& field )
{
	field.SetReadOnly( false );
	field.ModifyStyle( WS_DISABLED, 0 );
}

afx_msg LRESULT PageLOD::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	pImpl_->updating = pImpl_->updateCount != MeApp::instance().mutant()->updateCount("LOD");
	pImpl_->updateCount = MeApp::instance().mutant()->updateCount("LOD");

	//if (CModelEditorApp::instance()->pythonAdapter())
	//{
		// TODO: We will do stuff here...
	//}

	if (!pImpl_->ready)
	{
		OnInitDialog();

		setSel( 0 );

		pImpl_->ready = true;
	}

	if (MeApp::instance().mutant())
	{
		if (pImpl_->updating)
		{
			OnUpdateLODList();
			
			GUI::Manager::instance().update(); // Update the LOD toolbar

			if (MeApp::instance().mutant()->modelName() == "")	
			{
				pImpl_->toolbar.ModifyStyle( 0, WS_DISABLED );
			}
			else
			{
				pImpl_->toolbar.ModifyStyle( WS_DISABLED, 0 );
			}
		}
	}

	// Check whether the LOD selection has changed
	if ((pImpl_->updating) || (pImpl_->needsUpdate))
	{
		pImpl_->s_currPage = this;
	
		GUI::Manager::instance().update(); // Update the LOD toolbar

		int sel = pImpl_->list.GetSelectionMark();

		LODList* lodList = MeApp::instance().mutant()->lodList();
		bool missing = (sel > 0) && ((int)lodList->size() > sel) && ((*lodList)[sel].second == LOD_MISSING);
		bool hidden = false;
		float extent = 0.f;
		float lastExtent = 0.f;
		for (int i=0; i<=sel; i++)
		{
			if ((((*lodList)[i].second != LOD_HIDDEN) && ((*lodList)[i].second <= extent)) ||
				(extent == LOD_HIDDEN))
			{
				hidden = true;
			}
			else
			{
				hidden = false;
				lastExtent = extent;
				extent = (*lodList)[i].second;
			}
		}

		if (!hidden)
		{
			pImpl_->min.SetValue( lastExtent );
			if (extent != LOD_HIDDEN)
			{
				pImpl_->max.SetValue( extent );
			}
			else
			{
				pImpl_->max.Clear();
			}
			enableField( pImpl_->min );
			enableField( pImpl_->max );
		}
		else
		{
			pImpl_->min.Clear();
			pImpl_->max.Clear();
			disableField( pImpl_->min );
			disableField( pImpl_->max );
		}

		pImpl_->hidden.SetCheck( hidden ? BST_CHECKED : BST_UNCHECKED );

		bool locked = (pImpl_->locked) && (sel > 1);
			
		pImpl_->min.SetReadOnly( locked );
		pImpl_->min.ModifyStyle( locked ? 0 : WS_DISABLED,
			locked ? WS_DISABLED : 0);

		locked = (pImpl_->locked) && (sel > 0);
		
		pImpl_->max.SetReadOnly( locked );
		pImpl_->max.ModifyStyle( locked ? 0 : WS_DISABLED,
			locked ? WS_DISABLED : 0);

		pImpl_->hidden.ModifyStyle( locked ? 0 : WS_DISABLED,
			locked ? WS_DISABLED : 0);

		if ((!lodIsSelected()) || (missing))
		{
			pImpl_->min.SetReadOnly( true );
			pImpl_->min.ModifyStyle( 0, WS_DISABLED );
			pImpl_->max.SetReadOnly( true );
			pImpl_->max.ModifyStyle( 0, WS_DISABLED );
			pImpl_->hidden.ModifyStyle( 0, WS_DISABLED );
		}

		RedrawWindow();
	
		pImpl_->needsUpdate = false;
	}

	// Ensure that the parent lock is enabled
	int lockLod = Options::getOptionInt( "settings/lockLodParents", 0 );
	if ( lockLod != pImpl_->lastLockedParents )
	{
		locked( !!lockLod );
		pImpl_->lastLockedParents = lockLod;
	}

	pImpl_->lodDist = getLodDist();

	if ((pImpl_->updating) || ((int)(pImpl_->lodDist*10) != (int)(pImpl_->lastLodDist*10)))
	{
		if (!pImpl_->useVirtualDist) 
		{
			pImpl_->distSlider.SetPos( (int)(1024.f * pImpl_->lodDist / pImpl_->lodBar->maxRange()) );
			pImpl_->dist.SetValue( pImpl_->lodDist );
		}
		pImpl_->lastLodDist = pImpl_->lodDist;
	}

	if (pImpl_->dist.doUpdate())
	{
		pImpl_->updating = true;
		if (!pImpl_->useVirtualDist)
		{
			moveToLodDist( pImpl_->dist.GetValue() );
		}
		else
		{
			MeApp::instance().mutant()->virtualDist( pImpl_->dist.GetValue() );
			pImpl_->distSlider.SetPos( (int)(1024.f * pImpl_->dist.GetValue() / pImpl_->lodBar->maxRange()) );
		}
	}

	if (pImpl_->min.doUpdate())
	{
		updateMin();
	}
	if (pImpl_->max.doUpdate())
	{
		updateMax();
	}

	return 0;
}

void PageLOD::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pImpl_->updating) return;
	
	HWND target = pScrollBar->GetSafeHwnd();
	
	if (target == pImpl_->distSlider.GetSafeHwnd())
	{
		float dist = pImpl_->lodBar->maxRange() * pImpl_->distSlider.GetPos() / 1024.f;
		pImpl_->dist.SetValue( dist );
		if (!pImpl_->useVirtualDist)
		{
			moveToLodDist( pImpl_->dist.GetValue() );
		}
		else
		{
			MeApp::instance().mutant()->virtualDist( pImpl_->dist.GetValue() );
		}
	}

	CFormView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageLOD::OnUpdateLODList()
{
	pImpl_->ignoreSelChange = true;

	int oldSel = pImpl_->list.GetSelectionMark();
	
	//Firstly we need to delete any model path data stored
	clearData();
		
	DelayRedraw temp( &(pImpl_->list) );
	
	pImpl_->list.DeleteAllItems();

	LODList* lodList = MeApp::instance().mutant()->lodList();

	std::string text;

	float lastExtent = 0.f;
	for (unsigned i=0; i<lodList->size(); i++)
	{
		int nIndex = pImpl_->list.InsertItem( i, L("MODELEDITOR/PAGES/PAGE_LOD/NOTHING", i ));

		std::string* fileName = new std::string( (*lodList)[i].first.second );
		pImpl_->list.SetItemData( i, (DWORD)fileName );
		data_.push_back( fileName );

		if ((pImpl_->locked) && (i>0))
			text = L("MODELEDITOR/PAGES/PAGE_LOD/LOCKED", (*lodList)[i].first.first.c_str() );
		else
			text = L("MODELEDITOR/PAGES/PAGE_LOD/NOTHING", (*lodList)[i].first.first.c_str() );
		pImpl_->list.SetItemText( nIndex, 1, text.c_str() );

		pImpl_->list.SetItemText( nIndex, 2, L("MODELEDITOR/PAGES/PAGE_LOD/NOTHING", Formatter( lastExtent, "%g" )));

		if ((*lodList)[i].second != LOD_HIDDEN)
		{
			pImpl_->list.SetItemText( nIndex, 3, L("MODELEDITOR/PAGES/PAGE_LOD/NOTHING", Formatter( (*lodList)[i].second, "%g" )));
		}
		else
		{
			pImpl_->list.SetItemText( nIndex, 3, "...");
		}

		if ((*lodList)[i].second == LOD_MISSING)
		{
			pImpl_->list.SetItemText( nIndex, 2, "");
			pImpl_->list.SetItemText( nIndex, 3, "");
			pImpl_->list.SetItemText( nIndex, 4, L("MODELEDITOR/PAGES/PAGE_LOD/MISSING"));
		}
		else if ((((*lodList)[i].second != LOD_HIDDEN) && ((*lodList)[i].second <= lastExtent)) ||
			(lastExtent == LOD_HIDDEN))
		{
			pImpl_->list.SetItemText( nIndex, 2, "");
			pImpl_->list.SetItemText( nIndex, 3, "");
			pImpl_->list.SetItemText( nIndex, 4, L("MODELEDITOR/PAGES/PAGE_LOD/HIDDEN"));
		}
		else
		{
			lastExtent = (*lodList)[i].second;
		}
	}

	pImpl_->lodBar->redraw();

	if (!pImpl_->useVirtualDist) 
		pImpl_->distSlider.SetPos( (int)(1024.f * pImpl_->lodDist / pImpl_->lodBar->maxRange()) );

	setSel( oldSel );

	pImpl_->ignoreSelChange = false;
}

//-------

bool PageLOD::checkModel( const std::string& modelPath )
{
	CWaitCursor wait;

	std::vector< std::string > parents;
	MeApp::instance().mutant()->lodParents( modelPath, parents );

	for (unsigned i=0; i<parents.size(); i++)
	{
		if ( MeApp::instance().mutant()->hasParent( parents[i] ))
		{
			wait.Restore();
			ERROR_MSG( "Unable to add:\n"
				" \"%s\"\n"
				"to the LOD list since it shares the parent:\n"
				" \"%s\"\n"
				"with the current model.\n\n"
				"Please remove this model from the LOD list and try again.\n",
				modelPath.c_str(), parents[i].c_str() );
			return false;
		}
	}

	return true;
}

void PageLOD::addNewModel( const std::string& modelName )
{
	CWaitCursor wait;
	
	std::string modelPath = BWResource::dissolveFilename( modelName );

	if (!checkModel( modelPath ))
	{
		return;
	}

	modelPath = BWResource::removeExtension( modelPath );
	//modelPath.substr( 0, modelPath.rfind(".") );

	int i = pImpl_->list.GetItemCount();

	std::string* lastModelPath = NULL;
	if (i >= 1)
	{
		lastModelPath = (std::string*)(pImpl_->list.GetItemData(i-1));
	}

	std::string* secondLastModelPath = NULL;
	if (i >= 2)
	{
		secondLastModelPath = (std::string*)(pImpl_->list.GetItemData(i-2));
	}

	float extent = LOD_HIDDEN;
	if ( lastModelPath )
	{
		extent = MeApp::instance().mutant()->lodExtent( *lastModelPath );
	}
	if ((extent == LOD_HIDDEN) && (secondLastModelPath))
	{
		extent = MeApp::instance().mutant()->lodExtent( *secondLastModelPath );
		if (extent != LOD_HIDDEN)
		{
			MeApp::instance().mutant()->lodExtent( *lastModelPath, extent + 10.f );
		}
	}
	if ((extent == LOD_HIDDEN) && (!secondLastModelPath))
	{
		MeApp::instance().mutant()->lodExtent( *lastModelPath, 10.f );
	}
	
	MeApp::instance().mutant()->lodParent( *lastModelPath, modelPath );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/ADDING_LOD"), false );

	MeApp::instance().mutant()->ensureModelValid( modelPath + ".model", "add new LOD");

	MRU::instance().update( "models", modelPath + ".model", true );

	//Update the file menu too
	CModelEditorApp::instance().updateRecentList( "models" );

	MeApp::instance().mutant()->reloadAllLists();

	OnUpdateLODList();

	setSel( i );
}

void PageLOD::lodNew()
{
	if (lodIsMissing( pImpl_->list.GetItemCount() - 1 ))
	{
		setSel( pImpl_->list.GetItemCount() - 1 );
		lodChangeModel();
		return;
	}
		
	static char BASED_CODE szFilter[] =	"Model (*.model)|*.model||";
	BWFileDialog fileDlg (TRUE, "", "", OFN_FILEMUSTEXIST, szFilter);

	std::string modelsDir;
	MRU::instance().getDir( "models", modelsDir );
	fileDlg.m_ofn.lpstrInitialDir = modelsDir.c_str();

	if ( fileDlg.DoModal() == IDOK )
	{
		addNewModel( std::string( fileDlg.GetPathName() ));
	}
}

/*~ function ModelEditor.newLod
 *	@components{ modeleditor }
 *
 *	This function enables the Open File dialog, which allows a LOD model to be loaded.
 */
static PyObject * py_newLod( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodNew();

	Py_Return;
}
PY_MODULE_FUNCTION( newLod, ModelEditor )

void PageLOD::lodChangeModel()
{
	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;
	
	static char BASED_CODE szFilter[] =	"Model (*.model)|*.model||";
	BWFileDialog fileDlg (TRUE, "", "", OFN_FILEMUSTEXIST, szFilter);

	std::string modelsDir;
	MRU::instance().getDir( "models", modelsDir );
	fileDlg.m_ofn.lpstrInitialDir = modelsDir.c_str();

	if ( fileDlg.DoModal() == IDOK )
	{
		std::string modelPath = BWResource::dissolveFilename( std::string( fileDlg.GetPathName() ));

		if (!checkModel( modelPath ))
		{
			return;
		}

		modelPath = modelPath.substr( 0, modelPath.rfind(".") );

		std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData(i-1));

		MeApp::instance().mutant()->lodParent( *prevModelFile, modelPath );

		UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/CHANGING_LOD"), false );

		MeApp::instance().mutant()->ensureModelValid( modelPath + ".model", "change LOD");

		MeApp::instance().mutant()->reloadAllLists();

		OnUpdateLODList();

		setSel( i );
	}
}

/*~ function ModelEditor.changeLodModel
 *	@components{ modeleditor }
 *
 *	This function enables the Open File dialog, which allows the currently
 *	selected LOD model to be changed to a different model.
 */
static PyObject * py_changeLodModel( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodChangeModel();

	Py_Return;
}
PY_MODULE_FUNCTION( changeLodModel, ModelEditor )

void PageLOD::lodRemove()
{
	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData(i-1));

	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(i));

	std::string* nextModelFile = NULL;
	if (i != pImpl_->list.GetItemCount() - 1)
		nextModelFile = (std::string*)(pImpl_->list.GetItemData(i+1));

	if (nextModelFile)
	{
		MeApp::instance().mutant()->lodParent( *prevModelFile,
			nextModelFile->substr( 0, nextModelFile->rfind(".") ) );
	}
	else
	{
		MeApp::instance().mutant()->lodParent( *prevModelFile, "" );
	}

	MeApp::instance().mutant()->lodParent( *currModelFile, "" );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/REMOVING_LOD"), false );

	MeApp::instance().mutant()->reloadAllLists();

	OnUpdateLODList();
	
	//Select the item
	setSel( i );
}

/*~ function ModelEditor.removeLod
 *	@components{ modeleditor }
 *
 *	This function removes the currently selected LOD model.
 */
static PyObject * py_removeLod( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodRemove();

	Py_Return;
}
PY_MODULE_FUNCTION( removeLod, ModelEditor )

void PageLOD::lodMoveUp()
{
	if ( !lodCanMoveUp() ) return;

	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* prevPrevModelFile = (std::string*)(pImpl_->list.GetItemData(i-2));

	std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData(i-1));
	float prevModelExtent = MeApp::instance().mutant()->lodExtent( *prevModelFile );

	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(i));
	float currModelExtent = MeApp::instance().mutant()->lodExtent( *currModelFile );

	std::string* nextModelFile = NULL;
	if (i != pImpl_->list.GetItemCount() - 1)
		nextModelFile = (std::string*)(pImpl_->list.GetItemData(i+1));

	MeApp::instance().mutant()->lodParent( *prevPrevModelFile, 
		currModelFile->substr( 0, currModelFile->rfind(".") ) );

	if (nextModelFile)
	{
		MeApp::instance().mutant()->lodParent( *prevModelFile, 
			nextModelFile->substr( 0, nextModelFile->rfind(".") ) );

		MeApp::instance().mutant()->lodExtent( *prevModelFile, currModelExtent );
	}
	else
	{
		MeApp::instance().mutant()->lodParent( *prevModelFile, "" );
	}

	MeApp::instance().mutant()->lodParent( *currModelFile, 
		prevModelFile->substr( 0, prevModelFile->rfind(".") ) );

	MeApp::instance().mutant()->lodExtent( *currModelFile, prevModelExtent );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/LOD_UP"), false );
		
	MeApp::instance().mutant()->reloadAllLists();

	OnUpdateLODList();

	setSel( i-1 );
}

/*~ function ModelEditor.moveLodUp
 *	@components{ modeleditor }
 *
 *	This function moves the currently selected LOD model higher up the LOD list.
 */
static PyObject * py_moveLodUp( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodMoveUp();

	Py_Return;
}
PY_MODULE_FUNCTION( moveLodUp, ModelEditor )

void PageLOD::lodMoveDown()
{
	if ( !lodCanMoveDown() ) return;

	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData(i-1));
	
	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(i));
	float currModelExtent = MeApp::instance().mutant()->lodExtent( *currModelFile );
	
	std::string* nextModelFile = (std::string*)(pImpl_->list.GetItemData(i+1));
	float nextModelExtent = MeApp::instance().mutant()->lodExtent( *nextModelFile );

	std::string* nextNextModelFile = (std::string*)(pImpl_->list.GetItemData(i+2));

	MeApp::instance().mutant()->lodParent( *prevModelFile, 
		nextModelFile->substr( 0, nextModelFile->rfind(".") ) );

	if (nextNextModelFile)
	{
		MeApp::instance().mutant()->lodParent( *currModelFile, 
			nextNextModelFile->substr( 0, nextNextModelFile->rfind(".") ) );

		MeApp::instance().mutant()->lodExtent( *currModelFile, nextModelExtent );
	}
	else
	{
		MeApp::instance().mutant()->lodParent( *currModelFile, "" );
	}

	MeApp::instance().mutant()->lodParent( *nextModelFile, 
		currModelFile->substr( 0, currModelFile->rfind(".") ) );

	MeApp::instance().mutant()->lodExtent( *nextModelFile, currModelExtent );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/LOD_DOWN"), false );
	
	MeApp::instance().mutant()->reloadAllLists();

	OnUpdateLODList();

	setSel( i+1 );
}

/*~ function ModelEditor.moveLodDown
 *	@components{ modeleditor }
 *
 *	This function moves the currently selected LOD model lower down the LOD list.
 */
static PyObject * py_moveLodDown( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodMoveDown();

	Py_Return;
}
PY_MODULE_FUNCTION( moveLodDown, ModelEditor )

void PageLOD::lodSetToDist()
{
	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* modelFile = (std::string*)(pImpl_->list.GetItemData(i));
	
	MeApp::instance().mutant()->lodExtent( *modelFile, floorf(pImpl_->lodDist + 0.5f) );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/CHANGING_EXTENT"), false );

	MeApp::instance().mutant()->reloadModel();

	OnUpdateLODList();

	setSel( i );

}

/*~ function ModelEditor.lodSetToDist
 *	@components{ modeleditor }
 *
 *	This function sets the LOD model's state to 'Hidden'. If the LOD model is 
 *	the highest on the LOD list then its maximum LOD distance is set to the distance
 *	between the model and the camera position.
 */
static PyObject * py_setLodToDist( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodSetToDist();

	Py_Return;
}
PY_MODULE_FUNCTION( setLodToDist, ModelEditor )

void PageLOD::lodExtendForever()
{
	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* modelFile = (std::string*)(pImpl_->list.GetItemData(i));
	
	MeApp::instance().mutant()->lodExtent( *modelFile, LOD_HIDDEN );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/INFINITY"), false );

	MeApp::instance().mutant()->reloadModel();

	OnUpdateLODList();

	setSel( i );

}

/*~ function ModelEditor.extendLodForever
 *	@components{ modeleditor }
 *
 *	This function sets the currently selected LOD model's maximum LOD distance to infinity,
 *	actively making this LOD model visible for all distances further than its minimum LOD distance.
 */
static PyObject * py_extendLodForever( PyObject * args )
{
	if (PageLOD::currPage())
		PageLOD::currPage()->lodExtendForever();

	Py_Return;
}
PY_MODULE_FUNCTION( extendLodForever, ModelEditor )

bool PageLOD::lodIsSelected()
{
	return (pImpl_->list.GetSelectionMark() != -1);
}

/*~ function ModelEditor.lodSelected
 *	@components{ modeleditor }
 *
 *	This function returns whether a LOD model is currently selected.
 *
 *	@return Returns True (1) if a LOD model is currently selected, False (0) otherwise.
 */
static PyObject * py_lodSelected( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodIsSelected() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( lodSelected, ModelEditor )

bool PageLOD::lodIsLocked()
{
	return (pImpl_->locked);
}

/*~ function ModelEditor.isLockedLod
 *	@components{ modeleditor }
 *
 *	This function returns whether the selected LOD model is locked.
 *	If the 'lock LOD parents' option is enabled in Preferences dialog, then LOD models
 *	that are LOD parents will be locked from modifications.
 *
 *	@return Returns True (1) if the LOD model is locked, False (0) otherwise.
 */
static PyObject * py_isLockedLod( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodIsLocked() );
	return PyInt_FromLong( 1 );
}
PY_MODULE_FUNCTION( isLockedLod, ModelEditor )

bool PageLOD::lodIsFirst()
{
	return (pImpl_->list.GetSelectionMark() == 0);
}

/*~ function ModelEditor.isFirstLod
 *	@components{ modeleditor }
 *
 *	This function returns whether the currently selected LOD model is first on the LOD list.
 *
 *	@return Returns True (1) if the LOD model is first on the LOD list, False (0) otherwise.
 */
static PyObject * py_isFirstLod( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodIsFirst() );
	return PyInt_FromLong( 1 );
}
PY_MODULE_FUNCTION( isFirstLod, ModelEditor )

bool PageLOD::lodCanMoveUp()
{
	return (pImpl_->list.GetSelectionMark() > 1);
}

/*~ function ModelEditor.canMoveLodUp
 *	@components{ modeleditor }
 *
 *	This function returns whether the currently selected LOD model can be moved up the LOD list.
 *
 *	@return Returns True (1) if the LOD model can be moved up the LOD list, False (0) otherwise.
 */
static PyObject * py_canMoveLodUp( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodCanMoveUp() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( canMoveLodUp, ModelEditor )

bool PageLOD::lodCanMoveDown()
{
	return (pImpl_->list.GetSelectionMark() != 0) && 
		(pImpl_->list.GetSelectionMark() != pImpl_->list.GetItemCount() - 1);
}

/*~ function ModelEditor.canMoveLodDown
 *	@components{ modeleditor }
 *
 *	This function returns whether the currently selected LOD model can be moved down the LOD list.
 *
 *	@return Returns True (1) if the LOD model can be moved down the LOD list, False (0) otherwise.
 */
static PyObject * py_canMoveLodDown( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodCanMoveDown() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( canMoveLodDown, ModelEditor )

bool PageLOD::lodIsMissing( int sel )
{
	LODList* lodList = MeApp::instance().mutant()->lodList();
	if (lodList == NULL) return true;

	if (sel < 0) return true;

	int size = lodList->size();
	if (sel >= size) return true;

	return ((*lodList)[ sel ].second == LOD_MISSING);
}

bool PageLOD::lodIsMissing()
{
	return lodIsMissing( pImpl_->list.GetSelectionMark() );
}

/*~ function ModelEditor.lodIsMissing
 *	@components{ modeleditor }
 *
 *	Checks whether the currently selected LOD is missing.
 *	A missing LOD is one that is referenced in the main model's LOD list, but 
 *	was not loaded successfully.
 *
 *	@return	Returns True (1) if the LOD is missing, False (0) otherwise.
 */
static PyObject * py_isMissingLod( PyObject * args )
{
	if (PageLOD::currPage())
		return PyInt_FromLong( PageLOD::currPage()->lodIsMissing() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( isMissingLod, ModelEditor )

void PageLOD::setSel( int sel )
{
	pImpl_->ignoreSelChange = true;
	sel = (sel < pImpl_->list.GetItemCount() - 1) ? sel : pImpl_->list.GetItemCount() - 1;
	pImpl_->list.SetItemState( sel, LVIS_SELECTED, LVIS_SELECTED );
	pImpl_->list.SetSelectionMark( sel );
	pImpl_->needsUpdate = true;
	pImpl_->ignoreSelChange = false;
}

void PageLOD::OnLvnItemchangedLodList(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pImpl_->ignoreSelChange) return;

	if (GetFocus()->GetSafeHwnd() == pImpl_->min.GetSafeHwnd())
	{
		updateMin();
	}
	if (GetFocus()->GetSafeHwnd() == pImpl_->max.GetSafeHwnd())
	{
		updateMax();
	}

	//Wait until the fields are ready before trying again
	pImpl_->needsUpdate = true;

	*pResult = 0;
}

void PageLOD::updateMin()
{
	if (pImpl_->updating) return;
	
	if (pImpl_->needsUpdate) return;
	
	int i = pImpl_->list.GetSelectionMark();

	if ((i==-1) || (i==0)) return;

	std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData(i-1));
	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(i));
	
	float dist = pImpl_->min.isEmpty() ? LOD_HIDDEN : pImpl_->min.GetValue();
	
	//Exit if there has been no change
	if ( dist == LOD_HIDDEN && MeApp::instance().mutant()->isHidden( *currModelFile ) )
		return;
	if ( MeApp::instance().mutant()->lodExtent( *prevModelFile ) == dist)
		return;

	MeApp::instance().mutant()->lodExtent( *prevModelFile, dist );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/CHANGING_EXTENT"), false );

	MeApp::instance().mutant()->reloadModel();

	OnUpdateLODList();

	setSel( i );

}

void PageLOD::updateMax()
{
	if (pImpl_->updating) return;
	
	if (pImpl_->needsUpdate) return;
	
	int i = pImpl_->list.GetSelectionMark();

	if (i==-1) return;

	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(i));

	float dist = pImpl_->max.isEmpty() ? LOD_HIDDEN : pImpl_->max.GetValue();
	
	//Exit if there has been no change
	if ( dist == LOD_HIDDEN && MeApp::instance().mutant()->isHidden( *currModelFile ) )
		return;
	if ( MeApp::instance().mutant()->lodExtent( *currModelFile ) == dist)
		return;

	MeApp::instance().mutant()->lodExtent( *currModelFile, dist );

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/CHANGING_EXTENT"), false );
		
	MeApp::instance().mutant()->reloadModel();

	OnUpdateLODList();

	setSel( i );

}

void PageLOD::OnBnClickedLodHidden()
{
	int sel = pImpl_->list.GetSelectionMark();

	if (sel==-1) return;

	std::string* currModelFile = (std::string*)(pImpl_->list.GetItemData(sel));

	if (pImpl_->hidden.GetCheck())
	{
		MeApp::instance().mutant()->lodExtent( *currModelFile, 0.f );
	}
	else
	{
		
		LODList* lodList = MeApp::instance().mutant()->lodList();
		float extent = 0.f;
		float lastExtent = 0.f;
		bool lastModelSet = false;
		unsigned lastModel = 0;
		unsigned i;
		for (i=0; i<lodList->size(); i++)
		{
			if (((*lodList)[i].second == LOD_HIDDEN) || ((*lodList)[i].second > extent))
			{
				lastModel = i;
				lastModelSet = true;
				if (i > (unsigned)sel) break;
				lastExtent = extent;
				extent = (*lodList)[i].second;
				if (extent == LOD_HIDDEN) break;
			}
		}

		if (extent != LOD_HIDDEN)
		{
			lastExtent = extent;
			if (i < lodList->size())
				extent = (*lodList)[i].second;
				
			if ((i != lodList->size()) && (extent != LOD_HIDDEN))
			{
				MeApp::instance().mutant()->lodExtent( *currModelFile, floorf((lastExtent + extent) / 2.f) );
			}
			else
			{
				MeApp::instance().mutant()->lodExtent( *currModelFile, lastExtent + 10.f );
			}
		}
		else
		{
			if (lastModelSet)
			{
				std::string* prevModelFile = (std::string*)(pImpl_->list.GetItemData( lastModel ));
				MeApp::instance().mutant()->lodExtent( *prevModelFile, lastExtent + 10.f );
			}
			MeApp::instance().mutant()->lodExtent( *currModelFile, LOD_HIDDEN );
		}
	}

	UndoRedo::instance().barrier( L("MODELEDITOR/PAGES/PAGE_LOD/TOGGLE_VIS"), false );
	
	MeApp::instance().mutant()->reloadModel();

	OnUpdateLODList();

	setSel( sel );
}

void PageLOD::OnBnClickedLodVirtualDist()
{
	pImpl_->useVirtualDist = !!pImpl_->virtualDist.GetCheck();
	if (!pImpl_->useVirtualDist) 
	{
		MeApp::instance().mutant()->virtualDist();
		pImpl_->distSlider.SetPos( (int)(1024.f * pImpl_->lodDist / pImpl_->lodBar->maxRange()) );
		pImpl_->dist.SetValue( pImpl_->lodDist );
	}
}

bool PageLOD::modelDrop( UalItemInfo* ii )
{
	if (locked())
	{
		::MessageBox( this->GetSafeHwnd(),
			L("MODELEDITOR/PAGES/PAGE_LOD/UNABLE_LOD"),
			L("MODELEDITOR/PAGES/PAGE_LOD/LOD_LOCKED"), MB_OK | MB_ICONERROR );
		return false;
	}
	
	addNewModel( ii->longText() );
	return true;
}
