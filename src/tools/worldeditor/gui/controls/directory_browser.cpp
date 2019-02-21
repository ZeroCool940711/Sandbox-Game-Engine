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
#include "worldeditor/gui/controls/directory_browser.hpp"


IMPLEMENT_DYNAMIC(DirectoryBrowser, CListBox)

DirectoryBrowser::DirectoryBrowser()
: currentPath_("")
, startDirectory_("")
{
}

DirectoryBrowser::~DirectoryBrowser()
{
}

namespace
{

class SmartTruncator
{
	CFont* saveFont_;
	CDC* dc_;
	CWnd* wnd_;
	int maxWidth_;
	int currentMax_;
	static int const RIGHT_MARGIN = 10;// give the user some space to live
public:
	SmartTruncator( CDC* dc, CWnd* wnd ) : dc_( dc ), wnd_( wnd )
	{
		RECT rect;
		wnd_->GetClientRect( &rect );
		maxWidth_ = rect.right;
		currentMax_ = 0;
		saveFont_ = dc->SelectObject( wnd_->GetFont() );
	}
	int getWidth()
	{
		return ( currentMax_ + RIGHT_MARGIN < maxWidth_ )	?	currentMax_ + RIGHT_MARGIN	:
																maxWidth_;
	}
	CString feedFileName( CString fileName )
	{
		CString s = fileName;
		CSize size = dc_->GetOutputTextExtent( fileName );
		if( size.cx > currentMax_ )
			currentMax_ = size.cx;
		if( size.cx > maxWidth_ - RIGHT_MARGIN )
		{
			while( s.GetLength() )
			{
				s.Delete( s.GetLength() - 1 );
				fileName = s + "....";// one more dot for border :)
				if( dc_->GetOutputTextExtent( fileName ).cx < maxWidth_ - RIGHT_MARGIN )
				{
					fileName = s + "...";
					break;
				}
			}
		}
		return fileName;
	}
	~SmartTruncator()
	{
		dc_->SelectObject( saveFont_ );
	}
};

}

void DirectoryBrowser::fill()
{
	MF_ASSERT(GetParent());
	GetParent()->SendMessage(WM_CHANGE_DIRECTORY, 0, (LPARAM)currentPath_.GetBuffer());

	SetRedraw(FALSE);
	ResetContent();

	SmartTruncator SmartTruncator( GetDC(), this );
	mFileNameIndex.RemoveAll();
	mFileNameIndex.Add( "" );// guard from 1

	if (currentPath_.IsEmpty())
	{
		if (basePaths_.size() == 1)
		{
			currentPath_ = *basePaths_.begin();
		}
		else
		{
			// at the base level (no hierarchy)
			for (std::vector<CString>::iterator it = basePaths_.begin();
				it != basePaths_.end();
				it++)
			{
				CString newFileName = SmartTruncator.feedFileName( *it );
				if( newFileName != *it )
				{
					int index = mFileNameIndex.GetSize();
					mFileNameIndex.Add( *it );
					SetItemData( AddString( newFileName ), index );
				}
				else
				{
					SetItemData( AddString( *it ), 0 );
				}
			}
			SetColumnWidth( SmartTruncator.getWidth() );

			SetRedraw();
			InvalidateRect(NULL);
			return;
		}
	}

	CString searchString = currentPath_ + "*.*";
	CFileFind finder;
	if ( !finder.FindFile(searchString) )
	{
		currentPath_ = "";
		fill();
		return;
	}

	BOOL moreFiles = TRUE;
	while ( moreFiles )
	{
		moreFiles = finder.FindNextFile();

		if ( finder.IsDirectory() )
		{
			CString filename = finder.GetFileName();
			if ( filename == "." || filename == "CVS" || filename == ".bwthumbs" ) 
				continue;

			CString newFileName = SmartTruncator.feedFileName( filename );
			if( newFileName != filename )
			{
				int index = mFileNameIndex.GetSize();
				mFileNameIndex.Add( filename );
				SetItemData( AddString( newFileName ), index );
			}
			else
			{
				SetItemData( AddString(filename), 0 );
			}
		}
	}

	SetColumnWidth( SmartTruncator.getWidth() );

	SetRedraw();
	InvalidateRect(NULL);
}

void DirectoryBrowser::initialise(const std::vector<CString>& basePaths)
{
	MF_ASSERT(basePaths_.empty());

	for (std::vector<CString>::const_iterator it = basePaths.begin();
		it != basePaths.end();
		it++)
	{
		CString path = *it;

		// convert to \ and ensure there is a \ at the end
		path.Replace('/', '\\');
		if(path.Right(1)!="\\")
			path += "\\";

		basePaths_.push_back(path);
	}


	for (std::vector<CString>::const_iterator it = basePaths_.begin();
		it != basePaths_.end();
		it++)
	{
		if (startDirectory_.Find(*it) != -1)
		{
			currentPath_ = startDirectory_;
			break;
		}
	}

	fill();
}

void DirectoryBrowser::setStartDirectory(const CString& directory)
{
	startDirectory_ = directory;
	startDirectory_.Replace('/', '\\');
}


BEGIN_MESSAGE_MAP(DirectoryBrowser, CListBox)
	//{{AFX_MSG_MAP(DirectoryBrowser)
	ON_CONTROL_REFLECT(LBN_SELCHANGE, OnSectionChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// DirectoryBrowser message handlers


void DirectoryBrowser::OnSectionChange() 
{
	if (GetCurSel() == -1)
		return;

	CString selection;
	GetText(GetCurSel(), selection);

	// Xiaoming Shi : long file names friendly, for bug 4695{
	if( int itemData = GetItemData( GetCurSel() ) )
	{
		selection = mFileNameIndex[ itemData ];
	}
	// Xiaoming Shi : long file names friendly, for bug 4695}
	if(selection == "..")
	{
		if (std::find(basePaths_.begin(), basePaths_.end(), currentPath_) == basePaths_.end())
		{
			currentPath_ = currentPath_.Left(currentPath_.ReverseFind('\\'));
			currentPath_ = currentPath_.Left(currentPath_.ReverseFind('\\'));
			currentPath_ += '\\';
		}
		else
		{
			currentPath_ = "";
		}
	}
	else
	{
		//otherwise add another sub-dir to the path
		currentPath_ += selection;

		if(currentPath_.Right(1)!="\\")
			currentPath_ += "\\";
	}

	fill();
}
