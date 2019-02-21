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
#include "page_animations_impl.hpp"
#include "page_animations.hpp"

#include "controls/edit_commit.hpp"
#include "controls/edit_numeric.hpp"
#include "controls/slider.hpp"
#include "controls/user_messages.hpp"
#include "guimanager/gui_functor.hpp"
#include "guimanager/gui_functor_option.hpp"
#include "guimanager/gui_manager.hpp"
#include "guimanager/gui_menu.hpp"
#include "guimanager/gui_toolbar.hpp"
#include "main_frm.h"
#include "me_app.hpp"
#include "me_shell.hpp"
#include "moo/interpolated_animation_channel.hpp"
#include "python_adapter.hpp"
#include "resmgr/string_provider.hpp"
#include "model/super_model.hpp"
#include "shlwapi.h"
#include "undo_redo.hpp"
#include "utilities.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

void PageAnimations::calculateCompressionErrorFactors(
    float& scale, float& rotation, float& position )
{
    position = 0.00000000001f * powf(1.02f, pImpl_->compPosSldr.GetPos() + 300.f );
	rotation = 0.00001f * powf(1.03f, (float)pImpl_->compRotSldr.GetPos() );
    scale = 0.00000000001f * powf(1.02f, pImpl_->compScaleSldr.GetPos() + 300.f );
}

float logf( float base, float f )
{
    return logf( f ) / logf( base );
}

void PageAnimations::setSlidersCompressionErrorFactors(
    float scale, float rotation, float position )
{
    pImpl_->compPosSldr.SetPos( int(logf(1.02f, position / 0.00000000001f ) - 300.f) );
	pImpl_->compRotSldr.SetPos( int(logf(1.03f, rotation / 0.00001f) ));
    pImpl_->compScaleSldr.SetPos( int(logf(1.02f, scale / 0.00000000001f ) - 300.f) );
    
}

void PageAnimations::currentFrames( int& scale, int& rotation, int& position )
{
    scale = 0;
    rotation = 0;
    position = 0;

    Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    for (uint i = 0; i < anim->nChannelBinders(); i++)
    {
        Moo::AnimationChannelPtr channel = anim->channelBinder( i ).channel();

        MF_ASSERT( channel );

        if (channel->type() == 1 || channel->type() == 4)
        {
            Moo::InterpolatedAnimationChannel* iac =
                static_cast<Moo::InterpolatedAnimationChannel*>(&*channel);

            scale += iac->nScaleKeys();
            rotation += iac->nRotationKeys();
            position += iac->nPositionKeys();
        }
    }
}

void PageAnimations::updateCompression()
{
    Moo::AnimationPtr anim = MeApp::instance().mutant()->restoreChannels( selID() );

	if (!anim) return;

    float rErr = 0.f, sErr = 0.f, pErr = 0.f;
    calculateCompressionErrorFactors( sErr, rErr, pErr );

    int osk = 0, opk = 0, ork = 0;
    int tsk = 0, tpk = 0, trk = 0;
    int ask = 0, apk = 0, ark = 0;  // average keys
    int count = 0;

    int compressedSize = 0, origSize = 0;

    for (uint i = 0; i < anim->nChannelBinders(); i++)
    {
        Moo::AnimationChannelPtr channel = anim->channelBinder( i ).channel();
		Moo::NodePtr node = Moo::NodeCatalogue::find( channel->identifier().c_str() );

        MF_ASSERT( channel );

        if (channel->type() == 1 || channel->type() == 4)
        {
            count++;

            Moo::InterpolatedAnimationChannel* iac =
                static_cast<Moo::InterpolatedAnimationChannel*>(&*channel);

            origSize += iac->size();

            uint sk = iac->nScaleKeys();
            uint pk = iac->nPositionKeys();
            uint rk = iac->nRotationKeys();

            osk += sk;
            opk += pk;
            ork += rk;

            iac->reduceScaleKeys( sErr );
            iac->reducePositionKeys( pErr );
			iac->reduceRotationKeys( cosf( rErr ), node );

            // Record the current settings
            iac->scaleCompressionError( sErr );
            iac->positionCompressionError( pErr );
            iac->rotationCompressionError( rErr );

            sk = iac->nScaleKeys();
            pk = iac->nPositionKeys();
            rk = iac->nRotationKeys();

            tsk += sk;
            tpk += pk;
            trk += rk;

            ask += iac->nScaleKeys();
            apk += iac->nPositionKeys();
            ark += iac->nRotationKeys();

            compressedSize += iac->size();
        }
    }

	char buf[256];

	if (opk > 0)
	{
		bw_snprintf( buf, sizeof(buf), "%.0f %%", 100.f*(1.f - ((float)tpk / (float)opk)) );
	}
	else
	{
		buf[0] = 0;
	}
    pImpl_->compPos.SetWindowText( buf );

	if (ork > 0)
	{
		bw_snprintf( buf, sizeof(buf), "%.0f %%", 100.f*(1.f - ((float)trk / (float)ork)) );
    }
	else
	{
		buf[0] = 0;
	}
	pImpl_->compRot.SetWindowText( buf );
	
	if (osk > 0)
	{
		bw_snprintf( buf, sizeof(buf), "%.0f %%", 100.f*(1.f - ((float)tsk / (float)osk)) );
    }
	else
	{
		buf[0] = 0;
	}
	pImpl_->compScale.SetWindowText( buf );

	pImpl_->compTotal.SetWindowText( 
		L("MODELEDITOR/PAGES/PAGE_ANIMATIONS_COMP/COMPRESSED_DATA",
		Formatter( origSize / 1024.f, "%0.f" ), 
		Formatter(compressedSize / 1024.f, "%0.f" ), 
		Formatter( 100.f * (1.f - ((float)compressedSize / (float)origSize )), "%0.f" )));
}

