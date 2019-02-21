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
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/pages/page_chunk_texture.hpp"
#include "worldeditor/gui/pages/page_message_impl.hpp"
#include "worldeditor/gui/pages/page_objects.hpp"
#include "worldeditor/gui/pages/page_options_environment.hpp"
#include "worldeditor/gui/pages/page_options_general.hpp"
#include "worldeditor/gui/pages/page_options_histogram.hpp"
#include "worldeditor/gui/pages/page_options_navigation.hpp"
#include "worldeditor/gui/pages/page_options_weather.hpp"
#include "worldeditor/gui/pages/page_project.hpp"
#include "worldeditor/gui/pages/page_properties.hpp"
#include "worldeditor/gui/pages/page_terrain_filter.hpp"
#include "worldeditor/gui/pages/page_terrain_height.hpp"
#include "worldeditor/gui/pages/page_terrain_import.hpp"
#include "worldeditor/gui/pages/page_terrain_mesh.hpp"
#include "worldeditor/gui/pages/page_terrain_texture.hpp"
#include "worldeditor/gui/pages/page_chunk_watcher.hpp"
#include "worldeditor/gui/dialogs/placement_ctrls_dlg.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/framework/mainframe.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "appmgr/app.hpp"
#include "appmgr/options.hpp"
#include "ual/ual_dialog.hpp"
#include "ual/ual_history.hpp"
#include "ual/ual_manager.hpp"
#include "guitabs/guitabs.hpp"
#include "common/page_messages.hpp"
#include "common/cooperative_moo.hpp"
#include "common/user_messages.hpp"


BW_SINGLETON_STORAGE( PanelManager )


PanelManager::PanelManager() :
	mainFrame_( 0 ),
	ready_( false ),
	GUI::ActionMaker<PanelManager>( "doDefaultPanelLayout", &PanelManager::loadDefaultPanels ),
	GUI::ActionMaker<PanelManager, 1>( "doShowSidePanel", &PanelManager::showSidePanel ),
	GUI::ActionMaker<PanelManager, 2>( "doHideSidePanel", &PanelManager::hideSidePanel ),
	GUI::ActionMaker<PanelManager, 3>( "doLoadPanelLayout", &PanelManager::loadLastPanels ),
	GUI::UpdaterMaker<PanelManager>( "updateSidePanel", &PanelManager::updateSidePanel ),
	GUI::UpdaterMaker<PanelManager, 1>( "disableEnablePanels", &PanelManager::disableEnablePanels )
{
	// tools container
	panelNames_.insert( StrPair( "Tools", GUITABS::ContentContainer::contentID ) );

	// actual tools panels
	panelNames_.insert( StrPair( "Objects", PageObjects::contentID ) );
	panelNames_.insert( StrPair( "TerrainTexture", PageTerrainTexture::contentID ) );
	panelNames_.insert( StrPair( "TerrainHeight", PageTerrainHeight::contentID ) );
	panelNames_.insert( StrPair( "TerrainFilter", PageTerrainFilter::contentID ) );
	panelNames_.insert( StrPair( "TerrainMesh", PageTerrainMesh::contentID ) );
    panelNames_.insert( StrPair( "TerrainImpExp", PageTerrainImport::contentID ) );
	panelNames_.insert( StrPair( "Project", PageProject::contentID ) ); 	

	// Other panels
	panelNames_.insert( StrPair( "UAL", UalDialog::contentID ) );
	panelNames_.insert( StrPair( "Properties", PageProperties::contentID ) );
	panelNames_.insert( StrPair( "Options", PageOptionsGeneral::contentID ) );
	panelNames_.insert( StrPair( "Navigation", PageOptionsNavigation::contentID ) );
	panelNames_.insert( StrPair( "Weather", PageOptionsWeather::contentID ) );
    panelNames_.insert( StrPair( "Environment", PageOptionsEnvironment::contentID ) );
	panelNames_.insert( StrPair( "Histogram", PageOptionsHistogram::contentID ) );
	panelNames_.insert( StrPair( "Messages", PageMessages::contentID ) );
	panelNames_.insert( StrPair( "ChunkTextures", PageChunkTexture::contentID ) );
	if (Options::getOptionBool("panels/chunkWatcher", false))
		panelNames_.insert( StrPair( "ChunkWatcher", PageChunkWatcher::contentID ) );

	ignoredObjectTypes_.insert( "bmp" );
	ignoredObjectTypes_.insert( "tga" );
	ignoredObjectTypes_.insert( "png" );
	ignoredObjectTypes_.insert( "jpg" );
	ignoredObjectTypes_.insert( "dds" );
}


