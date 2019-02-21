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

#include "page_terrain_texture.hpp"

#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/gui/pages/panel_manager.hpp"
#include "worldeditor/gui/dialogs/noise_setup_dlg.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "worldeditor/terrain/editor_chunk_terrain.hpp"
#include "worldeditor/terrain/editor_chunk_terrain_cache.hpp"
#include "worldeditor/terrain/terrain_texture_utils.hpp"
#include "worldeditor/terrain/texture_mask_cache.hpp"
#include "worldeditor/undo_redo/terrain_tex_proj_undo.hpp"
#include "worldeditor/height/height_module.hpp"
#include "worldeditor/import/texture_mask_blit.hpp"
#include "worldeditor/world/world_manager.hpp"

#include "chunk/chunk_manager.hpp"
#include "chunk/chunk_space.hpp"

#include "appmgr/options.hpp"
#include "appmgr/module_manager.hpp"

#include "gizmo/tool_manager.hpp"

#include "romp/flora.hpp"

#include "terrain/terrain_texture_layer.hpp"
#include "terrain/terrain_settings.hpp"

#include "controls/utils.hpp"

#include "common/user_messages.hpp"
#include "common/utilities.hpp"
#include "common/string_utils.hpp"
#include "common/file_dialog.hpp"

#include "math/math_extra.hpp"

#include "moo/texture_manager.hpp"

#include <afxpriv.h>


BEGIN_MESSAGE_MAP(PageTerrainTexture, CFormView)
    ON_WM_HSCROLL()
	ON_WM_CTLCOLOR()
	ON_MESSAGE(WM_ACTIVATE_TOOL  , OnActivateTool  )
	ON_MESSAGE(WM_DEFAULT_PANELS , OnDefaultPanels ) 
	ON_MESSAGE(WM_LAST_PANELS    , OnDefaultPanels ) 
    ON_MESSAGE(WM_UPDATE_CONTROLS, OnUpdateControls)
	ON_MESSAGE(WM_CLOSING_PANELS , OnClosing       )
	ON_COMMAND(ID_TER_PAINT_EDITMODE                   , OnEditingButton           )
	ON_COMMAND(ID_TER_PAINT_SAVE_BTN                   , OnSaveBrushButton		   )
	ON_MESSAGE(WM_NEW_SPACE                            , OnNewSpace                )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_SIZE_EDIT           , OnEnChangeSizeEdit        )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_STRENGTH_EDIT       , OnEnChangeStrengthEdit    )
	ON_EN_CHANGE (IDC_PAGE_TERRAIN_OPACITY_EDIT        , OnEnChangeOpacityEdit     )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_SIZE_BUTTON         , OnBnClickedSizeButton     )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_LIMIT_LAYERS_CB     , OnMaxLayersCB			   )
	ON_EN_CHANGE (IDC_PAGE_TERRAIN_LIMIT_LAYERS_EDIT   , OnMaxLayersEdit		   )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_HEIGHT_MASK_BUTTON  , OnHeightButton            )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_HEIGHT_MIN_EDIT     , OnEnChangeMinHeightEdit   )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_HEIGHT_MAX_EDIT     , OnEnChangeMaxHeightEdit   )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_HEIGHT_FUZ_EDIT     , OnEnChangeFuzHeightEdit   )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_SLOPE_MASK_BUTTON   , OnSlopeButton             )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_SLOPE_MIN_EDIT      , OnEnChangeMinSlopeEdit    )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_SLOPE_MAX_EDIT      , OnEnChangeMaxSlopeEdit    )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_SLOPE_FUZ_EDIT      , OnEnChangeFuzSlopeEdit    )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_TEXMASK_BUTTON      , OnTexMaskButton           )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON , OnTexMaskIncludeProjButton)
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON  , OnTexMaskInvButton        )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON   , OnNoiseMaskButton         )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_NOISE_SETUP_BUTTON  , OnNoiseSetupButton        )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_IMPORTED_MASK_BUTTON, OnMaskImportButton        )
	ON_EN_CHANGE (IDC_PAGE_TERRAIN_MASK_STRENGTH_EDIT  , OnMaskStrengthEdit        )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_ANTICLOCKWISE_BTN         , OnAnticlockwiseBtn       )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_CLOCKWISE_BTN             , OnClockwiseBtn           )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_FLIPX_BTN                 , OnFlipXBtn               )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_FLIPY_BTN                 , OnFlipYBtn               )
    ON_BN_CLICKED(IDC_PAGE_TERRAIN_FLIPH_BTN                 , OnFlipHeightBtn          )
    ON_UPDATE_COMMAND_UI(IDC_PAGE_TERRAIN_ANTICLOCKWISE_BTN  , OnAnticlockwiseBtnEnable )
    ON_UPDATE_COMMAND_UI(IDC_PAGE_TERRAIN_CLOCKWISE_BTN      , OnClockwiseBtnEnable     )
    ON_UPDATE_COMMAND_UI(IDC_PAGE_TERRAIN_FLIPX_BTN          , OnFlipXBtnEnable         )
    ON_UPDATE_COMMAND_UI(IDC_PAGE_TERRAIN_FLIPY_BTN          , OnFlipYBtnEnable         )
    ON_UPDATE_COMMAND_UI(IDC_PAGE_TERRAIN_FLIPH_BTN          , OnFlipHeightBtnEnable    )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_BROWSE_MASK_BUTTON  , OnMaskBrowseButton       )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_ADJUST_MASK_BUTTON  , OnMaskAdjustButton		  )	
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_OVERLAY_MASK_BUTTON , OnMaskOverlayButton	  )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_YAW_ANGLE_EDIT      , OnEnChangeYawAngleEdit   )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_PITCH_ANGLE_EDIT    , OnEnChangePitchAngleEdit )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_ROLL_ANGLE_EDIT     , OnEnChangeRollAngleEdit  )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_UTILING_EDIT        , OnEnChangeUSizeEdit      )
    ON_EN_CHANGE (IDC_PAGE_TERRAIN_VTILING_EDIT        , OnEnChangeVSizeEdit      )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_LINK				   , OnBnClickedUVScaleLink	  )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_RESET_PROJ_BUTTON   , OnResetButton			  )
	ON_BN_CLICKED(IDC_PAGE_TERRAIN_MASK_PLACE_BUTTON   , OnPlaceMask			  )
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()


const std::string PageTerrainTexture::contentID = "PageTerrainTexture";
/*static*/ PageTerrainTexture* PageTerrainTexture::s_instance_ = NULL;


/**
 *  Constants used in the GUI and configuration files.
 */
namespace
{
	// Number of chunks before we do not do an undo/redo for mask import
	const size_t PROMPT_LARGE_MASK		= 1600; // chunks  

	// Number of chunks after which a progress is displayed for mask import
	const size_t PROGRESS_LARGE_MASK	=  100; // chunks 

	// Number of chunks in a texture replace before being prompted
	const size_t PROMPT_LARGE_REPLACE	=   25; // chunks

	// The message displayed when entering edit mode:
	const char * ENTER_EDIT_MSG			= "Editing texture projections - press ESC to cancel";

	// The message displayed when entering painting mode:
	const char * ENTER_PAINT_MSG			= "Painting textures";

    const uint32 SEP_OFFSET             = 4; // offset inwards of separators

	// Some values are passed to Python via the Options object.
	const char * OPTS_MASK_OVERLAY		= "terrain/texture/maskoverlay";
	const char * OPTS_SIZE              = "terrain/texture/size";
	const char * OPTS_STRENGTH          = "terrain/texture/strength";
    const char * OPTS_SIZE_MIN          = "terrain/texture/minsize";
    const char * OPTS_SIZE_MAX          = "terrain/texture/maxsize";
    const char * OPTS_SIZE_MIN_LIM      = "terrain/texture/minsizelimit";
    const char * OPTS_SIZE_MAX_LIM      = "terrain/texture/maxsizelimit";
									     
	const char * DEFAULT_BRUSH			= "resources/data/default.brush";

	const uint32 MASK_DIB_SIZE			= 128;

	const float	 SCALE_MIN				= 0.1f;
	const float	 SCALE_MAX				= 100.0f;

	const float  EDITING_EPSILON		= 0.01f;

	bool isEditingGizmo()
	{
		return InputDevices::isShiftDown();
	}

	bool isSampling()
	{
		return InputDevices::isAltDown();
	}

	bool isSamplingOpacity()
	{
		return InputDevices::isCtrlDown();
	}

	bool isMouseUp()
	{
		return !InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE );
	}

	template<typename CLASS, typename CONTROL, typename FUNCTION>
	void addTextureDrop( 
		CLASS &			instance, 
		CONTROL &		control, 
		FUNCTION &		function )
	{
		for(
			const char ** pExt = Moo::TextureManager::validTextureExtensions();
			*pExt != NULL;
			++pExt)
		{
			UalManager::instance().dropManager().add(
				new UalDropFunctor<CLASS>(
					&control,
					*pExt, 
					&instance,
					function ) );
		}
	}
}


// Compile in the brush thumbnail provider (prevent the linker from
// stripping it).
extern int BrushThumbProvider_token; 
static int s_useBrushThumbProvider = BrushThumbProvider_token;


/**
 *	This class is used by Gizmos to edit the texture projection
 *  matrix.
 */
class TexProjMatrixProxy : public MatrixProxy
{
public:
	explicit TexProjMatrixProxy( PageTerrainTexture * pPageTerrainTexture ):
		pPageTerrainTexture_( pPageTerrainTexture ),
		matrix_( pPageTerrainTexture->textureProjectionInverse() )
	{
	}

	void EDCALL getMatrix( Matrix & m, bool /*world*/ )
	{
		m = matrix_;
	}

	void EDCALL getMatrixContext( Matrix & m )
	{
		m = Matrix::identity;
	}

	void EDCALL getMatrixContextInverse( Matrix & m )
	{
		m = Matrix::identity;
	}

	bool EDCALL setMatrix( const Matrix & m )
	{
		matrix_ = m;
		// We don't lock the U and V values here even if the "lock" UI option
		// is set.
		pPageTerrainTexture_->textureProjectionInverse( matrix_, true );
		return true;
	}

	void EDCALL setMatrixAlone( const Matrix & m )
	{
		matrix_ = m;
	}

	void EDCALL recordState()
	{
		pPageTerrainTexture_->beginEditLayers();
		recordMatrix_ = matrix_;
	}

	bool EDCALL commitState( bool revertToRecord, bool /*addUndoBarrier*/ )
	{
		if (revertToRecord)
		{
			if (matrix_ != recordMatrix_)
			{
				matrix_ = recordMatrix_;
			}
		}
		else
		{
			pPageTerrainTexture_->textureProjectionInverse( matrix_, false );			
		}
		pPageTerrainTexture_->endEditLayers();
		return true;
	}

	virtual bool EDCALL hasChanged()
	{
		return !!(recordMatrix_ != matrix_);
	}

private:
	PageTerrainTexture		*pPageTerrainTexture_;
	Matrix					matrix_;
	Matrix					recordMatrix_;
};


/**
 *	This returns the PageTerrainTexture instance.  Note that this can be NULL,
 *	we cannot create one of these on demand.
 *
 *	@returns		The PageTerrainTexture instance.
 */
/*static*/ PageTerrainTexture * PageTerrainTexture::instance()
{
	return s_instance_;
}


/**
 *  This is the PageTerrainTexture constructor.  Most of the actual setting up 
 *  is done in initPage.
 */
PageTerrainTexture::PageTerrainTexture(): 
    CFormView( PageTerrainTexture::IDD ),
	linkIcon_( NULL ),
	inited_( false ),
	invTexProjection_( Matrix::identity ),
	invTexProjTrans_( Vector3::zero() ),
	filter_( 0 ),
	isActive_( false ),
	hasUVProjections_( false ),
	mode_( PAINTING ),
	inHScroll_( false ),
	samplingNormal_( false ),
	gizmoDrawState_( 0 ),
	paintLayer_( EditorChunkTerrain::NO_DOMINANT_BLEND ),
	isAdjustingMask_( false ),
	yaw_( 0.0f ),
	pitch_( 0.0f ),
	roll_( 0.0f ),
	uScale_( 0.0f ),
	vScale_( 0.0f ),
	layerUndoAdded_( 0 )
{
	s_instance_ = this;
}


/**
 *  This is the PageTerrainTexture destructor.
 */
/*virtual*/ PageTerrainTexture::~PageTerrainTexture()
{
	::DestroyIcon( linkIcon_ );
	linkIcon_ = NULL;

    s_instance_ = NULL;
}


/**
 *  This is called to initialise the dialog and its controls.
 */
void PageTerrainTexture::initPage()
{
	++filter_;

	// Create and try to load the default brush:
	DataSectionPtr pDefBrushSec = BWResource::openSection( DEFAULT_BRUSH );
	pBrush_ = TerrainPaintBrushPtr( new TerrainPaintBrush(), true );
	pBrush_->load( pDefBrushSec );

	this->checkTerrainVersion();

	inited_= true;

	// The image control for textures cannot not be subclassed, otherwise
	// they do not support drag and drop.  Instead we hide the statics that
	// we'd otherwise subclass and create the image controls in their 
	// places:
	CRect imgExt = 
		controls::childExtents( *this, IDC_PAGE_TERRAIN_CURRENT_IMAGE );
	CWnd * pImgStatic = GetDlgItem( IDC_PAGE_TERRAIN_CURRENT_IMAGE );
	pImgStatic->ShowWindow(SW_HIDE);
	currentTextureImage_.Create(
		WS_CHILD | WS_VISIBLE, 
		imgExt, 
		this, 
		IDC_PAGE_TERRAIN_MASKTEX_IMAGE );
	currentTextureImage_.text( L( "RCST_IDC_PAGE_TERRAIN_CURRENT_IMAGE" ) );
	imgExt = 
		controls::childExtents( *this, IDC_PAGE_TERRAIN_MASKTEX_IMAGE );
	pImgStatic = GetDlgItem( IDC_PAGE_TERRAIN_MASKTEX_IMAGE );
	pImgStatic->ShowWindow( SW_HIDE );
	maskTexture_.Create(
		WS_CHILD | WS_VISIBLE, 
		imgExt, 
		this, 
		IDC_PAGE_TERRAIN_MASKTEX_IMAGE );
	maskTexture_.text( L( "RCST_IDC_PAGE_TERRAIN_MASKTEX_IMAGE" ) );

	sizeSlider_.setRangeLimit( 
        Options::getOptionFloat( OPTS_SIZE_MIN_LIM,    0.3f ),
		Options::getOptionFloat( OPTS_SIZE_MAX_LIM, 4000.0f ) );
	sizeSlider_.setRange( 
        Options::getOptionFloat( OPTS_SIZE_MIN,   0.3f ),
		Options::getOptionFloat( OPTS_SIZE_MAX, 800.0f ) );
	sizeSlider_.setValue( Options::getOptionFloat( OPTS_SIZE, 0.3f ) );
    sizeSlider_.setDigits( 1 );
	
	sizeEdit_.SetNumDecimals( 1 );
	sizeButton_.setBitmapID( IDB_RANGESLIDER, IDB_RANGESLIDER );	

	strengthSlider_.setRangeLimit( 0.0f, 100.0f );
	strengthSlider_.setRange     ( 0.0f, 100.0f );
    strengthSlider_.setDigits( 1 );
	strengthEdit_  .SetNumDecimals( 1 );

	opacitySlider_.setDigits( 1 );
	opacitySlider_.setRangeLimit( 0.0f, 100.0f );
	opacityEdit_  .SetNumDecimals( 1 );

	maxLayersEdit_.SetNumericType( controls::EditNumeric::ENT_INTEGER );
	maxLayersEdit_.SetMinimum( 1.0 );

	saveBrushButton_.setBitmapID( IDB_SAVEBRUSH, IDB_SAVEBRUSHD );

	controls::setWindowText( *this, IDC_PAGE_TEX_DIM , "" );
	controls::setWindowText( *this, IDC_PAGE_TEX_SIZE, "" );

	maskStrengthEdit_.SetNumDecimals( 1 );
	maskStrengthSlider_.setDigits( 1 );
	maskStrengthSlider_.setRangeLimit( 0.0f, 5000.0f );
	maskStrengthSlider_.setRange( 0.0f, 5000.0f );

	controls::checkButton(
		*this,
		IDC_PAGE_TERRAIN_OVERLAY_MASK_BUTTON,
		Options::getOptionInt( OPTS_MASK_OVERLAY, 1 ) != 0 ? true : false );

	yawAngleEdit_  .SetNumDecimals( 1 );
	yawAngleSlider_.setDigits( 1 );
	yawAngleSlider_.setRange( -180.0f, +180.0f ); 
	yawAngleSlider_.setRangeLimit( -180.0f, +180.0f );

	pitchAngleEdit_  .SetNumDecimals( 1 );
	pitchAngleSlider_.setDigits( 1 );
	pitchAngleSlider_.setRange( -180.0f, +180.0f );
	pitchAngleSlider_.setRangeLimit( -180.0f, +180.0f );

	rollAngleEdit_  .SetNumDecimals( 1 );
	rollAngleSlider_.setDigits( 1 );
	rollAngleSlider_.setRange( -180.0f, +180.0f );
	rollAngleSlider_.setRangeLimit( -180.0f, +180.0f );	

	uSizeEdit_  .SetNumDecimals( 1 );
	uSizeEdit_  .SetMinimum( SCALE_MIN );
	uSizeEdit_  .SetMaximum( SCALE_MAX );
	uSizeSlider_.setDigits( 1 );
	uSizeSlider_.setRange( SCALE_MIN, SCALE_MAX );
	uSizeSlider_.setRangeLimit( SCALE_MIN, SCALE_MAX );	

	vSizeEdit_  .SetNumDecimals( 1 );
	vSizeEdit_  .SetMinimum( SCALE_MIN );
	vSizeEdit_  .SetMaximum( SCALE_MAX );	
	vSizeSlider_.setDigits( 1 );
	vSizeSlider_.setRange( SCALE_MIN, SCALE_MAX );
	vSizeSlider_.setRangeLimit( SCALE_MIN, SCALE_MAX );

	editProjButton_.setBitmapID( IDB_EDITPROJ, IDB_EDITPROJD );
	editProjButton_.toggleButton( true );

	linkIcon_ = (HICON)LoadImage( AfxGetInstanceHandle(),
		MAKEINTRESOURCE( IDI_PLACEMENT_LINK ),
		IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR );
	linkUVScale_.SetIcon( linkIcon_ );

	maskTextureYawEdit_  .SetNumDecimals( 1 );
	maskTexturePitchEdit_.SetNumDecimals( 1 );
	maskTextureRollEdit_ .SetNumDecimals( 1 );
	maskTextureUProjEdit_.SetNumDecimals( 1 );
	maskTextureVProjEdit_.SetNumDecimals( 1 );

	this->OnUpdateControls( 0, 0 );
	this->updateSliderEdits();
    this->OnSlopeButton();
    this->OnHeightButton();    

	addTextureDrop( 
		*this, currentTextureImage_, &PageTerrainTexture::onDropTexture );
	addTextureDrop( 
		*this, maskTexture_, &PageTerrainTexture::onDropTextureMask );

	UalManager::instance().dropManager().add(
		new UalDropFunctor<PageTerrainTexture>(
			this,
			"brush", 
			this,
			&PageTerrainTexture::onDropBrush ) );

	INIT_AUTO_TOOLTIP();

	// Initialise the toolbar after INIT_AUTO_TOOLTIP, otherwise it adds a
	// catch-all tooltip for every toolbar command.

    actionTB_.CreateEx(
        this, 
        TBSTYLE_FLAT, 
        WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP );
    actionTB_.LoadToolBarEx( IDR_IMP_MASK_TOOLBAR, IDR_IMP_MASK_DIS_TOOLBAR );
    actionTB_.SetBarStyle( CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY );              
    actionTB_.Subclass( IDC_PAGE_TERRAIN_MASK_ACTIONTB );
    actionTB_.ShowWindow( SW_SHOW );

	controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_PLACE_BUTTON, false );

	--filter_;

	// Match the controls with the brush:
	this->setTerrainBrush( pBrush_ );
	this->checkTerrainVersion(); // call this a second time to update the controls
}


