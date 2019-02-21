// page_scene_browser.cpp: archivo de implementación
//

#include "pch.hpp"
#include "resource.h"
#include "worldeditor/gui/pages/page_scene_browser.h"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/editor/editor_group.hpp"

#include "common/user_messages.hpp"

//BaseItem
BaseItem::BaseItem():
parent_(NULL),
isGroup_(false){
}

//Item
Item::Item(ChunkItem* chunk):BaseItem(),chunkItem_(chunk){
}

Item::~Item(){
	delete chunkItem_;
}

void Item::create(CWnd *parent){

	parent_ = parent;

}

//GroupItem
GroupItem::GroupItem(CString title):BaseItem(),
expanded_(false),title_(title){
	isGroup_ = true;
}

GroupItem::~GroupItem(){
	for(uint i =0; i<items_.size();i++){
		delete items_[0];
	}
}

void GroupItem::create(CWnd *parent){
	parent_ = parent;
}

//GroupManager
GroupManager::GroupManager(){

}

GroupManager::~GroupManager(){
}

void GroupManager::groupBy(CString groupBy){


	groups_.clear();
	//GroupBy Logic
	if(groupBy == "Chunk"){
		for(ChunkMap::iterator it = chunks_.begin(); it != chunks_.end(); ++it){
			for (uint i = 0; i < it->second.size(); i++){//probably not needed, just in case. It is a vector with only one item.
				Chunk* chunk = it->second[i];

				GroupItem* group = new GroupItem(CString(chunk->identifier().c_str()));

				for(uint j=0; j<chunk->getSelfItems().size(); j++){
					if(chunk->getSelfItems()[j]->edClassName() != "Portal")
						group->addItem(new Item(chunk->getSelfItems()[j].get()));
				}

				groups_.push_back(group);
			}
		}
	}else if(groupBy == "Asset Type"){
		for(ChunkMap::iterator it = chunks_.begin(); it != chunks_.end(); ++it){
			for (uint i = 0; i < it->second.size(); i++){//probably not needed, just in case. It is a vector with only one item.
				Chunk* chunk = it->second[i];

				for(uint j=0; j<chunk->getSelfItems().size(); j++){
					if(chunk->getSelfItems()[j]->edClassName() != "Portal"){
						//find group, if not exist create it
						bool finded = false;
						for(uint f = 0; f<groups_.size();f++){
							if(groups_[f]->getTitle() == chunk->getSelfItems()[j]->edClassName())
							{
								finded = true;
								groups_[f]->addItem(new Item(chunk->getSelfItems()[j].get()));
								break;
							}
						}
						if(!finded){
							groups_.push_back(new GroupItem(chunk->getSelfItems()[j]->edClassName()));
							groups_[groups_.size()-1]->addItem(new Item (chunk->getSelfItems()[j].get()));
						}				
												
					}
						
				}
			}
		}
	}

}

//ListSceneBrowser

IMPLEMENT_DYNAMIC(ListSceneBrowser, CListBox)
ListSceneBrowser::ListSceneBrowser()
{
	groupManager_ = new GroupManager();
}

ListSceneBrowser::~ListSceneBrowser()
{
	//Clean up the item data
}


void ListSceneBrowser::setSearchString( const CString searchString ){
	MessageBox(searchString);
}

void ListSceneBrowser::setGroupByString( const CString groupBy ){
		
	groupManager_->groupBy(groupBy);

	Draw();

}

void ListSceneBrowser::Draw(){
	//SetRedraw(FALSE);

	std::vector<GroupItem*> group = groupManager_->getGroup();
	ResetContent();
	int cont=0;
	for(uint i=0;i<group.size();i++){
		cont+=group[i]->getItems().size();
	}
	std::stringstream st;
	st<<cont;
	MessageBox(st.str().c_str());
	for(std::vector<GroupItem*>::iterator it=group.begin();it != group.end();++it){
		int index = InsertString( -1, "" );
		SetItemDataPtr(index, *it);
		(*it)->create(this);
		
		//for(std::vector<Item*>::iterator it2 = (*it)->getItems().begin(); it2 != (*it)->getItems().end(); it2++){
		//	(*it2)->create(this);//to create buttons
		//}
	}
	
	//SetRedraw(TRUE);
	//Invalidate();
}