/*static*/ bool PanelManager::init( CFrameWnd* mainFrame, CWnd* mainView )
{
	PlacementPresets::instance()->init( "resources/data/placement.xml" );

	PanelManager* manager = new PanelManager();
	instance().mainFrame_ = mainFrame;
	instance().panels().insertDock( mainFrame, mainView );

	if (!instance().initPanels())
	{
		return false;
	}

	PlacementPresets::instance()->readPresets(); // to fill all placement combo boxes

	return true;
}


/*static*/ void PanelManager::fini()
{
	if (pInstance())
	{
		instance().panels().broadcastMessage( WM_CLOSING_PANELS, 0, 0 );

		instance().ready_ = false;

		delete pInstance();

		PlacementPresets::fini();
	}
}


void PanelManager::finishLoad()
{
	// show the default panels
	this->panels().showPanel( UalDialog::contentID, true );
	this->panels().showPanel( PageObjects::contentID, true );

	PageMessages* msgs = (PageMessages*)(this->panels().getContent(PageMessages::contentID ));
	if (msgs)
	{
		msgs->mainFrame( mainFrame_ );
		msgs->pythonAdapter( WorldEditorApp::instance().pythonAdapter() );
		//This will get deleted in the PageMessages destructor
		msgs->msgsImpl( new BBMsgImpl( msgs ) );
	}

	ready_ = true;

	// set the default tool
	setDefaultToolMode();
}

bool PanelManager::initPanels()
{
	if ( ready_ )
		return false;

	CWaitCursor wait;

	// UAL Setup
	for ( int i = 0; i < BWResource::getPathNum(); i++ )
	{
		std::string path = BWResource::getPath( i );
		if ( path.find("worldeditor") != -1 )
			continue;
		UalManager::instance().addPath( path );
	}
	UalManager::instance().setConfigFile(
		Options::getOptionString(
			"ualConfigPath",
			"resources/ual/ual_config.xml" ) );

	UalManager::instance().setItemClickCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualItemClick ) );
	UalManager::instance().setItemDblClickCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualDblItemClick ) );
	UalManager::instance().setPopupMenuCallbacks(
		new UalFunctor2<PanelManager, UalItemInfo*, UalPopupMenuItems&>( pInstance(), &PanelManager::ualStartPopupMenu ),
		new UalFunctor2<PanelManager, UalItemInfo*, int>( pInstance(), &PanelManager::ualEndPopupMenu )
		);

	UalManager::instance().setStartDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualStartDrag ) );
	UalManager::instance().setUpdateDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualUpdateDrag ) );
	UalManager::instance().setEndDragCallback(
		new UalFunctor1<PanelManager, UalItemInfo*>( pInstance(), &PanelManager::ualEndDrag ) );

	UalManager::instance().setErrorCallback(
		new UalFunctor1<PanelManager, const std::string&>( pInstance(), &PanelManager::addSimpleError ) );

	this->panels().registerFactory( new UalDialogFactory() );


	// other panels setup
	this->panels().registerFactory( new PageObjectsFactory() );
	this->panels().registerFactory( new PageTerrainTextureFactory() );
	this->panels().registerFactory( new PageTerrainHeightFactory() );
	this->panels().registerFactory( new PageTerrainFilterFactory() );
	this->panels().registerFactory( new PageTerrainMeshFactory() );
    this->panels().registerFactory( new PageTerrainImportFactory() );
	this->panels().registerFactory( new PageProjectFactory() );

	this->panels().registerFactory( new PagePropertiesFactory() );
	this->panels().registerFactory( new PageChunkTextureFactory() );
	this->panels().registerFactory( new PageOptionsGeneralFactory() );
	this->panels().registerFactory( new PageOptionsNavigationFactory() );
	this->panels().registerFactory( new PageOptionsWeatherFactory() );
    this->panels().registerFactory( new PageOptionsEnvironmentFactory() );
	this->panels().registerFactory( new PageOptionsHistogramFactory() );
	this->panels().registerFactory( new PageMessagesFactory() );
	if (Options::getOptionBool("panels/chunkWatcher", false))
		this->panels().registerFactory( new PageChunkWatcherFactory() );
	
	this->panels().registerFactory( new PlacementCtrlsDlgFactory() );

	if ( ((MainFrame*)mainFrame_)->verifyBarState( "TBState" ) )
		mainFrame_->LoadBarState( "TBState" );

	if ( !this->panels().load() || !allPanelsLoaded() )
		loadDefaultPanels( 0 );

	finishLoad();

	return true;
}