/**
 *  This keeps the PageTerrainTexture in sync with its controls.
 *
 *  @param pDX      A pointer to a CDataExchange, used to swap data with the
 *                  controls.
 */
/*virtual*/ void PageTerrainTexture::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);

    DDX_Control( pDX, IDC_PAGE_TERRAIN_SIZE_EDIT           , sizeEdit_             );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_SIZE_SLIDER         , sizeSlider_           );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_SIZE_BUTTON         , sizeButton_           );	
    DDX_Control( pDX, IDC_PAGE_TERRAIN_STRENGTH_EDIT       , strengthEdit_         );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_STRENGTH_SLIDER     , strengthSlider_       );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_OPACITY_EDIT        , opacityEdit_          );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_OPACITY_SLIDER      , opacitySlider_        );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_LIMIT_LAYERS_EDIT   , maxLayersEdit_        );
	DDX_Control( pDX, ID_TER_PAINT_SAVE_BTN                , saveBrushButton_      );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_HEIGHT_MIN_EDIT     , minHeightEdit_        );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_HEIGHT_MAX_EDIT     , maxHeightEdit_        );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_HEIGHT_FUZ_EDIT     , fuzHeightEdit_        );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_SLOPE_MIN_EDIT      , minSlopeEdit_         );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_SLOPE_MAX_EDIT      , maxSlopeEdit_         );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_SLOPE_FUZ_EDIT      , fuzSlopeEdit_         );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_TEXMASK_YAW_EDIT    , maskTextureYawEdit_   );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_TEXMASK_PITCH_EDIT  , maskTexturePitchEdit_ );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_TEXMASK_ROLL_EDIT   , maskTextureRollEdit_  );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_TEXMASK_UTILING_EDIT, maskTextureUProjEdit_ );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_TEXMASK_VTILING_EDIT, maskTextureVProjEdit_ );
	DDX_Control( pDX, ID_TER_PAINT_EDITMODE                , editProjButton_       );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_MASK_STRENGTH_EDIT  , maskStrengthEdit_     );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_MASK_STRENGTH_SLIDER, maskStrengthSlider_   );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_MASK_SEP1           , separator_		       );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_YAW_ANGLE_EDIT      , yawAngleEdit_         );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_YAW_ANGLE_SLIDER    , yawAngleSlider_       );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_PITCH_ANGLE_EDIT    , pitchAngleEdit_       );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_PITCH_ANGLE_SLIDER  , pitchAngleSlider_     );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_ROLL_ANGLE_EDIT     , rollAngleEdit_        );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_ROLL_ANGLE_SLIDER   , rollAngleSlider_      );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_UTILING_EDIT        , uSizeEdit_            );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_UTILING_SLIDER      , uSizeSlider_          );
    DDX_Control( pDX, IDC_PAGE_TERRAIN_VTILING_EDIT        , vSizeEdit_            );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_VTILING_SLIDER      , vSizeSlider_          );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_LINK				   , linkUVScale_		   );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_LINKUP			   , linkUIcon_			   );
	DDX_Control( pDX, IDC_PAGE_TERRAIN_LINKDOWN			   , linkVIcon_			   );

    DDX_Control(
        pDX,
        IDC_PAGE_TERRAIN_MASK_IMAGE,
        maskImage_ );
}


/**
 *	This is called when the tool is made active.
 *
 *	@param wParam		A pointer to the name of the tool being activated.
 *	@param lParam		Not used.
 *	@returns			0.
 */
/*afx_msg*/ LRESULT PageTerrainTexture::OnActivateTool(
	WPARAM	wParam, 
	LPARAM	/*lParam*/ )
{
	if (!inited_)
	{
		this->initPage();
	}

	const char * pToolId = (const char *)wParam;
	if (getContentID() == pToolId)
	{
		oldCoordMode_ = 
			Options::getOptionString( "tools/coordFilter", "Local" );
		Options::setOptionString( "tools/coordFilter", "Local" );
		isActive_ = true;
		this->buildEditors();
		if (WorldEditorApp::instance().pythonAdapter())
		{
			WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect(
				"pgcTerrain", 
				"TerrainTextures" );
		}
		this->buildMaskImage();
		this->updatePython();

		this->onEnableMaskControls();
		this->checkTerrainVersion();

		this->mode( PAINTING );		
	}
	else
	{
		if (isAdjustingMask_)
		{
			this->endAdjustMask();
		}
		if (isActive_)
		{
			isActive_ = false;
			this->destroyEditors();
			Options::setOptionString( "tools/coordFilter", oldCoordMode_ );
			WorldManager::instance().resetCursor();
		}
	}

	return 0;
}


/**
 *	This is called when the panel layout is reset to the default or reloaded.
 *
 *	@param wParam		Not used.
 *	@param lParam		Not used.
 *	@returns			0.	
 */
/*afx_msg*/ LRESULT PageTerrainTexture::OnDefaultPanels(
	WPARAM	/*wParam*/, 
	LPARAM	/*lParam*/ )
{
	if (!inited_)
	{
		this->initPage();
	}

	if (isActive_)
	{
		this->destroyEditors();
		Options::setOptionString( "tools/coordFilter", oldCoordMode_ );
		isActive_ = false;
	}

	return 0;
}


/**
 *  This is called every frame to update the controls.
 *
 *  @param wparam       Not used.
 *  @param lparam       Not used.
 *  @returns            0.
 */
/*afx_msg*/ LRESULT 
PageTerrainTexture::OnUpdateControls(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/ )
{
	if (!inited_)
	{
		this->initPage();
	}

	if (!PanelManager::instance().isCurrentTool( 
		PageTerrainTexture::contentID ))
	{
		return 0;
	}

	++layerUndoAdded_;
	// Get the size/strength from Options.  This can be set via Python.
	float size     = Options::getOptionFloat( OPTS_SIZE    , 1.0f );
    float strength = Options::getOptionFloat( OPTS_STRENGTH, 1.0f );
	// Set the sliders to match.  The edits are set below.
	pBrush_->size_     = size;
	pBrush_->strength_ = strength;
	sizeSlider_    .setValue( size     );
    strengthSlider_.setValue( strength );

	Options::setOptionFloat( OPTS_SIZE_MIN, sizeSlider_.getMinRange() );
	Options::setOptionFloat( OPTS_SIZE_MAX, sizeSlider_.getMaxRange() );

	if (!inHScroll_)
	{
		this->updateSliderEdits();
	}

	if (isAdjustingMask_)
	{
		return 0;
	}

	this->updateGizmo( false );

	if (samplingNormal_ || (isEditingGizmo() && isSampling()))
	{
		if (this->canEditProjections())
		{
			this->onSampleNormal(isMouseUp());
		}
	}
	else if (isEditingGizmo())
	{
		this->onEditingGizmo( false );
	}
	else if (isSampling())
	{
		this->onSampleTexture( 
			isMouseUp(), EditorChunkTerrain::NO_DOMINANT_BLEND, NULL, true );
	}
	else
	{
		switch (mode_)
		{
		case PAINTING:
			onPainting( isMouseUp() );
			break;

		case REPLACING:
			onReplacing( isMouseUp() );
			break;

		case EDITING:
			onEditing();
			break;
		}
	}

	--layerUndoAdded_;
    return 0;
}


/**
 *	This is called when the panels are being closed.
 *
 *  @param wparam       Not used.
 *  @param lparam       Not used.
 *  @returns            0.
 */
/*afx_msg*/ LRESULT 
PageTerrainTexture::OnClosing(
    WPARAM      /*wParam*/, 
    LPARAM      /*lParam*/ )
{
	// Save the default brush:
	DataSectionPtr pDefBrushSec 
		= BWResource::openSection( DEFAULT_BRUSH, true );
	pBrush_->save( pDefBrushSec );
	pDefBrushSec->save();

	this->destroyEditors();
	pTileGizmo_ = NULL; // This forces visual, visual channels etc to not be held on to.
	editedLayer_.clear();
	seedTerrain_.terrain_ = NULL;

	return 0;
}


/**
 *  This is called when a slider is scrolled.
 *
 *  @param sbCode		The scroll code.
 *	@param pos			The scroll position.
 *  @param pScrollBar	The scroll-bar being modified.
 */
void PageTerrainTexture::OnHScroll(
    UINT        sBCode, 
    UINT        pos, 
    CScrollBar * pScrollBar )
{
	if (!inited_)
	{
		this->initPage(); 
	}

	if (filter_ == 0 && pScrollBar != NULL)
	{
		++layerUndoAdded_;
		this->updateSliderEdits();
		
		Options::setOptionFloat( OPTS_SIZE, sizeSlider_.getValue() );
		++filter_;

		if (!inHScroll_)
		{
			inHScroll_ = true;
			if (pTileGizmo_ != NULL)
			{
				if (mode_ != EDITING)
				{
					this->translateGizmoToMiddle();
				}
			}
			this->beginEditLayers();
		}

		float maskStrength = maskStrengthSlider_.getValue();
		maskStrengthEdit_.SetValue(maskStrength);
		HeightModule * pHeightModule = HeightModule::currentInstance();
		if (pHeightModule != NULL)
		{
			pHeightModule->textureImportStrength( maskStrength/100.0f );
		}

		if (pTileGizmo_ != NULL)
		{
			pTileGizmo_->drawOptions( 
				gizmoDrawState_ | TileGizmo::DRAW_FORCED );
			pTileGizmo_->opacity( (uint8)(2.55f*opacitySlider_.getValue()) );
		}

		bool endScroll = sBCode == TB_ENDTRACK;

		this->updateProjection( !endScroll );

		if (endScroll)
		{
			inHScroll_ = false;
			this->endEditLayers();
			if (pTileGizmo_ != NULL)
			{
				pTileGizmo_->drawOptions( gizmoDrawState_ );
			}
		}
		this->updatePython();		

		--filter_;
		--layerUndoAdded_;
	}
	else
	{
		++layerUndoAdded_;
		this->updateSliderEdits();
		
		Options::setOptionFloat( OPTS_SIZE, sizeSlider_.getValue() );

		--layerUndoAdded_;
	}
	CFormView::OnHScroll( sBCode, pos, pScrollBar );	
}


/**
 *  This is called when each item is about to be drawn.  We want limit slider edit
 *	to be highlighted is they are out of bounds.
 *
 *	@param pDC	Contains a pointer to the display context for the child window.
 *	@param pWnd	Contains a pointer to the control asking for the color.
 *	@param nCtlColor	Specifies the type of control.
 *	@return	A handle to the brush that is to be used for painting the control
 *			background.
 */
afx_msg HBRUSH PageTerrainTexture::OnCtlColor( CDC* pDC, CWnd* pWnd, UINT nCtlColor )
{
	HBRUSH brush = CFormView::OnCtlColor( pDC, pWnd, nCtlColor );

	sizeEdit_.SetBoundsColour( pDC, pWnd,
		sizeSlider_.getMinRangeLimit(),	sizeSlider_.getMaxRangeLimit() );
	strengthEdit_.SetBoundsColour( pDC, pWnd,
		strengthSlider_.getMinRangeLimit(), strengthSlider_.getMaxRangeLimit() );
	opacityEdit_.SetBoundsColour( pDC, pWnd,
		opacitySlider_.getMinRangeLimit(), opacitySlider_.getMaxRangeLimit() );
	yawAngleEdit_.SetBoundsColour( pDC, pWnd,
		yawAngleSlider_.getMinRangeLimit(), yawAngleSlider_.getMaxRangeLimit() );
	pitchAngleEdit_.SetBoundsColour( pDC, pWnd,
		pitchAngleSlider_.getMinRangeLimit(), pitchAngleSlider_.getMaxRangeLimit() );
	rollAngleEdit_.SetBoundsColour( pDC, pWnd,
		rollAngleSlider_.getMinRangeLimit(), rollAngleSlider_.getMaxRangeLimit() );
	uSizeEdit_.SetBoundsColour( pDC, pWnd,
		uSizeSlider_.getMinRangeLimit(), uSizeSlider_.getMaxRangeLimit() );
	vSizeEdit_.SetBoundsColour( pDC, pWnd,
		vSizeSlider_.getMinRangeLimit(), vSizeSlider_.getMaxRangeLimit() );
	maskStrengthEdit_.SetBoundsColour( pDC, pWnd,
		maskStrengthSlider_.getMinRangeLimit(), maskStrengthSlider_.getMaxRangeLimit() );

	return brush;
}


/**
 *  This is called when something is selected in the UAL.  It may or may not be 
 *	a texture though.
 *
 *  @param filename     The new texture.
 */
