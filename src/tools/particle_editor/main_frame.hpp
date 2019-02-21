/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef MAINFRAME_HPP
#define MAINFRAME_HPP

#include "fwd.hpp"
#include "undoredo_op.hpp"
#include "resmgr/xml_section.hpp"
#include "particle/particle_system.hpp"
#include "particle/meta_particle_system.hpp"
#include "common/base_mainframe.hpp"

class MainFrame : public BaseMainFrame
{	
protected: 
	MainFrame();

	DECLARE_DYNCREATE(MainFrame)

public:
    /*virtual*/ ~MainFrame();

	static MainFrame *instance() { return s_instance; }

	bool resizing() { return resizing_; }

	bool SelectParticleSystem(std::string const &name);

	bool IsMetaParticleSystem();

	MetaParticleSystemPtr GetMetaParticleSystem();

	bool IsCurrentParticleSystem();

	ParticleSystemPtr GetCurrentParticleSystem();

	void ChangeToActionPropertyWindow(int index, ParticleSystemActionPtr action);

	void SetPerformancePaneText(CString text);

	void CopyFromDataSection( int kind, DataSectionPtr pDS );

	void RefreshGUI(int actionKind);

	void 
    PotentiallyDirty
    (
        bool                    option,
        UndoRedoOp::ActionKind  actionKind          = UndoRedoOp::AK_PARAMETER,
        std::string             const &changeDesc   = "?",
        bool                    waitForLButtonUp    = false
    );

	void SaveUndoState( int actionKind, const std::string& changeDesc );

	void OnBatchedUndoOperationEnd();

	POINT CurrentCursorPosition() const;

	Vector3 GetWorldRay(int x, int y) const;

    BOOL CursorOverGraphicsWnd() const;

	CString ParticlesDirectory() { return particleDirectory_; }

	void ParticlesDirectory(CString directory);

	void InitialiseMetaSystemRegister();

	Moo::Colour BgColour() { return bgColour_; }

	void BgColour(Moo::Colour c);

	void UpdateBackgroundColor();

	void ForceActionPropertiesUpdate();

	void ForceActionPropertiesUpdateSkip() { skipForceActionPorpertiesUpdate_ = true; }

	bool PotentiallyDirty() { return potentiallyDirty_; }

    void UpdateGUI();

    ActionSelection *GetActionSelection() const;

    PsaProperties   *GetPsaProperties() const;

    void DereferenceColorDialogThread();

    void ForceSave();

    int PromptSave(UINT type, bool clearUndoStack = false);

    void SetDocumentTitle(std::string const &title);
    
    void UpdateTitle();

    /*virtual*/ CDocument *GetActiveDocument();

	void OnButtonViewFree();

	void OnButtonViewX();

	void OnButtonViewY();

	void OnButtonViewZ();  

	void OnButtonViewOrbit();

	void OnUndo();

    bool CanUndo() const;

	void OnRedo();

    bool CanRedo() const;

	void OnBackgroundColor();	

	bool showingBackgroundColor() { return colorDialogThread_ != NULL; }

    CStatusBar &getStatusBar() { return wndStatusBar_; }

    void appendOneShotPS();

    void clearAppendedPS();

    size_t numberAppendPS() const;

    MetaParticleSystem &getAppendedPS(size_t idx);

    void cleanupAppendPS();

private:

	/*virtual*/ BOOL PreCreateWindow(CREATESTRUCT& cs);

	bool Initialised() const { return initialised_; }	

    /*virtual*/ BOOL PreTranslateMessage(MSG *msg);

	afx_msg void OnSize(UINT nType, int cx, int cy);

	afx_msg LRESULT OnEnterSizeMove (WPARAM, LPARAM);
	
	afx_msg LRESULT OnExitSizeMove (WPARAM, LPARAM);

	virtual BOOL 
    Create
    (
        LPCTSTR                 lpszClassName, 
        LPCTSTR                 lpszWindowName, 
        DWORD                   dwStyle         = WS_OVERLAPPEDWINDOW, 
        RECT                    const &rect     = rectDefault, 
        CWnd                    *pParentWnd     = NULL, 
        LPCTSTR                 lpszMenuName    = NULL, 
        DWORD                   dwExStyle       = 0, 
        CCreateContext          *pContext       = NULL
    );

	afx_msg void OnClose();

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

    afx_msg void OnGUIManagerCommand(UINT nID);

    afx_msg void OnGUIManagerCommandUpdate(CCmdUI *cmdUI);

	afx_msg void OnUpdatePerformancePane(CCmdUI * pCmdUI);

    afx_msg void OnPlay();

    afx_msg void OnStop();

    afx_msg void OnPause();

	DECLARE_MESSAGE_MAP()

private:
	static MainFrame            *s_instance;
	CCreateContext	            context_;
	bool                        potentiallyDirty_;
	std::string                 pendingLBtnUpDesc_;
	int                         pendingLbtnUpKind_;	
	bool                        undoing_;
	CString                     particleDirectory_;
	bool                        skipForceActionPorpertiesUpdate_;
	CStatusBar                  wndStatusBar_;
	CString                     performancePaneString_;
	bool                        initialised_;
	ColorPickerDialogThread     *colorDialogThread_;
	bool						deferredGuiUpdate_;
	Moo::Colour                 bgColour_;
    std::string                 title_;
    PsaProperties               *psaDlg_;
	bool                        resizing_;
};

#endif // MAINFRAME_HPP