bool PanelManager::allPanelsLoaded()
{
	bool res = true;

	for ( std::map<std::string, std::string>::iterator i = panelNames_.begin();
		res && i != panelNames_.end(); ++i )
	{
		res = res && !!this->panels().getContent(
			(*i).second );
	}

	return res;
}

bool PanelManager::loadDefaultPanels( GUI::ItemPtr item )
{
	CWaitCursor wait;
	bool isFirstCall = ( item == 0 );
	if ( ready_ )
	{
		if ( MessageBox( mainFrame_->GetSafeHwnd(),
			L("WORLDEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT_LAYOUT_TEXT"),
			L("WORLDEDITOR/GUI/PANEL_MANAGER/LOAD_DEFAULT_LAYOUT_TITLE"),
			MB_YESNO|MB_ICONQUESTION ) != IDYES )
			return false;

		ready_ = false;
	}

	if ( item != 0 )
	{
		// not first panel load, so rearrange the toolbars
		((MainFrame*)WorldEditorApp::instance().mainWnd())->defaultToolbarLayout();
	}

	this->panels().broadcastMessage(WM_DEFAULT_PANELS, 0, 0);

	// clean it up first, just in case
	this->panels().removePanels();

	// Remember to add ALL INSERTED PANELS to the allPanelsLoaded() method above
	GUITABS::PanelHandle p = 0;
	p = this->panels().insertPanel( UalDialog::contentID, GUITABS::RIGHT, p );
	p = this->panels().insertPanel( PageProperties::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageChunkTexture::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageOptionsGeneral::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageOptionsNavigation::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageOptionsWeather::contentID, GUITABS::TAB, p );
    p = this->panels().insertPanel( PageOptionsEnvironment::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageOptionsHistogram::contentID, GUITABS::TAB, p );
	p = this->panels().insertPanel( PageMessages::contentID, GUITABS::TAB, p );
	if (Options::getOptionBool("panels/chunkWatcher", false))
		p = this->panels().insertPanel( PageChunkWatcher::contentID, GUITABS::TAB, p );

	p = this->panels().insertPanel(
		GUITABS::ContentContainer::contentID, GUITABS::TOP, p );
	this->panels().insertPanel( PageObjects::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageTerrainTexture::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageTerrainHeight::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageTerrainFilter::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageTerrainMesh::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageTerrainImport::contentID, GUITABS::SUBCONTENT, p );
	this->panels().insertPanel( PageProject::contentID, GUITABS::SUBCONTENT, p );

	if ( !isFirstCall )
		finishLoad();

	return true;
}

bool PanelManager::loadLastPanels( GUI::ItemPtr item )
{
	CWaitCursor wait;
	if ( MessageBox( mainFrame_->GetSafeHwnd(),
		L("WORLDEDITOR/GUI/PANEL_MANAGER/LOAD_LAST_LAYOUT_TEXT"),
		L("WORLDEDITOR/GUI/PANEL_MANAGER/LOAD_LAST_LAYOUT_TITLE"),
		MB_YESNO|MB_ICONQUESTION ) != IDYES )
		return false;

	ready_ = false;

	this->panels().broadcastMessage(WM_LAST_PANELS, 0, 0);

	if ( ((MainFrame*)mainFrame_)->verifyBarState( "TBState" ) )
		mainFrame_->LoadBarState( "TBState" );

	if ( !this->panels().load() || !allPanelsLoaded() )
		loadDefaultPanels( 0 );

	finishLoad();

	return true;
}

bool PanelManager::ready()
{
	return ready_;
}

const std::string PanelManager::getContentID( const std::string pyID )
{ 
	std::map<std::string, std::string>::iterator idIt = panelNames_.find( pyID );
	if ( idIt != panelNames_.end() )
		return (*idIt).second;

	return pyID; // it they passed in the GUITABS contentID of the panel, then use it
}

const std::string PanelManager::getPythonID( const std::string contentID )
{
	for ( std::map<std::string, std::string>::iterator i = panelNames_.begin();
		i != panelNames_.end(); ++i )
	{
		if ( (*i).second == contentID )
			return (*i).first;
	}
	return contentID; // it they passed in the GUITABS python ID of the panel, then use it
}

