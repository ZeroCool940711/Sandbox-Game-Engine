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
#include "worldeditor/gui/pages/page_options_general.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "appmgr/options.hpp"
#include "romp/time_of_day.hpp"
#include "controls/user_messages.hpp"
#include "common/user_messages.hpp"
#include "common/utilities.hpp"
#include "cstdmf/debug.hpp"
#include "cstdmf/watcher.hpp"


DECLARE_DEBUG_COMPONENT2( "WorldEditor", 0 )

// OptionsShowTree

IMPLEMENT_DYNAMIC(OptionsShowTree, CTreeCtrl)
OptionsShowTree::OptionsShowTree()
	: mousePoint_(0, 0)
{
}

OptionsShowTree::~OptionsShowTree()
{
	//Clean up the item data
	for (unsigned i=0; i<itemData_.size(); i++)
	{
		delete itemData_[i];
	}
}

void OptionsShowTree::populate( DataSectionPtr data, HTREEITEM item /* = NULL */  )
{
	if (data == NULL) return;

	int numChildren = data->countChildren();

	for (int i=0; i<numChildren; i++)
	{
		std::string itemName = data->childSectionName(i);
		std::replace( itemName.begin(), itemName.end(), '_' , ' ');

		HTREEITEM newItem = InsertItem( itemName.c_str(), item );
		SetItemState(newItem, TVIS_EXPANDED, TVIS_EXPANDED);

		//We will store the action script name in the item data
		DataSectionPtr newData = data->openChild( i );
		std::string* itemData = new std::string( newData->asString() );
		SetItemData( newItem, (DWORD)itemData );
			
		//Push it onto a vector so we can delete it later
		itemData_.push_back( itemData );

		populate( newData, newItem );
	}
}

BEGIN_MESSAGE_MAP(OptionsShowTree, CTreeCtrl)
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(NM_CLICK, OnNMClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMClick)
END_MESSAGE_MAP()


void OptionsShowTree::OnMouseMove(UINT nFlags, CPoint point)
{
	mousePoint_ = point;

	CTreeCtrl::OnMouseMove(nFlags, point);
}

void OptionsShowTree::execute( HTREEITEM item )
{
	std::string* pyCommand = (std::string*)GetItemData( item );

	if ((pyCommand) && (*pyCommand != ""))
	{
		int enabled = 0;
		int checked = 0;

		MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
			"OptionsShowTree::execute: PythonAdapter is NULL" );

		bool propagateUp =
			WorldEditorApp::instance().pythonAdapter()->ActionScriptUpdate( 
				*pyCommand, enabled, checked ) && !checked;

		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute( *pyCommand );

		if ( propagateUp )
		{
			item = GetParentItem( item );
			while ( item != NULL )
			{
				std::string* pyCommand = (std::string*)GetItemData( item );
				if ((pyCommand) &&
					(*pyCommand != "") &&
					(WorldEditorApp::instance().pythonAdapter()->ActionScriptUpdate( 
						*pyCommand, enabled, checked)) &&
					(!checked))
					{
						// If the parent is not checked and the child is then check it...
						WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute( *pyCommand );
					}
				item = GetParentItem( item );
			}
		}
	}
}

void OptionsShowTree::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point = mousePoint_;

	UINT uFlags;
	HTREEITEM hItem = HitTest(point, &uFlags);

	if ((hItem == NULL) || !( TVHT_ONITEMSTATEICON & uFlags ))
	{
		return;
	}

	execute( hItem );

	*pResult = 0;
}

void OptionsShowTree::update()
{
	if (!WorldEditorApp::instance().pythonAdapter())
		return;

	HTREEITEM item = GetRootItem();

	while (item != NULL)
	{
		std::string* pyCommand = (std::string*)GetItemData( item );

		int enabled = 0;
		int checked = 0;
		if (WorldEditorApp::instance().pythonAdapter()->ActionScriptUpdate( 
				*pyCommand, enabled, checked ))
		{
			SetCheck(item, checked);
		}

		item = GetNextItem(item, TVGN_NEXTVISIBLE);
	}
}


// PageOptionsGeneral

// GUITABS content ID ( declared by the IMPLEMENT_BASIC_CONTENT macro )
const std::string PageOptionsGeneral::contentID = "PageOptionsGeneral";


PageOptionsGeneral::PageOptionsGeneral()
	: GraphicsSettingsTable(PageOptionsGeneral::IDD),
	pageReady_( false ),
	dontUpdateFarClipEdit_( false ),
	farClipEdited_( false ),
	lastLightsType_( -1 )
{
	farPlaneEdit_.SetNumericType(controls::EditNumeric::ENT_INTEGER);
}

PageOptionsGeneral::~PageOptionsGeneral()
{
}


/**
 *	Helper method let know the page that settings that take effect after the 
 *	application restarts. Must call the needsRestart method in the base class.
 *
 *  @param		graphics setting string (label or option)
 */
