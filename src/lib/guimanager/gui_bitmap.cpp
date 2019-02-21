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
#include "gui_bitmap.hpp"
#include "gui_manager.hpp"
#include <algorithm>

BEGIN_GUI_NAMESPACE

HBITMAP clipBitmap( HBITMAP srcBmp, RECT rect )
{
	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( rect.right - rect.left, rect.bottom - rect.top,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
					dcSrc, rect.left, rect.top, SRCCOPY );
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

HBITMAP replaceButtonFace( HBITMAP srcBmp, COLORREF transparent )
{
	COLORREF buttonFace = GetSysColor( COLOR_BTNFACE );
	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( bitmap.bmWidth, bitmap.bmHeight,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
					dcSrc, 0, 0, SRCCOPY );
				for( int i = 0; i < bitmap.bmWidth; ++i )
					for( int j = 0; j < bitmap.bmHeight; ++j )
						if( GetPixel( dcMem, i, j ) == transparent )
							SetPixel( dcMem, i, j, buttonFace );
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

HBITMAP createGrayBitmap( HBITMAP srcBmp, COLORREF transparent )
{
	HBITMAP destBmp = NULL;

	BITMAP bitmap;
	HDC dcMem = NULL, dcSrc = NULL;
	HBITMAP bmpOldSrc = NULL, bmpOldMem = NULL;

	if( ( dcMem = CreateCompatibleDC( NULL ) ) && ( dcSrc = CreateCompatibleDC( NULL ) ) )
	{
		GetObject( srcBmp, sizeof( bitmap ), &bitmap );

		if( ( destBmp = CreateBitmap( bitmap.bmWidth, bitmap.bmHeight,
			bitmap.bmPlanes, bitmap.bmBitsPixel, NULL ) ) )
		{
			bmpOldMem = (HBITMAP)SelectObject( dcMem, destBmp );
			bmpOldSrc = (HBITMAP)SelectObject( dcSrc, srcBmp );

			if( bmpOldMem && bmpOldSrc )
			{
				BitBlt( dcMem, 0, 0, bitmap.bmWidth, bitmap.bmHeight,
					dcSrc, 0, 0, SRCCOPY );
				for( int i = 0; i < bitmap.bmWidth; ++i )
					for( int j = 0; j < bitmap.bmHeight; ++j )
					{
						COLORREF color = GetPixel( dcMem, i, j );
						if( color != transparent )
						{
							double grayScale = GetRValue( color ) * 0.3 +
								GetGValue( color ) * 0.59 +
								GetBValue( color ) * 0.11;
							grayScale = grayScale / 2 + 128;
							color = RGB( grayScale, grayScale, grayScale );
							SetPixel( dcMem, i, j, color );
						}
					}
				SelectObject( dcSrc, bmpOldSrc );
				SelectObject( dcMem, bmpOldMem );
			}
		}
	}

	if( dcMem )	DeleteDC( dcMem );
	if( dcSrc )	DeleteDC( dcSrc );
	return destBmp;
}

Bitmap::Bitmap( const std::string& name, COLORREF transparent,
			   const std::string& type/* = "NORMAL" */,	unsigned int width/* = SIZE_DEFAULT*/,
			   unsigned int height/* = SIZE_DEFAULT */)
			   : type_( type ), defaultSize_( width == SIZE_DEFAULT && height == SIZE_DEFAULT )
{
	RECT rect = { SIZE_DEFAULT, SIZE_DEFAULT, SIZE_DEFAULT, SIZE_DEFAULT };
	std::string bmpName = name.substr( 0, name.find( ':' ) );
	if( name.find( ':' ) != name.npos )
	{
		std::string rectDesc = name.substr( name.find( ':' ) + 1 );
		if( std::count( rectDesc.begin(), rectDesc.end(), ',' ) == 3 )
		{
			std::string::size_type second = rectDesc.find( ',' );
			std::string::size_type third = rectDesc.find( ',', second + 1 );
			std::string::size_type fourth = rectDesc.find( ',', third + 1 );
			rect.left = atoi( rectDesc.c_str() );
			rect.top = atoi( rectDesc.c_str() + second + 1 );
			rect.right = atoi( rectDesc.c_str() + third + 1 );
			rect.bottom = atoi( rectDesc.c_str() + fourth + 1 );
		}
	}
	bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), bmpName.c_str(), IMAGE_BITMAP,
		width == SIZE_DEFAULT	?	0	:	width,
		height == SIZE_DEFAULT	?	0	:	height,
		0 );
	if( !bitmap_ )
	{
		if( !bmpName.empty() && bmpName[0] == '#' )
		{
			bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), 
				MAKEINTRESOURCE( atoi( bmpName.c_str() + 1 ) ), IMAGE_BITMAP,
				width == SIZE_DEFAULT	?	0	:	width,
				height == SIZE_DEFAULT	?	0	:	height,
				0 );			
		}
	}
	if( !bitmap_ )
	{
		std::string fileName = bmpName;
		if( GUI::Manager::resolveFileName( fileName ) )
			bitmap_ = (HBITMAP)LoadImage( GetModuleHandle( NULL ), fileName.c_str(), IMAGE_BITMAP,
				width == SIZE_DEFAULT	?	0	:	width,
				height == SIZE_DEFAULT	?	0	:	height,
				LR_LOADFROMFILE );
	}
	if( !bitmap_ )
		0;	//	throw 1;
	if( rect.left != SIZE_DEFAULT )
	{
		HBITMAP temp = clipBitmap( bitmap_, rect );
		DeleteObject( bitmap_ );
		bitmap_ = temp;
	}
	if( type_ == "NORMAL" )
	{
	}
	else if( type_ == "HIGHLIGHT" )
	{
	}
	else if( type_ == "DISABLED" )
	{
		HBITMAP temp = createGrayBitmap( bitmap_, transparent );
		DeleteObject( bitmap_ );
		bitmap_ = temp;
	}
	HBITMAP temp = replaceButtonFace( bitmap_, transparent );
	DeleteObject( bitmap_ );
	bitmap_ = temp;
}

