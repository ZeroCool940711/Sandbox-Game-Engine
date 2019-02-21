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
#include "panel_manager.hpp"
#include "main_frame.hpp"
#include "action_selection.hpp"
#include "particle_editor.hpp"
#include "gui/action_selection.hpp"
#include "guitabs/guitabs.hpp"
#include "appmgr/options.hpp"
#include "common/user_messages.hpp"
#include "resmgr/string_provider.hpp"
#include "ual/ual_manager.hpp"
#include "ual/ual_dialog.hpp"
#include "ual/ual_callback.hpp"
#include "controls/message_box.hpp"
#include "cstdmf/restart.hpp"

using namespace std;


BW_SINGLETON_STORAGE( PanelManager )


PanelManager::~PanelManager()
{
}


/*static*/ bool PanelManager::init(CFrameWnd* mainFrame, CWnd* mainView)
{
	PanelManager* manager = new PanelManager();

    instance().m_mainFrame = mainFrame;
    instance().panels().insertDock(mainFrame, mainView);

	if ( !instance().initPanels() )
		return false;

    return true;
}


/*static*/ void PanelManager::fini()
{
	instance().panels().broadcastMessage(WM_CLOSING_PANELS, 0, 0);

	instance().m_ready = false;

	delete pInstance();
}


bool PanelManager::initPanels()
{
    if (m_ready)
        return false;

    CWaitCursor wait;

    // UAL Setup
    for (int i = 0; i < BWResource::getPathNum(); i++)
    {
        string path = BWResource::getPath(i);
        if ( path.find("particleeditor") != -1 )
            continue;
        UalManager::instance().addPath(path);
    }
    UalManager::instance().setConfigFile
    (
        Options::getOptionString
        (
            "ualConfigPath",
            "resources/ual/ual_config.xml" 
        ) 
    );

    UalManager::instance().setItemDblClickCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualItemDblClick)
    );
    UalManager::instance().setStartDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualStartDrag)
    );
    UalManager::instance().setUpdateDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualUpdateDrag)
    );
    UalManager::instance().setEndDragCallback
    (
        new UalFunctor1<PanelManager, UalItemInfo*>(pInstance(), &PanelManager::ualEndDrag)
    );
    
    this->panels().registerFactory(new UalDialogFactory());

    // Setup the map which is used for python
    m_contentID["Particle Editor1"] = ActionSelection::contentID;

    // other panels setup
    this->panels().registerFactory(new ActionSelectionFactory);

    if ( ( (MainFrame*)m_mainFrame )->verifyBarState( "TBState" ) )
        m_mainFrame->LoadBarState( "TBState" );

    if (!this->panels().load())
    {
        loadDefaultPanels(NULL);
    }

    // Show the particle editor panel by default:
    this->panels().showPanel(ActionSelection::contentID, true);

    // BAn - this is something of a hack - ActionSelection1::OnInitialUpdate is
    // not called.  I do this manually:
    ActionSelection *actionSelection =
            (ActionSelection*)this->panels().getContent
            (
                ActionSelection::contentID
            );

    finishLoad();

    return true;
}

bool PanelManager::ready()
{
    return m_ready;
}

void PanelManager::showPanel(string const &pyID, int show)
{
    this->panels().showPanel(pyID, show != 0);
}

bool PanelManager::isPanelVisible(string const& pyID)
{
    return this->panels().isContentVisible(pyID);
}

bool PanelManager::showSidePanel(GUI::ItemPtr item)
{
    bool isDockVisible = this->panels().isDockVisible();

    if (!isDockVisible)
    {
        this->panels().showDock(!isDockVisible);
        this->panels().showFloaters(!isDockVisible);
    }
    return true;
}

bool PanelManager::hideSidePanel(GUI::ItemPtr item)
{
    bool isDockVisible = this->panels().isDockVisible();

    if (isDockVisible)
    {
        this->panels().showDock(!isDockVisible);
        this->panels().showFloaters(!isDockVisible);
    }
    return true;
}


