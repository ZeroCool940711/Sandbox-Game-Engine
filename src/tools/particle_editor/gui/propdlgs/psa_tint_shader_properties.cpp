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
#include "gui/propdlgs/psa_tint_shader_properties.hpp"
#include "particle_editor.hpp"
#include "main_frame.hpp"
#include "gui/gui_utilities.hpp"
#include "particle/actions/tint_shader_psa.hpp"
#include "resmgr/string_provider.hpp"


using namespace controls;


DECLARE_DEBUG_COMPONENT2( "GUI", 0 )

IMPLEMENT_DYNCREATE(PsaTintShaderProperties, PsaProperties)


BEGIN_MESSAGE_MAP(PsaTintShaderProperties, PsaProperties)
    ON_BN_CLICKED(IDC_PSA_TINTSHADER_REPEAT, OnBnLoop)
    ON_MESSAGE(WM_CP_LBUTTONMOVE   , OnColorPickerSelMove)
    ON_MESSAGE(WM_CP_LBUTTONUP     , OnColorPickerSelUp)
    ON_MESSAGE(WM_EDITNUMERIC_CHANGE, OnEditClrText   )
    ON_MESSAGE(WM_CT_ADDED_COLOR   , OnAddedTint     )
    ON_MESSAGE(WM_CT_UPDATE_DONE   , OnUpdateTints   )
    ON_MESSAGE(WM_CT_NEW_SELECTION , OnNewTimelineSel)
    ON_BN_CLICKED(IDC_PSA_TINTSHADER_DELETETINT, OnBnDeleteTint)
    ON_BN_CLICKED(IDC_PSA_TINTSHADER_ADDNEWTINT, OnBnAddNewTint)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_PSA_TINTSHADER_FOG_BLEND_SLIDER, OnNMCustomdrawPsaTintshaderFogBlendSlider)
END_MESSAGE_MAP()

void 
PsaTintShaderProperties::OnNMCustomdrawPsaTintshaderFogBlendSlider
(
    NMHDR       *pNMHDR, 
    LRESULT     *pResult
)
{
    LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
    *pResult = 0;

    static int s_prevSliderPos = -1;
	
	int pos = fogAmountSlider_.GetPos();

    // if slider has moved, update the action and then update the CEdit control
    if (pos != s_prevSliderPos)
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_PARAMETER,
            L("PARTICLEEDITOR/GUI/PSA_TINTSHADER_PROPERTIES/SET_FOG_BLEND")
        );

        action()->fogAmount( pos / 100.f );

        SET_FLOAT_PARAMETER( SET_CONTROL, fogAmount );

        s_prevSliderPos = pos;
    }
}

PsaTintShaderProperties::PsaTintShaderProperties() : 
    PsaProperties(PsaTintShaderProperties::IDD),
    filterChange_(0)
{
    fogAmount_.SetNumericType( controls::EditNumeric::ENT_FLOAT);
    fogAmount_.SetMinimum( 0.0 );
    fogAmount_.SetMaximum( 1.0 );
	
	pickerRed_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    pickerRed_.SetMinimum( 0 );
    pickerRed_.SetMaximum( 255 );

    pickerGreen_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    pickerGreen_.SetMinimum( 0 );
    pickerGreen_.SetMaximum( 255 );
    
    pickerBlue_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    pickerBlue_.SetMinimum( 0 );
    pickerBlue_.SetMaximum( 255 );
    
    pickerAlpha_.SetNumericType( controls::EditNumeric::ENT_INTEGER);
    pickerAlpha_.SetMinimum( 0 );
    pickerAlpha_.SetMaximum( 255 ); 
}


PsaTintShaderProperties::~PsaTintShaderProperties()
{
}


void PsaTintShaderProperties::OnInitialUpdate()
{
    PsaProperties::OnInitialUpdate();

    ++filterChange_;

    // Set the loop time and recycling:
    period_.SetValue(action()->period());
    repeat_.SetCheck(action()->repeat() ? BST_CHECKED : BST_UNCHECKED);

    // Create the color timeline:
    CWnd * colorWnd = GetDlgItem(IDC_PSA_TINT_COLORS);
    CRect theRect;
    colorWnd->GetWindowRect( &theRect );
    ScreenToClient(&theRect);
    ColorScheduleItems colorSchedule;
    ColorScheduleItem  newItem;
    std::map<float, Vector4>::iterator it;
    std::map<float, Vector4>::iterator itEnd = action()->tintSet().end();
    float period = action()->period();
    if (period == 0.0f)
        period = 1.0f;
    float invPeriod = 1.0f / period;
    for(it = action()->tintSet().begin(); it != itEnd; it++)
    {
        newItem.normalisedTime_ = it->first * invPeriod;
        newItem.color_ = it->second;
        colorSchedule.push_back(newItem);
    }    
    colorTimelineWnd_.Create
    (
        WS_CHILD | WS_VISIBLE, 
        theRect, 
        this, 
        colorSchedule
    );
    colorTimelineWnd_.totalScheduleTime(action()->period());

    // Create the color picker:
    CWnd *pickerWnd = GetDlgItem(IDC_PSA_TINTSHADER_COLORPICKER);
    CRect pickerRect;
    pickerWnd->GetWindowRect( &pickerRect );
    ScreenToClient(&pickerRect);
    colorPicker_.Create(WS_CHILD|WS_VISIBLE, pickerRect, this);

    // Set the edit controls:
    Vector4 color = colorPicker_.getRGBA();
    pickerRed_  .SetIntegerValue((int)(255.0f*color.x));
    pickerGreen_.SetIntegerValue((int)(255.0f*color.y));
    pickerBlue_ .SetIntegerValue((int)(255.0f*color.z));
    pickerAlpha_.SetIntegerValue((int)(255.0f*color.w));

    --filterChange_;
}


