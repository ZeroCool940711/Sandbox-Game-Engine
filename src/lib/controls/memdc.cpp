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
#include "controls/memdc.hpp"


using namespace controls;


/**
 *	This is the MemDC constructor.
 */
MemDC::MemDC(): 
	CDC(),
	m_oldBitmap(NULL),
	m_pDC(NULL),
	m_rect(0, 0, 0, 0),
	m_bMemDC(TRUE)
{
}


/**
 *	This is the MemDC destructor.
 */
MemDC::~MemDC()
{
	cleanup();
}


/**
 *	This begins using the MemDC for drawing.
 *
 *  @param dc		The device context to use.
 *  @param pRect	The update rectangle.  If NULL then this is the entire 
 *					clipping area (usually the entire window that the DC is 
 *					being done on).
 */
void MemDC::begin(CDC &dc, const CRect* pRect /*= NULL*/)
{
	// Get the rectangle to draw
	CRect rect;
	if (pRect == NULL) 
		 dc.GetClipBox(&rect);
	else 
		 rect = *pRect;

	m_pDC = &dc;

	if (rect != m_rect || m_bMemDC != !dc.IsPrinting())
	{
		cleanup();
		m_bMemDC = !dc.IsPrinting();
		m_rect   = rect;
		if (m_bMemDC) 
		{
			// Create a Memory DC
			CreateCompatibleDC(&dc);
			dc.LPtoDP(&m_rect);
			m_bitmap.CreateCompatibleBitmap
			(
				&dc, 
				m_rect.Width(), m_rect.Height()
			);
		} 
	}
	if (m_bMemDC != FALSE)
	{		
		m_oldBitmap = SelectObject(&m_bitmap);
		SetMapMode(dc.GetMapMode());
		SetWindowExt(dc.GetWindowExt());
		SetViewportExt(dc.GetViewportExt());
		dc.DPtoLP(&m_rect);
		SetWindowOrg(m_rect.left, m_rect.top);
	}
	else
	{
		 // Make a copy of the relevent parts of the current 
		 // DC for printing
		 m_bPrinting = dc.m_bPrinting;
		 m_hDC       = dc.m_hDC;
		 m_hAttribDC = dc.m_hAttribDC;		
	}

	// Fill background 
	FillSolidRect(m_rect, m_pDC->GetBkColor());
}
	   

/**
 *	Stop drawing onto the MemDC and blit it to the DC set in the call to
 *	begin.
 */
void MemDC::end()     
{          
	if (m_bMemDC != FALSE) 
	{		
		// Copy the offscreen bitmap onto the screen.
		m_pDC->BitBlt
		(
			m_rect.left, m_rect.top, 
			m_rect.Width(),  m_rect.Height(),
			this, 
			m_rect.left, m_rect.top, 
			SRCCOPY
		);            
		SelectObject(m_oldBitmap);        
	} 
	else 
	{
		 // All we need to do is replace the DC with an illegal
		 // value, this keeps us from accidentally deleting the 
		 // handles associated with the CDC that was passed to 
		 // the constructor.              
		 m_hDC = m_hAttribDC = NULL;
	}       
}


/**
 *	This cleans up memory usage of the MemDC.
 */
void MemDC::cleanup()
{
	HGDIOBJ bmp = m_bitmap.Detach();
	if (bmp != NULL) 
		::DeleteObject(bmp);
	HDC hdc = Detach();
	if (hdc != NULL)
		::DeleteDC(hdc);
}


/**
 *	This is the MemDCScope contructor.  It calls MemDC::begin.
 *
 *	@param memDC		The MemDC that this object is operating on.
 *  @param dc			The dc that the MemDC should be using.
 *	@param rect			The (optional) update rectangle. 
 */
MemDCScope::MemDCScope(MemDC &memDC, CDC &dc, const CRect *rect /*= NULL*/):
	memDC_(&memDC)
{
	memDC_->begin(dc, rect);
}


/**
 *	This is the MemDCScope destructor.
 */
MemDCScope::~MemDCScope()
{
	memDC_->end();
}
