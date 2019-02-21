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
#include "controls/color_static.hpp"


using namespace controls;


BEGIN_MESSAGE_MAP(ColorStatic, CStatic)
    ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()


/**
 *  Constructor.
 */
ColorStatic::ColorStatic():
	CStatic(),
	m_backBrush(),
	m_textColour(::GetSysColor(COLOR_BTNTEXT)),
	m_backColour(::GetSysColor(COLOR_3DFACE))
{
    m_backBrush.CreateSolidBrush(m_backColour);
}


/**
 *  Destructor.
 */
ColorStatic::~ColorStatic()
{
    m_backBrush.DeleteObject();
}


/**
 *  Set the text colour.
 * 
 *  @param colour       The new text colour.
 */
void ColorStatic::SetTextColour(COLORREF colour)
{
    m_textColour = colour;
    RedrawWindow();
}


/**
 *  Get the text colour.
 * 
 *  @returns            The text colour.
 */
COLORREF ColorStatic::GetTextColour() const
{
    return m_textColour;
}


/**
 *  Set the background colour.
 * 
 *  @param colour       The new background colour.
 */
void ColorStatic::SetBkColour(COLORREF colour)
{
    m_backColour = colour;
    m_backBrush.DeleteObject();
    m_backBrush.CreateSolidBrush(m_backColour);
    RedrawWindow();
}


/**
 *  Get the background colour.
 * 
 *  @returns            The background colour.
 */
COLORREF ColorStatic::GetBkColour() const
{
    return m_backColour;
}


/**
 *  This is called by MFC to set the control's colour.  
 *
 *  @param dc			The dc being drawn on.
 *  @param cltColour	Specifies the type of control.
 *  @returns			A brush handle used to paint the background.
 */
HBRUSH ColorStatic::CtlColor(CDC *dc, UINT /*ctlColour*/)
{
    if (m_backColour != TransparentBackground())
    {
        dc->SetBkColor(m_backColour); 
	    dc->SetTextColor(m_textColour);
        return m_backBrush;
    }
    else
    {
        dc->SetBkMode(TRANSPARENT);
        dc->SetTextColor(m_textColour);
        return (HBRUSH)::GetStockObject(NULL_BRUSH);
    }
}


/**
 *	This is a special colour used to denote a transparent background.
 *
 *  @returns			A colour that can be used to denote a transparent
 *						background colour.
 */
/*static*/ COLORREF ColorStatic::TransparentBackground()
{
    return 0xff000000;
}