void PsaTintShaderProperties::SetParameters(SetOperation task)
{
    ASSERT(action_);

    if (task == SET_PSA)
    {
        // clear the psa and start again
        action()->clearAllTints();

        std::vector<ColorScheduleItem>::const_iterator iterColors    = 
            colorTimelineWnd_.colourScheduleItems().begin();
        std::vector<ColorScheduleItem>::const_iterator iterColorsEnd = 
            colorTimelineWnd_.colourScheduleItems().end();

        for( ; iterColors != iterColorsEnd; iterColors++)
        {
            float time = iterColors->normalisedTime_ * period_.GetValue();
            action()->addTintAt(time, iterColors->color_);
        }

        // advise window of new time
        colorTimelineWnd_.totalScheduleTime(action()->period());
        action()->repeat(repeat_.GetCheck() == BST_CHECKED ? true : false);
        action()->period(period_.GetValue());
      	colorTimelineWnd_.Invalidate();
    }
    else
    {
        if (::IsWindow(colorTimelineWnd_.GetSafeHwnd()))
        {
            period_.SetValue(action()->period());
            repeat_.SetCheck(action()->repeat() ? BST_CHECKED : BST_UNCHECKED);

            // Rebuild colour timeline:
            colorTimelineWnd_.clearSchedule();
            ColorScheduleItem  newItem;
            std::map<float, Vector4>::iterator it;
            std::map<float, Vector4>::iterator itEnd = action()->tintSet().end();
            float invPeriod = 1.0f / action()->period();
            for (it = action()->tintSet().begin(); it != itEnd; ++it)
            {
                newItem.normalisedTime_ = it->first*invPeriod;
                newItem.color_ = it->second;
                colorTimelineWnd_.addScheduleItem(newItem);
            }
            colorTimelineWnd_.Invalidate();
        }
    }

    SET_CHECK_PARAMETER(task, repeat);

	SET_FLOAT_PARAMETER(task, fogAmount);

    // set the slider with the new position
    fogAmountSlider_.SetPos( max((int)(action()->fogAmount() * 100), 0) );
}


void PsaTintShaderProperties::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_PSA_TINTSHADER_PERIOD, period_);
    DDX_Control(pDX, IDC_PSA_TINTSHADER_REPEAT, repeat_);

    DDX_Control(pDX, IDC_PSA_TINTSHADER_FOG_BLEND, fogAmount_  );
	DDX_Control(pDX, IDC_PSA_TINTSHADER_FOG_BLEND_SLIDER, fogAmountSlider_ );

	DDX_Control(pDX, IDC_PSA_TINTSHADER_PICKER_RED  , pickerRed_  );
    DDX_Control(pDX, IDC_PSA_TINTSHADER_PICKER_GREEN, pickerGreen_);
    DDX_Control(pDX, IDC_PSA_TINTSHADER_PICKER_BLUE , pickerBlue_ );
    DDX_Control(pDX, IDC_PSA_TINTSHADER_PICKER_ALPHA, pickerAlpha_);

    DDX_Control(pDX, IDC_PSA_TINTSHADER_ADDNEWTINT, addTintButton_);

    if (pDX->m_bSaveAndValidate)
    {
        SetParameters(SET_PSA);
    }

    // update the add tint button
    if (colorTimelineWnd_.waitingToAddColor())
        addTintButton_.SetState(true);
    else
        addTintButton_.SetState(false);
}


void PsaTintShaderProperties::OnBnLoop()
{
    MainFrame::instance()->PotentiallyDirty
    (
        true,
        UndoRedoOp::AK_PARAMETER,
        L("PARTICLEEDITOR/GUI/PSA_TINT_SHADER_PROPERTIES/TOGGLE_TINT_CYCLE")
    );
    UpdateData();
}