void ListSceneBrowser::PreSubclassWindow() 
{
	dividerPos_ = 0;
	dividerMove_ = false;
	dividerTop_ = 0;
	dividerBottom_ = 0;
	dividerLastX_ = 0;

	cursorSize_ = AfxGetApp()->LoadStandardCursor(IDC_SIZEWE);
	cursorArrow_ = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
}

void ListSceneBrowser::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct) 
{
	lpMeasureItemStruct->itemHeight = 20;
}

const int childIndent = 16;

void ListSceneBrowser::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{

	//if (delayRedraw_) return;

	UINT index = lpDrawItemStruct->itemID;
	if (index == (UINT)-1)
		return;

	BaseItem* item = (BaseItem*)GetItemDataPtr( index );
	// draw two rectangles, one for the property lable, one for the value
	// (unless is a group)
	int nCrBackground, nCrText;
	if( (lpDrawItemStruct->itemAction | ODA_SELECT) && 
		(lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(!item->isGroup()) )
	{
		nCrBackground = COLOR_HIGHLIGHT;
		nCrText = COLOR_HIGHLIGHTTEXT;
	}
	else
	{
		nCrBackground = COLOR_WINDOW;
		nCrText = COLOR_WINDOWTEXT;
	}
	COLORREF crBackground = ::GetSysColor(nCrBackground);
	COLORREF crText = ::GetSysColor(nCrText);

	CRect rectItem = lpDrawItemStruct->rcItem;

	CDC dc;
	dc.Attach( lpDrawItemStruct->hDC );
	dc.FillSolidRect(rectItem, crBackground);

	int border = 1;
	rectItem.right -= border;
	rectItem.left += border;
	rectItem.top += border;
	rectItem.bottom -= border;

	CRect rectLabel = lpDrawItemStruct->rcItem;
	CRect rectValue = lpDrawItemStruct->rcItem;
	CRect rectColour = lpDrawItemStruct->rcItem;

	rectLabel.left =  (item->isGroup()? 1 : 0) * childIndent;


	if (item->isGroup())
	{
		// change the background colour
		nCrBackground = COLOR_INACTIVECAPTIONTEXT;
		crBackground = ::GetSysColor(nCrBackground);
		//dc.FillSolidRect(rectLabel, crBackground);
		CRect rectForTitleGroup = rectLabel;
		rectForTitleGroup.left += 5;
		std::stringstream st;
		st<<((GroupItem*)item)->getGroupDepth();
		dc.DrawText( ((GroupItem*)item)->getTitle()+ " ("+st.str().c_str()+" items)",rectForTitleGroup, DT_LEFT | DT_SINGLELINE );

		// + / -
		CRect rcSign(lpDrawItemStruct->rcItem);
		rcSign.top = (int)(0.5f * (rcSign.bottom - rcSign.top)) + rcSign.top;
		rcSign.bottom = rcSign.top;
		rcSign.right = rectLabel.left - (int)(childIndent * 0.5f);
		rcSign.left = rcSign.right;
		rcSign.InflateRect(5, 5, 7, 7);

		dc.DrawEdge( rcSign, EDGE_RAISED, BF_RECT );

		CPoint ptCenter( rcSign.CenterPoint() );
		ptCenter.x -= 1;
		ptCenter.y -= 1;

		CPen pen_( PS_SOLID, 1, crText );
		CPen* oldPen = dc.SelectObject( &pen_ );

		// minus		
		dc.MoveTo(ptCenter.x - 3, ptCenter.y);
		dc.LineTo(ptCenter.x + 4, ptCenter.y);

		GroupItem * gItem = (GroupItem *)(item);
		if (!gItem->getExpanded())
		{
			// plus
			dc.MoveTo(ptCenter.x, ptCenter.y - 3);
			dc.LineTo(ptCenter.x, ptCenter.y + 4);
		}

		dc.SelectObject( oldPen );
	}

	dc.DrawEdge( rectLabel, EDGE_ETCHED,BF_BOTTOMRIGHT );

	border = 3;
	rectLabel.right -= border;
	rectLabel.left += border + childIndent + 5;
	rectLabel.top += border;
	rectLabel.bottom -= border;

	rectValue.right -= border;
	rectValue.left += border;
	rectValue.top += border;
	rectValue.bottom -= border;

	// write the property name in the first rectangle
	// value in the second rectangle
	COLORREF crOldBkColor = dc.SetBkColor(crBackground);
	COLORREF crOldTextColor = dc.SetTextColor(crText);

	if(!item->isGroup()){
		dc.DrawText(((Item*) item)->getChunkItem()->edClassName(), rectLabel, DT_LEFT | DT_SINGLELINE);
	}

	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();

}

BEGIN_MESSAGE_MAP(ListSceneBrowser, CListBox)
	ON_MESSAGE(WM_CHANGED_CHUNK_STATE, OnChangedChunk)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

void ListSceneBrowser::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if (dividerMove_)
	{
		// redraw columns for new divider position
		dividerMove_ = false;

		//if mouse was captured then release it
		if (GetCapture() == this)
			::ReleaseCapture();

		::ClipCursor(NULL);

		DrawDivider(point.x);	// remove the divider

		//set the divider position to the new value
		dividerPos_ = point.x + 2;

		//Do this to ensure that the value field(s) are moved to the new divider position
		//selChange( false );

		//redraw
		Invalidate();
	}
	else
	{
		// select the item under the cursor
		BOOL out;
		UINT index = ItemFromPoint(point,out);
		//if (!out && index != (uint16)-1)
		//	Select( index );

		CListBox::OnLButtonUp(nFlags, point);
	}
}

