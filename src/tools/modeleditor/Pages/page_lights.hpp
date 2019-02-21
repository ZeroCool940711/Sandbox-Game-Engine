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

#include "guitabs/guitabs_content.hpp"

#include "controls/auto_tooltip.hpp"

#include "common/property_table.hpp"

#include "resmgr/string_provider.hpp"

#include "lights.hpp"

class UalItemInfo;

class LightList: public CTreeCtrl
{
protected:
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnClick( NMHDR *pNMHDR, LRESULT *pResult );
	BOOL PreTranslateMessage( MSG* pMsg );
};

// PageLights

class PageLights: public PropertyTable, public GUITABS::Content
{
	IMPLEMENT_BASIC_CONTENT( 
		L("MODELEDITOR/PAGES/PAGE_LIGHTS/SHORT_NAME"), 
		L("MODELEDITOR/PAGES/PAGE_LIGHTS/LONG_NAME"), 
		285, 638, NULL )

	DECLARE_AUTO_TOOLTIP( PageLights, PropertyTable )

public:
	PageLights();
	~PageLights();

	void fini();

	static PageLights* currPage();
	
	//These are exposed to python as:
	void lightsNew();	// newLights()
	void lightsOpen();	// openLights()
	void lightsSave();	// saveLights()

	bool openLightFile( const std::string& lightFile );

// Dialog Data
	enum { IDD = IDD_LIGHTS };

protected:
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	
public:
	
	virtual BOOL OnInitDialog();

private:
	
	SmartPointer< struct PageLightsImpl > pImpl_;
	size_t filter_;
	
	void OnGUIManagerCommand(UINT nID);
	void OnGUIManagerCommandUpdate(CCmdUI * cmdUI);
	afx_msg LRESULT OnShowTooltip(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHideTooltip(WPARAM wParam, LPARAM lParam);

	void OnSize( UINT nType, int cx, int cy );
	
	afx_msg LRESULT OnChangePropertyItem(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDblClkPropertyItem(WPARAM wParam, LPARAM lParam);
		
	void updateCheck( CButton& button, const std::string& actionName, int test = true );
		
	void redrawLightList( bool sel );	

	void updateChecks();

	bool applyLights( UalItemInfo* ii );

	void enableCustomLighting( bool enable );

public:
	afx_msg LRESULT OnUpdateControls(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTvnSelchangedLightList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeLightsSetups();
	afx_msg void OnBnClickedLightsUseCustom();
	afx_msg void OnBnClickedLightsCamera();
	afx_msg void OnBnClickedLightsModels();
	afx_msg void OnBnClickedLightsUseGame();
};

IMPLEMENT_BASIC_CONTENT_FACTORY( PageLights )