bool PanelManager::setLanguage(GUI::ItemPtr item)
{
	std::string languageName = (*item)[ "LanguageName" ];
	std::string countryName = (*item)[ "CountryName" ];

	// Do nothing if we are not changing language
	if (currentLanguageName_ == languageName && currentCountryName_ == countryName)
	{
		return true;
	}

	unsigned int result;
	if (MainFrame::instance()->PotentiallyDirty())
	{
		result = MsgBox( L("RESMGR/CHANGING_LANGUAGE_TITLE"), L("RESMGR/CHANGING_LANGUAGE"),
			L("RESMGR/SAVE_AND_RESTART"), L("RESMGR/DISCARD_AND_RESTART"),
			L("RESMGR/RESTART_LATER"), L("RESMGR/CANCEL") ).doModal();
	}
	else
	{
		result = MsgBox( L("RESMGR/CHANGING_LANGUAGE_TITLE"), L("RESMGR/CHANGING_LANGUAGE"),
			L("RESMGR/RESTART_NOW"), L("RESMGR/RESTART_LATER"), L("RESMGR/CANCEL") ).doModal() + 1;
	}
	switch (result)
	{
	case 0:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		MainFrame::instance()->ForceSave();
		startNewInstance();
		AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
		break;
	case 1:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		MainFrame::instance()->PotentiallyDirty( false );
		startNewInstance();
		AfxGetApp()->GetMainWnd()->PostMessage( WM_COMMAND, ID_APP_EXIT );
		break;
	case 2:
		Options::setOptionString( "currentLanguage", languageName );
		Options::setOptionString( "currentCountry", countryName );
		currentLanguageName_ = languageName;
		currentCountryName_ = countryName;
		break;
	case 3:
		break;
	}
	return true;
}


unsigned int PanelManager::updateSidePanel(GUI::ItemPtr item)
{
	if (this->panels().isContentVisible(ActionSelection::contentID))
        return 0;
    else
        return 1;
}

unsigned int PanelManager::updateUalPanel(GUI::ItemPtr item)
{
	if (this->panels().isContentVisible(UalDialog::contentID))
        return 0;
    else
        return 1;
}


unsigned int PanelManager::updateLanguage(GUI::ItemPtr item)
{
	if (currentLanguageName_.empty())
	{
		currentLanguageName_ = StringProvider::instance().currentLanguage()->getIsoLangName();
		currentCountryName_ = StringProvider::instance().currentLanguage()->getIsoCountryName();
	}
	return currentLanguageName_ == (*item)[ "LanguageName" ] && currentCountryName_ == (*item)[ "CountryName" ];
}


void PanelManager::updateControls()
{
    this->panels().broadcastMessage(WM_UPDATE_CONTROLS, 0, 0);
    ActionSelection *actionSelection =
        (ActionSelection*)this->panels().getContent
        (
            ActionSelection::contentID
        );
    actionSelection->SendMessageToDescendants
    (
        WM_IDLEUPDATECMDUI,
        (WPARAM)TRUE, 
        0, 
        TRUE, 
        TRUE
    );
}

void PanelManager::onClose()
{
    if (Options::getOptionBool("panels/saveLayoutOnExit", true))
    {
        this->panels().save();
        m_mainFrame->SaveBarState( "TBState" );
    }
    this->panels().showDock( false );
	UalManager::instance().fini();
}

void PanelManager::ualAddItemToHistory(string filePath)
{
    // called from python
    string fname    = BWResource::getFilename(filePath);
    string longText = BWResource::resolveFilename(filePath);
    replace(longText.begin(), longText.end(), '/', '\\');
    UalManager::instance().history().add(AssetInfo("FILE", fname, longText));
}

bool PanelManager::loadDefaultPanels(GUI::ItemPtr item)
{
    CWaitCursor wait;
    bool isFirstCall = true;
    if (m_ready)
    {
        if 
        (
            MessageBox
            (
                m_mainFrame->GetSafeHwnd(),
                L("PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT_Q"),
                L("PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT"),
                MB_YESNO | MB_ICONQUESTION 
            ) != IDYES 
        )
        {
            return false;
        }

        m_ready = false;
        isFirstCall = false;
        // already has something in it, so clean up first
        this->panels().removePanels();

        // not first panel load, so rearrange the toolbars
        ((MainFrame*)m_mainFrame)->defaultToolbarLayout();
    }

    this->panels().insertPanel
    (
        ActionSelection::contentID, GUITABS::RIGHT
    );
    this->panels().insertPanel
    (
        UalDialog::contentID, GUITABS::BOTTOM
    );

    if (!isFirstCall)
        finishLoad();

    return true;
}