void ListSceneBrowser::OnLButtonDown(UINT nFlags, CPoint point) 
{
	BOOL out;
	int index = ItemFromPoint(point,out);

	CRect rect;
	GetItemRect( index, rect );

	//Determine the item
	BaseItem* item = NULL;
	if ((!out) && (index != (uint16)(-1)) && (rect.PtInRect( point )))
		item = (BaseItem*)GetItemDataPtr( index );

	if ((point.x >= dividerPos_ - 4) && (point.x <= dividerPos_ - 1))
	{
		// get ready to resize the divider
		::SetCursor(cursorSize_);

		// keep mouse inside the control
		CRect windowRect;
		GetWindowRect(windowRect);
		windowRect.left += 10; windowRect.right -= 10;
		::ClipCursor(windowRect);

		//Unselect the item first
		//deselectCurrentItem();

		CRect clientRect;
		GetClientRect(clientRect);
		dividerMove_ = true;
		dividerTop_ = clientRect.top;
		dividerBottom_ = clientRect.bottom;
		dividerLastX_ = point.x;

		DrawDivider(dividerLastX_);

		//capture the mouse
		SetCapture();
		SetFocus();

		return;
	}

	// select the item
	CListBox::OnLButtonDown(nFlags, point);

	if (item)
	{
		if (item->isGroup())
		{
			GroupItem * gItem = (GroupItem *)(item);
			int xBoundUpper = childIndent;
			int xBoundLower = 0;

			if (point.x >= xBoundLower  &&  point.x <= xBoundUpper)
			{
				if (gItem->getExpanded())
					collapseGroup( gItem, index );
				else
					expandGroup( gItem, index );
			}
		}
	}

	dividerMove_ = false;
}

/*afx_msg*/ LRESULT
ListSceneBrowser::OnChangedChunk(WPARAM wparam, LPARAM lparam)
{
	//variables needed for future work: update only the chunk that changed its state
	//	int16 x = (int16)wparam;
	//	int16 z = (int16)lparam;
	
	return 0;
}

void ListSceneBrowser::DrawDivider(int xpos)
{
	CClientDC dc(this);
	int nOldMode = dc.SetROP2(R2_NOT);	// draw inverse of screen colour
	dc.MoveTo(xpos, dividerTop_);
	dc.LineTo(xpos, dividerBottom_);
	dc.SetROP2(nOldMode);
}

