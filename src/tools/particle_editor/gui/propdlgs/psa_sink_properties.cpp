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
#include "gui/gui_utilities.hpp"
#include "gui/propdlgs/psa_sink_properties.hpp"
#include "particle/actions/sink_psa.hpp"
#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

namespace
{
    const float SliderScale = 3.3333f; // I know... magic numbers are bad.
}

IMPLEMENT_DYNCREATE(PsaSinkProperties, PsaProperties)

BEGIN_MESSAGE_MAP(PsaSinkProperties, PsaProperties)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_PSA_SINK_MAXIMUMAGE_SLIDER, OnNMCustomdrawPsaSinkMaximumageSlider)
	ON_BN_CLICKED(IDC_PSA_SINK_OUTSIDE_ONLY, &PsaSinkProperties::OnBnClickedPsaSinkOutsideOnly)
END_MESSAGE_MAP()

PsaSinkProperties::PsaSinkProperties()
: 
PsaProperties(PsaSinkProperties::IDD)
{
    maximumAge_.SetMaximum( 30.f );
}

PsaSinkProperties::~PsaSinkProperties()
{
}

void PsaSinkProperties::SetParameters(SetOperation task)
{
    ASSERT(action_);

    SET_FLOAT_PARAMETER(task, maximumAge);
    SET_FLOAT_PARAMETER(task, minimumSpeed);
    SET_CHECK_PARAMETER(task, outsideOnly);

    // set the slider with the new position
    prevSliderPos_ = max((int)(action()->maximumAge() * SliderScale), 0);
    maximumAgeSlider_.SetPos(prevSliderPos_);
}

void PsaSinkProperties::OnInitialUpdate()
{
    PsaProperties::OnInitialUpdate();    
}

void PsaSinkProperties::DoDataExchange(CDataExchange* pDX)
{
    PsaProperties::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_PSA_SINK_MAXIMUMAGE, maximumAge_);
    DDX_Control(pDX, IDC_PSA_SINK_MINIMUMSPEED, minimumSpeed_);
    DDX_Control(pDX, IDC_PSA_SINK_MAXIMUMAGE_SLIDER, maximumAgeSlider_);
    DDX_Control(pDX, IDC_PSA_SINK_OUTSIDE_ONLY, outsideOnly_);
}

void 
PsaSinkProperties::OnNMCustomdrawPsaSinkMaximumageSlider
(
    NMHDR       *pNMHDR, 
    LRESULT     *pResult
)
{
    // this is dupe.  Do sliders a better way!

    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;

    int pos = maximumAgeSlider_.GetPos();

    // if slider has moved, update the action and then update the CEdit control
    if (pos != prevSliderPos_)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_PARAMETER,
            L("PARTICLEEDITOR/GUI/PSA_SINK_PROPERTIES/SET_SINK")
        );

        if (pos == 0)
            action()->maximumAge(-1.0f);
        else
            action()->maximumAge(pos / SliderScale);

        SET_FLOAT_PARAMETER(SET_CONTROL, maximumAge);
        prevSliderPos_ = pos;
    }
}

void PsaSinkProperties::OnBnClickedPsaSinkOutsideOnly()
{
	CopyDataToPSA();
}
