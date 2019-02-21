#include "terrain_common.fxh"

USE_TERRAIN_NORMAL_MAP
USE_TERRAIN_HORIZON_MAP
BW_DIFFUSE_LIGHTING
BW_SPECULAR_LIGHTING
USE_TERRAIN_LIGHTING
BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER

// The output from the vertex shader
struct TerrainVertexOutput
{
	float4 position		: POSITION;
	float4 worldPosition: TEXCOORD0;
	float2 normalUV		: TEXCOORD1;	
    float2 horizonUV    : TEXCOORD2;	
    float2 skyLightMap	: TEXCOORD3;
};

//----------------------------------------------------------------------------
// Shader body 
//----------------------------------------------------------------------------

TerrainVertexOutput vs30_terrain(const TerrainVertex vertex)
{
	TerrainVertexOutput oVertex = (TerrainVertexOutput) 0;
	
	// Get the position on the xz plane.  No need to perform Geomorphing / lodding calculations.	
	oVertex.position = float4( vertex.xz.x * 2.0 - 1.0, vertex.xz.y * -2.0 + 1.0, 0.1, 1 );
	
	// But calculate the world position anyway because we need to use lighting
	// Note we don't need to do geomorphing etc., just use the one vertex height
	oVertex.worldPosition = mul( float4( vertex.xz.x * xzScale.x, vertex.height.x, vertex.xz.y * xzScale.y, 1 ), world );
	
	// Calculate the texture coordinate for cloud shadows
	BW_SKY_MAP_COORDS( oVertex.skyLightMap, oVertex.worldPosition )
	
	// Calculate the texture coordinate for the normal map
	oVertex.normalUV = inclusiveTextureCoordinate( vertex, float2( normalMapSize, normalMapSize ) );
	// Calculate the texture coordinate for the horizon map
	oVertex.horizonUV = inclusiveTextureCoordinate( vertex, float2( horizonMapSize, horizonMapSize ) );
	
	return oVertex;
};


/*
 * Shader body
 */
float4 ps30_terrain( const TerrainVertexOutput terrainFragment ) : COLOR0
{
	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV);
	
	// Calculate the horizon shadows
    float2 angles = tex2D( horizonMapSampler, terrainFragment.horizonUV ).xy; 
    float2 angleDiff = (sunAngle - angles) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	float shade = angleDiff.x * angleDiff.y;
	
	// Calculate the cloud shadows
	float skyMap = BW_SAMPLE_SKY_MAP( terrainFragment.skyLightMap )
	shade = min( shade, skyMap );
	
	// Calculate the diffuse lighting
	float3 diffuse = diffuseLighting( terrainFragment.worldPosition, normal, shade );
    float4 colour = float4(diffuse.x,diffuse.y,diffuse.z,1);
    return colour;
};


//--------------------------------------------------------------//
// Technique Sections for shader hardware
//--------------------------------------------------------------//
technique shader_version_3_0
{
	pass lighting_only_pass
	{        
        ALPHABLENDENABLE = FALSE;
        SRCBLEND = ONE;
        DESTBLEND = ZERO;
        ALPHATESTENABLE = FALSE;
        ZWRITEENABLE = FALSE;
        ZFUNC = EQUAL;
        ZENABLE = FALSE;
        FOGENABLE = FALSE;				
		CULLMODE = NONE;		
        
        VertexShader = compile vs_3_0 vs30_terrain();
		PixelShader = compile ps_3_0 ps30_terrain();
	}	
}
