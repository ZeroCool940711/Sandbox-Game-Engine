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

namespace Moo
{

INLINE
const std::string& Vertices::resourceID( ) const
{
	return resourceID_;
}

INLINE
void Vertices::resourceID( const std::string& resourceID )
{
	resourceID_ = resourceID;
}

INLINE
uint32 Vertices::nVertices( ) const
{
	return nVertices_;
}

INLINE
const std::string& Vertices::format( ) const
{
	return format_;
}

INLINE
Moo::VertexBuffer Vertices::vertexBuffer( ) const
{
	return vertexBuffer_;
}

}

/*vertices.ipp*/