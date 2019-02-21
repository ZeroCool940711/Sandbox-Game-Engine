
#include "terrain_common.fxh"

float4x4 shadowProj;
float shadowTexelSize;
float shadowIntensity = 1.f;
texture depthTexture;
float shadowDistance = 10.f;
float shadowFadeStart = 9.f;
int shadowQuality = 0;
float3 shadowDir : ShadowDir;

// We want to use a normal map
USE_TERRAIN_NORMAL_MAP

// Need this for blending setup
BW_NON_EDITABLE_ALPHA_TEST

// The output from the vertex shader
struct TerrainVertexOutput
{
	float4 position		: POSITION;	
    float2 shadowUV		: TEXCOORD1;
    float depth			: TEXCOORD2;
    float fadeout		: TEXCOORD3;    
    float2 normalUV		: TEXCOORD4;
};


//----------------------------------------------------------------------------
// Vertex shader
//----------------------------------------------------------------------------

TerrainVertexOutput vs20_terrain(const TerrainVertex vertex)
{
	TerrainVertexOutput o  = (TerrainVertexOutput) 0;
	
	// Calculate the position of the vertex
	float4 worldPosition = terrainVertexPosition( vertex );
	o.position = mul( worldPosition, viewProj );
	
	// Calculate the texture coordinate for the normal map
	o.normalUV = inclusiveTextureCoordinate( vertex, float2( normalMapSize, normalMapSize ) );
	
    // Calculate the shadowing information
    float4 shadow = mul( worldPosition, shadowProj );

	o.shadowUV = shadow.xy;
	o.depth = (1.f - shadow.z) * 1.01;

	float sf = shadowFadeStart;
	
	if (shadowDistance < (shadowFadeStart + 0.2))
		sf = shadowDistance - 0.2;
		
	o.fadeout = ( shadow.z - 1.f ) / ((sf - shadowDistance) / shadowDistance );

	return o;
};

//----------------------------------------------------------------------------
// Pixel shader
//----------------------------------------------------------------------------
sampler shadowSampler = sampler_state
{
	Texture = <depthTexture>;
	MIPFILTER = POINT;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
};

float2 uvOffset1 = float2( ((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 0.5f) );
float2 uvOffset2 = float2( -((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 0.5f) );

float2 uvOffset3 = float2( ((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 0.5f) );
float2 uvOffset4 = float2( ((1.f/1024.f) * 1.5f), -((1.f/1024.f) * 0.5f) );
float2 uvOffset5 = float2( ((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 1.5f) );
float2 uvOffset6 = float2( ((1.f/1024.f) * 0.5f), -((1.f/1024.f) * 1.5f) );
	
float2 uvOffset7 = float2( ((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 1.5f) );
float2 uvOffset8 = float2( -((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 1.5f) );

/*
 * Shader body
 */
float4 ps20_detail0( const TerrainVertexOutput terrainFragment ) : COLOR0
{
	float depth = max( terrainFragment.depth, 0.01f );		
	float depthOn = tex2D( shadowSampler, terrainFragment.shadowUV ).r > depth;	

	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV);
    
	float fadeout = saturate(-dot(normalize(normal), normalize(shadowDir)));
	depthOn *= saturate(terrainFragment.fadeout) * shadowIntensity * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}


float4 ps20_detail1( const TerrainVertexOutput terrainFragment ) : COLOR0
{
	float depth = max( terrainFragment.depth, 0.01f );	
	float depthOn = tex2D( shadowSampler, terrainFragment.shadowUV ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset1 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset2 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset2 ).r > terrainFragment.depth;

	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV);

    float fadeout = saturate(-dot(normalize(normal), normalize(shadowDir)));
	depthOn *= saturate(terrainFragment.fadeout) * shadowIntensity * (1.f / 4.f) * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}


float4 ps20_detail2( const TerrainVertexOutput terrainFragment ) : COLOR0
{
	float depth = max( terrainFragment.depth, 0.01f );
	
	float depthOn = tex2D( shadowSampler, terrainFragment.shadowUV ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset1 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset2 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset2 ).r > terrainFragment.depth;
	depthOn *= 1.5f;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset3 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset3 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset4 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset4 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset5 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset5 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV + uvOffset6 ).r > terrainFragment.depth;
	depthOn += tex2D( shadowSampler, terrainFragment.shadowUV - uvOffset6 ).r > terrainFragment.depth;

	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV);

    float fadeout = saturate(-dot(normalize(normal), normalize(shadowDir)));
	depthOn *= saturate(terrainFragment.fadeout) * shadowIntensity * (1.f / 14.f) * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}


PixelShader pixelShaders[3] =
{
	compile ps_2_0 ps20_detail0(),
	compile ps_2_0 ps20_detail1(),
	compile ps_2_0 ps20_detail2()
};


//--------------------------------------------------------------//
// Technique Sections
//--------------------------------------------------------------//
technique hole_map_2_0
{
	pass shadow_receive
	{
		ALPHATESTENABLE = FALSE;
		ALPHAFUNC = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ZERO;
		DESTBLEND = INVSRCCOLOR;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = EQUAL;
		CULLMODE = BW_CULL_CCW;
		FOGENABLE=FALSE;        
        
		VertexShader = compile vs_2_0 vs20_terrain();
		PixelShader = (pixelShaders[(min(2,max(shadowQuality,0)))]);
	}
}