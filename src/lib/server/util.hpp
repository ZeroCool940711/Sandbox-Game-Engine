/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef UTIL_HPP
#define UTIL_HPP

#include "common.hpp"

#include <stdlib.h>

#include "math/vector2.hpp"
#include "math/vector3.hpp"
#include "network/basictypes.hpp"


/**
 *	This namespace contains simple utility functions.
 */
namespace Util
{
	/**
	 *	This function returns the projection of a Vector3 onto the XZ plane.
	 */
	inline
	Vector2 projectXZ( const Vector3 & pos )
	{
		return Vector2( pos.x, pos.z );
	}


	/**
	 *	This function returns the square of the input value.
	 */
	template <class X>
	inline X sqr( X x )
	{
		return x * x;
	}


	/**
	 *	This returns a random value between the two input values.
	 */
	inline float randomPos( long min, long max )
	{
		return (float)((rand() % (max - min)) + min);
	}

	inline float distSqrBetween(const Vector2 & v1,
			const Vector2 & v2)
	{
		return sqr(v1.x - v2.x) + sqr(v1.y - v2.y);
	}


	/**
	 *	This function returns the square of the distance between the two input
	 *	positions.
	 */
	inline float distSqrBetween(const Vector3 & v1,
			const Vector3 & v2)
	{
		return sqr(v1.x - v2.x) +
			sqr(v1.y - v2.y) +
			sqr(v1.z - v2.z);
	}


	/**
	 *	This function converts from bits per second to a packet size (in bytes
	 *	per	packets).
	 *
	 *	@param bitsPerSecond	The data rate in bits per second that is to be
	 *							converted.
	 *	@param updateHertz		The number of packets sent per second.
	 *
	 *	@return The packet size required to achieve the input data rate.
	 */
	inline
	int bpsToPacketSize( int bitsPerSecond, int updateHertz )
	{
		return (bitsPerSecond / (NETWORK_BITS_PER_BYTE * updateHertz)) -
			UDP_OVERHEAD;
	}


	/**
	 *	This function converts from bits per second to bytes per tick.
	 *
	 *	@param bitsPerSecond	The data rate in bits per second that is to be
	 *							converted.
	 *	@param updateHertz		The number of packets sent per second.
	 *
	 *	@return Byte count
	 */
	inline
	int bpsToBytesPerTick( int bitsPerSecond, int updateHertz )
	{
		return (bitsPerSecond / (NETWORK_BITS_PER_BYTE * updateHertz));
	}


	/**
	 *	This function converts from a packet size (in bytes per packets) to the
	 *	resultant bits per second data rate.
	 *
	 *	@param packetSize		The size of the packets being sent in bytes.
	 *	@param updateHertz		The number of packets sent per second.
	 *
	 *	@return The packet size required to achieve the input data rate.
	 */
	inline
	int packetSizeToBPS( int packetSize, int updateHertz )
	{
		return (packetSize + UDP_OVERHEAD) * updateHertz *
			NETWORK_BITS_PER_BYTE;
	}
}; // namespace Util


#ifdef CODE_INLINE
//#include "util.ipp"
#endif


#endif // UTIL_HPP

/*util.hpp*/
