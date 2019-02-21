// -----------------------------------------------------------------------------
// This file contains common shader methods used by the BigWorld terrain engine
// -----------------------------------------------------------------------------

#include "stdinclude.fxh"

// -----------------------------------------------------------------------------
// Terrain vertex format
// -----------------------------------------------------------------------------
struct TerrainVertex
{
	// The height is stored in two values 
	// one for the current lod level and 
	// one for the next one down
	float2	height	: POSITION;

	// the xz coordinate stores the gradient 
	// along the x and z axis of the terrain 
	// block, these values are used to calculate
	// the position of the vertex and projecting 
	// textures on the terrain block
	float2	xz		: TEXCOORD0;
};

struct SpecularInfo
{
	float power;
	float multiplier;
	float fresnelExp;
	float fresnelConstant;
};

// -----------------------------------------------------------------------------
// Constants needed to transform the vertices
// -----------------------------------------------------------------------------

// The world transform of the terrain block
matrix world;

// View projection transform is needed to transform to view space
matrix viewProj;

// The x/z scale of the terrain block
float2 xzScale = { 100.f, 100.f };

// The lod values for this terrain block
float lodStart = 9999.f;
float lodEnd = 10000.f;

// Zero and one so we can saturate without saturate().
// See bug 22135
float zero = 0.0;
float one = 1.0;

// The camera position in world space
float3 cameraPosition;

// The specular blend value
float specularBlend = 0.5;

// Other specular values.
SpecularInfo terrain2Specular =
{ 
	60.0,  	// power
	1.04, 	// multiplier
	4.5,	// fresnelExp
	0.05	// fresnelConstant
};

// -----------------------------------------------------------------------------
// Texture definition helpers
// -----------------------------------------------------------------------------

#define USE_TERRAIN_NORMAL_MAP \
float normalMapSize = 256.f;\
texture normalMap;\
sampler normalMapSampler = sampler_state\
{\
	Texture = (normalMap);\
	ADDRESSU = CLAMP;\
	ADDRESSV = CLAMP;\
	MAGFILTER = LINEAR;\
	MINFILTER = LINEAR;\
	MIPFILTER = NONE;\
};

#define USE_TERRAIN_HOLES_MAP \
float holesMapSize = 128.f;\
float holesSize = 100.f;\
texture holesMap;\
sampler holesMapSampler = sampler_state\
{\
	Texture = (holesMap);\
	ADDRESSU = CLAMP;\
	ADDRESSV = CLAMP;\
	MAGFILTER = POINT;\
	MINFILTER = POINT;\
	MIPFILTER = NONE;\
};

#define USE_TERRAIN_HORIZON_MAP\
float4 rcpPenumbra : PenumbraSize = {0.1,0.1,0.1,0.1};\
float4 sunAngle : SunAngle = {0.5,0.5,0.5,0.5};\
float horizonMapSize = 256.f;\
texture horizonMap;\
sampler horizonMapSampler = sampler_state\
{\
	Texture = (horizonMap);\
	ADDRESSU = CLAMP;\
	ADDRESSV = CLAMP;\
	MAGFILTER = LINEAR;\
	MINFILTER = LINEAR;\
	MIPFILTER = NONE;\
};

#define USE_TERRAIN_BLEND_TEXTURE\
float blendMapSize = 256.f;\
texture blendMap;\
sampler blendMapSampler = sampler_state\
{\
	Texture = (blendMap);\
	ADDRESSU = CLAMP;\
	ADDRESSV = CLAMP;\
	MAGFILTER = LINEAR;\
	MINFILTER = (minMagFilter);\
	MIPFILTER = (mipFilter);\
	MAXANISOTROPY = (maxAnisotropy);\
};

#define TERRAIN_TEXTURE( name )\
float4 name##UProjection;\
float4 name##VProjection;\
texture name;\
sampler name##Sampler = sampler_state\
{\
	Texture = (name);\
	ADDRESSU = WRAP;\
	ADDRESSV = WRAP;\
	MAGFILTER = LINEAR;\
	MINFILTER = (minMagFilter);\
	MIPFILTER = (mipFilter);\
	MAXANISOTROPY = (maxAnisotropy);\
};

// -----------------------------------------------------------------------------
// Helper methods
// -----------------------------------------------------------------------------

// This method calculates the position of this terrain vertex in world space
// It uses the lod start and end values to calculate the geo morphing
// @param vertex the terrain vertex
// @return position in world space
float4 terrainVertexPosition( const TerrainVertex vertex )
{
	// Get the position on the xz plane
	float4 position = mul( float4( vertex.xz.x * xzScale.x, 0, vertex.xz.y * xzScale.y, 1 ), world );
	
	// Calculate the distance from the camera on the xz plane
	float distance = length( position.xz - cameraPosition.xz );
	
	// Calculate the lod value, we linearly interpolate between the two lod distances
	// Avoid using saturate as this generates broken code in shader model 3 (bug 22135)
//	float lod = saturate( (distance - lodStart) / (lodEnd - lodStart) );
	float lod = ( (distance - lodStart) / (lodEnd - lodStart) );
	lod = max( zero, lod );
	lod = min( one, lod );

	// Calculate the new height
	float height = lerp( vertex.height.x, vertex.height.y, lod );
	
	// transform the height and add it to the position
	position.xyz += mul( float3( 0.f, height, 0.f ), world );
	
	return position;
}