void PageTerrainTexture::currentTexture( const std::string & filename )
{
	Vector4 uproj, vproj;
	Terrain::TerrainTextureLayer::defaultUVProjections( uproj, vproj );
	this->currentTexture( filename, uproj, vproj );
}


/**
 *	This is called to set the current texture.
 *
 *  @param filename	The filename of the new texture.
 *  @param u		The u-projection.
 *  @param v		The v-projection.
 */
void PageTerrainTexture::currentTexture(
	const std::string	& filename, 
	const Vector4		& u, 
	const Vector4		& v )
{
	bool textureValid = true;	

	if (!Moo::TextureManager::instance()->isTextureFile( filename ))
	{
		textureValid = false;
	}

    if (currentTextureFilename_ != filename && textureValid)
	{
		controls::enableWindow( 
			*this, IDC_PAGE_TERRAIN_MASK_PLACE_BUTTON, true );

		textureValid = false; // assume load or conversion to DIB fails
		// Try load the texture, allow animations, don't insist that the 
		// texture exists, load the texture if it does exist but it is not in
		// the cache:
		Moo::BaseTexturePtr pBaseTexture = 
			Moo::TextureManager::instance()->get( 
				filename, true, false, true );
		if (
			pBaseTexture != NULL
			&&
			pBaseTexture->pTexture()->GetType() == D3DRTYPE_TEXTURE )
		{
			DX::Texture * pDxTexture =
				static_cast< DX::Texture * >(pBaseTexture->pTexture());
			controls::DibSection32 newTexture;
			textureValid = newTexture.copyFromTexture( pDxTexture );
			if (textureValid)
			{
				pBrush_->paintTexture_ = filename;
				currentTextureFilename_ = filename;
				currentTextureImage_.image( newTexture );
				std::string file = BWResource::getFilename( filename );
				controls::setWindowText( *this, IDC_PAGE_TEXNAME_STATIC, file );
				if (pTileGizmo_ != NULL)
				{
					pTileGizmo_->texture(texture());
				}
				D3DSURFACE_DESC texDesc;
				HRESULT hr = pDxTexture->GetLevelDesc( 0, &texDesc );
				MF_ASSERT(SUCCEEDED(hr));
				std::string sizeTxt = 
					L(
						"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/SIZE", 
						texDesc.Width, 
						texDesc.Height );
				controls::setWindowText(*this, IDC_PAGE_TEX_DIM, sizeTxt);
				std::string memTxt =
					L(
						"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/MEMORY", 
						DX::textureSize( pDxTexture )/1024 ); // size in kb 
				controls::setWindowText( *this, IDC_PAGE_TEX_SIZE, memTxt );
			}
		}
	}

	if (textureValid)
    {       
		invTexProjection_ = buildMatrix( u, v );
		invTexProjection_.invert();		
		invTexProjTrans_  = invTexProjection_.applyToOrigin();

        this->updateEditsFromProjection( true );
        this->updatePython();        
    }
}


/**
 *  This is called to get the u-projection for the current texture operation.
 *
 *  @returns            The current u-projection.
 */
Vector4 PageTerrainTexture::uProjection() const
{
	Matrix texProj = invTexProjection_;
	texProj.translation( invTexProjTrans_ );
	texProj.invert();
    return texProj.column( 0 );
}


/**
 *  This is called to get the v-projection for the current texture operation.
 *
 *  @returns            The current v-projection.
 */
Vector4 PageTerrainTexture::vProjection() const
{
	Matrix texProj = invTexProjection_;
	texProj.translation( invTexProjTrans_ );
	texProj.invert();
    return texProj.column( 2 );
}


/**
 *	This is called to set the terrain painting brush.
 *
 *	@param pBrush		The new brush to use.  If this is NULL then the 
 *						empty brush is used.
 */
void PageTerrainTexture::setTerrainBrush( TerrainPaintBrushPtr pBrush )
{
	++filter_;
	++layerUndoAdded_;
		
	// Use the empty brush?
	if (!pBrush)
	{
		pBrush = TerrainPaintBrushPtr( new TerrainPaintBrush(), true );
	}

	// Texture and projection:
	this->currentTexture( 
		pBrush->paintTexture_, pBrush->paintUProj_, pBrush->paintVProj_ );
	linkUVScale_.SetCheck( pBrush->uvLocked_ ? BST_CHECKED : BST_UNCHECKED );
	linkUIcon_.ShowWindow( pBrush->uvLocked_ ? SW_SHOW : SW_HIDE );
	linkVIcon_.ShowWindow( pBrush->uvLocked_ ? SW_SHOW : SW_HIDE );

	// Brush size:
	sizeSlider_.setValue( pBrush->size_ );
	sizeEdit_  .SetValue( pBrush->size_ );
	Options::setOptionFloat( OPTS_SIZE, pBrush->size_ );

	// Brush strength:
	strengthSlider_.setValue( pBrush->strength_ );
	strengthEdit_  .SetValue( pBrush->strength_ );	
    Options::setOptionFloat( OPTS_STRENGTH, pBrush->strength_ );

	// Brush opacity:
	opacitySlider_.setValue( pBrush->opacity_/2.55f );
	opacityEdit_  .SetValue( pBrush->opacity_/2.55f );

	// Maximum layers and limit layers:
	maxLayersEdit_.SetIntegerValue( pBrush->maxLayers_ );
	controls::checkButton(
		*this, 
		IDC_PAGE_TERRAIN_LIMIT_LAYERS_CB,
		pBrush->maxLayerLimit_ );

	// Height mask:
    controls::checkButton(
        *this,
        IDC_PAGE_TERRAIN_HEIGHT_MASK_BUTTON,
        pBrush->heightMask_ );
	float heightFuz = 
		0.5f*((pBrush->h4_ - pBrush->h3_) + (pBrush->h2_ - pBrush->h1_));
    minHeightEdit_.SetValue( pBrush->h2_ );
    maxHeightEdit_.SetValue( pBrush->h3_ );
    fuzHeightEdit_.SetValue( heightFuz   );

	// Slope mask:
    controls::checkButton(
        *this,
        IDC_PAGE_TERRAIN_SLOPE_MASK_BUTTON,
        pBrush->slopeMask_ );
	float slopeFuz = 
		0.5f*((pBrush->s4_ - pBrush->s3_) + (pBrush->s2_ - pBrush->s1_));
    minSlopeEdit_.SetValue( pBrush->s2_ );
    maxSlopeEdit_.SetValue( pBrush->s3_ );
    fuzSlopeEdit_.SetValue( slopeFuz );

	// Import mask:
	float impStrength = pBrush->importMaskMul_*65535.0f*100.0f; // convert to 16 bit range in %
	maskStrengthEdit_  .SetValue( impStrength );
	maskStrengthSlider_.setValue( impStrength );
	controls::checkButton(
		*this,
		IDC_PAGE_TERRAIN_OVERLAY_MASK_BUTTON,
		pBrush->importMask_	);

	// Mask texture:
	this->setTextureMaskTexture(
		pBrush->textureMaskTexture_, 
		pBrush->textureMaskUProj_, pBrush->textureMaskVProj_,
		pBrush->textureMask_,
		pBrush->textureMaskIncludeProj_,
		pBrush->textureMaskInvert_,
		true );

	// Noise mask:
	controls::checkButton(
		*this, IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON, pBrush->noiseMask_ );

	pBrush_ = pBrush;

	--layerUndoAdded_;
	--filter_;

	// Disable/enable the mask controls as appropriate and make sure that the
	// mask overlay is consistent with what is being shown.
	this->onEnableMaskControls();
	this->OnMaskOverlayButton();
}


/**
 *	This gets called when the matrix proxy is updated. It makes sure
 *	that the UV projection scales are equal if they are meant to be locked.
 *
 *	@param m				The matrix that needs its scale locked.
 *	@param initialM			The initial state of the matrix. Used to see 
 *							whether	the x or z directions were scaled.
 */
void PageTerrainTexture::lockProjectionScale( 
	Matrix &			m, 
	const Matrix &		initialM )
{
	if (!pBrush_->uvLocked_)
	{
		return;
	}

	// Remove rotation of the given matrix
	Matrix proj1 = m;
	Matrix rot1;
	rot1.setRotate( proj1.yaw(), proj1.pitch(), proj1.roll() );
	Matrix rotInv1 = rot1;
	rotInv1.invert();
	proj1.postMultiply( rotInv1 );

	// Remove rotation of the recorded matrix
	Matrix proj2 = initialM;
	Matrix rot2;
	rot2.setRotate( proj2.yaw(), proj2.pitch(), proj2.roll() );
	Matrix rotInv2 = rot2;
	rotInv2.invert();
	proj2.postMultiply( rotInv2 );

	// Equalise X-Z scales
	float x1 = proj1[0][0];
    float z1 = proj1[2][2];

	float x2 = proj2[0][0];
    float z2 = proj2[2][2];

	if (!almostEqual( x1, x2 ) && !almostEqual( z1, z2 ))
	{
		x1 = (x1 + z1)/2;
		z1 = x1;
	}
	else if (!almostEqual( x1, x2 ))
	{
		z1 = x1;
	}
	else if (!almostEqual( z1, z2 ))
	{
		x1 = z1;	
	}

	proj1[0][0] = x1;
	proj1[2][2] = z1;

	// Restore the rotation
	proj1.postMultiply( rot1 );

	m = proj1;
}

/**
 *	This function gets the texture projection as a Matrix.
 *
 *  @returns			The texture projection as a Matrix.  The w-axis is
 *						the cross product of u and v.
 */
Matrix PageTerrainTexture::textureProjection() const
{
	Matrix m = invTexProjection_;
	m.translation( invTexProjTrans_ );
	m.invert();
	return m;
}


/**
 *	This function sets the texture projections via a Matrix.
 *
 *  @param m			The new texture projection matrix.
 *	@param temporary	If true then this the new matrix is only temporary
 *						(e.g. during a mouse-down event).
 */
void PageTerrainTexture::textureProjection(
	const Matrix &		m, 
	bool				temporary /*=false*/ )
{
	invTexProjection_ = m;	
	invTexProjection_.invert();	
	invTexProjTrans_ = invTexProjection_.applyToOrigin();

	this->updateEditsFromProjection();
	this->updatePython();
	this->updateEditedLayers( temporary );
}


/**
 *	This function gets the texture projections as a Matrix.
 *
 *  @returns			The texture projection as a Matrix.  The w-axis is
 *						the cross product of u and v.
 */
Matrix PageTerrainTexture::textureProjectionInverse() const
{
	Matrix result = invTexProjection_;
	result.translation( invTexProjTrans_ );
	return result;
}


/**
 *	This function sets the texture projections via a Matrix.
 *
 *  @param m			The new texture projection matrix.
 *	@param temporary	If true then this the new matrix is only temporary
 *						(e.g. during a mouse-down event).
 */
void PageTerrainTexture::textureProjectionInverse
(
	Matrix				const &m, 
	bool				temporary /*=false*/
)
{
	invTexProjection_ = m;
	invTexProjTrans_  = invTexProjection_.applyToOrigin();

	this->updateEditsFromProjection();
	this->updatePython();
	this->updateEditedLayers( temporary );
}


/**
 *	This function gets the texture being used to paint the terrain.
 *
 *  @returns			The texture being used to paint the terrain.
 */
Moo::BaseTexturePtr PageTerrainTexture::texture() const
{
	Moo::BaseTexturePtr texture = 
		Moo::TextureManager::instance()->get( currentTextureFilename_ );
	return texture;
}


/**
 *	This returns whether a texture has been selected yet.
 *
 *	@returns			True if a texture has been selected.
 */
bool PageTerrainTexture::hasTexture() const
{
	return !currentTextureImage_.image().isEmpty();
}


/**
 *	This gets the current mode that the user is in.
 *
 *	@returns		The current editing mode.
 */
PageTerrainTexture::Mode PageTerrainTexture::mode() const
{
	return mode_;
}


/**
 *	This is called just before editing of layers begins.  It provides an
 *	oportunity for undo/redo of the currently edited layer.
 */
void PageTerrainTexture::beginEditLayers()
{
	if (mode_ == EDITING && !editedLayer_.empty() && !layerUndoAdded_)
	{
		for (
			TerrainTextureUtils::LayerSetIter it = editedLayer_.begin();
			it != editedLayer_.end();
			++it)
		{
			TerrainTexProjUndo * pUndoOp =
				new TerrainTexProjUndo( it->terrain_, it->layerIdx_ );
			UndoRedo::instance().add( pUndoOp );
		}
	}
	++layerUndoAdded_;
}


/**
 *	This is called just after editing of layers ends.  It provides an 
 *	opportunity to stick in an undo/redo barrier.
 */
void PageTerrainTexture::endEditLayers()
{
	--layerUndoAdded_;
	if (mode_ == EDITING && !editedLayer_.empty() && !layerUndoAdded_)
	{
		UndoRedo::instance().barrier( "Edit texture projection", true );
	}
}


/**
 *	This gets the tool's current position.
 *
 *  @returns			The tool's current position.
 */
Vector3 PageTerrainTexture::toolPos() const
{
	if ( 
	    ToolManager::instance().tool() 
        && 
        ToolManager::instance().tool()->locator() )
	{
		return
			ToolManager::instance().tool()->locator()->transform().applyToOrigin();
	}
	else
	{
		return Vector3::zero();
	}
}


/**
 *	This gets the terrain texture layers at the given point.
 *
 *	@param point		The point to get the layers at.
 *	@param layers		This is filled in with the layer information.
 */
void PageTerrainTexture::layersAtPoint(
	const Vector3 &					point,
	TerrainTextureUtils::LayerVec &	layers )
{
	TerrainTextureUtils::layerInfo( point, layers );
}


/**
 *	This sets editing mode and selects the given layer as the edited layer.
 *
 *	@param layer	The layer to edit.
 *	@param pPos		If not NULL then the gizmo is placed at this location.  If
 *					it is NULL then the gizmo is placed at the center of the
 *					layer's chunk.
 *  @returns		True if the layer could be edited.
 */
bool PageTerrainTexture::editTextureLayer(
	const TerrainTextureUtils::LayerInfo &	layer,
	const Vector3 *							pPos		/*=NULL*/ )
{
	// Enter editing mode:
	this->mode( EDITING );

	// Get the layer to edit:
	editedLayer_.clear();
	TerrainTextureUtils::FindError error = 
		TerrainTextureUtils::findNeighboursWithTexture(
			layer, 
			editedLayer_ );
	TerrainTextureUtils::printErrorMessage( error );
	seedTerrain_ = layer;

	// Set the texture and projections:
	Terrain::EditorBaseTerrainBlock &block = layer.terrain_->block();
	Terrain::TerrainTextureLayer & texLayer = 
		block.textureLayer( layer.layerIdx_ );
	std::string textureName = texLayer.textureName();
	Vector4 u( 1.0f, 0.0f, 0.0f, 0.0f );
	Vector4 v( 0.0f, 0.0f, 1.0f, 0.0f );
	if (texLayer.hasUVProjections())
	{
		u = texLayer.uProjection();
		v = texLayer.vProjection();
	}
	this->currentTexture( textureName, u, v );

	// Find the position 
	Vector3 middleLocal = 
		Vector3( 0.5f*GRID_RESOLUTION, 0.0f, 0.5f*GRID_RESOLUTION );
	middleLocal.y =
		layer.terrain_->block().heightAt( middleLocal.x, middleLocal.z );
	Vector3 middleWorld = 
		layer.terrain_->chunk()->transform().applyPoint( middleLocal );
	Vector3 gizmoPos = (pPos != NULL) ? *pPos : middleWorld;

	// Update the gizmo's position:
	invTexProjection_.translation( gizmoPos );
	if (pMatrixProxy_ != NULL)
	{
		pMatrixProxy_->setMatrixAlone( invTexProjection_ );
	}

	return true;
}


/**
 *	This returns true if editing texture projections is possible.
 *
 *	@returns			True if editing texture projections is possible.
 */
bool PageTerrainTexture::canEditProjections() const
{
	return hasUVProjections_;
}


/**
 *	This edits the texture projection of the idx'th layer under the cursor.
 *
 *	@param point		The point to start editing texture layers.
 *	@param idx			The index of the layer to edit.  If this is 
 *						EditorChunkTerrain::NO_DOMINANT_BLEND then the 
 *						dominant texture is edited.
 */