Bitmap::~Bitmap()
{
	DeleteObject( bitmap_ );
}

const std::string& Bitmap::type() const
{
	return type_;
}

unsigned int Bitmap::width() const
{
	BITMAP bitmap;
	GetObject( bitmap_, sizeof( bitmap ), &bitmap );
	return bitmap.bmWidth;
}

unsigned int Bitmap::height() const
{
	BITMAP bitmap;
	GetObject( bitmap_, sizeof( bitmap ), &bitmap );
	return bitmap.bmHeight;
}

bool Bitmap::defaultSize() const
{
	return defaultSize_;
}

Bitmap::operator HBITMAP()
{
	return bitmap_;
}

BitmapPtr BitmapManager::find( const std::string& name, const std::string& type/* = "NORMAL" */,
	unsigned int width/* = SIZE_DEFAULT*/, unsigned int height/* = SIZE_DEFAULT */)
{
	BitmapPtr result( NULL );
	if( bitmaps_.find( name ) != bitmaps_.end() )
	{
		std::vector<BitmapPtr>& bitmaps = bitmaps_[ name ];
		bool isDefault = ( width == SIZE_DEFAULT && height == SIZE_DEFAULT );
		for( std::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter)->type() == type )
			{
				if( (*iter)->defaultSize() && isDefault ||
					(*iter)->width() == width && (*iter)->height() == height )
				{
					result = (*iter);
					break;
				}
			}
		}
	}
	return result;
}

BitmapPtr BitmapManager::get( const std::string& name, COLORREF transparent,
	const std::string& type/* = "NORMAL" */, unsigned int width/* = SIZE_DEFAULT*/, unsigned int height/* = SIZE_DEFAULT */)
{
	BitmapPtr result( NULL );
	if( bitmaps_.find( name ) != bitmaps_.end() )
	{
		std::vector<BitmapPtr>& bitmaps = bitmaps_[ name ];
		bool isDefault = ( width == SIZE_DEFAULT && height == SIZE_DEFAULT );
		for( std::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter)->type() == type )
			{
				if( (*iter)->defaultSize() && isDefault ||
					(*iter)->width() == width && (*iter)->height() == height )
				{
					result = (*iter);
					break;
				}
			}
		}
	}
	if( !result )
	{
		try
		{
			result = new Bitmap( name, transparent, type, width, height );
		}
		catch(...)//TODO: refine exceptions
		{}
	}
	if( result )
		bitmaps_[ name ].push_back( result );

	return result;
}

void BitmapManager::free( BitmapPtr& bitmap )
{
	for( std::map<std::string, std::vector<BitmapPtr> >::iterator miter = bitmaps_.begin();
		miter != bitmaps_.end(); ++miter )
	{
		std::vector<BitmapPtr>& bitmaps = miter->second;
		for( std::vector<BitmapPtr>::iterator iter = bitmaps.begin(); iter != bitmaps.end();
			++iter )
		{
			if( (*iter) == bitmap )
			{
				bitmap = NULL;
				if( (*iter)->refCount() == 1 )
				{
					bitmaps.erase( iter );
					return;
				}
			}
		}
	}
}

void BitmapManager::clear()
{
	bitmaps_.clear();
}


END_GUI_NAMESPACE