// This method calculates the inclusive texture coordinate for the input dimensions
// based on the current xz coordinate. This makes sure the texture coordinates
// go from the centre of the first texel to the centre of the last texel
// @param vertex the terrain vertex
// @param dimensions the integer dimensions of the texture
// @return the uv coordinates
float2 inclusiveTextureCoordinate( const TerrainVertex vertex, const float2 dimensions )
{
	// calculate the scale factor for recalculating the uvs
	float2 scale = float2( float(dimensions.x -1 ) / float( dimensions.x ) , float(dimensions.y -1 ) / float( dimensions.y ) );
	
	// calculate the offset to the middle of the first texel
	float2 offset = float2( 0.5f / float( dimensions.x ), 0.5f / float( dimensions.y ) );
	
	return vertex.xz * scale + offset;
}

// This method gets the normal from the compressed normal map
float3 terrainNormal( sampler normalMapSampler, float2 normalUV )
{
    // Use swizzled normal map : Load X from alpha, Z from Green, and generate Y
    // This assumes input normal is unit length.
    float3 normal;
    normal.xy = tex2D( normalMapSampler, normalUV ).ag * 2 - float2(1,1);
    normal.z  = sqrt( 1 - normal.x*normal.x - normal.y*normal.y ); 

    // Compiler doesn't like initializing normal.xz, so we swap Y and Z
    return normal.xzy;
}

// terrain version of this function has multiplier and variable exponent
float3 terrainDirectionalSpecLight( in float3 normal, in float3 eye, in DirectionalLight light, 
									in float exponent, in float multiplier )
{
	float3 h = normalize(eye + light.direction);
	float att = saturate(dot(normal, h)) * multiplier;
	att = pow(att, exponent);
	return att * light.colour.xyz;
}

// terrain version of this function adds has multiplier material component to the specular
float3 terrainPointSpecLight( in float3 position, in float3 normal, in float3 eye,
	in PointLight light, in float exponent, in float multiplier )
{
	float3 lightDir = light.position - position;
	float lightLen = length( lightDir );
	lightDir /= lightLen;

	float3 h = normalize(eye + lightDir);
	float att = saturate(dot(normal, h)) * multiplier;
	att = pow( att, exponent );
	return saturate((-lightLen + light.attenuation.x) * light.attenuation.y) * att * light.colour.xyz;
}

// These are in defines because the global variables for directional lights need to be
// defined before these functions.
#define USE_TERRAIN_LIGHTING 															\
float3 diffuseLighting( const float3 worldPosition, float3 worldNormal, float shade )	\
{																						\
	float3 colour = float3(0,0,0);														\
	if (nDirectionalLights > 0)															\
	{																					\
		colour += directionalLight( worldNormal, directionalLights[0] ) * shade;		\
	}																					\
	for (int i = 1; i < nDirectionalLights; i++)										\
	{																					\
		colour += directionalLight( worldNormal, directionalLights[i] );				\
	}																					\
	for (int i = 0; i < nPointLights; i++)												\
	{																					\
		colour += pointLight( worldPosition, worldNormal, pointLights[i] );				\
	}																					\
	for (int i = 0; i < nSpotLights; i++)												\
	{																					\
		colour += spotLight( worldPosition, worldNormal, spotLights[i] );				\
	}																					\
	return colour;																		\
}																						\
																						\
float3 specularLighting(in float3 worldPosition, in float3 worldNormal,					\
						in float3 eye, in float shade, in float specularExponent, 		\
						in float specularMultiplier )									\
{																						\
	float3 colour = float3(0,0,0);														\
	if (nSpecularDirectionalLights > 0)													\
	{																					\
		/* material component has been taken out so ps2_0 version may use function. */	\
		colour += terrainDirectionalSpecLight( worldNormal, eye, 						\
			specularDirectionalLights[0], specularExponent, specularMultiplier ) * shade;\
	}																					\
	for (int i = 1; i < nSpecularDirectionalLights; i++)								\
	{																					\
		colour += terrainDirectionalSpecLight( worldNormal, eye, 						\
			specularDirectionalLights[i], specularExponent, specularMultiplier );		\
	}																					\
	for (int i = 0; i < nSpecularPointLights; i++)										\
	{																					\
		colour += terrainPointSpecLight( worldPosition, worldNormal, eye,				\
			specularPointLights[i], specularExponent, specularMultiplier );				\
	}																					\
	return colour;																		\
}\
float3 diffuseLightingSimple( const float3 worldPosition, float3 worldNormal, float shade )	\
{																						\
	float3 colour = float3(0,0,0);														\
	colour += directionalLight( worldNormal, directionalLights[0] ) * shade;			\
	return colour;																		\
}																						\
																						\
float3 specularLightingSimple(in float3 worldPosition, in float3 worldNormal,			\
						in float3 eye, in float shade, in float specularExponent, 		\
						in float specularMultiplier )									\
{																						\
	float3 colour = float3(0,0,0);														\
	/* material component has been taken out so ps2_0 version may use function.	*/		\
	colour += terrainDirectionalSpecLight( worldNormal, eye, 							\
		specularDirectionalLights[0], specularExponent, specularMultiplier ) * shade;	\
	return colour;																		\
}