void PageTerrainTexture::editProjectionAtPoint(
	const Vector3 &		point,
	size_t				idx )
{
	size_t					domIdx;
	EditorChunkTerrain	*	pEditorChunkTerrain	= NULL;
	Chunk				*	pChunk				= NULL;	

	this->mode( EDITING );

	if (TerrainTextureUtils::dominantTexture(
		point, &domIdx, &pEditorChunkTerrain, &pChunk ))
	{
		if (idx == EditorChunkTerrain::NO_DOMINANT_BLEND) 
		{
			idx = domIdx;
		}
		this->onSampleTexture( false, idx, &point ); // Select the texture for editing
		this->destroyEditors();
	}
}


/**
 *	This selects a texture under the given point as the new painting texture.
 *
 *	@param point		The point to get texture layers.
 *	@param idx			The index of the layer to edit.  If this is 
 *						EditorChunkTerrain::NO_DOMINANT_BLEND then the dominant 
 *						texture is selected.
 */
void PageTerrainTexture::selectTextureAtPoint(
	const Vector3 &		point, 
	size_t				idx )
{
	size_t					domIdx;
	EditorChunkTerrain	*	pEditorChunkTerrain	= NULL;
	Chunk				*	pChunk				= NULL;	

	this->mode(PAINTING);

	if (TerrainTextureUtils::dominantTexture(
		point, &domIdx, &pEditorChunkTerrain, &pChunk ))
	{
		if (idx == EditorChunkTerrain::NO_DOMINANT_BLEND) 
		{
			idx = domIdx;
		}
		this->onSampleTexture( false, idx, &point ); // Select the texture for editing
		editedLayer_.clear();
	}	
}


/**
 *	This function sets the opacity to match the given layer's strength.
 *
 *	@param percent		The opacity to use.
 */
void PageTerrainTexture::setOpacity( float percent )
{
	++filter_;
	++layerUndoAdded_;
	opacityEdit_.SetValue( percent );
	opacitySlider_.setValue( percent );
	pBrush_->opacity_ = (uint8)(Math::clamp( 0.0f, 2.55f*percent, 255.0f ));
	this->updatePython();

	--layerUndoAdded_;
	--filter_;
}


/**
 *	This selects a texture for the texture mask.
 *
 *	@param point		The point to sample the texture.
 *	@param idx			The index of the texture to get.  If this is set to
 *						EditorChunkTerrain::NO_DOMINANT_BLEND then the dominant 
 *						texture is selected.
 *	@param includeProj	Include the texture projection as part of the mask?
 */
void PageTerrainTexture::selectTextureMaskAtPoint(
	const Vector3		& point,
	size_t				idx,
	bool				includeProj )
{
	size_t					domIdx;
	EditorChunkTerrain	*	pEditorChunkTerrain	= NULL;
	Chunk				*	pChunk				= NULL;	

	this->mode( PAINTING );

	if (TerrainTextureUtils::dominantTexture(
		point, &domIdx, &pEditorChunkTerrain, &pChunk ))
	{
		if (idx == EditorChunkTerrain::NO_DOMINANT_BLEND) 
		{
			idx = domIdx;
		}
		const Terrain::TerrainTextureLayer & layer = 
			pEditorChunkTerrain->block().textureLayer( idx );
		std::string textureFile = layer.textureName();
		Vector4 uProj = layer.uProjection();
		Vector4 vProj = layer.vProjection();
		this->setTextureMaskTexture(
			textureFile, uProj, vProj, true, includeProj, false, includeProj );
	}
}


/**
 *	This sets and enables the texture mask.
 *
 *	@param texture		The texture of the mask.
 *	@param includeProj	Should the projection be enabled?
 *	@param uproj		The u-projection.
 *	@param vproj		The v-projection.
 *	@param invert		If true then painting paints outside the texture,
 *						if false then painting paints over the texture.
 */
void PageTerrainTexture::selectTextureMask(
	const std::string &	texture, 
	bool				includeProj,
	const Vector4 &		uproj, 
	const Vector4 &		vproj,
	bool				invert		/*= false*/ )
{
	this->setTextureMaskTexture(
		texture, 
		uproj, vproj, 
		true, 
		includeProj, 
		invert, 
		includeProj );
}


/**
 *	This functions sets the index of the last painted layer.  Nothing is done 
 *	if not in PAINTING mode or the left-mouse button is not down.
 *
 *	@param idx			The index of the last painted layer.
 */
void PageTerrainTexture::paintLayer( int idx )
{
	if (
		mode_ == PAINTING 
		&& 
		InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE ) )
	{
		paintLayer_ = idx;
	}
}


/**
 *	This function sets the position of the last paint.  Nothing is done if not
 *	in PAINTING mode or the left-mouse button is not down.
 *
 *  @param pos			The position of the last paint.
 */
void PageTerrainTexture::paintPos( const Vector3 &pos )
{
	if (
		mode_ == PAINTING 
		&& 
		InputDevices::isKeyDown( KeyEvent::KEY_LEFTMOUSE ) )
	{
		paintPos_ = pos;
	}
}


/** 
 *	This is called by the tool if the user hit the ESC key.  We use this to 
 *	turn off editing projection mode.
 *
 *	@param hitEscKey	Non-zero if the user hit the ESC key.
 */
void PageTerrainTexture::onEscapeKey( int hitEscKey )
{
	if (mode_ == EDITING && hitEscKey != 0)
	{
		this->mode( PAINTING );
	}
}


/**
 *	This checks that the current terrain version is up to date.
 */
void PageTerrainTexture::checkTerrainVersion()
{
	hasUVProjections_ = 
		WorldManager::instance().pTerrainSettings()->uvProjections();
	this->updateProjectionControls();
	if (hasUVProjections_)
	{
		maxLayersEdit_.EnableWindow( TRUE );
	}
	else
	{
		maxLayersEdit_.EnableWindow( FALSE );
		maxLayersEdit_.SetIntegerValue( 4 );

		if (pBrush_)
		{
			pBrush_->maxLayerLimit_ = true;
			pBrush_->maxLayers_     = 4;
		}
	}

	if (inited_)
	{
		this->updatePythonMask();
	}
}


/**
 *	This checks that the edited layer's details still make sense.  They may
 *	not make sense if the user does some undoes of painting operations.  If 
 *	this happens then we clear the edited layer.
 */
void PageTerrainTexture::checkEditedLayer()
{
	for (
		TerrainTextureUtils::LayerSetIter it = editedLayer_.begin();
		it != editedLayer_.end();
		++it)
	{
		TerrainTextureUtils::LayerInfo & info = *it;

		bool ok = true;
		if (
			info.layerIdx_ >= info.terrain_->block().numberTextureLayers() )
		{ 
			ok = false;
		}

		if (ok)
		{
			Terrain::TerrainTextureLayer & textureLayer = 
				info.terrain_->block().textureLayer(info.layerIdx_);
			if (seedTerrain_.textureName_ != textureLayer.textureName())
			{
				ok = false;
			}
		}

		if (!ok)
		{
			editedLayer_.clear();
			break;
		}
	}
}


/**
 *	This makes sure that the gizmo is in a unusable state.
 *
 *  @param forceReposition	Force repositioning of the Gizmo even if in
 *							editing mode.
 *	@param pPos				The position to reposition to.  If this is NULL
 *							then the tool's position is used.
 */
void PageTerrainTexture::updateGizmo(
	bool			forceReposition,
	const Vector3 *	pPos	/*= NULL*/ )
{
	if (GeneralEditor::currentEditors().size() == 0)
	{
		this->buildEditors(); // lost editor because of undo
	}

	if ( 
	    ToolManager::instance().tool() 
        && 
        ToolManager::instance().tool()->locator() )
    {
		Vector3 toolpos = (pPos != NULL) ? *pPos : toolPos();
		if (pTileGizmo_ != NULL)
		{
			// If editing and we have selected a layer selected then we should
			// match the orientation of the gizmo with the selected layer. 
			// They can be out of sync if the user does an undo for example
			if (mode_ == EDITING && !editedLayer_.empty())
			{
				this->checkEditedLayer();
				if (!editedLayer_.empty())
				{
					const TerrainTextureUtils::LayerInfo & layerInfo = 
						*editedLayer_.begin();
					Terrain::TerrainTextureLayer & layer = 
						layerInfo.terrain_->block().textureLayer(
							layerInfo.layerIdx_ );
					if (layer.hasUVProjections())
					{
						Vector3 trans = invTexProjection_.applyToOrigin();
						Vector4 uproj = layer.uProjection();
						Vector4 vproj = layer.vProjection();
						invTexProjection_ =	buildMatrix( uproj, vproj );					
						invTexProjection_.invert();
						invTexProjection_.translation( trans );
						if (pMatrixProxy_ != NULL)
						{
							pMatrixProxy_->setMatrixAlone( invTexProjection_ );
						}
						float sz = ToolManager::instance().tool()->size();
						this->updateEditsFromProjection();
					}
				}
				else
				{
					editProjButton_.toggle( false );
					this->mode( PAINTING );
				}
			}

			// Update the Gizmo position:
			if (!isEditingGizmo() && (mode_ != EDITING) || forceReposition)
			{
				invTexProjection_.translation( toolpos );
				if (pMatrixProxy_ != NULL)
				{
					pMatrixProxy_->setMatrixAlone( invTexProjection_ );
				}
			} 
		}
   }
}


/**
 *	This puts the gizmo at the middle of the screen.
 */
void PageTerrainTexture::translateGizmoToMiddle()
{
	Vector3 worldRay = middleOfScreen();
	ToolPtr tool = ToolManager::instance().tool();
	if (tool)
	{
		tool->calculatePosition( worldRay );
	}
	this->updateGizmo( true );
}


/**
 *  This is called to synchronize the edit controls with their sliders.
 */
void PageTerrainTexture::updateSliderEdits()
{
	if (filter_ == 0)
	{
		++filter_;

		++layerUndoAdded_;

		float newVal, oldVal;

		newVal = sizeSlider_.getValue();
		oldVal = sizeEdit_.GetValue();
		if (
			(GetFocus() == &sizeSlider_) 
			|| 
			(GetFocus() != &sizeEdit_ && newVal != oldVal) )
		{
			sizeEdit_.SetValue( newVal );
			Options::setOptionFloat( OPTS_SIZE, newVal );
			gizmoDrawState_ = 0;
		}

		newVal = strengthSlider_.getValue();
		oldVal = strengthEdit_.GetValue();
		if ( 
			(GetFocus() == &strengthSlider_) 
			|| 
			(GetFocus() != &strengthEdit_ && newVal != oldVal) )
		{
			strengthEdit_.SetValue( newVal );
			Options::setOptionFloat( OPTS_STRENGTH, newVal );
			gizmoDrawState_ = 0;
		}

		newVal = opacitySlider_.getValue();
		oldVal = opacityEdit_.GetValue();
		if (
			(GetFocus() == &opacitySlider_) 
			|| 
			(GetFocus() != &opacityEdit_ && newVal != oldVal) )
		{
			opacityEdit_.SetValue( newVal );
			gizmoDrawState_ = TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE;
			pBrush_->opacity_ = 
				(uint8)( Math::clamp( 0.0f, newVal*2.55f, 255.0f ) );
		}

		newVal = yawAngleSlider_.getValue();
		oldVal = yawAngleEdit_.GetValue();
		if ( 
			(GetFocus() == &yawAngleSlider_) 
			||
			(GetFocus() != &yawAngleEdit_ && newVal != oldVal) )
		{
			yawAngleEdit_.SetValue( newVal );
			gizmoDrawState_ = 
				TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE | TileGizmo::DRAW_ROTATION;
		}

		newVal = pitchAngleSlider_.getValue();
		oldVal = pitchAngleEdit_.GetValue();
		if ( 
			(GetFocus() == &pitchAngleSlider_) 
			|| 
			(GetFocus() != &pitchAngleEdit_ && newVal != oldVal) )
		{
			pitchAngleEdit_.SetValue( newVal );
			gizmoDrawState_ = 
				TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE | TileGizmo::DRAW_ROTATION;
		}

		newVal = rollAngleSlider_.getValue();
		oldVal = rollAngleEdit_.GetValue();
		if ( 
			(GetFocus() == &rollAngleSlider_) 
			|| 
			(GetFocus() != &rollAngleEdit_ && newVal != oldVal) )
		{
			rollAngleEdit_.SetValue( newVal );
			gizmoDrawState_ = 
				TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE | TileGizmo::DRAW_ROTATION;
		}

		int uvLocked = pBrush_->uvLocked_ ? 1 : 0;

		// we need this as the slider's rounded float is not very accurate
		newVal = uSizeEdit_.GetRoundedNumber( uSizeSlider_.getValue() );
		oldVal = uSizeEdit_.GetValue();
		
		if ((GetFocus() == &uSizeSlider_ || GetFocus() != &uSizeEdit_) && newVal != oldVal)
		{
			uSizeEdit_.SetValue(newVal);
			if (uvLocked)
			{
				vSizeEdit_.SetValue( newVal );
				vSizeSlider_.setValue( newVal );
			}
			gizmoDrawState_ = 
				TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE | TileGizmo::DRAW_SCALE;
		}

		// we need this as the slider's rounded float is not very accurate
		newVal = vSizeEdit_.GetRoundedNumber( vSizeSlider_.getValue() );
		oldVal = vSizeEdit_.GetValue();
		
		if ((GetFocus() == &vSizeSlider_ || GetFocus() != &vSizeEdit_) && newVal != oldVal)
		{
			vSizeEdit_.SetValue( newVal );
			if (uvLocked)
			{
				uSizeEdit_.SetValue( newVal );
				uSizeSlider_.setValue( newVal );
			}
			gizmoDrawState_ = 
				TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE | TileGizmo::DRAW_SCALE;
		}

		newVal = maskStrengthSlider_.getValue();
		oldVal = maskStrengthEdit_.GetValue();
		if ( 
			(GetFocus() == &maskStrengthSlider_) 
			|| 
			(GetFocus() != &maskStrengthEdit_ && newVal != oldVal) )
		{
			maskStrengthEdit_.SetValue( newVal );
		}

		--layerUndoAdded_;
		--filter_;
	}
}


/**
 *	This is called to enable/disable the projection controls because the
 *	terrain may have changed and terrain texture projections may or may
 *	not make sense anymore.
 */
void PageTerrainTexture::updateProjectionControls()
{
	// enable/disable edit boxes
	BOOL enableUV = hasUVProjections_ ? TRUE : FALSE;
	yawAngleEdit_    .EnableWindow( enableUV );
	yawAngleSlider_  .EnableWindow( enableUV );
	pitchAngleEdit_  .EnableWindow( enableUV );
	pitchAngleSlider_.EnableWindow( enableUV );
	rollAngleEdit_   .EnableWindow( enableUV );
	rollAngleSlider_ .EnableWindow( enableUV );
	yawAngleEdit_    .EnableWindow( enableUV );
	yawAngleSlider_  .EnableWindow( enableUV );
	uSizeEdit_       .EnableWindow( enableUV );
	uSizeSlider_     .EnableWindow( enableUV );
	vSizeEdit_       .EnableWindow( enableUV );
	vSizeSlider_     .EnableWindow( enableUV );
	linkUVScale_	 .EnableWindow( enableUV );
	controls::enableWindow(
		*this, IDC_PAGE_TERRAIN_RESET_PROJ_BUTTON, hasUVProjections_ );
	editProjButton_.EnableWindow( hasUVProjections_ ? TRUE : FALSE );

	if (!hasUVProjections_)
	{
		Vector4 uProj, vProj;
		Terrain::TerrainTextureLayer::defaultUVProjections( uProj, vProj );
		float defaultULen = 1.0f/uProj.length();
		float defaultVLen = 1.0f/vProj.length();

		// if not enabled, set to the default values to avoid showing
		// meaningless stuff in the disabled controls
		yawAngleEdit_    .SetValue( 0.0f );
		yawAngleSlider_  .setValue( 0.0f );
		pitchAngleEdit_  .SetValue( 0.0f );
		pitchAngleSlider_.setValue( 0.0f );
		rollAngleEdit_   .SetValue( 0.0f );
		rollAngleSlider_ .setValue( 0.0f );
		uSizeEdit_       .SetValue( defaultULen );
		uSizeSlider_     .setValue( defaultULen );
		vSizeEdit_       .SetValue( defaultVLen );
		vSizeSlider_     .setValue( defaultVLen );
		this->updateProjection();

		// destroyEditors, so next time the terrain texture tool is the active
		// one, buildEditors is called. buildEditors will check the terrain
		// version and will avoid creating the editors if version is 100.
		this->destroyEditors();
	}
}


