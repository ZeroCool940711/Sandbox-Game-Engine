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
#include "gui/controls/drag_listbox.hpp"
#include "common/user_messages.hpp"


BEGIN_MESSAGE_MAP(DragListBox, CListBox)
    ON_WM_LBUTTONDOWN()
    ON_WM_MOUSEMOVE()
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
END_MESSAGE_MAP()


DragListBox::DragListBox()
:
CListBox(),
dragIndex_(LB_ERR),
tooltipCB_(NULL),
data_(NULL)
{
}


void 
DragListBox::setTooltipCallback
(
    ToolTipCB   cb, 
    void        *data   /*= NULL*/
)
{
    tooltipCB_  = cb;
    data_       = data;
}


/*virtual*/ void DragListBox::PreSubclassWindow()
{
    CListBox::PreSubclassWindow();
    EnableToolTips(TRUE);
}


/*virtual*/ int DragListBox::OnToolHitTest(CPoint point, TOOLINFO *ti) const
{
    int row;
    RECT cellRect; // cellrect - to hold the bounding rect
    BOOL tmp = FALSE;
    row = ItemFromPoint(point, tmp); 
    if (row == -1)
        return -1;

    GetItemRect(row, &cellRect);
    ti->rect     = cellRect;
    ti->hwnd     = m_hWnd;
    ti->uId      = (UINT)((row)); 
    ti->lpszText = LPSTR_TEXTCALLBACK;
    return ti->uId;
}


BOOL DragListBox::OnToolTipText(UINT id, NMHDR *pNMHDR, LRESULT *result)
{
    // Need to handle both ANSI and UNICODE versions of the message:
    TOOLTIPTEXTA *pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW *pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    std::string strTipText;
    unsigned int selection = pNMHDR->idFrom;  //list box item index 
    if (tooltipCB_ != NULL)
        tooltipCB_(selection, strTipText, data_);
    else
        return FALSE;

    // Display item text as tool tip:
#ifndef _UNICODE
    if (pNMHDR->code == TTN_NEEDTEXTA)
        lstrcpyn(pTTTA->szText, strTipText.c_str(), 80);
else
    _mbstowcsz(pTTTW->szText, strTipText.c_str(), 80);
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
        _wcstombsz(pTTTA->szText, strTipText.c_str(), 80);
    else
        lstrcpyn(pTTTW->szText, strTipText.c_str(), 80);
    #endif
    *result = 0;

    return TRUE;
}


void DragListBox::OnLButtonDown(UINT flags, CPoint point)
{
    CListBox::OnLButtonDown(flags, point);

    dragIndex_ = LB_ERR;
    BOOL outside;
    UINT index = ItemFromPoint(point, outside);
    if (index != LB_ERR && outside == FALSE)
    {
        dragIndex_ = index;
        SetSel(index);
        // Inform the parent about the change in selection:
        CWnd *parent = GetParent();
        if (parent != NULL)
        {
            parent->SendMessage
            (
                WM_COMMAND, 
                MAKEWPARAM(GetDlgCtrlID(), LBN_SELCHANGE), 
                (LPARAM)((HWND)*this)
            );
        }
    }
}


void DragListBox::OnMouseMove(UINT flags, CPoint point)
{
    if (dragIndex_ != LB_ERR && (flags & MK_LBUTTON) != 0)
    {
        // Get the selection string:
        int selIdx = GetCurSel();
        if (selIdx < 0)
            return;
        CString selText;
        GetText(selIdx, selText);

        // Create a chunk of memory that stores the selection as a string.
        int sz = selText.GetLength() + 1;
        HGLOBAL hdata = ::GlobalAlloc(GMEM_MOVEABLE, sz);
        char *cdata = (char *)::GlobalLock(hdata);
        strncpy(cdata, selText.GetBuffer(), sz);
        ::GlobalUnlock(hdata);

        // Do the drag-and-drop:
        if (hdata != NULL)
        {
            CWnd *parent = GetParent();
            if (parent != NULL)
            {
                parent->SendMessage(WM_DRAG_START, 0, 0);
            }
            COleDataSource dataSource;
            dataSource.CacheGlobalData(CF_TEXT, hdata);
            dataSource.DoDragDrop(DROPEFFECT_COPY);            
            if (parent != NULL)
            {
                parent->SendMessage(WM_DRAG_DONE, 0, 0);
            }
        }

        dragIndex_ = LB_ERR;
    }
}