void PanelManager::setToolMode( const std::string id )
{
	if ( !ready_ )
		return;

	std::string pyID = getPythonID( id );

	if ( currentTool_ == pyID )
		return;

	std::string contentID = getContentID( id );

	if ( contentID.empty() )
		return;

	updateUIToolMode( pyID );

	this->panels().showPanel( contentID, true );
	this->panels().broadcastMessage
	( 
		WM_ACTIVATE_TOOL, 
		(WPARAM)contentID.c_str(), 
		0 
	);

	// hide tool-dependent popups:
	this->panels().showPanel( PlacementCtrlsDlg::contentID, false );
}

void PanelManager::setDefaultToolMode()
{
	setToolMode( PageObjects::contentID );
} 

const std::string PanelManager::currentTool()
{
	return currentTool_;
}

bool PanelManager::isCurrentTool( const std::string& id )
{
	return currentTool() == getPythonID( id );
}

void PanelManager::updateUIToolMode( const std::string& pyID )
{
	std::string pythonID = getPythonID( pyID );
	if ( pythonID.empty() )
		currentTool_ = pyID;
	else
		currentTool_ = pythonID; // the pyID was actually a contentID, so setting the obtained pythonID
	GUI::Manager::instance().update();
}

void PanelManager::showPanel( const std::string pyID, int show )
{
	std::string contentID = getContentID( pyID );

	if ( !contentID.empty() )
		this->panels().showPanel( contentID, show != 0 );
}

int PanelManager::isPanelVisible( const std::string pyID )
{
	std::string contentID = getContentID( pyID );

	if ( !contentID.empty() )
		return this->panels().isContentVisible( contentID );
	else
		return 0;
}

void PanelManager::ualItemClick( UalItemInfo* ii )
{
	if (!WorldEditorApp::instance().pythonAdapter() || !ii)
	{
		return;
	}

	UalManager::instance().history().prepareItem( ii->assetInfo() );
	WorldEditorApp::instance().pythonAdapter()->onBrowserObjectItemSelect( 
		"Ual" , ii->longText().c_str(), false );
}

void PanelManager::ualDblItemClick( UalItemInfo* ii )
{
	if (!WorldEditorApp::instance().pythonAdapter() || !ii)
	{
		return;
	}

	UalManager::instance().history().prepareItem( ii->assetInfo() );
	WorldEditorApp::instance().pythonAdapter()->onBrowserObjectItemSelect( 
		"Ual" , ii->longText().c_str(), true );
}

void PanelManager::ualStartPopupMenu( UalItemInfo* ii, UalPopupMenuItems& menuItems )
{
	if (!WorldEditorApp::instance().pythonAdapter() || !ii)
	{
		return;
	}

	HMENU menu = ::CreatePopupMenu();

	std::map<int,std::string> pyMenuItems;
	WorldEditorApp::instance().pythonAdapter()->contextMenuGetItems(
		ii->type(),
		ii->longText().c_str(),
		pyMenuItems );

	if (!pyMenuItems.size())
	{
		return;
	}

	for (std::map<int,std::string>::iterator i = pyMenuItems.begin();
		i != pyMenuItems.end(); ++i)
	{
		menuItems.push_back( UalPopupMenuItem( (*i).second, (*i).first ) );
	}
}

void PanelManager::ualEndPopupMenu( UalItemInfo* ii, int result )
{
	if (!WorldEditorApp::instance().pythonAdapter() || !ii)
	{
		return;
	}

	WorldEditorApp::instance().pythonAdapter()->contextMenuHandleResult(
		ii->type(),
		ii->longText().c_str(),
		result );
}


void PanelManager::ualStartDrag( UalItemInfo* ii )
{
	if (!WorldEditorApp::instance().pythonAdapter() || !ii)
	{
		return;
	}
	
	// Start drag
	UalManager::instance().dropManager().start( 
		BWResource::getExtension( ii->longText() ));
}

void PanelManager::ualUpdateDrag( UalItemInfo* ii )
{
	if ( !ii )
		return;

	SmartPointer< UalDropCallback > dropable = UalManager::instance().dropManager().test( ii );
	bool onGraphicWindow = ::WindowFromPoint( CPoint( ii->x(), ii->y() ) ) == WorldManager::instance().hwndGraphics(); 
	std::string filename = ii->text();
	std::string ext = filename.rfind( '.' ) == filename.npos ?	""	:
						filename.substr( filename.rfind( '.' ) + 1 );
	bool isIgnored = ignoredObjectTypes_.find( ext ) != ignoredObjectTypes_.end()
		|| currentTool_ == "TerrainImpExp" || currentTool_ == "Project";
	if ( ii->isFolder() || dropable || ( onGraphicWindow && !isIgnored ) )
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
	else
		SetCursor( AfxGetApp()->LoadStandardCursor( IDC_NO ) );

	if ( !ii->isFolder() && CooperativeMoo::canUseMoo( true ) )
		WorldEditorApp::instance().mfApp()->updateFrame( false );
}