/**
 *	This is called when the user clicks the editing button.
 */
void PageTerrainTexture::OnEditingButton()
{
	bool isEditing = editProjButton_.isToggled();
	if (isEditing)
	{
		this->mode( EDITING );

		bool editingLastPaint = false;

		if (paintLayer_ != EditorChunkTerrain::NO_DOMINANT_BLEND)
		{
			EditorChunkTerrainPtr terrain = 
				EditorChunkTerrainCache::instance().findChunkFromPoint( paintPos_ );
			
			if (terrain != NULL)
			{
				if (paintLayer_ < (int)terrain->block().numberTextureLayers())
				{
					const Terrain::TerrainTextureLayer & textureLayer = 
						terrain->block().textureLayer( paintLayer_ );
					TerrainTextureUtils::LayerInfo layerInfo;
					layerInfo.layerIdx_    = paintLayer_;
					layerInfo.terrain_     = terrain;
					layerInfo.textureName_ = textureLayer.textureName();
					editingLastPaint = 
						this->editTextureLayer( layerInfo, &paintPos_ );
				}
			}
		}
		
		if (!editingLastPaint)
		{
			this->translateGizmoToMiddle();
		}
	}
	else
	{
		this->mode( PAINTING );
	}
	this->updatePython();
}


/**
 *	This is called when the user pressed the button to save the brush to a 
 *	file.
 */
void PageTerrainTexture::OnSaveBrushButton()
{
	static char brushFilter[] = 
        "Brushes (*.brush)|*.brush|"
        "All Files (*.*)|*.*||";

	std::string suggestedName = pBrush_->suggestedFilename();
	BWFileDialog saveBrushDlg(
		FALSE, "brush", suggestedName.c_str(), OFN_OVERWRITEPROMPT, 
		brushFilter );
	if (saveBrushDlg.DoModal() == IDOK)
	{
		std::string newFilename = saveBrushDlg.GetPathName().GetBuffer();
        StringUtils::replace( newFilename, '\\', '/' );
        std::string dissolvedFilename = 
            BWResource::dissolveFilename( newFilename );

        // The saved to file must be in the resource paths:
        if (strcmpi( dissolvedFilename.c_str(), newFilename.c_str() ) == 0)
        {
			std::string msg = L("RCST_IDS_NOTRESPATH");
            AfxMessageBox(msg.c_str());
        }
		else
		{
			DataSectionPtr pBrushDataSection = 
				BWResource::openSection( dissolvedFilename, true );
			pBrushDataSection->delChildren();
			pBrush_->save( pBrushDataSection );
			pBrushDataSection->save();
			UalManager::instance().refreshAllDialogs();
		}
	}
}


/**
 *  This is called when the size edit control is changed.
 */
void PageTerrainTexture::OnEnChangeSizeEdit()
{
	// This can be called very early on, before the page has been initialised.
	// This occurs when the page is created and it gets focus.
	if (!inited_)
	{
		return;
	}

	if (filter_ == 0)
	{
		++filter_;
		float size = sizeEdit_.GetValue();
		sizeSlider_.setValue( size );
		pBrush_->size_ = size;
		Options::setOptionFloat( OPTS_SIZE, size );
		--filter_;
	}
}


/**
 *  This is called when the strength edit control is changed.
 */
void PageTerrainTexture::OnEnChangeStrengthEdit()
{
	// This can be called very early on, before the page has been initialised.
	// This occurs when the page is created and it gets focus.
	if (!inited_)
	{
		return;
	}

	if (filter_ == 0)
	{
		++filter_;
		float strength = strengthEdit_.GetValue();
		strengthSlider_.setValue( strength );
		pBrush_->strength_ = strength;
		Options::setOptionFloat( OPTS_STRENGTH, strength );
		--filter_;
	}
}


/**
 *	This is called when the user edits the opacity.
 */
void PageTerrainTexture::OnEnChangeOpacityEdit()
{
	if (filter_ == 0)
	{
		++filter_;
		opacitySlider_.setValue( opacityEdit_.GetValue() );
		pBrush_->opacity_ = 
			(uint8)(
				Math::clamp( 0.0f, 2.55f*opacitySlider_.getValue(), 255.0f) );
		--filter_;
	}
}


/**
 *  This is called when the size button is pressed.
 */
void PageTerrainTexture::OnBnClickedSizeButton()
{
    sizeSlider_.beginEdit();
}


/**
 *	This is called when the user clicks the button to limit the number of
 *	layers when painting.
 */
void PageTerrainTexture::OnMaxLayersCB()
{
	if (filter_ == 0)
	{
		++filter_;
		bool isChecked = 
			controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_LIMIT_LAYERS_CB );
		if (hasUVProjections_)
		{
			maxLayersEdit_.EnableWindow( isChecked ? TRUE : FALSE );
		}
		pBrush_->maxLayerLimit_ = isChecked;
		pBrush_->maxLayers_     = maxLayersEdit_.GetIntegerValue();
		this->updatePythonMask();
		--filter_;
	}
}


/**
 *	This is called when the user changes the number of maximum layers.
 */
void PageTerrainTexture::OnMaxLayersEdit()
{
	this->OnMaxLayersCB();
}


/**
 *  This is called when the height button is toggled.
 */
void PageTerrainTexture::OnHeightButton()
{
	this->onEnableMaskControls();
}


/**
 *  This is called when the minimum height mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeMinHeightEdit()
{
	if (filter_ == 0)
	{
		++filter_;
		pBrush_->h1_ = minHeightEdit_.GetValue() - fuzHeightEdit_.GetValue();
		pBrush_->h2_ = minHeightEdit_.GetValue();
		pBrush_->h3_ = maxHeightEdit_.GetValue();
		pBrush_->h4_ = maxHeightEdit_.GetValue() + fuzHeightEdit_.GetValue();
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *  This is called when the maximum height mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeMaxHeightEdit()
{
	this->OnEnChangeMinHeightEdit();
}


/**
 *  This is called when the maximum height mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeFuzHeightEdit()
{
	this->OnEnChangeMinHeightEdit();
}


/**
 *  This is called when the slope button is toggled.
 */
void PageTerrainTexture::OnSlopeButton()
{
	this->onEnableMaskControls();
}


/**
 *  This is called when the minimum slope mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeMinSlopeEdit()
{
	if (filter_ == 0)
	{
		++filter_;
		pBrush_->s1_ = minSlopeEdit_.GetValue() - fuzSlopeEdit_.GetValue();
		pBrush_->s2_ = minSlopeEdit_.GetValue();
		pBrush_->s3_ = maxSlopeEdit_.GetValue();
		pBrush_->s4_ = maxSlopeEdit_.GetValue() + fuzSlopeEdit_.GetValue();
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *  This is called when the maximum slope mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeMaxSlopeEdit()
{
	this->OnEnChangeMinSlopeEdit();
}


/**
 *  This is called when the fuzziness of the slope mask edit is changed.
 */
void PageTerrainTexture::OnEnChangeFuzSlopeEdit()
{
	this->OnEnChangeMinSlopeEdit();
}


/**
 *	This is called when the mask texture button is pressed.
 */
void PageTerrainTexture::OnTexMaskButton()
{
	this->onEnableMaskControls();

	if (filter_ == 0)
	{
		++filter_;		
		bool isChecked =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON );
		pBrush_->textureMask_ = isChecked;
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *	This is called when the "include projection" button is pressed.
 */
void PageTerrainTexture::OnTexMaskIncludeProjButton()
{
	if (filter_ == 0)
	{
		++filter_;
		bool isChecked =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON );
		pBrush_->textureMaskIncludeProj_ = isChecked;
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *	This is called when the user inverts the texture mask.
 */
void PageTerrainTexture::OnTexMaskInvButton()
{
	if (filter_ == 0)
	{
		++filter_;
		bool isChecked =
			controls::isButtonChecked(
				*this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON );
		pBrush_->textureMaskInvert_ = isChecked;	
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *	This is called when the noise texture button is pressed.
 */
void PageTerrainTexture::OnNoiseMaskButton()
{
	this->onEnableMaskControls();

	if (filter_ == 0)
	{
		++filter_;		
		bool isChecked =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON );
		pBrush_->noiseMask_ = isChecked;
		if (isChecked && pBrush_->noise_.octaves().empty())
		{
			NoiseSetupDlg::defaultNoise( pBrush_->noise_ );
		}
        this->updatePythonMask();
		--filter_;
	}
}


/**
 *	This is called when the noise texture setup button is pressed.
 */
void PageTerrainTexture::OnNoiseSetupButton()
{
    NoiseSetupDlg noiseSetupDlg;
    noiseSetupDlg.simplexNoise( pBrush_->noise_ );
    noiseSetupDlg.octaves( pBrush_->noise_.octaves() );
	noiseSetupDlg.minSaturate( pBrush_->noiseMinSat_ );
	noiseSetupDlg.maxSaturate( pBrush_->noiseMaxSat_ );
	noiseSetupDlg.minStrength( pBrush_->noiseMinStrength_ );
	noiseSetupDlg.maxStrength( pBrush_->noiseMaxStrength_ );
	
	// If the brush has no noise then setup some default noise:
	if (pBrush_->noise_.octaves().size() == 0)
	{
		NoiseSetupDlg::defaultNoise( pBrush_->noise_ );
	}
	
    if (noiseSetupDlg.DoModal() == IDOK)
    {
        pBrush_->noise_            = noiseSetupDlg.simplexNoise();
        pBrush_->noise_.octaves( noiseSetupDlg.octaves() );
		pBrush_->noiseMinSat_      = noiseSetupDlg.minSaturate();
		pBrush_->noiseMaxSat_      = noiseSetupDlg.maxSaturate();
		pBrush_->noiseMinStrength_ = noiseSetupDlg.minStrength();
		pBrush_->noiseMaxStrength_ = noiseSetupDlg.maxStrength();
		this->updatePython();
        this->updatePythonMask();
    }
}


/**
 *	This is called when the user clicks on the import mask button.
 */
void PageTerrainTexture::OnMaskImportButton()
{
	if (isAdjustingMask_)
	{
		this->endAdjustMask();
	}
	this->onEnableMaskControls();
}


/**
 *	This is called when the user clicks on the mask browse button.
 */
void PageTerrainTexture::OnMaskBrowseButton()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule == NULL)
	{
        return;
	}

	static char szFilter[] = 
        "Greyscale Heightmaps (*.r16;*.raw;*.bmp)|*.r16;*.raw;*.bmp|"
        "Terragen Files (*.ter)|*.ter|"
        "DTED2 Files (*.dt2)|*.dt2|"
        "All Files (*.*)|*.*||";

	BWFileDialog openFile( TRUE, "r16", NULL, OFN_FILEMUSTEXIST, szFilter );

	if (openFile.DoModal() != IDOK)
	{
		return;
	}

	CString filename = openFile.GetPathName();
    ImportImagePtr image = new ImportImage();
    float left     = std::numeric_limits< float >::max();
    float top      = std::numeric_limits< float >::max();
    float right    = std::numeric_limits< float >::max();
    float bottom   = std::numeric_limits< float >::max();
    ImportCodec::LoadResult loadResult = 
        image->load( 
			filename.GetBuffer(), &left, &top, &right, &bottom, NULL, true );
    switch (loadResult)
    {
    case ImportCodec::LR_OK:
        {
		image->setScale( 0.0f, 1.0f ); // Use image on an absolute scale
        pHeightModule->importData(
            image,
            left   != std::numeric_limits< float >::max() ? &left   : NULL,
            top    != std::numeric_limits< float >::max() ? &top    : NULL,
            right  != std::numeric_limits< float >::max() ? &right  : NULL,
            bottom != std::numeric_limits< float >::max() ? &bottom : NULL );
		pHeightModule->textureImportStrength( 
			maskStrengthSlider_.getValue()/100.0f );
		this->buildMaskImage();
		this->onEnableMaskControls();		
        }
        break;
    case ImportCodec::LR_BAD_FORMAT:
		{
		std::string msg = L( "RCST_IDS_FAILED_IMPORT_TERRAIN" );
        AfxMessageBox( msg.c_str(), MB_OK );
        pHeightModule->importData( NULL );
		}
        break;
    case ImportCodec::LR_CANCELLED:            
        pHeightModule->importData( NULL );
        break;
    case ImportCodec::LR_UNKNOWN_CODEC:
		{
		std::string msg = L( "RCST_IDS_BADCODEC_IMPORT_TERRAIN" );
        AfxMessageBox( msg.c_str(), MB_OK );
        pHeightModule->importData( NULL );
		}
        break;
    }
}


/**
 *	This is called when the user clicks on the import mask adjust button.
 */
void PageTerrainTexture::OnMaskAdjustButton()
{
	if (filter_ == 0)
	{
		++filter_;
		if (controls::isButtonChecked( 
			*this, IDC_PAGE_TERRAIN_ADJUST_MASK_BUTTON ))
		{
			this->beginAdjustMask();
		}
		else
		{
			this->endAdjustMask();
		}
		--filter_;
	}
}


/**
 *	This is called when the user edits the import mask strength.
 */
void PageTerrainTexture::OnMaskStrengthEdit()
{
	if (filter_ == 0)
	{
		++filter_;
		float oldValue = pBrush_->importMaskMul_*65535.0f*100.0f;
		float value    = maskStrengthEdit_.GetValue();
		if (!almostEqual( oldValue, value, EDITING_EPSILON )) 
		{
			maskStrengthSlider_.setValue( value );
			pBrush_->importMaskMul_ = (value/65535.0f)/100.0f;
			HeightModule * pHeightModule = HeightModule::currentInstance();
			if (pHeightModule != NULL)
			{
				pHeightModule->textureImportStrength(
					maskStrengthSlider_.getValue()/100.0f );
			}
            this->updatePythonMask();
		}
		--filter_;
	}
}


/**
 *	This is called when the user rotates the imported mask 
 *	anti-clockwise.
 */
void PageTerrainTexture::OnAnticlockwiseBtn()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule != NULL)
    {
        pHeightModule->rotateImportData( false ); // false = anticlockwise
		this->buildMaskImage();
    }
}


/**
 *	This is called when the user rotates the imported mask clockwise.
 */
void PageTerrainTexture::OnClockwiseBtn()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule != NULL)
    {
        pHeightModule->rotateImportData( true ); // true = clockwise
		this->buildMaskImage();
    }
}


/**
 *	This is called when the user flips the imported mask about the 
 *	x-axis.
 */
void PageTerrainTexture::OnFlipXBtn()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule != NULL)
    {
        pHeightModule->flipImportData( HeightModule::FLIP_X );
		this->buildMaskImage();
    }
}


/**
 *	This is called when the user flips the imported mask about the 
 *	y-axis.
 */
void PageTerrainTexture::OnFlipYBtn()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule != NULL)
    {
        pHeightModule->flipImportData( HeightModule::FLIP_Y );
		this->buildMaskImage();
    }
}


/**
 *	This is called when the user flips the import mask.
 */
void PageTerrainTexture::OnFlipHeightBtn()
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
    if (pHeightModule != NULL)
    {
        pHeightModule->flipImportData( HeightModule::FLIP_HEIGHT );
		this->buildMaskImage();
    }
}


/**
 *	This is called to determine whether the anti-clockwise button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainTexture::OnAnticlockwiseBtnEnable( CCmdUI * cmdui )
{
    HeightModule * pHeightModule = HeightModule::currentInstance();
	BOOL enabled = TRUE;
    if (pHeightModule == NULL)
	{
        enabled = FALSE;
	}
	if (enabled != FALSE)
	{
		enabled = (pHeightModule->hasImportData() ? TRUE : FALSE);
	}
    cmdui->Enable( enabled );
}


/**
 *	This is called to determine whether the clockwise button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainTexture::OnClockwiseBtnEnable( CCmdUI * cmdui )
{
    this->OnAnticlockwiseBtnEnable( cmdui ); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-x button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainTexture::OnFlipXBtnEnable( CCmdUI * cmdui )
{
    this->OnAnticlockwiseBtnEnable( cmdui); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-y button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainTexture::OnFlipYBtnEnable( CCmdUI * cmdui )
{
    this->OnAnticlockwiseBtnEnable( cmdui ); // these are enabled/disabled together
}


/**
 *	This is called to determine whether the flip-height button should be
 *	enabled.
 *
 *  @param cmdui		The CCmdUI to enable/disable the button.
 */