void PageOptionsGeneral::needsRestart( const std::string& setting )
{
	GraphicsSettingsTable::needsRestart( setting );
	onSizeInternal();
	messageText_.SetWindowText(
		(std::string( "* " ) + L("WORLDEDITOR/GUI/PAGE_OPTIONS_GENERAL/NEED_RESTART")).c_str() );
	messageText_.ShowWindow( SW_SHOW );
	RedrawWindow();
}


void PageOptionsGeneral::DoDataExchange(CDataExchange* pDX)
{
	GraphicsSettingsTable::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPTIONS_SHOW_TREE, showTree_);

	DDX_Control(pDX, IDC_OPTIONS_FARPLANE_SLIDER, farPlaneSlider_);
	DDX_Control(pDX, IDC_OPTIONS_FARPLANE_EDIT, farPlaneEdit_);

	DDX_Control(pDX, IDC_OPTIONS_LIGHTING_STANDARD, standardLighting_);
	DDX_Control(pDX, IDC_OPTIONS_LIGHTING_DYNAMIC, dynamicLighting_);
	DDX_Control(pDX, IDC_OPTIONS_LIGHTING_SPECULAR, specularLighting_);

	DDX_Control(pDX, IDC_OPTIONS_READ_ONLY_MODE, readOnlyMode_);
	DDX_Control(pDX, IDC_GRAPHICS_SETTINGS_MSG, messageText_);
}

/*afx_msg*/ HBRUSH PageOptionsGeneral::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );
	
	farPlaneEdit_.SetBoundsColour( pDC, pWnd,
		farPlaneEdit_.GetMinimum(), farPlaneEdit_.GetMaximum() );

	return brush;
}

void PageOptionsGeneral::InitPage()
{
	INIT_AUTO_TOOLTIP();
	CString name;
	GetWindowText(name);
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", name.GetBuffer());
	}

	std::string optionsPage = Options::getOptionString( "tools/optionsPage", "resources/data/options_page.xml" );
	
	DataSectionPtr optionsData = BWResource::openSection( optionsPage );

	if (optionsData != NULL)
	{
		showTree_.populate( optionsData );
		// This ModifyStyle call is a known workaround for a problem with CTreeCtrl
		// and horizontal ScrollBars. The resource must not have the CHECKBOXES
		// flag set.
		showTree_.ModifyStyle( 0, TVS_CHECKBOXES );
	}
	else
	{
		ERROR_MSG("There was a problem loading \"%s\"", optionsPage.c_str());
	}

	farPlaneSlider_.SetRangeMin(50);
	farPlaneSlider_.SetRangeMax((int)WorldManager::instance().getMaxFarPlane());
	farPlaneSlider_.SetPageSize(0);

	// set initial position and override any python feelings
	farPlaneSlider_.SetPos(Options::getOptionInt( "graphics/farclip", 500 ));
	farPlaneEdit_.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	farPlaneEdit_.SetAllowNegative( false );
	farPlaneEdit_.SetMinimum( 50 );
	farPlaneEdit_.SetMaximum( WorldManager::instance().getMaxFarPlane() );

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageOptionsGeneral::InitPage: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->onSliderAdjust("slrFarPlane", 
											farPlaneSlider_.GetPos(), 
											farPlaneSlider_.GetRangeMin(), 
											farPlaneSlider_.GetRangeMax());

//	OnUpdateControls(0, 0);
	updateSliderEdits();
}


BEGIN_MESSAGE_MAP(PageOptionsGeneral, GraphicsSettingsTable)
	ON_WM_SHOWWINDOW()
	ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_BN_CLICKED(IDC_OPTIONS_LIGHTING_STANDARD, OnBnClickedOptionsLightingStandard)
	ON_BN_CLICKED(IDC_OPTIONS_LIGHTING_DYNAMIC, OnBnClickedOptionsLightingDynamic)
	ON_BN_CLICKED(IDC_OPTIONS_LIGHTING_SPECULAR, OnBnClickedOptionsLightingSpecular)
	ON_BN_CLICKED(IDC_OPTIONS_READ_ONLY_MODE, OnBnClickedReadOnlyMode)
	ON_EN_CHANGE(IDC_OPTIONS_FARPLANE_EDIT, OnEnChangeOptionsFarplaneEdit)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// PageOptionsGeneral message handlers
afx_msg void PageOptionsGeneral::OnShowWindow( BOOL bShow, UINT nStatus )
{
	GraphicsSettingsTable::OnShowWindow( bShow, nStatus );

	if ( bShow == FALSE )
	{
	}
	else
	{
		OnUpdateControls( 0, 0 );
	}
}

