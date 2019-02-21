/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// ModelEditorView.cpp : implementation of the CModelEditorView class
//

#include "pch.hpp"
#include "model_editor.h"
#include "main_frm.h"

#include "model_editor_doc.h"
#include "model_editor_view.h"

#include "common/cooperative_moo.hpp"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DECLARE_DEBUG_COMPONENT2( "ModelEditor", 0 )


// CModelEditorView

IMPLEMENT_DYNCREATE(CModelEditorView, CView)

BEGIN_MESSAGE_MAP(CModelEditorView, CView)

	ON_WM_SIZE()

	ON_WM_PAINT()

END_MESSAGE_MAP()

// CModelEditorView construction/destruction

CModelEditorView::CModelEditorView() :
	lastRect_( 0, 0, 0, 0 )
{
	// TODO: add construction code here

}

CModelEditorView::~CModelEditorView()
{
}

BOOL CModelEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
   cs.lpszClass = AfxRegisterWndClass(
	   CS_OWNDC | CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), 0 );
   cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
   cs.style &= ~WS_BORDER;

   return CView::PreCreateWindow(cs);
} 

void CModelEditorView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize( nType, cx, cy );

//	No longer changing Moo mode here, since it's too slow
	//if ((cx > 0 && cy > 0) && Moo::rc().device() && Moo::rc().windowed())
	//{
	//	Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed(), true);
	//}
}

void CModelEditorView::OnPaint()
{
	CView::OnPaint();

	CRect rect;
	GetClientRect( &rect );

	if ( CModelEditorApp::instance().initDone() )
	{
		if ( CooperativeMoo::beginOnPaint() )
		{		
			// changing mode when a paint message is received and the size of the
			// window is different than last stored size.
			if ( lastRect_ != rect &&
				Moo::rc().device() && Moo::rc().windowed() &&
				rect.Width() && rect.Height() &&
				!((CMainFrame*)CModelEditorApp::instance().mainWnd())->resizing() )
			{
				CWaitCursor wait;
				Moo::rc().changeMode(Moo::rc().modeIndex(), Moo::rc().windowed(), true);
				lastRect_ = rect;
			}

			CModelEditorApp::instance().mfApp()->updateFrame( false );

			CooperativeMoo::endOnPaint();
		}
	}
	else
	{
		CWindowDC dc( this );

		dc.FillSolidRect( rect, ::GetSysColor( COLOR_BTNFACE ) );
	}
} 

// CModelEditorView drawing

void CModelEditorView::OnDraw(CDC* /*pDC*/)
{
	CModelEditorDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
}


// CModelEditorView diagnostics

#ifdef _DEBUG
void CModelEditorView::AssertValid() const
{
	CView::AssertValid();
}

void CModelEditorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CModelEditorDoc* CModelEditorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CModelEditorDoc)));
	return (CModelEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CModelEditorView message handlers
