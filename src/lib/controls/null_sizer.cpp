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
#include "null_sizer.hpp"


using namespace controls;


/**
 *  This is the NullSizer constructor.
 *
 *  @param minWidth     The minimum width.
 *  @param minHeight    The minimum height.
 */
NullSizer::NullSizer(uint32 minWidth, uint32 minHeight):
    minWidth_(minWidth),
    minHeight_(minHeight)
{
    extents(CRect(0, 0, minWidth, minHeight));
}


/**
 *  This is called to fit the NullSizer into a rectangle.
 *
 *  @param extents      The area that the window should be fitted into.
 */
/*virtual*/ void NullSizer::onSize(CRect const &e)
{
    Sizer::onSize(e);
}


/**
 *  This gets the minimum size of the NullSizer.
 *
 *  @returns            The size of the window when the NullSizer was 
 *                      constructed.
 */
/*virtual*/ CSize NullSizer::minimumSize() const
{
    return CSize(minWidth_, minHeight_);
}


/**
 *  This gets called to draw the NullSizer for debugging.
 *
 *  @param dc       The device context to draw into.
 */
/*virtual*/ void NullSizer::draw(CDC *dc)
{
    drawRect(dc, extents(), RGB(0, 255, 0));
}
