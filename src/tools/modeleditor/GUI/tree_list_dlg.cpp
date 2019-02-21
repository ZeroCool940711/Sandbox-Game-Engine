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

#include "me_app.hpp"

#include "delay_redraw.hpp"

#include "tree_list_dlg.hpp"

#include "resmgr/string_provider.hpp"

DECLARE_DEBUG_COMPONENT( 0 )

// TreeListDlg

IMPLEMENT_DYNCREATE(TreeListDlg, CDialog) 

TreeListDlg::TreeListDlg( UINT dialogID, TreeRoot* treeRoot, const std::string& what ):
	selID_ ("",""),
	selItem_(NULL),
	search_str_ (""),
	ignoreSelChange_ (false),
	CDialog( dialogID ),
	treeRoot_ ( treeRoot ),
	what_(what)
{
}

TreeListDlg::~TreeListDlg()
{
	std::vector<std::string*>::iterator pathDataIt = pathData_.begin();
	std::vector<std::string*>::iterator pathDataEnd = pathData_.end();
	while (pathDataIt != pathDataEnd)
	{
		delete (*pathDataIt);
		pathDataIt++;
	}
	pathData_.clear();
}

void TreeListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_SEARCH_BKG, search_bkg_);
	DDX_Control(pDX, IDC_SEARCH_TEXT, search_);
	DDX_Control(pDX, IDC_SEARCH_BUTTON, search_button_);
	DDX_Control(pDX, IDC_SEARCH_CANCEL, search_cancel_);
	DDX_Control(pDX, IDC_SEARCH_TREE, tree_);
}

BEGIN_MESSAGE_MAP(TreeListDlg, CDialog)

	ON_STN_CLICKED(IDC_SEARCH_BUTTON, OnEnSetFocusSearch)
	ON_EN_CHANGE(IDC_SEARCH_TEXT, OnEnChangeSearch)
	ON_STN_CLICKED(IDC_SEARCH_CANCEL, OnStnClickedCancelSearch)
	
	ON_NOTIFY(TVN_SELCHANGED, IDC_SEARCH_TREE, OnTvnSelChangedTree)

END_MESSAGE_MAP()

void TreeListDlg::OnUpdateTreeList()
{
	if (MeApp::instance().mutant())
	{
		//Save the focus
		CWnd* currFocus = GetFocus();

		//Save the current selection
		StringPair currID = selID_;
		
		//Firstly we need to delete any model path data stored
		std::vector<std::string*>::iterator pathDataIt = pathData_.begin();
		std::vector<std::string*>::iterator pathDataEnd = pathData_.end();
		while (pathDataIt != pathDataEnd)
		{
			delete (*pathDataIt);
			pathDataIt++;
		}
		pathData_.clear();

		//We need to scope the "DelayRedraw" since it interfers with "EnsureVisible"
		{
			DelayRedraw temp( &tree_ );
		
			//Now delete the old tree
			ignoreSelChange_ = true;
			tree_.DeleteAllItems();
			ignoreSelChange_ = false;

			selItem_ = NULL;
			
			for (unsigned m=0; m<treeRoot_->size(); m++)
			{
				HTREEITEM model = tree_.InsertItem( (*treeRoot_)[m].first.first.c_str() );
				
				//Not pretty but needs to be done since it is possible to have multiple
				//model names the same and thus we can't just use a map.
				std::string* fileName = new std::string( (*treeRoot_)[m].first.second );
				tree_.SetItemData( model, (DWORD)fileName );
				pathData_.push_back( fileName );

				tree_.SetItemState( model, TVIS_EXPANDED | TVIS_BOLD, TVIS_EXPANDED | TVIS_BOLD );

				if ((what_ == "MODELEDITOR/ANIMATIONS") && (!MeApp::instance().mutant()->canAnim( *fileName )))
				{
					tree_.InsertItem( L("MODELEDITOR/GUI/TREE_LIST_DLG/NON_ANIMATED"), model );
				}
				else if ((*treeRoot_)[m].second.size() == 0)
				{
					tree_.InsertItem( L("MODELEDITOR/GUI/TREE_LIST_DLG/NO_ANIMATIONS", L( what_.c_str() ) ), model );
				}

				for (unsigned a=0; a<(*treeRoot_)[m].second.size(); a++)
				{
					std::string name = (*treeRoot_)[m].second[a];
					std::string nameLower = name;

					//convert it to lowercase
					std::transform (nameLower.begin(),
						nameLower.end(), nameLower.begin(), tolower);

					if (nameLower.find( search_str_, 0 ) != std::string::npos)
					{
						HTREEITEM anim = tree_.InsertItem( name.c_str(), model );
						if (StringPair( name, *fileName ) == currID)
						{
							selItem_ = anim;
							selID_ = currID;
						}
					}
				}
			}
		}

		if (selItem_)
		{
			tree_.SelectItem( selItem_ );
			tree_.EnsureVisible( selItem_ );
		}
		else
		{
			tree_.SelectItem( tree_.GetFirstVisibleItem() );
			tree_.EnsureVisible( tree_.GetFirstVisibleItem() );
		}

		//Restore the focus
		if (currFocus)
			currFocus->SetFocus();
	}
}

void TreeListDlg::OnTvnSelChangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (ignoreSelChange_) return;
	
	//Ensure that we defocus the current field before making a new selection.
	//We do this to ensure it is updated...
	tree_.SetFocus();

	//Save the item index
	selItem_ = tree_.GetSelectedItem();

	CString itemText = tree_.GetItemText( selItem_ );

	std::string* modelPath = (std::string*)(tree_.GetItemData(selItem_));
	
	//Save the animation ID
	HTREEITEM parent = tree_.GetParentItem(selItem_);
	std::string model = *(std::string*)(tree_.GetItemData(parent ? parent : selItem_));
	selID_ = StringPair( std::string(itemText), model );

	//Inform children of change
	selChange( selID_ );

	*pResult = 0;
}

void TreeListDlg::OnEnChangeSearch()
{
	//First lets get the value entered
	CString search_cstr;
	search_.GetWindowText( search_cstr );
	search_str_ = std::string( search_cstr );

	//convert it to lowercase
	std::transform (search_str_.begin(), search_str_.end(), search_str_.begin(), tolower);

	search_cancel_.ShowWindow( search_str_ != "" );

	OnUpdateTreeList();
}

void TreeListDlg::OnEnSetFocusSearch()
{
	search_.SetFocus();
}

void TreeListDlg::OnStnClickedCancelSearch()
{
	search_.SetWindowText("");
}
