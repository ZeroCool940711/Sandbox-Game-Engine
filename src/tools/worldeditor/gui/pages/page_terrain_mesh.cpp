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
#include "worldeditor/gui/pages/page_terrain_mesh.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "guimanager/gui_manager.hpp"


// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const std::string PageTerrainMesh::contentID = "PageTerrainMesh";


PageTerrainMesh::PageTerrainMesh()
	: PageTerrainBase(PageTerrainMesh::IDD),
	pageReady_( false )
{
    sizeEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
}


PageTerrainMesh::~PageTerrainMesh()
{
}


void PageTerrainMesh::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_SLIDER, sizeSlider_);
	DDX_Control(pDX, IDC_TERRAIN_SIZE_EDIT, sizeEdit_);
	DDX_Control(pDX, IDC_TERRAIN_CUT, cut_);
	DDX_Control(pDX, IDC_TERRAIN_REPAIR, repair_);
	DDX_Control(pDX, IDC_SIZERANGE, mSizeRange);
}


BEGIN_MESSAGE_MAP(PageTerrainMesh, PageTerrainBase)
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE (WM_ACTIVATE_TOOL, OnActivateTool)
	ON_MESSAGE (WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_TERRAIN_CUT, OnBnClickedTerrainCut)
	ON_BN_CLICKED(IDC_TERRAIN_REPAIR, OnBnClickedTerrainRepair)
	ON_EN_CHANGE(IDC_TERRAIN_SIZE_EDIT, OnEnChangeTerrainSizeEdit)
	ON_BN_CLICKED(IDC_SIZERANGE, OnBnClickedSizerange)
END_MESSAGE_MAP()


// PageTerrainMesh message handlers


LRESULT PageTerrainMesh::OnActivateTool(WPARAM wParam, LPARAM lParam)
{
	const char *activePageId = (const char *)(wParam);
	if (getContentID() == activePageId)
	{
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgcTerrain", "TerrainTools");
		}
	}
	return 0;
}



BOOL PageTerrainMesh::OnInitDialog()
{
	PageTerrainBase::OnInitDialog();

	sizeSlider_.setRangeLimit( Options::getOptionFloat( "terrain/cutRepair/minsizelimit", 1 ),
		Options::getOptionFloat( "terrain/cutRepair/maxsizelimit", 4000 ) );
	sizeSlider_.setRange( Options::getOptionFloat( "terrain/cutRepair/minsize", 1 ),
		Options::getOptionFloat( "terrain/cutRepair/maxsize", 800 ) );
	sizeSlider_.setValue( Options::getOptionFloat( "terrain/cutRepair/size", 1 ) );

	OnUpdateControls(0, 0);
	updateSliderEdits();

	mSizeRange.SetWindowPos( NULL, 0, 0, 16, 16, SWP_NOMOVE | SWP_NOZORDER );
	mSizeRange.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );

	pageReady_ = true;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


afx_msg void PageTerrainMesh::OnSize( UINT nType, int cx, int cy )
{
	CDialog::OnSize( nType, cx, cy );

	if ( !pageReady_ )
		return;

	// move the controls as appropriate
	// stretch the file selector to be long enough to fit into the window
	CRect rect;
	GetClientRect(rect);
	static const int MARGIN = 10;
	const int newWidth = std::max( 1, rect.Width() - MARGIN * 2 );

	CRect sliderRect, buttonRect;
	sizeSlider_.GetWindowRect(sliderRect);
	mSizeRange.GetWindowRect(buttonRect);
	ScreenToClient( &sliderRect );
	ScreenToClient( &buttonRect );

	mSizeRange.SetWindowPos(NULL, rect.Width() - MARGIN - buttonRect.Width(), buttonRect.top,
		0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

	sizeSlider_.SetWindowPos(NULL, 0, 0,
		newWidth - buttonRect.Width() - sliderRect.left, sliderRect.Height(),
		SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

	RedrawWindow();
}


void PageTerrainMesh::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// this captures all the scroll events for the page
	// upadate all the slider buddy windows
	updateSliderEdits();

	Options::setOptionFloat( "terrain/cutRepair/size", sizeSlider_.getValue() );

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageTerrainMesh::updateSliderEdits()
{
	int new_val = (int)sizeSlider_.getValue();
	int old_val = (int)sizeEdit_.GetIntegerValue();
	if (GetFocus() != &sizeEdit_ && new_val != old_val)
	{
		sizeEdit_.SetIntegerValue( new_val );
	}
}

afx_msg LRESULT PageTerrainMesh::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	sizeSlider_.setValue( Options::getOptionFloat( "terrain/cutRepair/size", 1 ) );

	updateSliderEdits();

	cut_.SetCheck( !Options::getOptionInt( "terrain/cutRepair/brushMode", 0 ) );
	repair_.SetCheck( Options::getOptionInt( "terrain/cutRepair/brushMode", 0 ) );

	return 0;
}

void PageTerrainMesh::OnBnClickedTerrainCut()
{
	Options::setOptionInt( "terrain/cutRepair/brushMode", 0 );
}

void PageTerrainMesh::OnBnClickedTerrainRepair()
{
	Options::setOptionInt( "terrain/cutRepair/brushMode", 1 );
}

void PageTerrainMesh::OnEnChangeTerrainSizeEdit()
{
	sizeSlider_.setValue( static_cast<float>(sizeEdit_.GetIntegerValue()) );
	Options::setOptionFloat( "terrain/cutRepair/size", sizeSlider_.getValue() );
}

void PageTerrainMesh::OnBnClickedSizerange()
{
	sizeSlider_.beginEdit();
}

/**
 *  This is called when each item is about to be drawn.  We want limit slider edit
 *	to be highlighted is they are out of bounds.
 *
 *	@param pDC	Contains a pointer to the display context for the child window.
 *	@param pWnd	Contains a pointer to the control asking for the color.
 *	@param nCtlColor	Specifies the type of control.
 *	@return	A handle to the brush that is to be used for painting the control
 *			background.
 */
HBRUSH PageTerrainMesh::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH brush = PageTerrainBase::OnCtlColor( pDC, pWnd, nCtlColor );

	sizeEdit_.SetBoundsColour( pDC, pWnd,
		sizeSlider_.getMinRangeLimit(), sizeSlider_.getMaxRangeLimit() );
		
	return brush;
}
