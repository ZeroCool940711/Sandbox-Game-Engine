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

#include "afxwin.h"
#include "afxcmn.h"
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/edit_numeric.hpp"

class SinkPSA;

// PsaSinkProperties form view

class PsaSinkProperties : public PsaProperties
{
    DECLARE_DYNCREATE(PsaSinkProperties)

public:
    enum { IDD = IDD_PSA_SINK_PROPERTIES };

    PsaSinkProperties(); 

    /*virtual*/ ~PsaSinkProperties();

    SinkPSA * action() { return reinterpret_cast<SinkPSA *>(&*action_); }

    void SetParameters(SetOperation task);

protected:
    /*virtual*/ void OnInitialUpdate();

    /*virtual*/ void DoDataExchange(CDataExchange* pDX); 

    afx_msg void 
    OnNMCustomdrawPsaSinkMaximumageSlider
    (
        NMHDR       *pNMHDR, 
        LRESULT     *pResult
    );

    DECLARE_MESSAGE_MAP()

private:
    controls::EditNumeric    maximumAge_;
    controls::EditNumeric    minimumSpeed_;
    CSliderCtrl     maximumAgeSlider_;
    int             prevSliderPos_;
    CButton			outsideOnly_;
public:
	afx_msg void OnBnClickedPsaSinkOutsideOnly();
};