void ListSceneBrowser::collapseGroup(GroupItem* gItem, int index)
{
	if (!gItem->getExpanded())
		return;

	std::vector<Item*> & children = gItem->getItems();

	// hide all the children
	for (std::vector<Item*>::iterator it = children.begin();
		it != children.end();
		it++)
	{
		DeleteString(index + 1);

		// remove all of their children
		if ( (*it)->isGroup() ) 
		{
			GroupItem * g = (GroupItem *)(*it);
			collapseGroup( g, index + 1 );
		}
	}

	gItem->setExpanded( false );
}

void ListSceneBrowser::expandGroup(GroupItem* gItem, int index)
{
	if (gItem->getExpanded())
		return;

	std::vector<Item*> children = gItem->getItems();

	// show all the children (one 1 level)
	for (std::vector<Item*>::iterator it = children.begin();
		it != children.end();
		it++)
	{
		index++;
		InsertString( index, "" );
		SetItemDataPtr(index, *it);
		(*it)->create(this);
	}

	gItem->setExpanded( true );
}

void ListSceneBrowser::insertElements(const ChunkMap chunk){
	groupManager_->setChunkItems(chunk);
	groupManager_->groupBy("Chunk");
	Draw();
}


// SceneBrowser

const std::string PageSceneBrowser::contentID = "PageSceneBrowser";


PageSceneBrowser::PageSceneBrowser()
: CFormView(PageSceneBrowser::IDD),
pageReady_(false)
{
	lastPos_ = Vector3();
}

PageSceneBrowser::~PageSceneBrowser()
{
}

void PageSceneBrowser::InitPage()
{
	groupBy_.AddString("Chunk");
	groupBy_.AddString("Asset Type");
	groupBy_.AddString("Asset Name");
	groupBy_.AddString("Path");
	groupBy_.AddString("Primitives");
	groupBy_.AddString("Tris");
	groupBy_.SetCurSel(0);

	cancelIcon_.ShowWindow(false);

	pageReady_ = true;

	UpdateChunkView();
}

void PageSceneBrowser::UpdateChunkView()
{	
	ChunkSpacePtr space = ChunkManager::instance().cameraSpace();
	if(!space.exists())
		return;

	list_.insertElements(space->chunks());

	//chunks_ = space->chunks();
	//list_.DeleteAllItems();

	/*int nItem = 0; 

	int nume = 0;

	for(ChunkMap::iterator it = chunks_.begin(); it != chunks_.end(); ++it){*/

	//TODO: need optimization, it takes to much time for complex spaces

	//for (uint i = 0; i < it->second.size(); i++)//probably not needed, just in case
	//{
	//	Chunk* chunk = it->second[i];
	//nItem = list_.InsertItem(0, chunk->identifier().c_str());
	//CString  num;
	//num.Format("%d",nume++);
	/*
	CString  sizeStaticItems;
	sizeStaticItems.Format("%d",chunk->sizeStaticItems());*//*
	nItem = list_.InsertItem(0, num);
	list_.SetItemText(nItem,1, chunk->identifier().c_str());*/
	//list_.SetItemText(nItem, 2, std::string(chunk->mapping()->path()+"/"+chunk->mapping()->outsideChunkIdentifier(chunk->x(),chunk->z())).c_str());
	/*list_.SetItemText(nItem,3, chunk->binFileName().c_str());
	list_.SetItemText(nItem,4, chunk->resourceID().c_str());
	CString  aux;
	aux.Format("%d",chunk->hasInternalChunks());
	list_.SetItemText(nItem,5, aux);
	list_.SetItemText(nItem,6, chunk->label().c_str());*/

	//CString  self;
	//self.Format("%d",chunk->getSelfItems().size());
	//list_.SetItemText(nItem,2, self);

	//CString  dyno;
	//dyno.Format("%d",chunk->getDynoItems().size());
	//list_.SetItemText(nItem,3, dyno);

	//CString  sway;
	//sway.Format("%d",chunk->getSwayItems().size());
	//list_.SetItemText(nItem,4, sway);

	//listing selfItems of each Chunk
	//		for(uint j=0; j<chunk->getSelfItems().size(); j++){
	//			CString  num;
	//			num.Format("%d",++nume);
	//			nItem = list_.InsertItem(0, num);
	//			list_.SetItemText(nItem,1, chunk->identifier().c_str());
	//			list_.SetItemText(nItem,2,chunk->getSelfItems()[j]->edClassName());
	//			list_.SetItemText(nItem,3,chunk->getSelfItems()[j]->pOwnSect()->readString("resource").c_str());
	//			nume++;
	//		}
	//	}
	//}
}