void PageTerrainTexture::OnFlipHeightBtnEnable( CCmdUI * cmdui )
{
    this->OnAnticlockwiseBtnEnable( cmdui ); // these are enabled/disabled together
}


/**
 *	This is called when the user clicks the mask overlay button.
 */
void PageTerrainTexture::OnMaskOverlayButton()
{
	bool isChecked =
		controls::isButtonChecked( 
			*this, IDC_PAGE_TERRAIN_OVERLAY_MASK_BUTTON );
	Options::setOptionInt( OPTS_MASK_OVERLAY, isChecked ? 1 : 0 );
    this->updatePythonMask();
}


/** 
 *  This is called when the yaw-angle is edited.
 */
void PageTerrainTexture::OnEnChangeYawAngleEdit()
{
	++filter_;
	float value = yawAngleEdit_.GetValue();
	if (yaw_ != value)
	{
		this->beginEditLayers();
		yawAngleSlider_.setValue( value );
		yaw_ = value;
		this->updateProjection();
		this->endEditLayers();
	}
	--filter_;
}


/** 
 *  This is called when the pitch-angle is edited.
 */
void PageTerrainTexture::OnEnChangePitchAngleEdit()
{
	++filter_;
	float value = pitchAngleEdit_.GetValue();
	if (pitch_ != value)
	{
		this->beginEditLayers();
		pitchAngleSlider_.setValue( value );
		pitch_ = value;
		this->updateProjection();
		this->endEditLayers();
	}
	--filter_;
}


/** 
 *  This is called when the roll-angle is edited.
 */
void PageTerrainTexture::OnEnChangeRollAngleEdit()
{
	++filter_;
	float value = rollAngleEdit_.GetValue();
	if (roll_ != value)
	{
		this->beginEditLayers();
		rollAngleSlider_.setValue( value );
		roll_ = value;
		this->updateProjection();
		this->endEditLayers();
	}
	--filter_;
}


/** 
 *  This is called when the u-size is edited.
 */
void PageTerrainTexture::OnEnChangeUSizeEdit()
{
	++filter_;
	float value = uSizeEdit_.GetValue();
	if (uScale_ != value)
	{
		this->beginEditLayers();
		uScale_ = value;
		uSizeSlider_.setValue( value );
		if (filter_ <= 1 && pBrush_->uvLocked_)
		{
			vSizeEdit_.SetValue( value );
			vSizeSlider_.setValue( value );
			vScale_ = value;
		}
		this->updateProjection();
		this->endEditLayers();
	}
	--filter_;
}


/** 
 *  This is called when the v-size is edited.
 */
void PageTerrainTexture::OnEnChangeVSizeEdit()
{
	++filter_;
	float value = vSizeEdit_.GetValue();
	if (vScale_ != value)
	{
		this->beginEditLayers();
		vScale_ = value;
		vSizeSlider_.setValue( value );
		if (filter_ <= 1 && pBrush_->uvLocked_)
		{
			uSizeEdit_.SetValue( value );
			uSizeSlider_.setValue( value );
			uScale_ = value;
		}
		this->updateProjection();
		this->endEditLayers();
	}
	--filter_;
}


/**
 *	This is called when the user clicks on the UV scale lock checkbox
 */
void PageTerrainTexture::OnBnClickedUVScaleLink()
{
	bool isChecked = linkUVScale_.GetCheck() == BST_CHECKED;
	linkUIcon_.ShowWindow( isChecked ? SW_SHOW : SW_HIDE );
	linkVIcon_.ShowWindow( isChecked ? SW_SHOW : SW_HIDE );
	pBrush_->uvLocked_ = isChecked;
}


/**
 *	This is called when the user clicks the reset button (for the texture
 *	projection).
 */
void PageTerrainTexture::OnResetButton()
{
	this->beginEditLayers();
	Vector4 uproj, vproj;
	Terrain::TerrainTextureLayer::defaultUVProjections( uproj, vproj );
	invTexProjection_ = this->buildMatrix( uproj, vproj );
	invTexProjection_.invert();
	invTexProjTrans_ = invTexProjection_.applyToOrigin();
	this->updateEditsFromProjection();
	this->updateEditedLayers( false );
	this->updatePython();
	this->destroyEditors();
	this->endEditLayers();
}


/**
 *	This is called when the user clicks the button which places the mask on
 *	the whole space.
 */
void PageTerrainTexture::OnPlaceMask()
{
	CWaitCursor waitCursor; // this may take a while

	TerrainPaintBrushPtr pBrush = TerrainPaintBrushPtr( new TerrainPaintBrush(), true );
	this->paintBrushFromControls( *pBrush );

	size_t numChunks = TextureMaskBlit::numChunksAffected( pBrush );

	bool doUndo			= numChunks <= PROMPT_LARGE_MASK;
	bool showProgress	= numChunks >  PROGRESS_LARGE_MASK;
	bool forceSave		= !doUndo;

	if (!doUndo)
	{
		std::string msg = L( "RCST_IDS_LARGE_IMPORT_MASK" );
        bool ok = AfxMessageBox( msg.c_str(), MB_YESNO ) == IDYES;
		if (!ok)
		{
			return;
		}
	}

	bool result = true;
	if (doUndo)
	{
		result =
			TextureMaskBlit::saveUndoForImport(
			    pBrush,
				L("WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/PAINT_MASK"),
				L("WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/GENERATE_UNDO_REDO_FOR_MASK"),
				showProgress );
	}
	if (result)
	{
		result =
			TextureMaskBlit::import(
			    pBrush,
				currentTextureFilename_,
				uProjection(), vProjection(),
				showProgress,
				L("WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/PAINTING_MASK"),
				forceSave );
	}
}


/**
 *	This is called when a new space is loaded.
 *
 *  @param wParam		Width of the space in chunks.
 *	@param lParam		Height of the space in chunks.
 *	@returns			0.
 */
/*afx_msg*/ LRESULT PageTerrainTexture::OnNewSpace(
	WPARAM /*wParam*/, 
	LPARAM /*lParam*/ )
{
	this->checkTerrainVersion();
	editedLayer_.clear();
	return 0;
}


/**
 *	This is called when the UI wants to query for a tooltip text for the toolbar.
 *
 *	@param UINT			Not used - here to match a function prototype.
 *	@param pNMHDR		A TOOLTIPTEXTA or TOOLTIPTEXTW with the tooltip
 *						structure.
 *	@param result		Set to 0 - here to match a function prototype.
 *	@returns			TRUE - the function was handled.
 */
BOOL PageTerrainTexture::OnToolTipText( 
	UINT, 
	NMHDR *			pNMHDR, 
	LRESULT *		result )
{
    // Allow top level routing frame to handle the message
    if (GetRoutingFrame() != NULL)
	{
        return FALSE;
	}

    // Need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    TCHAR szFullText[256];
    CString cstTipText;
    CString cstStatusText;

    UINT nID = pNMHDR->idFrom;
    if 
    (
        pNMHDR->code == TTN_NEEDTEXTA 
        && 
        (pTTTA->uFlags & TTF_IDISHWND) 
        ||
        pNMHDR->code == TTN_NEEDTEXTW 
        && 
        (pTTTW->uFlags & TTF_IDISHWND)
    )
    {
        // idFrom is actually the HWND of the tool
        nID = ((UINT)(WORD)::GetDlgCtrlID( (HWND)nID) );
    }

    if (nID != 0) // will be zero on a separator
    {
        AfxLoadString( nID, szFullText );
		// this is the command id, not the button index
        cstTipText    = szFullText;
        cstStatusText = szFullText;
    }

    // Non-UNICODE Strings only are shown in the tooltip window...
    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        lstrcpyn(
            pTTTA->szText, 
            cstTipText,
            sizeof( pTTTA->szText )/sizeof( pTTTA->szText[0] ) );
    }
    else
    {
        _mbstowcsz(
            pTTTW->szText, 
            cstTipText,
            sizeof( pTTTW->szText )/sizeof( pTTTW->szText[0] ) );
    }
    *result = 0;

    // bring the tooltip window above other popup windows
    ::SetWindowPos(
        pNMHDR->hwndFrom, 
        HWND_TOP, 
        0, 
        0, 
        0, 
        0,
        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE );

    return TRUE;    // message was handled
}


/*~ function WorldEditor.setCurrentTexture
 *	@components{ worldeditor }
 *
 *	This function changes the current texture used to paint the terrain,
 *	given only the texture file. It uses the default UV projection values.
 *
 *	@param pTextureFile The texture file to use for terrain painting.
 */
PY_MODULE_STATIC_METHOD(PageTerrainTexture, setCurrentTexture, WorldEditor )

PyObject * PageTerrainTexture::py_setCurrentTexture( PyObject * pArgs )
{
	char * pTextureFile = NULL;

	if (!PyArg_ParseTuple( pArgs, "s", &pTextureFile ))
	{
		PyErr_SetString(
			PyExc_TypeError, "py_setCurrentTexture: Argument parsing error." );
		return NULL;
	}
    if (s_instance_ != NULL)
    {
		s_instance_->mode( PAINTING );
	    s_instance_->currentTexture( pTextureFile );
    }
	Py_Return;
}


/*~ function WorldEditor.setCurrentTextureFull 
 *	@components{ worldeditor }
 *
 *	This function changes the current texture used to paint the terrain.
 *
 *	@param pTextureFile The texture file to use for terrain painting.
 *	@param uProjection The u-Projection of the texture.
 *	@param vProjection The v-Projection of the texture.
 */
PY_MODULE_STATIC_METHOD(PageTerrainTexture, setCurrentTextureFull, WorldEditor )

