/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PS_RENDERER_PROPERTIES_HPP
#define PS_RENDERER_PROPERTIES_HPP

#include "afxwin.h"
#include "fwd.hpp"
#include "gui/gui_utilities.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/ccombobox_auto_complete.hpp"
#include "controls/separator.hpp"
#include "controls/image_button.hpp"
#include "controls/auto_tooltip.hpp"

class PsRendererProperties : public CFormView
{
	DECLARE_DYNCREATE(PsRendererProperties)

public:
    enum { IDD = IDD_PS_RENDERER_PROPERTIES };

	PsRendererProperties(); 

	/*virtual*/ ~PsRendererProperties();

	virtual void OnInitialUpdate();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	afx_msg LRESULT OnUpdatePsRenderProperties(WPARAM mParam, LPARAM lParam);

	void SetParameters(SetOperation task);
	void SetSpriteEnabledState(bool option);
	void SetMeshEnabledState  (bool option);
    void SetVisualEnabledState(bool option);
	void SetAmpEnabledState   (bool option);
	void SetTrailEnabledState (bool option);
	void SetBlurEnabledState  (bool option);

	ParticleSystemRendererPtr renderer();

    void resetParticles();
	void copyRendererSettings( ParticleSystemRendererPtr src, ParticleSystemRenderer * dst );

    afx_msg void OnGenericBtn();
	afx_msg void OnWorldDependentBtn();
	afx_msg void OnViewDependentBtn();
	afx_msg void OnLocalDependentBtn();
	afx_msg void OnSpriteBtn();
	afx_msg void OnMeshBtn();
    afx_msg void OnVisualBtn();
	afx_msg void OnAmpBtn();
	afx_msg void OnTrailBtn();
	afx_msg void OnBlurBtn();
    afx_msg void OnSpriteTexturenameDirectoryBtn();
    afx_msg void OnMeshVisualnameDirectoryBtn();
    afx_msg void OnVisualVisualnameDirectoryBtn();
    afx_msg void OnAmpTexturenameDirectoryBtn();
    afx_msg void OnTrailTexturenameDirectoryBtn();
    afx_msg void OnBlurTexturenameDirectoryBtn();
    afx_msg void OnPointSpriteBtn();

	DECLARE_MESSAGE_MAP()

    DECLARE_AUTO_TOOLTIP(PsRendererProperties, CFormView)

    bool DropSpriteTexture(UalItemInfo *ii);
    bool DropMesh         (UalItemInfo *ii);
    bool DropVisual       (UalItemInfo *ii);
    bool DropAmpTexture   (UalItemInfo *ii);
    bool DropTrailTexture (UalItemInfo *ii);
    bool DropBlurTexture  (UalItemInfo *ii);

    CRect CanDropMesh(UalItemInfo *ii);

private:
	bool                    initialised_;
	CButton					worldDependent_;
	CButton					localDependent_;
	CButton                 viewDependent_;
	CButton                 rendererSprite_;
	CButton                 rendererMesh_;
    CButton                 rendererVisual_;
	CButton                 rendererAmp_;
	CButton                 rendererTrail_;
	CButton                 rendererBlur_;
	controls::CComboBoxAutoComplete   textureName_;
    controls::ImageButton   textureNameDirectoryBtn_;
    CEdit                   textureNameDirectoryEdit_;
	controls::CComboBoxAutoComplete   spriteMaterialFX_;
	controls::EditNumeric            frameCount_;
	controls::EditNumeric            frameRate_;
    CButton                 pointSprite_;
	CStatic                 spriteStatic3_;
	CStatic                 spriteStatic4_;
	controls::CComboBoxAutoComplete   meshName_;
	controls::ImageButton   meshNameDirectoryBtn_;
    CEdit                   meshNameDirectoryEdit_;
    controls::CComboBoxAutoComplete   meshMaterialFX_;
    controls::CComboBoxAutoComplete   meshSort_;
	controls::CComboBoxAutoComplete   visualName_;
	controls::ImageButton   visualNameDirectoryBtn_;
    CEdit                   visualNameDirectoryEdit_;
	controls::EditNumeric            width_;
	controls::EditNumeric            height_;
	controls::EditNumeric            steps_;
	controls::EditNumeric            variation_;
	CButton                 circular_;
	controls::CComboBoxAutoComplete   ampTextureName_;
	controls::ImageButton   ampTextureNameDirectoryBtn_;
    CEdit                   ampTextureNameDirectoryEdit_;
	CStatic                 ampStatic4_;
	CStatic                 ampStatic3_;
	CStatic                 ampStatic2_;
	CStatic                 ampStatic1_;
	controls::EditNumeric            trailWidth_;
	controls::EditNumeric            trailSteps_;
	CComboBox               trailTextureName_;
	controls::ImageButton   trailTextureNameDirectoryBtn_;
    CEdit                   trailTextureNameDirectoryEdit_;
	CStatic                 trailStatic1_;
	CStatic                 trailStatic2_;
	controls::EditNumeric            blurTime_;
	controls::EditNumeric            blurWidth_;
	CComboBox               blurTextureName_;
	controls::ImageButton   blurTextureNameDirectoryBtn_;
    CEdit                   blurTextureNameDirectoryEdit_;
	CStatic                 blurStaticT_;
	CStatic                 blurStaticW_;
    controls::Separator     hline1_;
    controls::Separator     hline2_;
    controls::Separator     hline3_;
    controls::Separator     hline4_;
    controls::Separator     hline5_;
    controls::Separator     hline6_;
    bool                    filterChanges_;
};

#endif // PS_RENDERER_PROPERTIES_HPP
