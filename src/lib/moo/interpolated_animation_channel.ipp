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
	void InterpolatedAnimationChannel::reduceKeyFrames( float angleError, float scaleError, float positionError )
	{
		reduceRotationKeys( angleError );
		reduceScaleKeys( scaleError );
		reducePositionKeys( positionError );
	}

}

/*interpolated_animation_channel.ipp*/