PyObject * PageTerrainTexture::py_setCurrentTextureFull( PyObject * pArgs )
{
	char * pTextureFile = NULL;
	float u0, u1, u2, u3;
	float v0, v1, v2, v3;

	if (!PyArg_ParseTuple( pArgs, "s(ffff)(ffff)",
		&pTextureFile,
		&u0, &u1, &u2, &u3,
		&v0, &v1, &v2, &v3 ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setCurrentTextureFull: Argument parsing error." );
		return NULL;
	}
    if (s_instance_ != NULL)
    {
		Vector4 uProjection( u0, u1, u2, u3 );
		Vector4 vProjection( v0, v1, v2, v3 );
	    s_instance_->currentTexture( pTextureFile, uProjection, vProjection );
    }
	Py_Return;
}


/*~ function WorldEditor.setCurrentBrush 
 *	@components{ worldeditor }
 *
 *	This function changes the current painting brush.
 *
 *	@param brushFile The brush file to use for terrain painting.
 */
PY_MODULE_STATIC_METHOD(PageTerrainTexture, setCurrentBrush, WorldEditor )

PyObject * PageTerrainTexture::py_setCurrentBrush( PyObject * pArgs )
{
	char * pBrushFile = NULL;

	if (!PyArg_ParseTuple( pArgs, "s", &pBrushFile ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setCurrentBrush: Argument parsing error." );
		return NULL;
	}
    if (s_instance_ != NULL)
    {
		DataSectionPtr pBrushSec = BWResource::openSection( pBrushFile );
		if (pBrushSec)
		{
			TerrainPaintBrushPtr brush = TerrainPaintBrushPtr( new TerrainPaintBrush(), true );
			if (brush->load( pBrushSec ))
			{
				s_instance_->setTerrainBrush( brush );
			}
			else
			{
				WARNING_MSG( 
					"The brush could not be loaded %s\n", 
					pBrushFile );
			}
		}
    }
	Py_Return;
}


/*~ function WorldEditor.setPaintLayer 
 *	@components{ worldeditor }
 *
 *	This function sets the index of the last layer painted.
 *
 *	@param idx		The index of the layer last painted.
 */
PY_MODULE_STATIC_METHOD(PageTerrainTexture, setPaintLayer, WorldEditor )

PyObject * PageTerrainTexture::py_setPaintLayer( PyObject * pArgs )
{
	int idx = (int)EditorChunkTerrain::NO_DOMINANT_BLEND;
	if (!PyArg_ParseTuple( pArgs, "i", &idx ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setPaintLayer: Argument parsing error." );
		return NULL;
	}
	if (s_instance_ != NULL)
	{
		s_instance_->paintLayer( idx );
	}
	Py_Return;
}


/*~ function WorldEditor.setPaintPos
 *	@components{ worldeditor }
 *
 *	This function sets the position that the last layer was painted at.
 *
 *	@param pos		The position that the last layer was painted at.
 */
PY_MODULE_STATIC_METHOD( PageTerrainTexture, setPaintPos, WorldEditor )

PyObject *PageTerrainTexture::py_setPaintPos( PyObject * pArgs )
{
	float x, y, z;
	if (!PyArg_ParseTuple( pArgs, "(fff)", &x, &y, &z ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setPaintPos: Argument parsing error." );
		return NULL;
	}
	if (s_instance_ != NULL)
	{
		s_instance_->paintPos( Vector3( x, y, z ) );
	}
	Py_Return;
}


/*~ function WorldEditor.setTerrainPaintEscKey
 *	@components{ worldeditor }
 *
 *	This function is possible called by the tool functor when the ESC key is
 *	pressed.
 *
 *	@param hitESC		Non-zero if the escape key is pressed.
 */
PY_MODULE_STATIC_METHOD( 
 PageTerrainTexture, setTerrainPaintEscKey, WorldEditor )

PyObject * PageTerrainTexture::py_setTerrainPaintEscKey( PyObject * pArgs )
{
	int hitEsc;

	if (!PyArg_ParseTuple( pArgs, "i", &hitEsc ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setTerrainPaintEscKey: Argument parsing error." );
		return NULL;
	}
    if (s_instance_ != NULL)
    {
	    s_instance_->onEscapeKey( hitEsc );
    }
	Py_Return;
}


/*~ function WorldEditor.isTerrainTexture
 *	@components{ worldeditor }
 *
 *	This function checks whether a file is suitable for terrain painting.
 *
 *	@param pBrushFile	A filename for testing.
 *	@return				Non-zero if the file is a texture file, zero otherwise.
 */
PY_MODULE_STATIC_METHOD( PageTerrainTexture, isTerrainTexture, WorldEditor )

PyObject * PageTerrainTexture::py_isTerrainTexture( PyObject * pArgs )
{
	char * pBrushFile = NULL;
	if (!PyArg_ParseTuple( pArgs, "s", &pBrushFile ))
	{
		PyErr_SetString(
			PyExc_TypeError, 
			"py_setCurrentBrush: Argument parsing error." );
		return NULL;
	}
	
	bool isTexture = 
		Moo::TextureManager::instance()->isTextureFile( pBrushFile );

	return PyInt_FromLong( isTexture ? 1 : 0 );
}


/**
 *	This is called when the user drops something from the UAL onto the current 
 *	texture.
 *
 *	@param pItemInfo	The dragged item's information.
 *  @return				True if the drag was successful.
 */
bool PageTerrainTexture::onDropTexture( UalItemInfo * pItemInfo )
{
	if (pItemInfo == NULL)
	{
		return false;
	}
	
	std::string texture = 
		BWResource::dissolveFilename( pItemInfo->longText() );
	this->currentTexture( texture );

	return true;
}


/**
 *	This is called when the user drops a texture from the UAL onto the texture 
 *	mask.
 *
 *	@param pItemInfo	The dragged item's information.
 *  @return				True if the drag was successful.
 */
bool PageTerrainTexture::onDropTextureMask( UalItemInfo * pItemInfo )
{
	if (pItemInfo == NULL)
	{
		return false;
	}	
	
	std::string texture = pItemInfo->longText();
	controls::DibSection32 img;
	if (!img.load(texture))
	{
		return false;
	}
	maskTexture_.image( img );

	texture = BWResource::dissolveFilename( texture ); 

	controls::checkButton ( *this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON     , true              );
	controls::checkButton ( *this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON, false             );
	controls::checkButton ( *this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON , false             );
	controls::enableWindow( *this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON, hasUVProjections_ );
	controls::enableWindow( *this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON , true              );

	pBrush_->textureMask_				= true;
	pBrush_->textureMaskTexture_		= texture;
	pBrush_->textureMaskIncludeProj_	= false;
	pBrush_->textureMaskInvert_			= false;

    this->updatePythonMask();

	return true;
}


/**
 *	This is called when the user drops a brush from the UAL onto the page.
 *
 *	@param pItemInfo	The dragged item's information.
 *  @return				True if the drag was successful.
 */
bool PageTerrainTexture::onDropBrush( UalItemInfo * pItemInfo )
{
	if (pItemInfo == NULL)
	{
		return false;
	}
	
	std::string brushFile = pItemInfo->longText();
	DataSectionPtr brushDataSection = BWResource::openSection( brushFile );
	if (!brushDataSection)
	{
		return false;
	}

	TerrainPaintBrushPtr pBrush = TerrainPaintBrushPtr( new TerrainPaintBrush(), true );
	if (pBrush->load( brushDataSection ))
	{
		setTerrainBrush( pBrush );
		return true;
	}
	else
	{
		WARNING_MSG(
			"Cannot load brush %s\n",
			brushFile.c_str() );
		return false;
	}
}


/**
 *  This gets called when the UV projections need updating from the 
 *  edit controls.
 *
 *	@param temporary		If true then any changes are temporary
 */
void PageTerrainTexture::updateProjection( bool temporary /*= false*/ )
{
	Matrix proj;
	Vector3 trans = Vector3::zero();
	Utilities::compose(
		proj,
		Vector3( 1.0f/uScale_, 1.0f, 1.0f/vScale_ ),
		trans,
		Vector3( yaw_, pitch_, roll_ ) );
	trans = invTexProjection_.applyToOrigin();	
	proj.invert();
	invTexProjection_ = proj;
	invTexProjection_.translation( trans );
	if (pMatrixProxy_ != NULL)
	{
		pMatrixProxy_->setMatrixAlone( invTexProjection_ );
	}

    this->updatePython(); // Let Python know about the projection change
	this->updateEditedLayers( temporary );
}


/**
 *  This updates the edit fields if the u-v projections have been changed.
 *	The brush's projection is not updated, called updatePython to do this.
 *  
 *	@param forceUpdateEdits	If true then the edit and slider controls are 
 *							always updated,	if false then they are updated
 *							only if their values where changed.
 */
void PageTerrainTexture::updateEditsFromProjection( 
	bool forceUpdateEdits /*= false*/ )
{
    // Get the matrix components:
    Vector3 trans, scale, rot;
	Matrix proj = textureProjection();
    Utilities::decompose( proj, scale, trans, rot );

    // The scales:
    uScale_ = 1.0f/scale.x;
    vScale_ = 1.0f/scale.z;

    // The rotations:
    yaw_    = rot.x;
    pitch_  = rot.y;
    roll_   = rot.z;
   
	++filter_;

	++layerUndoAdded_;
	// Update the edit fields.  Be careful only to update if there were 
	// changes:

	float yawEdit = yawAngleEdit_.GetValue();
	if (forceUpdateEdits || !almostEqual( yawEdit, yaw_, EDITING_EPSILON ))
	{
		yawAngleEdit_  .SetValue( yaw_ );
		yawAngleSlider_.setValue( yaw_ );
	}

	float pitchEdit = pitchAngleEdit_.GetValue();
	if (forceUpdateEdits || !almostEqual( pitchEdit, pitch_, EDITING_EPSILON ))
	{
		pitchAngleEdit_  .SetValue( pitch_ );
		pitchAngleSlider_.setValue( pitch_ );
	}

	float rollEdit = rollAngleEdit_.GetValue();
	if (forceUpdateEdits || !almostEqual( rollEdit, roll_, EDITING_EPSILON ))
	{
		rollAngleEdit_  .SetValue( roll_ );
		rollAngleSlider_.setValue( roll_ );
	}

	float uScaleEdit = uSizeEdit_.GetValue();
	if (forceUpdateEdits || !almostEqual( uScaleEdit, uScale_, EDITING_EPSILON ))
	{
		uSizeEdit_  .SetValue( uScale_ );
		uSizeSlider_.setValue( uScale_ );
	}

	float vScaleEdit = vSizeEdit_.GetValue();
	if (forceUpdateEdits || !almostEqual( vScaleEdit, vScale_, EDITING_EPSILON ))
	{
		vSizeEdit_  .SetValue( vScale_ );
		vSizeSlider_.setValue( vScale_ );
	}
	--layerUndoAdded_;
	--filter_;
}


/**
 *  This calls back Python whenever one of the textures, scalings etc is
 *  changed.
 */
void PageTerrainTexture::updatePython()
{
	if (inited_)
	{
		Vector4 uProj = uProjection();
		Vector4 vProj = vProjection();

		MF_ASSERT( 
			WorldEditorApp::instance().pythonAdapter() != NULL &&
			"PageTerrainTexture::updatePython: PythonAdapter is NULL" );

		pBrush_->paintUProj_ = uProj;
		pBrush_->paintVProj_ = vProj;
	 
		WorldEditorApp::instance().pythonAdapter()->setTerrainPaintMode( mode_ );
		WorldEditorApp::instance().pythonAdapter()->setTerrainPaintBrush( pBrush_ );
	}
}


/**
 *  This calls back python whenever one of the mask parameters changes.
 */
void PageTerrainTexture::updatePythonMask()
{
	MF_ASSERT( 
		WorldEditorApp::instance().pythonAdapter() != NULL &&
		"PageTerrainTexture::updatePythonMask: PythonAdapter is NULL" );

    WorldEditorApp::instance().pythonAdapter()->setTerrainPaintBrush( pBrush_ );
}


/**
 *	This function is called to build the editors for terrain painting.
 */
void PageTerrainTexture::buildEditors()
{
	if ( !hasUVProjections_ )
	{
		return;
	}

	destroyEditors(); // just to be safe

	int enabler = mode_ == EDITING ? Gizmo::ALWAYS_ENABLED : MODIFIER_SHIFT;

	pMatrixProxy_ = new TexProjMatrixProxy(this);
	pTileGizmo_ = 
		new TileGizmo(
			pMatrixProxy_, 
			enabler, 
			MODIFIER_CTRL );
	pTileGizmo_->texture( texture() );
	GizmoManager::instance().addGizmo( pTileGizmo_ );
	// Must tell the smartpointer that the reference is already incremented,
	// because the PyObjectPlus base class increments the refcnt (!)
	GeneralEditorPtr editor( new GeneralEditor(), true );
	editor->addProperty(
		new GenScaleProperty(
			L(
				"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/SCALE"), 
				pMatrixProxy_, 
				false, 
				false ) );
	editor->addProperty(
		new GenRotationProperty(
			L(
				"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/ROTATION" ), 
				pMatrixProxy_, 
				false ) );
	GeneralEditor::Editors newEds;
	newEds.push_back( editor );
	GeneralEditor::currentEditors( newEds );
	pTileGizmo_->opacity( (uint8)(2.55f*opacitySlider_.getValue()) );
}


/** 
 *	This function is called to destroy the editors when editing has been done.
 */
void PageTerrainTexture::destroyEditors()
{
	GizmoManager::instance().removeAllGizmo();
	GeneralEditor::Editors newEds;
	GeneralEditor::currentEditors( newEds );

	pTileGizmo_   = NULL;
	pMatrixProxy_ = NULL;
}


/**
 *	This aligns the texture to the surface normal at the given point.
 *  
 *  @param pos			The position to get the normal at.
 *	@param temporary	If true then the alignment is only temporary.
 *  @returns			True if the texture was realligned.
 */
bool PageTerrainTexture::alignTextureToNormal(
	const Vector3 &	pos,
	bool			temporary )
{
	// Get the terrain normal at pos:
	EditorChunkTerrainPtr terrain = 
		EditorChunkTerrainCache::instance().findChunkFromPoint( pos );
	if (terrain == NULL)
	{
		return false;
	}
	Matrix worldToLocal = terrain->chunk()->transform();
	worldToLocal.invert();
	Vector3 cpos   = worldToLocal.applyPoint( pos );
	Vector3 normal = terrain->block().normalAt( cpos.x, cpos.z );

	// The normal is the second column of the texture projection matrix.  We
	// get the first and third rows by using Gram-Schmidt on the normal and
	// uProjection, vProjection.
	Vector3 trans  = invTexProjection_.applyToOrigin();
	Matrix  proj   = textureProjection();
	Vector4 uproj4 = proj.column( 0 );
	Vector4 vproj4 = proj.column( 2 );
	Vector3 uproj  = Vector3( uproj4.x, uproj4.y, uproj4.z );
	Vector3 vproj  = Vector3( vproj4.x, vproj4.y, vproj4.z );

	// Come up with a basis that is orthogonal with the normal:
	Vector3 u1, u2, u3;
	BW::orthogonalize( normal, uproj, vproj, u1, u2, u3 );

	// Retain the original scales:
	u2 *= uproj.length(); 
	u3 *= vproj.length();

	// uProjection_ and vProjection are now u2 and u3:
	invTexProjection_ = 
		this->buildMatrix(
			Vector4( u2.x, u2.y, u2.z, 0.0f),
			Vector4( u3.x, u3.y, u3.z, 0.0f) );
	invTexProjection_.invert(); // convert from projection to its inverse
	invTexProjection_.translation( trans );
	invTexProjTrans_ = trans;
	pMatrixProxy_->setMatrixAlone( invTexProjection_ );

    this->updateEditsFromProjection();
    this->updatePython();
	this->updateEditedLayers( temporary );	

	return true;
}


/**
 *	This function creates the mask image.
 */
void PageTerrainTexture::buildMaskImage()
{
	controls::DibSection32 maskDib;

	ImportImagePtr mask = HeightModule::maskImage();
	if (mask && !mask->isEmpty())
	{
		maskDib.resize( MASK_DIB_SIZE, MASK_DIB_SIZE );

		for (uint32 y = 0; y < MASK_DIB_SIZE; ++y)
		{
			uint32 sy = Math::lerp( y, (uint32)0, MASK_DIB_SIZE, (uint32)0, mask->height() );
			for (uint32 x = 0; x < MASK_DIB_SIZE; ++x)
			{
				uint32 sx = Math::lerp( x, (uint32)0, MASK_DIB_SIZE, (uint32)0, mask->width() );
				uint8  v  = mask->get( sx, sy )/257; // convert from uint16 to uint8
				maskDib.set( x, y, RGB( v, v, v ) );
			}
		}
	}
	maskImage_.image( maskDib );
}


/**
 *	This is called on every frame when the user is painting.
 *
 *  @param mouseUp		True if the mouse is up.
 */
void PageTerrainTexture::onPainting( bool mouseUp )
{
	if (pTileGizmo_ != NULL && !inHScroll_)
	{
		pTileGizmo_->drawOptions( TileGizmo::DRAW_ALL );
	}

	if (!mouseUp)
	{
		editedLayer_.clear();
	}

	if (hasTexture())
	{
		WorldManager::instance().resetCursor();
	}
	else
	{
		// No texture loaded and trying to paint, so set cursor to
		// show some feedback.
		static HCURSOR cursor = AfxGetApp()->LoadStandardCursor( IDC_NO );
		WorldManager::instance().setCursor( cursor );		
	}
}


/**
 *	This is called on every frame when the user is replacing textures.
 *
 *  @param mouseUp		True if the mouse is up.
 */
void PageTerrainTexture::onReplacing( bool mouseUp )
{
	if (pTileGizmo_ != NULL && !inHScroll_)
	{
		pTileGizmo_->drawOptions( TileGizmo::DRAW_ALL );
	}

	if (hasTexture())
	{
		WorldManager::instance().resetCursor();
	}
	else
	{
		// No texture loaded and trying to paint, so set cursor to
		// show some feedback.
		static HCURSOR cursor = AfxGetApp()->LoadStandardCursor( IDC_NO );
		WorldManager::instance().setCursor( cursor );		
	}

	if (!mouseUp)
	{
		CWaitCursor waitCursor;
		editedLayer_.clear();
		paintLayer_ = (int)replaceDominantTexture();
		paintPos_	= toolPos();
	}
}


/**
 *	This is called on every frame when the user is sampling textures.
 *
 *  @param mouseUp		True if the mouse is up.
 *	@param idx			The index of the layer to sample.  If this is 
 *						EditorChunkTerrain::NO_DOMINANT_BLEND then the dominant 
 *						layer is chosen.
 *	@param pPoint		The point to sample at.  If this is NULL then the 
 *						point under the cursor is chosen.
 *  @param canSampleOpacity If true and the CTRL key is down then the opacity
 *						is also sampled.
 */
void PageTerrainTexture::onSampleTexture
(
	bool			mouseUp, 
	size_t			idx					/*= EditorChunkTerrain::NO_DOMINANT_BLEND*/,
	const Vector3 *	pPoint				/*= NULL*/,
	bool			canSampleOpacity	/*= false*/
)
{
	if (pTileGizmo_ != NULL && !inHScroll_)
	{
		pTileGizmo_->drawOptions( TileGizmo::DRAW_ALL );
	}

	static HCURSOR cursor = AfxGetApp()->LoadCursor( IDC_HEIGHTPICKER );
	WorldManager::instance().setCursor(cursor);

	if (!mouseUp)
	{
		editedLayer_.clear();

		size_t					domIdx				= EditorChunkTerrain::NO_DOMINANT_BLEND;
		EditorChunkTerrain *	pEditorChunkTerrain	= NULL;
		Chunk *					pChunk				= NULL;
		uint8					strength			= 0;

		Vector3	pos			= (pPoint != NULL) ? *pPoint : toolPos();

		if (
			TerrainTextureUtils::dominantTexture(
				pos, &domIdx, &pEditorChunkTerrain, &pChunk, &strength ) )
		{
			if (mode_ != PAINTING && !chunkWritable( pChunk ))
			{
				ERROR_MSG(
					L( "WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/TEXTURE_NOT_WRITABLE" ) );
			}
			else
			{
				if (idx == EditorChunkTerrain::NO_DOMINANT_BLEND)
				{
					idx = domIdx;
				}
				paintPos_ = pos;
				paintLayer_ = domIdx;
				const Terrain::TerrainTextureLayer & textureLayer = 
					pEditorChunkTerrain->block().textureLayer( idx );
				std::string textureName = textureLayer.textureName();
				if (!textureName.empty())
				{
					if (textureName != currentTextureFilename_)
					{
						WorldManager::instance().addCommentaryMsg( 
							textureName );
					}
					TerrainTextureUtils::FindError findError =
						TerrainTextureUtils::findNeighboursWithTexture(
							pEditorChunkTerrain,
							textureLayer.textureName(),
							textureLayer.uProjection(),
							textureLayer.vProjection(),
							editedLayer_ );					
					if (mode_ != PAINTING)
					{
						TerrainTextureUtils::printErrorMessage( findError );
					}
					seedTerrain_.layerIdx_    = idx;
					seedTerrain_.terrain_     = pEditorChunkTerrain;
					seedTerrain_.textureName_ = textureName;
					this->currentTexture( textureName );
					if (textureLayer.hasUVProjections())
					{
						invTexProjection_ = 
							this->buildMatrix(
								textureLayer.uProjection(),
								textureLayer.vProjection() );						
						invTexProjection_.invert();
						invTexProjTrans_ = invTexProjection_.applyToOrigin();
						this->updateEditsFromProjection();
						this->updatePython();
					}
				}
				this->updateGizmo( true, &pos );
				if (canSampleOpacity && isSamplingOpacity())
				{
					this->setOpacity( strength/2.54f );
				}
			}
		}
	}
}


/**
 *	This is called on every frame when the user is sampling normals.
 *
 *  @param mouseUp		True if the mouse is up.
 */
void PageTerrainTexture::onSampleNormal(bool mouseUp)
{
	if (pTileGizmo_ != NULL && !inHScroll_)
	{
		pTileGizmo_->drawOptions( 
			TileGizmo::DRAW_GRID | TileGizmo::DRAW_TEXTURE );
	}
	
	static HCURSOR cursor = AfxGetApp()->LoadCursor(IDC_HEIGHTPICKER);
	WorldManager::instance().setCursor(cursor);

	if (!mouseUp && !samplingNormal_)
	{
		this->beginEditLayers();
		samplingNormal_ = true;
	}

	if (!mouseUp || samplingNormal_)
	{
		Vector3 toolpos = toolPos();
		invTexProjection_.translation( toolpos );
		this->alignTextureToNormal( toolpos, !mouseUp );
	}

	// Have we stopped sampling the normal?
	if ((mouseUp || !isEditingGizmo() || !isSampling()) && samplingNormal_)
	{
		this->endEditLayers();
		samplingNormal_ = false;
	}
}


/**
 *	This is called when the user is editing the Gizmo.
 *
 *  @param mouseUp			True if the mouse is up.
 */
void PageTerrainTexture::onEditingGizmo( bool /*mouseUp*/ )
{
	this->onEditing();
}


/**
 *	This is called on every frame when the user is editing.
 */
void PageTerrainTexture::onEditing()
{
	WorldManager::instance().resetCursor();

	if (pTileGizmo_ != NULL && !inHScroll_)
	{
		pTileGizmo_->drawOptions( TileGizmo::DRAW_ALL );
	}
}


/**
 *	This gets the middle of the screen in world coordinates.
 *
 *	@returns			The middle of the screen in world coordinates.
 */
Vector3 PageTerrainTexture::middleOfScreen() const
{
	int sw = (int)Moo::rc().screenWidth();
	int sh = (int)Moo::rc().screenHeight();
	return WorldManager::instance().getWorldRay( sw/2, sh/2 );
}


/**
 *	This finds the dominant texture under the mouse cursor.
 *
 *	@param idx					This is set to the index of the texture layer.
 *	@param pEditorChunkTerain	This is set to the chunk terrain.
 *	@param pChunk				This is set to the chunk.
 *	@returns					True if there is a dominant texture.
 */
bool PageTerrainTexture::dominantTexture
(
	size_t *					idx,
	EditorChunkTerrain **		pEditorChunkTerain,
	Chunk **					pChunk
) const
{
	return TerrainTextureUtils::dominantTexture( 
		toolPos(), idx, pEditorChunkTerain, pChunk );
}


/**
 *	This replaces the dominant texture under the cursor with the selected
 *	texture and projection.
 *
 *  @returns					The index of the dominant texture that was
 *								replaced and 
 *								EditorChunkTerrain::NO_DOMINANT_BLEND if 
 *								nothing was replaced.
 */
size_t PageTerrainTexture::replaceDominantTexture()
{
    if (!hasTexture())
	{
        return EditorChunkTerrain::NO_DOMINANT_BLEND;
	}

	size_t				idx						= EditorChunkTerrain::NO_DOMINANT_BLEND;
	EditorChunkTerrain	*pEditorChunkTerrain	= NULL;
	Chunk				*pChunk					= NULL;

	if (this->dominantTexture(&idx, &pEditorChunkTerrain, &pChunk))
	{
		// Find the number of affected chunks:
		TerrainTextureUtils::LayerSet layers;
		Terrain::EditorBaseTerrainBlock & block = pEditorChunkTerrain->block();
		Terrain::TerrainTextureLayer & textureLayer = 
			block.textureLayer( idx );
		TerrainTextureUtils::findNeighboursWithTexture(
			pEditorChunkTerrain, 
			textureLayer.textureName(), 
			textureLayer.uProjection(),
			textureLayer.vProjection(),
			layers );

		// Is is possible to replace the texture?
		Vector4 uProj = uProjection();
		Vector4 vProj = vProjection();
		bool ok =	
			TerrainTextureUtils::canReplaceTexture(
				idx, 
				pEditorChunkTerrain, 
				currentTextureFilename_, 
				uProj, vProj );

		// If there are more than PROMPT_LARGE_REPLACE chunks affected then
		// prompt whether the replace should be done:
		if (ok && (layers.size() > PROMPT_LARGE_REPLACE))
		{
			std::string msg = 
				L(
					"WORLDEDITOR/GUI/PAGE_TERRAIN_TEXTURE/LARGE_REPLACE", 
					layers.size() );
			int msgres = AfxMessageBox( msg.c_str(), MB_YESNO );
			ok = (msgres == IDYES);
		}

		// If all is well then do the replace:
		if (ok)
		{

			TerrainTextureUtils::replaceTexture(
				idx, 
				pEditorChunkTerrain, 
				currentTextureFilename_, 
				uProj, vProj );
			return idx;
		}			
	}
	return EditorChunkTerrain::NO_DOMINANT_BLEND;
}


/**
 *	This function sets the texture mask's texture.
 *
 *	@param textureFile		The texture to use.
 *	@param uProj			The u-projection.
 *	@param vProj			The v-projection.
 *	@param enableTex		Turn on the texture mask?
 *	@param includeProj		Include the texture projections in the mask?
 *	@param invert			Invert the sense of the texture mask?
 *	@param updateProj		Update the projection values?
 */
void PageTerrainTexture::setTextureMaskTexture(
	const std::string &		textureFile,
	const Vector4 &			uProj,
	const Vector4 &			vProj,
	bool					enableTex,
	bool					includeProj,
	bool					invert,
	bool					updateProj )
{
	++filter_;

	++layerUndoAdded_;
	std::string unResTexFile = BWResource::resolveFilename( textureFile );
	controls::DibSection32 texImg;
	bool loaded = texImg.load( unResTexFile );
	maskTexture_.image( texImg );
	if (loaded)
	{		
		pBrush_->textureMaskTexture_	= textureFile;
		
	}
	else
	{
		pBrush_->textureMaskTexture_	= "";
		enableTex						= false;
	}

	pBrush_->textureMask_		    	= enableTex;
	pBrush_->textureMaskIncludeProj_	= includeProj;
	pBrush_->textureMaskInvert_			= invert;

	if (updateProj)
	{
		pBrush_->textureMaskUProj_ = uProj;
		pBrush_->textureMaskVProj_ = vProj;
	}

	controls::checkButton( *this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON     , enableTex   );
	controls::checkButton( *this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON, includeProj );
	controls::checkButton( *this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON , invert      );

	this->onEnableMaskControls( true ); // force update the enabled states

	// Update the projection edit controls:
	if (updateProj)
	{
		Matrix projMatrix = buildMatrix( uProj, vProj );
		Vector3 trans, scale, rot;
		Utilities::decompose( projMatrix, scale, trans, rot );
		float rx = rot.x;
		float ry = rot.y;
		float rz = rot.z;
		float sx = 1.0f/scale.x;
		float sz = 1.0f/scale.z;
		maskTextureYawEdit_  .SetValue( rx );
		maskTexturePitchEdit_.SetValue( ry );
		maskTextureRollEdit_ .SetValue( rz );			
		maskTextureUProjEdit_.SetValue( sx );
		maskTextureVProjEdit_.SetValue( sz );
	}

    // Update Python:
    this->updatePythonMask();
	--layerUndoAdded_;
	--filter_;
}


/**
 *	This synchronizes the projections of the edited areas with the current
 *	projection.
 *
 *	@param temporary			If true then the update is only temporary, and
 *								so a lower quality update can be done.
 */
void PageTerrainTexture::updateEditedLayers( bool temporary )
{
	if (mode_ == EDITING)
	{
		Vector4 uProj = uProjection();
		Vector4 vProj = vProjection();
		checkEditedLayer();
		if (!editedLayer_.empty())
		{
			TerrainTextureUtils::setProjections(
				editedLayer_, 
				uProj, vProj,
				temporary );
		}
	}
}


/**
 *	This is called to enable/disable the mask controls.
 *
 *	@param forceUpdate	If true then the controls are updated regardless of the
 *						filter, if false then we only update if the filter
 *						is zero.
 */
void PageTerrainTexture::onEnableMaskControls( bool forceUpdate /*= false*/ )
{
	if (forceUpdate || filter_ == 0)
	{
		++filter_;

		bool isPainting = mode_ == PAINTING;

		bool isHeight = 
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_HEIGHT_MASK_BUTTON );

		bool isSlope = 
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_SLOPE_MASK_BUTTON );

		bool isTexture =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON );

		bool isNoise =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON );

		bool isImportMask =
			controls::isButtonChecked( 
				*this, IDC_PAGE_TERRAIN_IMPORTED_MASK_BUTTON );

		bool hasImport = !maskImage_.image().isEmpty();

		pBrush_->heightMask_	= isHeight;
		pBrush_->slopeMask_		= isSlope;
		pBrush_->importMask_	= isImportMask && hasImport;
		
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_OVERLAY_MASK_BUTTON , isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_HEIGHT_MASK_BUTTON  , isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_HEIGHT_MIN_EDIT     , isPainting && isHeight                       );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_HEIGHT_MAX_EDIT     , isPainting && isHeight                       );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_HEIGHT_FUZ_EDIT     , isPainting && isHeight                       );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_SLOPE_MASK_BUTTON   , isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_SLOPE_MIN_EDIT      , isPainting && isSlope                        );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_SLOPE_MAX_EDIT      , isPainting && isSlope                        );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_SLOPE_FUZ_EDIT      , isPainting && isSlope                        );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON      , isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON , isPainting && isTexture && hasUVProjections_ );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON  , isPainting && isTexture                      );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON   , isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_NOISE_SETUP_BUTTON  , isPainting && isNoise                        );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_IMPORTED_MASK_BUTTON, isPainting                                   );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_ADJUST_MASK_BUTTON  , isPainting && isImportMask                   );	
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_ACTIONTB       , isPainting && isImportMask && hasImport      );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_STRENGTH_EDIT  , isPainting && isImportMask && hasImport      );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_STRENGTH_SLIDER, isPainting && isImportMask && hasImport      );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_BROWSE_MASK_BUTTON  , isPainting && isAdjustingMask_               );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_STRENGTH_EDIT  , isPainting && isAdjustingMask_ && hasImport  );
		controls::enableWindow( *this, IDC_PAGE_TERRAIN_MASK_STRENGTH_SLIDER, isPainting && isAdjustingMask_ && hasImport  );

		this->updatePythonMask();

		--filter_;
	}
}