void PageAnimations::OnBnClickedAnimCompPosMinus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compPosSldr.GetPos();
	while (pos > pImpl_->compPosSldr.GetRangeMin())
    {
        pos--;
		pImpl_->compPosSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curPosition != origPosition)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

void PageAnimations::OnBnClickedAnimCompPosPlus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compPosSldr.GetPos();
	while (pos < pImpl_->compPosSldr.GetRangeMax())
    {
        pos++;
		pImpl_->compPosSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curPosition != origPosition)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

void PageAnimations::OnBnClickedAnimCompRotMinus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compRotSldr.GetPos();
	while (pos > pImpl_->compRotSldr.GetRangeMin())
    {
        pos--;
		pImpl_->compRotSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curRotation != origRotation)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

void PageAnimations::OnBnClickedAnimCompRotPlus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compRotSldr.GetPos();
	while (pos < pImpl_->compRotSldr.GetRangeMax())
    {
        pos++;
		pImpl_->compRotSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curRotation != origRotation)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

void PageAnimations::OnBnClickedAnimCompScaleMinus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compScaleSldr.GetPos();
	while (pos > pImpl_->compScaleSldr.GetRangeMin())
    {
        pos--;
		pImpl_->compScaleSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curScale != origScale)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

void PageAnimations::OnBnClickedAnimCompScalePlus()
{
	Moo::AnimationPtr anim = MeApp::instance().mutant()->getMooAnim( selID() );

    if (!anim)
        return;

    int origScale, origRotation, origPosition;
    currentFrames( origScale, origRotation, origPosition );

    pImpl_->updating = true;
	int pos = pImpl_->compScaleSldr.GetPos();
	while (pos < pImpl_->compScaleSldr.GetRangeMax())
    {
        pos++;
		pImpl_->compScaleSldr.SetPos( pos );
        updateCompression();

        int curScale, curRotation, curPosition;
        currentFrames( curScale, curRotation, curPosition );

        if (curScale != origScale)
            break;
    }
	pImpl_->updating = false;
	MeApp::instance().mutant()->reloadModel();
}

bool PageAnimations::canViewAnimCompression()
{
	return true;
}

/*~ function ModelEditor.canViewAnimComp
 *	@components{ modeleditor }
 *
 *	This function checks whether it is possible to view the uncompressed animation.
 *
 *	@return Returns True (1) if it is possible to view the uncompressed animation, False (0) otherwise.
 */
static PyObject * py_canViewAnimComp( PyObject * args )
{
	if (PageAnimations::currPage())
		return PyInt_FromLong( PageAnimations::currPage()->canViewAnimCompression() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( canViewAnimComp, ModelEditor )

bool PageAnimations::canSaveAnimCompression()
{
	return !MeApp::instance().mutant()->isReadOnly();
}

/*~ function ModelEditor.canSaveAnimComp
 *	@components{ modeleditor }
 *
 *	This function checks whether it is possible to save the compressed animation.
 *
 *	@return Returns True (1) if it is possible to save the compressed animation, False (0) otherwise.
 */
static PyObject * py_canSaveAnimComp( PyObject * args )
{
	if (PageAnimations::currPage())
		return PyInt_FromLong( PageAnimations::currPage()->canSaveAnimCompression() );
	return PyInt_FromLong( 0 );
}
PY_MODULE_FUNCTION( canSaveAnimComp, ModelEditor )

void PageAnimations::saveAnimCompression()
{
	if (MeApp::instance().mutant()->animName( selID() ) == "") return;
		
	Moo::AnimationPtr anim = MeApp::instance().mutant()->restoreChannels( selID() );

    float rErr, sErr, pErr;
    calculateCompressionErrorFactors( sErr, rErr, pErr );

    for (uint i = 0; i < anim->nChannelBinders(); i++)
    {
        Moo::AnimationChannelPtr channel = anim->channelBinder( i ).channel();

        MF_ASSERT( channel );

        if (channel->type() == 1 || channel->type() == 4)
        {
            Moo::InterpolatedAnimationChannel* iac =
                static_cast<Moo::InterpolatedAnimationChannel*>(&*channel);

            iac->scaleCompressionError( sErr );
            iac->positionCompressionError( pErr );
            iac->rotationCompressionError( rErr );
        }
    }

    std::string animName = MeApp::instance().mutant()->animFile( selID() );

	if (animName == "")
	{
		ERROR_MSG("Could not locate the animation file, unable to save.\n");
		return;
	}
	
	// save the animation
    anim->save( animName );

    // mark this as the on disk state
    MeApp::instance().mutant()->backupChannels( selID() );

	// Get the compression sliders to echo the new settings
	updateCompression();

	// Reload the model using the new settings
	MeApp::instance().mutant()->reloadModel();
}
/*~ function ModelEditor.saveAnimComp
 *	@components{ modeleditor }
 *
 *	This function saves the compressed animation.
 */
static PyObject * py_saveAnimComp( PyObject * args )
{
	if (PageAnimations::currPage())
		PageAnimations::currPage()->saveAnimCompression();

	Py_Return;
}
PY_MODULE_FUNCTION( saveAnimComp, ModelEditor )
