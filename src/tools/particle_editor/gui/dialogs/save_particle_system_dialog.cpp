/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#error File no longer in use.  Allan 04/03/03

// D:\mf\src\tools\particle_editor\gui\save_particle_system_dialog.cpp : implementation file
//

#include "stdafx.h"
#include "particle_editor.h"
#include "save_particle_system_dialog.hpp"


// SaveParticleSystemDialog dialog

IMPLEMENT_DYNAMIC(SaveParticleSystemDialog, CDialog)
SaveParticleSystemDialog::SaveParticleSystemDialog(CWnd* pParent /*=NULL*/)
	: CDialog(SaveParticleSystemDialog::IDD, pParent)
{
}

SaveParticleSystemDialog::~SaveParticleSystemDialog()
{
}

void SaveParticleSystemDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SaveParticleSystemDialog, CDialog)
END_MESSAGE_MAP()


// SaveParticleSystemDialog message handlers