/**
 *	This is called to set the mode.
 *
 *  @para m			The new mode.
 */
void PageTerrainTexture::mode( Mode m )
{
	bool changed = mode_ != m;

	mode_ = m;
	switch (mode_)
	{
	case PAINTING:
		editProjButton_.toggle( false );
		if (changed)
		{
			WorldManager::instance().addCommentaryMsg( ENTER_PAINT_MSG );
		}
		break;
	case EDITING:
		editProjButton_.toggle( true );
		if (changed)
		{
			WorldManager::instance().addCommentaryMsg( ENTER_EDIT_MSG );
		}
		break;
	case REPLACING:
		editProjButton_.toggle( false );
		break;
	}
	this->onEnableMaskControls();
	this->destroyEditors();
	this->updatePython();
}


/**
 *	This is called when the user starts adjusting the mask.
 */
void PageTerrainTexture::beginAdjustMask()
{
	isAdjustingMask_ = true;
	ModuleManager::instance().push( "HeightModule" );
	HeightModule * pHeightModule = HeightModule::currentInstance();
	if (pHeightModule != NULL)
	{
		pHeightModule->mode( HeightModule::IMPORT_MASK );
		pHeightModule->textureImportStrength(
			maskStrengthSlider_.getValue()/100.0f );
		this->onEnableMaskControls( true );
	}
}


/**
 *	This is called when the user stops adjusting the mask.  This will happen
 *	if the user pressed the "adjust" button, or if the page becomes inactive.
 */
void PageTerrainTexture::endAdjustMask()
{
	++filter_;
	
	isAdjustingMask_ = false;

	pBrush_->importMaskTL_		= HeightModule::topLeft( true );
	pBrush_->importMaskBR_		= HeightModule::bottomRight( true );
	pBrush_->importMaskImage_	= HeightModule::maskImage();
	pBrush_->importMaskAdd_		= 0.0f;
	pBrush_->importMaskMul_		= (maskStrengthSlider_.getValue()/65535.0f)/100.0f;

	ModuleManager::instance().pop();
	controls::checkButton( *this, IDC_PAGE_TERRAIN_ADJUST_MASK_BUTTON, false );
	this->updatePython();
	this->onEnableMaskControls();

	--filter_;
}


/**
 *	This fills out paint brush with the values from the user interface.
 *
 *	@param brush		This is filled with the paint brush values.
 */
void PageTerrainTexture::paintBrushFromControls( 
	TerrainPaintBrush &		brush ) const
{
	// Texture:
	int intOpacity = Math::lerp( opacityEdit_.GetValue(), 0.0f, 100.0f, 0, 255 );
	brush.paintTexture_				= currentTextureFilename_;
	brush.size_						= sizeEdit_.GetValue();
	brush.strength_					= strengthEdit_.GetValue();
	brush.paintUProj_				= uProjection();
	brush.paintVProj_				= vProjection();
	brush.opacity_					= (uint8)(Math::clamp( 0, intOpacity, 255 )); // convert % to uint8
	brush.maxLayerLimit_			= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_LIMIT_LAYERS_CB );
	brush.maxLayers_				= maxLayersEdit_.GetIntegerValue();

	// Height
	brush.heightMask_				= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_HEIGHT_MASK_BUTTON );
	brush.h1_						= minHeightEdit_.GetValue() - fuzHeightEdit_.GetValue();
	brush.h2_						= minHeightEdit_.GetValue();
	brush.h3_						= maxHeightEdit_.GetValue();
	brush.h4_						= maxHeightEdit_.GetValue() + fuzHeightEdit_.GetValue();

	// Slope:
	brush.slopeMask_				= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_SLOPE_MASK_BUTTON );
	brush.s1_						= minSlopeEdit_.GetValue() - fuzSlopeEdit_.GetValue();
	brush.s2_						= minSlopeEdit_.GetValue();
	brush.s3_						= maxSlopeEdit_.GetValue();
	brush.s4_						= maxSlopeEdit_.GetValue() + fuzSlopeEdit_.GetValue();

	// Texture mask:
	brush.textureMask_				= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_TEXMASK_BUTTON );
	brush.textureMaskIncludeProj_	= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_TEXMASK_PROJ_BUTTON ) && hasUVProjections_;
	brush.textureMaskUProj_			= pBrush_->textureMaskUProj_;
	brush.textureMaskVProj_			= pBrush_->textureMaskVProj_;
	brush.textureMaskTexture_		= pBrush_->textureMaskTexture_;
	brush.textureMaskInvert_		= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_TEXMASK_INV_BUTTON );

    // Noise:
    brush.noiseMask_				= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_NOISE_MASK_BUTTON );
	brush.noiseMinSat_				= pBrush_->noiseMinSat_;
	brush.noiseMaxSat_				= pBrush_->noiseMaxSat_;
	brush.noiseMinStrength_			= pBrush_->noiseMinStrength_;
	brush.noiseMaxStrength_			= pBrush_->noiseMaxStrength_;
	brush.noise_.octaves( pBrush_->noise_.octaves() );

	// Import mask:
	brush.importMask_				= controls::isButtonChecked( *this, IDC_PAGE_TERRAIN_IMPORTED_MASK_BUTTON )
										&&
										HeightModule::maskImage() != NULL;
	brush.importMaskImage_			= HeightModule::maskImage();
	brush.importMaskMul_			= (maskStrengthEdit_.GetValue()/100.0f)/65535.0f;
	brush.importMaskAdd_			= 0.0f;

	if (brush.importMaskImage_ != NULL && brush.importMask_)
	{
		brush.importMaskTL_	= HeightModule::topLeft(true);
		brush.importMaskBR_	= HeightModule::bottomRight(true);
	}
	else
	{
		int gxmin, gymin, gxmax, gymax;
		TerrainUtils::terrainSize( "", gxmin, gymin, gxmax, gymax );

		float left   = GRID_RESOLUTION*gxmin;
		float top    = GRID_RESOLUTION*gymin;
		float right  = (gxmax + 1)*GRID_RESOLUTION;
		float bottom = (gymax + 1)*GRID_RESOLUTION;

		brush.importMaskTL_	= Vector2( left , top    );
		brush.importMaskBR_	= Vector2( right, bottom );
	}
}


/**
 *	This builds a matrix from u, v components.
 *
 *	@param u		The u component (column 0).
 *	@param v		The v component (column 2).
 *	@returns		A matrix with u and v in the 0 and 2 columns.  The
 *					1 column is the cross product of v,u (as Vector3s 
 *					ignoring the w component).  Column 4 is (0, 0, 0, 1).
 */
/*static*/ Matrix PageTerrainTexture::buildMatrix(
	const Vector4 &	u, 
	const Vector4 &	v )
{
	Matrix result;
	result.setIdentity();

	Vector3 uproj = Vector3( u.x, u.y, u.z );
	Vector3 vproj = Vector3( v.x, v.y, v.z );
	Vector3 c1;
	c1.crossProduct( vproj, uproj );
	c1.normalise();
	Vector4 col1( c1.x, c1.y, c1.z, 0.0f ); 

	result.setIdentity();
	result.column( 0, u    );
	result.column( 1, col1 );
	result.column( 2, v    );

	return result;
}
