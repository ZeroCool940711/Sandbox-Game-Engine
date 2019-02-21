/**
 *	This include file has all of the vertex declarations that are passed between vertex and pixel shader.
 */

//---------------------------------------
// Vertex input definitions
//---------------------------------------

struct VertexXYZNUVIIIWWTB
{
   float4 pos:		POSITION;
   float3 indices:	BLENDINDICES;
   float2 weights:	BLENDWEIGHT;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
};

//note - this is not used currently
struct VertexXYZNUVIIIWWTB_D
{
   float4 pos:		POSITION;
   float3 indices:	BLENDINDICES;
   float2 weights:	BLENDWEIGHT;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZNUVIIIWW
{
   float4 pos:		POSITION;
   float3 indices:	BLENDINDICES;
   float2 weights:	BLENDWEIGHT;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
};

//note - this is not used currently
struct VertexXYZNUVIIIWW_D
{
   float4 pos:		POSITION;
   float3 indices:	BLENDINDICES;
   float2 weights:	BLENDWEIGHT;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZNUVITB
{
   float4 pos:		POSITION;
   float  index:	BLENDINDICES;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
};


//note - this is not used currently
struct VertexXYZNUVITB_D
{
   float4 pos:		POSITION;
   float  index:	BLENDINDICES;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZNUVI
{
   float4 pos:		POSITION;
   float   index:	BLENDINDICES;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
};

//note - this is not used currently
struct VertexXYZNUVI_D
{
   float4 pos:		POSITION;
   float   index:	BLENDINDICES;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZNUVTB
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
};


struct VertexXYZNUVTB_D
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float3 binormal:	BINORMAL;
   float3 tangent:	TANGENT;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZNUV
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
};


struct VertexXYZNUV_D
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
   float4 diffuse:	COLOR;
};


struct VertexXYZL
{
   float4 pos:		POSITION;
   float4 diffuse:	COLOR;
};


struct VertexXYZDUV
{
   float4 pos:		POSITION;
   float4 diffuse:	COLOR;
   float2 tc:		TEXCOORD0;
};


//---------------------------------------
// Vertex output, Pixel input definitions
//---------------------------------------
struct OutputDiffuseLighting
{
	float4 pos:				POSITION;
	float3 tcDepth:			TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap:		TEXCOORD1;
#endif
#ifdef USE_MOTION_BLUR
	float4 currentPos:		TEXCOORD2;
#endif
	float4 sunlight:		COLOR;
	float4 diffuse:			COLOR1;
	float  fog: FOG;
};


struct OutputDiffuseLighting2
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap:	TEXCOORD2;
	float2 tc2:     TEXCOORD1;
#else
	float2 tc2:     TEXCOORD1;
#endif
	float4 sunlight: COLOR;
	float4 diffuse: COLOR1;
	float  fog: FOG;
};


struct OutputDiffuseLighting3
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap:	TEXCOORD3;
	float2 tc2:     TEXCOORD1;
	float2 tc3:     TEXCOORD2;
#else
	float2 tc2:     TEXCOORD1;
	float2 tc3:     TEXCOORD2;
#endif	
	float4 sunlight: COLOR;
	float4 diffuse: COLOR1;
	float  fog: FOG;
};
