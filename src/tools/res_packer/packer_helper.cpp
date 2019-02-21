/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "packer_helper.hpp"

#include "resmgr/bwresource.hpp"
#include "resmgr/auto_config.hpp"
#include "cstdmf/debug.hpp"
#ifndef MF_SERVER
#include "moo/init.hpp"
#include "moo/render_context.hpp"
#include "moo/texture_manager.hpp"
#endif // MF_SERVER


DECLARE_DEBUG_COMPONENT( 0 )


int PackerHelper::s_argc_ = 0;
char** PackerHelper::s_argv_ = NULL;
std::string PackerHelper::s_basePath_;
std::string PackerHelper::s_inPath;
std::string PackerHelper::s_outPath;

void PackerHelper::paths( const std::string& inPath, const std::string& outPath )
{
	s_inPath = inPath;
	s_outPath = outPath;
}

const std::string& PackerHelper::inPath()
{
	return s_inPath;
}

const std::string& PackerHelper::outPath()
{
	return s_outPath;
}

bool PackerHelper::initResources()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
	GetModuleFileName( NULL, buffer, sizeof( buffer ) );
	std::string path( buffer );
	SetCurrentDirectory( BWResource::getFilePath( path ).c_str() );
#endif // _WIN32
	if ( !BWResource::init( s_argc_, (const char **)s_argv_ ) )
	{
		printf( "Error: Couldn't initialise BWResource.\n" );
		return false;
	}

	// make sure it searches the basePath as the first path as well
	if ( s_basePath_.empty() )
	{
		ERROR_MSG("Empty base path, can't init resources.\n");
		return false;
	}

	BWResource::addPath( s_basePath_, 0 );

#ifndef MF_SERVER
	// init auto-config strings
	if ( !AutoConfig::configureAllFrom( "resources.xml" ) )
	{
		printf( "Error: Couldn't load auto-config strings from resource.xml\n" );
		return false;
	}
#endif

	return true;
}


#ifndef MF_SERVER
LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	return DefWindowProc( hWnd, msg, wParam, lParam );
}


// AutoConfig and method to get the cursor textures, so we can set them to the
// appropriate format. It has to be done this way to avoid the overkill of
// having to modify the MouseCursor class and including ashes, pyscript,
// python and probably some other libs, all for this simple task.
// Be aware that if the class MouseCursor in src/lib/ashes changes, this
// AutoConfig and method need to be checked and changed if necesary to reflect
// any changes in the tag names and/or the required texture format.
static const AutoConfigString s_packerHelperCursorDefFile(
		"gui/cursorsDefinitions",
		"gui/mouse_cursors.xml" );

void PackerHelper::initTextureFormats()
{
	DataSectionPtr ds = BWResource::openSection( s_packerHelperCursorDefFile );
	if ( !ds )
		return;

	for ( int i = 0; i < ds->countChildren(); ++i )
	{
		Moo::TextureManager::instance()->setFormat(
			ds->openChild( i )->readString( "texture" ),
			D3DFMT_A8R8G8B8 );
	}
}
#endif // MF_SERVER


bool PackerHelper::initMoo()
{
#ifndef MF_SERVER
	// Initialise moo
	Moo::init();

    // Create window to be able to init moo
    WNDCLASS wc = { 0, WndProc, 0, 0, NULL, NULL, NULL, NULL, NULL, "packer_helper" };
    if( !RegisterClass( &wc ) )
	{
		printf( "Error: Couldn't register Moo's window class\n" );
        return false;
	}

	// create the actual window
    HWND hWnd = CreateWindow(
		"packer_helper", "packer_helper",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		100, 100,
		NULL, NULL, NULL, 0 );
	if ( !hWnd )
	{
		printf( "Error: Couldn't create Moo's window\n" );
		return false;
	}

	if ( !Moo::rc().createDevice( hWnd,0,0,true,false,Vector2(0,0),true,true ) )	
	{
		printf( "Error: Couldn't create Moo's render device\n" );
		return false;
	}

	// init textures that require a special texture format
	initTextureFormats();
#endif // MF_SERVER
	return true;
}

bool PackerHelper::copyFile( const std::string& src, const std::string& dst, bool silent )
{
	#define PACKER_COPY_BUF_SIZE 32768	// copy buffer size
	static char buf[ PACKER_COPY_BUF_SIZE ]; // copy buffer

	FILE *fsrc;
	FILE *fdst;
	if ( (fsrc = fopen( src.c_str(), "rb")) == NULL )
	{
		if ( !silent )
			printf( "Error: Cannot open source file %s.\n", src.c_str() );
		return false;
	}

	if ( (fdst = fopen( dst.c_str(), "wb")) == NULL )
	{
		if ( !silent )
			printf( "Error: Cannot open destination file %s.\n", dst.c_str() );
		return false;
	}

	while ( !feof( fsrc ) )
	{
		int bytes = fread( buf, 1, PACKER_COPY_BUF_SIZE, fsrc );
		if ( ferror( fsrc ) )
		{
			if ( !silent )
				printf( "Error: Error reading source file %s.\n", src.c_str() );
			return false;
		}

		if( bytes )
			fwrite( buf, 1, bytes, fdst );

		if( ferror( fdst ) )
		{
			if ( !silent )
				printf( "Error: Error writing destination file %s.\n", dst.c_str() );
			return false;
		}
	}

	fclose( fdst );
	fclose( fsrc );

	return true;
}

bool PackerHelper::fileExists( const std::string& file )
{
	FILE* fh = fopen( file.c_str(), "rb" );
	if ( !fh )
		return false;

	fclose( fh );
	return true;
}

bool PackerHelper::isFileNewer( const std::string& file1, const std::string& file2 )
{
#ifdef _WIN32

	WIN32_FILE_ATTRIBUTE_DATA data1;
	WIN32_FILE_ATTRIBUTE_DATA data2;
	if ( !GetFileAttributesEx( file1.c_str(), GetFileExInfoStandard, &data1 ) ||
		!GetFileAttributesEx( file2.c_str(), GetFileExInfoStandard, &data2 ) ||
		data1.dwFileAttributes == -1 || data2.dwFileAttributes == -1 )
	{
		printf( "Error: Could not query file dates\n" );
		return false;
	}
	return *(uint64*)&data1.ftLastWriteTime > *(uint64*)&data2.ftLastWriteTime;

#else // _WIN32

	// ***TODO***
	return false;

#endif // _WIN32
}