afx_msg LRESULT PageOptionsGeneral::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	// must call the GraphicsSettingsTable method
	GraphicsSettingsTable::OnUpdateControls( wParam, lParam );

	if ( !pageReady_ )
	{
		InitPage();
		pageReady_ = true;
	}

	if ( !IsWindowVisible() )
		return 0;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageOptionsGeneral::OnUpdateControls: PythonAdapter is NULL" );
	showTree_.update();

	bool readOnly = Options::getOptionInt("objects/readOnlyMode", 0) != 0;
	readOnlyMode_.SetCheck(readOnly ? BST_CHECKED : BST_UNCHECKED);

	int enabled = 0;
	int checked = 0;

	//Ensure the correct light preview type is selected
	int lightType = Options::getOptionInt( "render/lighting");
	if ( lightType != lastLightsType_)
	{
		standardLighting_.SetCheck( lightType == 0 ? BST_CHECKED : BST_UNCHECKED );
		dynamicLighting_.SetCheck( lightType == 1 ? BST_CHECKED : BST_UNCHECKED );
		specularLighting_.SetCheck( lightType == 2 ? BST_CHECKED : BST_UNCHECKED );
		lastLightsType_ = lightType;
	}

	static int s_lastFarClip = -1;
	int farClip = farPlaneSlider_.GetPos();
	if ( farClipEdited_ )
	{
		s_lastFarClip = farClip;
		farClipEdited_ = false;
	}
	if (farClip != s_lastFarClip)
	{
		farPlaneEdit_.SetIntegerValue( farClip );
		s_lastFarClip = farClip;
	}

	return 0;
}

void PageOptionsGeneral::OnBnClickedOptionsLightingStandard()
{
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actLightingStd");
	}
}

void PageOptionsGeneral::OnBnClickedOptionsLightingDynamic()
{
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actLightingDynamic");
	}
}

void PageOptionsGeneral::OnBnClickedOptionsLightingSpecular()
{
	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->ActionScriptExecute("actLightingSpecular");
	}
}

void PageOptionsGeneral::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// this captures all the scroll events for the page
	// upadate all the slider buddy windows
	updateSliderEdits();

	if (WorldEditorApp::instance().pythonAdapter())
	{
		WorldEditorApp::instance().pythonAdapter()->onSliderAdjust("slrFarPlane", 
												farPlaneSlider_.GetPos(), 
												farPlaneSlider_.GetRangeMin(), 
												farPlaneSlider_.GetRangeMax());

		Options::setOptionInt( "graphics/farclip", farPlaneSlider_.GetPos() );

		if( !dontUpdateFarClipEdit_ )
			farPlaneEdit_.SetIntegerValue( farPlaneSlider_.GetPos() );
	}

	GraphicsSettingsTable::OnHScroll(nSBCode, nPos, pScrollBar);
}

void PageOptionsGeneral::updateSliderEdits()
{
	farPlaneEdit_.SetIntegerValue(farPlaneSlider_.GetPos());
}

void PageOptionsGeneral::OnBnClickedReadOnlyMode()
{
	bool readOnly = (readOnlyMode_.GetCheck() == BST_CHECKED);
	Options::setOptionInt("objects/readOnlyMode", readOnly);
}

void PageOptionsGeneral::OnEnChangeOptionsFarplaneEdit()
{
	farClipEdited_ = true;

	dontUpdateFarClipEdit_ = true;
	farPlaneSlider_.SetPos( farPlaneEdit_.GetIntegerValue() );
	dontUpdateFarClipEdit_ = false;

	MF_ASSERT( WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageOptionsGeneral::OnEnChangeOptionsFarplaneEdit: PythonAdapter is NULL" );
	WorldEditorApp::instance().pythonAdapter()->onSliderAdjust("slrFarPlane", 
		farPlaneSlider_.GetPos(), 
		farPlaneSlider_.GetRangeMin(), 
		farPlaneSlider_.GetRangeMax());

	Options::setOptionInt( "graphics/farclip", farPlaneSlider_.GetPos() );
}


/**
 *	The page has been resized.
 */
void PageOptionsGeneral::OnSize( UINT nType, int cx, int cy )
{
	GraphicsSettingsTable::OnSize( nType, cx, cy );
	onSizeInternal();
	RedrawWindow();
}


/**
 *	Helper method to layout the page's controls according to the page's size.
 */
void PageOptionsGeneral::onSizeInternal()
{
	// resize to correspond with the size of the wnd
	int unusedInt;
	CSize newSize;
	CSize unusedCSize;
	GetDeviceScrollSizes( unusedInt, newSize, unusedCSize, unusedCSize );

	CRect rectPage;
	GetClientRect(rectPage);
	if ( rectPage.Width() > newSize.cx )
		newSize.cx = rectPage.Width();
	if ( rectPage.Height() > newSize.cy )
		newSize.cy = rectPage.Height();

	CRect msgRect;
	messageText_.GetWindowRect( msgRect );

	int yOffset = 5;
	if ( GraphicsSettingsTable::needsRestart() )
		yOffset += msgRect.Height();

	Utilities::stretchToRight( this, showTree_, newSize.cx, 5 );

	Utilities::stretchToBottomRight(
		this, pImpl_->propertyList,
		newSize.cx, 5,
		newSize.cy, yOffset );

	Utilities::moveToBottom( this, messageText_, newSize.cy, 5 );
	Utilities::stretchToRight( this, messageText_, newSize.cx, 5 );
}
