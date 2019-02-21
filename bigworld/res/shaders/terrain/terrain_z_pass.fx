
#include "terrain_common.fxh"

// We want to use the holes map
USE_TERRAIN_HOLES_MAP

// Need this for blending setup
BW_NON_EDITABLE_ALPHA_TEST

// The output from the vertex shader
struct TerrainVertexOutput
{
	float4 position		: POSITION;
    float2 holesUV		: TEXCOORD0;
};

//----------------------------------------------------------------------------
// Vertex shader
//----------------------------------------------------------------------------

TerrainVertexOutput vs20_terrain(const TerrainVertex vertex)
{
	TerrainVertexOutput oVertex  = (TerrainVertexOutput) 0;
	
	// Calculate the position of the vertex
	float4 worldPosition = terrainVertexPosition( vertex );
	oVertex.position = mul( worldPosition, viewProj );
	
    // Calculate the texture coordinate for the holes map
    oVertex.holesUV = vertex.xz * (holesSize / holesMapSize);

	return oVertex;
};

//----------------------------------------------------------------------------
// Pixel shader
//----------------------------------------------------------------------------

/*
 * Shader body
 */
float4 ps20_terrain( const TerrainVertexOutput terrainFragment, uniform bool hasHoles ) : COLOR0
{
    if (hasHoles)
    {
        // Do the holes
        float value = -tex2D( holesMapSampler, terrainFragment.holesUV ).x;
        clip(value);
    }
    
	return float4(0,0,0,1);
};

PixelShader pixelShaders_2_0[2] =
{
    compile ps_2_0 ps20_terrain(false),
    compile ps_2_0 ps20_terrain(true)
};


//--------------------------------------------------------------//
// Technique Sections
//--------------------------------------------------------------//
technique hole_map_2_0
{
	pass z_only_pass
	{
        ALPHATESTENABLE = FALSE;
        ALPHABLENDENABLE = FALSE;
        ZENABLE = TRUE;
        ZWRITEENABLE = TRUE;
        ZFUNC = LESSEQUAL;
		CULLMODE = BW_CULL_CCW;
        
		VertexShader = compile vs_2_0 vs20_terrain();
		PixelShader = (pixelShaders_2_0[int(holesSize > 0)]);
	}
}