void PageSceneBrowser::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_LIST1, list_);
	DDX_Control(pDX, IDC_SEARCH, searchEdit_);
	DDX_Control(pDX, IDC_LIST_SCENE_BROWSER, list_);
	DDX_Control(pDX, IDC_COMBO_GROUPBY, groupBy_);
	DDX_Control(pDX, IDC_CANCEL_SEARCH, cancelIcon_);
}

BEGIN_MESSAGE_MAP(PageSceneBrowser, CFormView)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_ACTIVATE_TOOL  , OnActivateTool  )
	ON_CBN_SELCHANGE(IDC_COMBO_GROUPBY, &PageSceneBrowser::OnCbnSelchangeComboGroupby)
	ON_STN_CLICKED(IDC_CANCEL_SEARCH, &PageSceneBrowser::OnStnClickedCancelSearch)
	ON_STN_CLICKED(IDC_SEARCH_ICON, &PageSceneBrowser::OnStnClickedSearchIcon)
	ON_EN_UPDATE(IDC_SEARCH, &PageSceneBrowser::OnEnUpdateSearch)
END_MESSAGE_MAP()


// Diagnósticos de SceneBrowser

#ifdef _DEBUG
void SceneBrowser::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void SceneBrowser::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// Controladores de mensajes de SceneBrowser
afx_msg LRESULT PageSceneBrowser::OnUpdateControls(WPARAM wParam, LPARAM lParam)
{
	if ( !pageReady_ )
	{
		InitPage();
		pageReady_ = true;
	}
	if ( !IsWindowVisible() )
		return 0;
	//get changes on camera
	Matrix view = WorldEditorCamera::instance().currentCamera().view();
	view.invert();
	Vector3 pos = view.applyToUnitAxisVector(3);
	if ((fabsf( pos.x - lastPos_.x ) > 0.1f) ||
		(fabsf( pos.y - lastPos_.y ) > 0.1f) ||
		(fabsf( pos.z - lastPos_.z ) > 0.1f))
	{
		UpdateChunkView();

		lastPos_ = pos;
	}

	return 0;
}
int PageSceneBrowser::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	//We might use this later...
	return 1;
}

afx_msg void PageSceneBrowser::OnShowWindow( BOOL bShow, UINT nStatus )
{
	CFormView::OnShowWindow( bShow, nStatus );

	if ( bShow == FALSE )
	{
	}
	else
	{
		OnUpdateControls( 0, 0 );
	}
}

LRESULT PageSceneBrowser::OnActivateTool( WPARAM wParam, LPARAM lParam )
{

	if ( !pageReady_ )
		InitPage();
	return 0;

}

void PageSceneBrowser::OnCbnSelchangeComboGroupby()
{
	// TODO: It is executed when user change option on the group by control
	CString groupByText;
	groupBy_.GetWindowText(groupByText);
	list_.setGroupByString(groupByText);
}

void PageSceneBrowser::OnStnClickedCancelSearch()
{
	// TODO: It is executed when user press cancel icon
	searchEdit_.SetWindowText("");
}

void PageSceneBrowser::OnStnClickedSearchIcon()
{
	// TODO: It is executed when user press search incon
	MessageBox("OnStnClickedSearchIcon");
}

void PageSceneBrowser::OnEnUpdateSearch()
{
	// TODO:  Si éste es un control RICHEDIT, el control no
	// enviará esta notificación a menos que se invalide la función __super::OnInitDialog()
	// función para enviar el mensaje EM_SETEVENTMASK al control
	// con el marcador ENM_UPDATE ORed en la máscara lParam.

	// TODO:  It is executed when user change value on search edit control
	CString text;
	searchEdit_.GetWindowText(text);
	cancelIcon_.ShowWindow( !text.IsEmpty() );
	list_.setSearchString(text);
}