bool PanelManager::loadLastPanels(GUI::ItemPtr item)
{
    CWaitCursor wait;
    if 
    ( 
        MessageBox
        ( 
            m_mainFrame->GetSafeHwnd(),
            L("PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT_Q"),
            L("PARTICLEEDITOR/GUI/PANEL_MANAGER/LOAD_RECENT"),
            MB_YESNO | MB_ICONQUESTION 
        ) != IDYES 
    )
    {
        return false;
    }

    m_ready = false;

    if ( ( (MainFrame*)m_mainFrame )->verifyBarState( "TBState" ) )
        m_mainFrame->LoadBarState( "TBState" );

    if (!this->panels().load())
        loadDefaultPanels(NULL);

    finishLoad();

    return true;
}

PanelManager::PanelManager() :
    GUI::ActionMaker<PanelManager   >
    (
        "doShowSidePanel", 
        &PanelManager::showSidePanel
    ),
    GUI::ActionMaker<PanelManager, 1>
    (
        "doHideSidePanel", 
        &PanelManager::hideSidePanel
    ),
    GUI::ActionMaker<PanelManager, 2>
    (
        "setLanguage",
		&PanelManager::setLanguage
    ),
    GUI::UpdaterMaker<PanelManager>
    (
        "updateSidePanel", 
        &PanelManager::updateSidePanel
    ),
	GUI::UpdaterMaker<PanelManager, 1>
    (
        "updateUalPanel", 
        &PanelManager::updateUalPanel
    ),
	GUI::UpdaterMaker<PanelManager, 2>
    (
        "updateLanguage",
		&PanelManager::updateLanguage
    ),
    m_contentID(),
    m_currentTool(-1),
    m_mainFrame(NULL),
    m_mainView(NULL),
    m_ready(false),
    m_dropable(false)
{
}

void PanelManager::finishLoad()
{
    m_ready = true;
}

void PanelManager::ualItemDblClick(UalItemInfo* ii)
{
    string fullname = ii->longText();
    BWResource::dissolveFilename(fullname);
    string dir       = BWResource::getFilePath(fullname);
    string file      = BWResource::getFilename(fullname);
    string extension = BWResource::getExtension(fullname);
    if (strcmpi(extension.c_str(), "xml") == 0)
    {
        ParticleEditorApp *app = (ParticleEditorApp *)AfxGetApp();
        app->openDirectory(dir);
        file = BWResource::removeExtension(file);
        ActionSelection *as = ActionSelection::instance();
        if (as != NULL)
		{
            as->selectMetaParticleSystem( file );
			UalManager::instance().history().add( ii->assetInfo() );
		}
    }
}

void PanelManager::ualStartDrag(UalItemInfo* ii)
{
    if (ii == NULL)
        return;

    UalManager::instance().dropManager().start
    ( 
        BWResource::getExtension( ii->longText())
    );
}

void PanelManager::ualUpdateDrag(UalItemInfo* ii)
{
    if (ii == NULL)
        return;

    CWnd *activeView = MainFrame::instance()->GetActiveView();
    if (activeView == NULL)
        return;

    bool inView = 
        ::WindowFromPoint
        (
            CPoint(ii->x(), ii->y())
        ) 
        == 
        activeView->m_hWnd;

    if ((ii->isFolder()) || (UalManager::instance().dropManager().test(ii)))
        SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
    else
        SetCursor(AfxGetApp()->LoadStandardCursor(IDC_NO));
}

void PanelManager::ualEndDrag(UalItemInfo* ii)
{
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

    if (ii == NULL)
        return;

    if (ii->isFolder())
    {
        // folder drag
        CPoint pt;
        GetCursorPos(&pt);
        AfxGetMainWnd()->ScreenToClient(&pt);
        this->panels().clone
        (
            (GUITABS::Content*)(ii->dialog()),
            pt.x - 5, 
            pt.y - 5 
        );
    }
    else
    {
        UalManager::instance().dropManager().end(ii);
    }
}
