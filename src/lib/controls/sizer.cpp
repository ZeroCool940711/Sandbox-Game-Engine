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
#include "sizer.hpp"


using namespace controls;


Sizer::Sizer():
    extents_(0, 0, 0, 0)
{
}


Sizer::~Sizer()
{
}


void Sizer::onStart(CWnd *wnd)
{
	CRect rect;
	wnd->GetClientRect(&rect);
	onSize(rect.Width(), rect.Height());
}


CRect Sizer::extents() const
{
    return extents_;
}


void Sizer::onSize(int cx, int cy)
{
    CRect rect(0, 0, cx, cy);
    onSize(rect);
}


/*virtual*/ void Sizer::onSize(CRect const &e)
{
    extents_ = e;
}


void Sizer::onGetMinMaxInfo(MINMAXINFO *mmi)
{
    if (mmi != NULL)
    {
        CSize minSize = minimumSize();
        mmi->ptMinTrackSize.x = minSize.cx;
        mmi->ptMinTrackSize.y = minSize.cy;
    }
}


void Sizer::extents(CRect const &ext)
{
    extents_ = ext;
}


void Sizer::drawRect(CDC *dc, CRect const &rect, COLORREF clr)
{
    if (dc == NULL)
        return;

    CPen pen(PS_SOLID, 1, clr);
    CPen *oldPen = dc->SelectObject(&pen);
    dc->MoveTo(rect.left , rect.top   );
    dc->LineTo(rect.right, rect.top   );
    dc->LineTo(rect.right, rect.bottom);
    dc->LineTo(rect.left , rect.bottom);
    dc->LineTo(rect.left , rect.top   );
    dc->SelectObject(oldPen);
}
