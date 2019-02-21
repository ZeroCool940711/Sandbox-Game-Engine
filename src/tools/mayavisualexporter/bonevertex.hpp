/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef __bonevertex_hpp__
#define __bonevertex_hpp__

#include "vector3.hpp"

typedef vector3<float> Point3;

struct BoneVertex
{
	BoneVertex( const Point3& position = Point3( 0, 0, 0 ), int index1 = 0, int index2 = 0, int index3 = 0, float weight1 = 0, float weight2 = 0, float weight3 = 0 );
	
	inline void addWeight( int index, float weight )
	{
		// set it to an weight of 0 if possible
		if( weight1 == 0 )
		{
			index1 = index;
			weight1 = weight;
			return;
		}
		
		if( weight2 == 0 )
		{
			index2 = index;
			weight2 = weight;
			return;
		}

		if( weight3 == 0 )
		{
			index3 = index;
			weight3 = weight;
			return;
		}
		
		// else if the weight is > than the smallest weight, replace that weight
		if( weight > weight1 && weight1 < weight2 && weight1 < weight3 )
		{
			index1 = index;
			weight1 = weight;
			return;
		}
		
		if( weight > weight2 && weight2 < weight3 )
		{
			index2 = index;
			weight2 = weight;
			return;
		}

		if( weight > weight3 )
		{
			index3 = index;
			weight3 = weight;
			return;
		}
	}
	
	inline void sortWeights()
	{
		int tempindex;
		float tempweight;
		
		int* indexes = &index1;
		float* weights = &weight1;
		
		for( uint32 i = 0; i < 3; ++i )
			for( uint32 j = i; j < 3; ++j )
				if( weights[i] < weights[j] )
				{
					tempindex = indexes[i];
					indexes[i] = indexes[j];
					indexes[j] = tempindex;
					
					tempweight = weights[i];
					weights[i] = weights[j];
					weights[j] = tempweight;
				}
	}
	
	inline void normaliseWeights()
	{
		float total = weight1 + weight2 + weight3;
		
		if( total > 0 )
		{
			weight1 /= total;
			weight2 /= total;
			weight3 /= total;
		}
	}
	
	Point3 position;
	int index1;
	int index2;
	int index3;
	float weight1;
	float weight2;
	float weight3;
};

#endif // __bonevertex_hpp__