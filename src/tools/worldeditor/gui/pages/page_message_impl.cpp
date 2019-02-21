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
#include "worldeditor/framework/world_editor_app.hpp"
#include "worldeditor/gui/pages/page_message_impl.hpp"
#include "worldeditor/world/world_manager.hpp"
#include "worldeditor/misc/world_editor_camera.hpp"
#include "worldeditor/scripting/we_python_adapter.hpp"
#include "chunk/chunk.hpp"
#include "chunk/chunk_item.hpp"


BBMsgImpl::BBMsgImpl( PageMessages* msgs ):
	msgs_( msgs ), boldFont( NULL )
{
	CFont* font = msgs_->msgList_.GetFont();
	if (font)
	{
		// now, get the complete LOGFONT describing this font
		LOGFONT lf;
		if (font->GetLogFont( &lf ))
		{
			// set the weight to bold
			lf.lfWeight = FW_BOLD;

			// recreate this font with just the weight changed
			boldFont = CreateFontIndirect(&lf);
		}
	}
}

BBMsgImpl::~BBMsgImpl()
{
	if( boldFont )
		DeleteObject( boldFont );
}

void BBMsgImpl::OnNMClickMsgList(NMHDR *pNMHDR, LRESULT *pResult)
{
	CPoint point;
	GetCursorPos( &point );

	msgs_->msgList_.ScreenToClient( &point ); 

	UINT index = msgs_->msgList_.HitTest( point );

	CRect rect;
	msgs_->msgList_.GetItemRect( index, rect, 0 );
	
	BWMessageInfo* clicked = NULL;
	if ((index != (uint16)(-1)) && (rect.PtInRect( point )))
	{
		clicked = (BWMessageInfo*)(msgs_->msgList_.GetItemData( index ));
	}
	
	if (clicked)
	{
		// move the view to where the chunk or item is
		if (clicked->item() && clicked->item()->chunk())
		{
			// move and zoom the camera to view the item
			WorldEditorCamera::instance().zoomExtents( clicked->item(), 4.f );
			std::vector<ChunkItemPtr> chunkItems;
			chunkItems.push_back( clicked->item() );
			WorldManager::instance().setSelection( chunkItems );

			// change to object mode (bit hacky)
			if (WorldEditorApp::instance().pythonAdapter())
			{
				WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", "Object");
			}

		}
		else if (clicked->chunk())
		{
			const Vector3 worldPosition = clicked->chunk()->centre();

			// Set the view matrix to the new world coords
			Matrix view = WorldEditorCamera::instance().currentCamera().view();
			view.setTranslate( worldPosition + Vector3( 0.f, 50.f, 0.f ) );
			view.preRotateX( DEG_TO_RAD( 30.f ) );
			view.invert();
			WorldEditorCamera::instance().currentCamera().view( view );

			// change to object mode (bit hacky)
			if (WorldEditorApp::instance().pythonAdapter())
			{
				WorldEditorApp::instance().pythonAdapter()->onPageControlTabSelect("pgc", "Object");
			}
		}
		else if (clicked->stackTrace() != NULL)
		{
			::MessageBox( AfxGetApp()->m_pMainWnd->GetSafeHwnd(),
				clicked->stackTrace(), L("WORLDEDITOR/GUI/PAGE_MESSAGE_IMPL/SCRIPT_ERROR"), MB_OK | MB_TASKMODAL );
		}
		else if (clicked->priority() == MESSAGE_PRIORITY_ASSET)
		{
			msgs_->expanded_[ clicked->msgStr() ] = ! msgs_->expanded_[ clicked->msgStr() ];
			msgs_->selected_ = std::string( clicked->msgStr() );
			msgs_->redrawList();
		}
	}


	*pResult = 0;
}

void BBMsgImpl::OnNMCustomdrawMsgList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	if ( CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if ( CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage )
	{
		std::map< std::string, int >::iterator it = msgs_->index_.begin();
		std::map< std::string, int >::iterator end = msgs_->index_.end();
		for (; it != end; ++it)
		{
			if ((*it).second == pLVCD->nmcd.dwItemSpec + 1)
			{
				HDC dc		= pLVCD->nmcd.hdc;
				HFONT old = (HFONT)SelectObject( dc, boldFont );
									
				*pResult = CDRF_NEWFONT;
			}
		}
	}
}