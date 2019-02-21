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
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/propdlgs/psa_collide_properties.hpp"
#include "controls/dir_dialog.hpp"
#include "controls/utils.hpp"
#include "particle/actions/collide_psa.hpp"
#include "resmgr/string_provider.hpp"
#include "fmodsound/soundmanager.hpp"

using namespace std;

namespace 
{
	bool validSoundTag( const std::string & filename )
    {
        return true;
    }
}

DECLARE_DEBUG_COMPONENT2("GUI", 0)

IMPLEMENT_DYNCREATE(PsaCollideProperties, PsaProperties)

BEGIN_MESSAGE_MAP(PsaCollideProperties, PsaProperties)
    ON_BN_CLICKED(IDC_PSA_COLLIDE_SOUND_ENABLED, OnBnClickedPsaCollideSoundEnabled)
	ON_CBN_SELCHANGE(IDC_PSA_COLLIDE_SOUND_PROJECT_LIST, &PsaCollideProperties::OnCbnSelchangePsaCollideSoundProjectList)
	ON_CBN_SELCHANGE(IDC_PSA_COLLIDE_SOUND_GROUP_LIST, &PsaCollideProperties::OnCbnSelchangePsaCollideSoundGroupList)
	ON_CBN_SELCHANGE(IDC_PSA_COLLIDE_SOUND_NAME_LIST, &PsaCollideProperties::OnCbnSelchangePsaCollideSoundNameList)
END_MESSAGE_MAP()

PsaCollideProperties::PsaCollideProperties()
:
PsaProperties(PsaCollideProperties::IDD)
{
}

PsaCollideProperties::~PsaCollideProperties()
{
}


void PsaCollideProperties::updateSoundLists()
{
	std::string soundBank = "";
	std::string soundGroup = "";
	std::string soundName = "";
	
	std::list< std::string >::iterator it;

	std::list< std::string > soundProjects;
	SoundManager::instance().getSoundProjects( soundProjects );
	soundProject_.ResetContent();
	for ( it = soundProjects.begin(); it != soundProjects.end(); ++it )
	{
		soundProject_.AddString( it->c_str() );
	}
	if (soundProject_.SelectString( -1, action()->soundProject().c_str() ) == CB_ERR)
	{
		action()->soundProject( "" );
		action()->soundGroup( "" );
		action()->soundName( "" );
	}
	
	std::list< std::string > soundGroups;
	SoundManager::instance().getSoundGroups( action()->soundProject(), soundGroups );
	soundGroup_.ResetContent();
	for ( it = soundGroups.begin(); it != soundGroups.end(); ++it )
	{
		soundGroup_.AddString( it->c_str() );
	}
	if (soundGroup_.SelectString( -1, action()->soundGroup().c_str() ) == CB_ERR)
	{
		action()->soundGroup( "" );
		action()->soundName( "" );
	}

	std::list< std::string > soundNames;
	SoundManager::instance().getSoundNames( action()->soundProject(), action()->soundGroup(), soundNames );
	soundName_.ResetContent();
	for ( it = soundNames.begin(); it != soundNames.end(); ++it )
	{
		soundName_.AddString( it->c_str() );
	}
	if (soundName_.SelectString( -1, action()->soundName().c_str() ) == CB_ERR)
	{
		action()->soundName( "" );
	}
}

void PsaCollideProperties::OnInitialUpdate()
{
    PsaProperties::OnInitialUpdate();
	updateSoundLists();
}

void PsaCollideProperties::DoDataExchange(CDataExchange* pDX)
{
    PsaProperties::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PSA_COLLIDE_ELASTICITY, elasticity_);
    DDX_Control(pDX, IDC_PSA_COLLIDE_SOUND_ENABLED,			soundEnabled_ );
    DDX_Control(pDX, IDC_PSA_COLLIDE_SOUND_PROJECT_LIST,	soundProject_ );
    DDX_Control(pDX, IDC_PSA_COLLIDE_SOUND_GROUP_LIST,		soundGroup_ );
    DDX_Control(pDX, IDC_PSA_COLLIDE_SOUND_NAME_LIST,		soundName_ );
}

void PsaCollideProperties::SetParameters(SetOperation task)
{
    ASSERT(action_);

    SET_FLOAT_PARAMETER( task, elasticity  );
    SET_CHECK_PARAMETER( task, soundEnabled );

    if (task == SET_CONTROL)
    {
		updateSoundLists();
        UpdateState();
    }
}

void PsaCollideProperties::UpdateState()
{
    bool enableWindow = (soundEnabled_.GetCheck() == BST_CHECKED);

    soundProject_.EnableWindow(enableWindow ? TRUE : FALSE);
    soundGroup_.EnableWindow(enableWindow ? TRUE : FALSE);
    soundName_.EnableWindow(enableWindow ? TRUE : FALSE);

	controls::enableWindow( *this, IDC_PSACOLLIDE_PROJECT_LABEL, enableWindow );
	controls::enableWindow( *this, IDC_PSACOLLIDE_GROUP_LABEL, enableWindow );
	controls::enableWindow( *this, IDC_PSACOLLIDE_SOUND_LABEL, enableWindow );
}

void PsaCollideProperties::OnBnClickedPsaCollideSoundEnabled()
{
	MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_PARAMETER,
        L("PARTICLEEDITOR/GUI/PSA_COLLIDE_PROPERTIES/ENABLE_SOUND")
    );
    CollidePSA *collideAction = action();
    collideAction->soundEnabled(!collideAction->soundEnabled());
    UpdateState();
}

void PsaCollideProperties::OnCbnSelchangePsaCollideSoundProjectList()
{
    MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_PARAMETER,
        L("PARTICLEEDITOR/GUI/PSA_COLLIDE_PROPERTIES/CHANGE_SOUND_BANK")
    );
    string soundProjectStr;
    if (soundProject_.GetCurSel() != -1)
    {
        CString soundProjectCStr;
        soundProject_.GetWindowText( soundProjectCStr );
        soundProjectStr = soundProjectCStr.GetBuffer();
    }
    CollidePSA *collideAction = action();
    collideAction->soundProject( soundProjectStr );

	updateSoundLists();
}

void PsaCollideProperties::OnCbnSelchangePsaCollideSoundGroupList()
{
    MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_PARAMETER,
        L("PARTICLEEDITOR/GUI/PSA_COLLIDE_PROPERTIES/CHANGE_SOUND_GROUP")
    );
    string soundGroupStr;
    if (soundGroup_.GetCurSel() != -1)
    {
        CString soundGroupCStr;
        soundGroup_.GetWindowText( soundGroupCStr );
        soundGroupStr = soundGroupCStr.GetBuffer();
    }
    CollidePSA *collideAction = action();
    collideAction->soundGroup( soundGroupStr );

	updateSoundLists();
}

void PsaCollideProperties::OnCbnSelchangePsaCollideSoundNameList()
{
    MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_PARAMETER,
        L("PARTICLEEDITOR/GUI/PSA_COLLIDE_PROPERTIES/CHANGE_SOUND_NAME")
    );
    string soundNameStr;
    if (soundName_.GetCurSel() != -1)
    {
        CString soundNameCStr;
        soundName_.GetWindowText( soundNameCStr );
        soundNameStr = soundNameCStr.GetBuffer();
    }
    CollidePSA *collideAction = action();
    collideAction->soundName( soundNameStr );
}