void PanelManager::ualEndDrag( UalItemInfo* ii )
{
	SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );

	if ( !ii )
		return;

	if ( ii->isFolder() )
	{
		// folder drag successfull
		CPoint pt;
		GetCursorPos( &pt );
		mainFrame_->ScreenToClient( &pt );
		this->panels().clone( (GUITABS::Content*)( ii->dialog() ),
			pt.x - 5, pt.y - 5 );
	}
	else
	{
		// item drag successfull
		if ( ::WindowFromPoint( CPoint( ii->x(), ii->y() ) ) == WorldManager::instance().hwndGraphics() &&
			currentTool_ != "TerrainImpExp" && currentTool_ != "Project" )
		{
			setToolMode( "Objects" );
			WorldManager::instance().update( 0 );
			UalManager::instance().history().prepareItem( ii->assetInfo() );

			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_WAIT ) );
			if (WorldEditorApp::instance().pythonAdapter())
			{
				WorldEditorApp::instance().pythonAdapter()->onBrowserObjectItemAdd();
			}

			SetCursor( AfxGetApp()->LoadStandardCursor( IDC_ARROW ) );
		}
		else
		{
			UalManager::instance().dropManager().end(ii);
		}
	}
}

void PanelManager::ualAddItemToHistory( std::string str, std::string type )
{
	// called from python
	UalManager::instance().history().addPreparedItem();
}


void PanelManager::addSimpleError( const std::string& msg )
{
	WorldManager::instance().addError( 0, 0, msg.c_str() );
}

bool PanelManager::showSidePanel( GUI::ItemPtr item )
{
	bool isDockVisible = this->panels().isDockVisible();

	if ( !isDockVisible )
	{
		this->panels().showDock( !isDockVisible );
		this->panels().showFloaters( !isDockVisible );
//		GetMenu()->GetSubMenu( 2 )->CheckMenuItem( ID_MENU_SHOWSIDEPANEL, MF_BYCOMMAND | MF_CHECKED );
	}
	return true;
}

bool PanelManager::hideSidePanel( GUI::ItemPtr item )
{
	bool isDockVisible = this->panels().isDockVisible();

	if ( isDockVisible )
	{
		this->panels().showDock( !isDockVisible );
		this->panels().showFloaters( !isDockVisible );
//		GetMenu()->GetSubMenu( 2 )->CheckMenuItem( ID_MENU_SHOWSIDEPANEL, MF_BYCOMMAND | MF_UNCHECKED );
	}
	return true;
}

unsigned int PanelManager::updateSidePanel( GUI::ItemPtr item )
{
	if ( this->panels().isDockVisible() )
		return 0;
	else
		return 1;
}

unsigned int PanelManager::disableEnablePanels( GUI::ItemPtr item )
{
	if ( this->panels().isDockVisible() )
		return 1;
	else
		return 0;
}

void PanelManager::updateControls()
{
	this->panels().broadcastMessage( WM_UPDATE_CONTROLS, 0, 0 );
}

void PanelManager::onClose()
{
	CWaitCursor wait;
	if ( Options::getOptionBool( "panels/saveLayoutOnExit", true ) )
	{
		this->panels().save();
		mainFrame_->SaveBarState( "TBState" );
	}
	this->panels().showDock( false );
	UalManager::instance().fini();
}

void PanelManager::onNewSpace(unsigned int width, unsigned int height)
{
    this->panels().broadcastMessage( WM_NEW_SPACE, width, height );
}


void PanelManager::onChangedChunkState(int x, int z)
{
	this->panels().broadcastMessage( WM_CHANGED_CHUNK_STATE, (LPARAM)x, (WPARAM)z );
}


void PanelManager::onNewWorkingChunk()
{
    this->panels().broadcastMessage(WM_WORKING_CHUNK, 0, 0);
}

void PanelManager::onBeginSave()
{
    this->panels().broadcastMessage( WM_BEGIN_SAVE, 0, 0 );
}

void PanelManager::onEndSave()
{
    this->panels().broadcastMessage( WM_END_SAVE, 0, 0 );
}

void PanelManager::showItemInUal( const std::string& vfolder, const std::string& longText )
{
	UalManager::instance().showItem( vfolder, longText );
	showPanel( "UAL", true ); // make sure it's visible
}
