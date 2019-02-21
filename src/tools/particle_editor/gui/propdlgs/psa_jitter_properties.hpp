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
#include "gui/propdlgs/psa_properties.hpp"
#include "controls/ccombobox_auto_complete.hpp"
#include "gui/vector_generator_custodian.hpp"

class JitterPSA;


// PsaJitterProperties form view

class PsaJitterProperties : public PsaProperties
{
	DECLARE_DYNCREATE(PsaJitterProperties)

public:
	PsaJitterProperties(); 
	virtual ~PsaJitterProperties();

	enum { IDD = IDD_PSA_JITTER_PROPERTIES };
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	JitterPSA *		action() { return reinterpret_cast<JitterPSA *>(&*action_); }
	void			SetParameters(SetOperation task);

	CButton affectPosition_;
	CButton affectVelocity_;
	afx_msg void OnBnClickedPsaJitterAffectPosition();
	afx_msg void OnBnClickedPsaJitterAffectVelocity();
	afx_msg void OnBnClickedPsaJitterButton();

private:	
	VectorGeneratorCustodian<JitterPSA>     positionGeneratorCustodian_;	
	VectorGeneratorCustodian<JitterPSA>     velocityGeneratorCustodian_;
    controls::CComboBoxAutoComplete                   positionGenerator_;
    controls::CComboBoxAutoComplete                   velocityGenerator_;
    bool                                    filterUpdatePSA_;
};
