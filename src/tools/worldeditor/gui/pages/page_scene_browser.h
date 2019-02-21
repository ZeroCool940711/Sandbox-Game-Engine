#pragma once

#include "guitabs/guitabs_content.hpp"
#include "chunk/base_chunk_space.hpp"
#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"
#include "afxcmn.h"
#include "afxwin.h"

class BaseItem
{
public:
	BaseItem();
	virtual ~BaseItem(){}

	virtual void create(CWnd* parent) = 0;
	inline bool isGroup(){ return isGroup_; }

protected:
	CWnd* parent_;
	bool isGroup_;

};

class Item : public BaseItem
{
public:
	Item(ChunkItem*);
	~Item();

	virtual void create(CWnd* parent);

	inline ChunkItem* getChunkItem(){ return chunkItem_; }

private:
	ChunkItem* chunkItem_;
};

class GroupItem : public BaseItem
{
public:
	GroupItem(CString title);
	~GroupItem();

	virtual void create(CWnd* parent);

	inline void setTitle(CString title){ this->title_ = title; }
	inline CString getTitle(){ return this->title_; }
	inline std::vector<Item*> getItems() { return this->items_; }
	inline void addItem(Item* item){ this->items_.push_back(item); }
	inline void setExpanded( bool option ) { expanded_ = option; }
	inline bool getExpanded() { return expanded_; }
	inline int getGroupDepth() { return items_.size(); }

private:
	CString title_;
	std::vector<Item*> items_;
	bool expanded_;
};


//class to handle grouping types (Chunk, Asset Type, Asset Name, etc.)
class GroupManager
{
public:
	GroupManager();
	~GroupManager();

	void groupBy(CString groupBy);
	 
	inline void setChunkItems(ChunkMap chunks){ this->chunks_ = chunks; }

	inline std::vector<GroupItem*> getGroup(){ return groups_; }

private:
	ChunkMap chunks_;
	std::vector<GroupItem*> groups_;

};

class ListSceneBrowser : public CListBox
{
	DECLARE_DYNAMIC(ListSceneBrowser)

public:
	ListSceneBrowser();
	virtual ~ListSceneBrowser();

	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);	

	void collapseGroup(GroupItem* gItem, int index);
	void expandGroup(GroupItem* gItem, int index);

	inline void setDividerPos( int x ) { dividerPos_ = x; };

	void insertElements(const ChunkMap chunkMap);

private:

	void DrawDivider(int xpos);
	void Draw();

	int dividerPos_;
	int dividerTop_;
	int dividerBottom_;
	int dividerLastX_;
	bool dividerMove_;
	HCURSOR cursorArrow_;
	HCURSOR cursorSize_;
	GroupManager* groupManager_;

protected:
	virtual void PreSubclassWindow();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()

public:
	
	void setSearchString( const CString searchString );
	void setGroupByString( const CString groupBy );

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNMClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnChangedChunk(	WPARAM wparam, LPARAM lparam );
	afx_msg void OnNMClickTreeSceneBrowser(NMHDR *pNMHDR, LRESULT *pResult);
};


// Vista de formulario de SceneBrowser

class PageSceneBrowser : public CFormView, public GUITABS::Content
{

	IMPLEMENT_BASIC_CONTENT
    ( 
        L("WORLDEDITOR/GUI/PAGE_SCENE_BROWSER/SHORT_NAME"),					// short name
        L("WORLDEDITOR/GUI/PAGE_SCENE_BROWSER/LONG_NAME"),					// long name
        290, 380,                           // width, height
        NULL                                // icon
    )

public:
	enum { IDD = IDD_SceneBrowser };

	PageSceneBrowser();           // Constructor protegido utilizado por la creación dinámica
	virtual ~PageSceneBrowser();
private:
	bool pageReady_;
	//ChunkMap chunks_;
	//Vector3 lastPos_;
	void InitPage();
	void UpdateChunkView();
	
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Compatibilidad con DDX/DDV

	DECLARE_MESSAGE_MAP()
public:
	
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnActivateTool(WPARAM wParam, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void OnCbnSelchangeComboGroupby();
	afx_msg void OnStnClickedCancelSearch();
	afx_msg void OnStnClickedSearchIcon();
	afx_msg void OnEnUpdateSearch();

private:
	//CListCtrl list_;
	ListSceneBrowser list_;
	CComboBox groupBy_;
	CEdit searchEdit_;
	CStatic cancelIcon_;
	Vector3 lastPos_;
};

IMPLEMENT_BASIC_CONTENT_FACTORY(PageSceneBrowser)