/*afx_msg*/ LRESULT PsaTintShaderProperties::OnColorPickerSelMove
(
    WPARAM      /*mParam*/, 
    LPARAM      /*lParam*/
)
{
    if (filterChange_ == 0)
    {
        ++filterChange_;
        Vector4 color = colorPicker_.getRGBA();
        pickerRed_  .SetIntegerValue((int)(255.0f*color.x));
        pickerGreen_.SetIntegerValue((int)(255.0f*color.y));
        pickerBlue_ .SetIntegerValue((int)(255.0f*color.z));
        pickerAlpha_.SetIntegerValue((int)(255.0f*color.w));
        if (colorTimelineWnd_.itemSelected())
            colorTimelineWnd_.setColorScheduleItemSelectedColor(color);
        --filterChange_;
    }
    return TRUE;
}


/*afx_msg*/ LRESULT PsaTintShaderProperties::OnColorPickerSelUp
(
    WPARAM      mParam, 
    LPARAM      lParam
)
{
    OnColorPickerSelMove(mParam, lParam);
    if (filterChange_ == 0)
    {
        ++filterChange_;
        if (colorTimelineWnd_.itemSelected())
        {          
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_PARAMETER,
                L("PARTICLEEDITOR/GUI/PSA_TINT_SHADER_PROPERTIES/CHANGE_COLOUR_TINT")
            );
            UpdateData();
        }
        --filterChange_;
    }
    return TRUE;
}


/*afx_msg*/ LRESULT PsaTintShaderProperties::OnUpdateTints
(
    WPARAM      mParam, 
    LPARAM      lParam
)
{
    if (filterChange_ == 0)
    {
        ++filterChange_;
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_PARAMETER,
            L("PARTICLEEDITOR/GUI/PSA_TINT_SHADER_PROPERTIES/MODIFIED_COLOUR_TINT")
        );
        UpdateData();
        --filterChange_;
    }
    return TRUE;
}


/*afx_msg*/ LRESULT PsaTintShaderProperties::OnAddedTint
(
    WPARAM      mParam, 
    LPARAM      lParam
)
{
    Vector4 colour = colorTimelineWnd_.getColorScheduleItemSelectedColor();
    colorPicker_.setRGBA(colour);
    return OnUpdateTints(mParam, lParam);
}


/*afx_msg*/ 
LRESULT 
PsaTintShaderProperties::OnNewTimelineSel
(
    WPARAM      mParam, 
    LPARAM      lParam
)
{
    if (filterChange_ == 0)
    {
        ++filterChange_;
        Vector4 color = colorTimelineWnd_.getColorScheduleItemSelectedColor();
        colorPicker_.setRGBA(color);
        pickerRed_  .SetIntegerValue((int)(255.0f*color.x));
        pickerGreen_.SetIntegerValue((int)(255.0f*color.y));
        pickerBlue_ .SetIntegerValue((int)(255.0f*color.z));
        pickerAlpha_.SetIntegerValue((int)(255.0f*color.w));
        --filterChange_;
    }
    return TRUE;
}


/*afx_msg*/ LRESULT PsaTintShaderProperties::OnEditClrText
(
    WPARAM      wParam, 
    LPARAM      /*lParam*/
)
{
    if (filterChange_ == 0)
    {
        ++filterChange_;
        if 
        (
            colorTimelineWnd_.itemSelected() 
            || 
            wParam == IDC_PSA_TINTSHADER_PERIOD
        )
        {
            MainFrame::instance()->PotentiallyDirty
            (
                true,
                UndoRedoOp::AK_PARAMETER,
                L("PARTICLEEDITOR/GUI/PSA_TINT_SHADER_PROPERTIES/CHANGE_TINT_CYCLE")
            );
        }
        float red   = pickerRed_  .GetIntegerValue()/255.f;
        float green = pickerGreen_.GetIntegerValue()/255.f;
        float blue  = pickerBlue_ .GetIntegerValue()/255.f;
        float alpha = pickerAlpha_.GetIntegerValue()/255.f;   
        Vector4 colour = Vector4(red, green, blue, alpha);
        colorPicker_.setRGBA(colour);
        colorTimelineWnd_.setColorScheduleItemSelectedColor(colour);
        UpdateData();
        --filterChange_;
    }
    return TRUE;
}


/*afx_msg*/ void PsaTintShaderProperties::OnBnDeleteTint()
{
    if 
    (
        colorTimelineWnd_.itemSelected() 
        &&  
        colorTimelineWnd_.removeSelectedColor()
    )
    {
        MainFrame::instance()->PotentiallyDirty
        (
            true,
            UndoRedoOp::AK_PARAMETER,
            L("PARTICLEEDITOR/GUI/PSA_TINT_SHADER_PROPERTIES/DELETE_COLOUR_TINT")
        );
        UpdateData();
    }    
}


/*afx_msg*/ void PsaTintShaderProperties::OnBnAddNewTint()
{
    colorTimelineWnd_.addColorAtLButton();
    addTintButton_.SetState(true);
}
