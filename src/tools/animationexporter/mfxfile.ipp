/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/


#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// INLINE void MFXFile::inlineFunction()
// {
// }

INLINE
void MFXFile::writeChunkHeader( ChunkID id, int totalSize, int size )
{
	writeInt( id );
	writeInt( totalSize );
	writeInt( size );
}

INLINE
void MFXFile::writePoint( const Point3 &p )
{
	writeFloat( p.x );
	writeFloat( p.z );
	writeFloat( p.y );
}

INLINE
void MFXFile::writeColour( const Color &c )
{
	writeFloat( c.r );
	writeFloat( c.g );
	writeFloat( c.b );
}

INLINE
void MFXFile::writeUV( const Point3 &uv )
{
	writeFloat( uv.x );
	writeFloat( uv.y );

}

INLINE
void MFXFile::writeMatrix( const Matrix3 &m )
{
	Point3 p = m.GetRow(0);
	writePoint( p );
	p = m.GetRow(2);
	writePoint( p );
	p = m.GetRow(1);
	writePoint( p );
	p = m.GetRow(3);
	writePoint( p );
}

INLINE
void MFXFile::writeInt( int i )
{
//	MF_ASSERT( stream_ );
	fwrite( &i, MFX_INT_SIZE, 1, stream_ );
}

INLINE
void MFXFile::writeFloat( float f )
{
//	MF_ASSERT( stream_ );
	fwrite( &f, MFX_FLOAT_SIZE, 1, stream_ );

}

INLINE
void MFXFile::writeString( const std::string &s )
{
	int length = s.length();

	writeInt( length );
	if( length )
	{
//		MF_ASSERT( stream_ );
		fwrite( s.c_str(), length, 1, stream_ );
	}
}

INLINE
void MFXFile::writeTriangle( unsigned int a, unsigned int b, unsigned int c )
{
	writeInt( a );
	writeInt( c );
	writeInt( b );
}


INLINE
unsigned int MFXFile::getStringSize( const std::string &s )
{
	return s.length() + 4;
}


INLINE
bool MFXFile::readChunkHeader( ChunkID &id, int &totalSize, int &size )
{
	bool good = true;

	nextChunkStart_ = ftell( stream_ );

	good &= readInt( *(int*)&id );
	good &= readInt( totalSize );
	good &= readInt( size );

	nextChunkStart_ += totalSize;

	return good;
}

INLINE
bool MFXFile::readInt( int &i )
{
	return fread( &i, MFX_INT_SIZE, 1, stream_ ) == 1;
}

INLINE
bool MFXFile::readString( std::string &s )
{
	int length;
	if (!readInt( length )) return false;

	if( length )
	{
		char	buf[512];	// done in rcontainer loader
		if (fread( buf, length, 1, stream_ ) != 1) return false;
		s.assign( buf, length );
	}
	else
	{
		s = "";
	}

	return true;
}

INLINE
void MFXFile::nextChunk()
{
	fseek( stream_, nextChunkStart_, SEEK_SET );
}


/*mfx.ipp*/
