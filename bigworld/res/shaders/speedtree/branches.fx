#include "speedtree.fxh"

bool    g_cullEnabled   = true;

//----------------------------------------------------------------------------
// Shader Model 2 (no normal maps)
//----------------------------------------------------------------------------

SpeedtreeVertexOutput vs_branches_20(		
		const VS_INPUT_BRANCHES input,
		uniform int  nDirectionals, 
		uniform int  nPoints, 
		uniform int  nSpots,
		uniform bool combineLights )
{
	SpeedtreeVertexOutput output = (SpeedtreeVertexOutput) 0;
	
	output.tcDepth.xy  = input.texCoords.xy;
	output.pos = branchesOutputPosition(input);
	
	SPT_WORLD_POS_NORMAL(input)
	SPT_FOG_SUNLIGHT(output, nSpots, 0)
	BW_SKY_MAP_COORDS( output.skyLightMap, worldPos )
	
	output.diffuse = speedtreeDiffuse(worldNormal, worldPos, g_material[0], g_material[1], nDirectionals, nPoints, nSpots);

	BW_DEPTH(output.tcDepth.z, output.pos.z)
	
	return output;
};

//----------------------------------------------------------------------------
// Shader Model 2 (with normal maps)
//----------------------------------------------------------------------------

#if SPTFX_ENABLE_NORMAL_MAPS

SpeedtreeVertexOutputBumped vs_branches_20_bumped(
		const VS_INPUT_BRANCHES i,
		uniform int nDirectionals, 
		uniform int nPoints, 
		uniform int nSpecularDirectionals, 
		uniform int nSpecularPoints )
{
	SpeedtreeVertexOutputBumped output = (SpeedtreeVertexOutputBumped) 0;
	
	output.tcDepth.xy  = i.texCoords.xy;
	output.pos = branchesOutputPosition(i);
	
	SPT_WORLD_POS_NORMAL(i)	
	SPT_FOG_SUN_DIF_ATT(output, 0) 
	BW_SKY_MAP_COORDS(output.skyLightMap, worldPos)
	BW_DEPTH(output.tcDepth.z, output.pos.z)
	return output;
};

SpeedtreeVertexOutputBumped vs_branches_20_bumped_spot(
		const VS_INPUT_BRANCHES i,
		uniform int nDirectionals, 
		uniform int nPoints, 
		uniform int nSpecularDirectionals, 
		uniform int nSpecularPoints )
{
	SpeedtreeVertexOutputBumped output = (SpeedtreeVertexOutputBumped) 0;
	
	output.tcDepth.xy  = i.texCoords.xy;
	output.pos = branchesOutputPosition(i);
	
	SPT_WORLD_POS_NORMAL(i)
	SPT_FOG_SUN_DIF_ATT_SPOT(output, 0)
	BW_SKY_MAP_COORDS( output.skyLightMap, worldPos )
	BW_DEPTH(output.tcDepth.z, output.pos.z)
	
	return output;
};


SPT_LIGHTING_SEPARATE_SPOT_VSHADERS_BUMP(
	branchVertexShaders_20, vs_2_0,
	vs_branches_20,
	vs_branches_20_bumped, vs_branches_20_bumped_spot)

PixelShader branchPixelShaders_20[3] =
{
	compile ps_2_0 ps_speedtree_lighting_20(),
	compile ps_2_0 ps_speedtree_lighting_20_bumped(false),
	compile ps_2_0 ps_speedtree_lighting_20_bumped_spot(false)
};


technique branches_20_bumped
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		DESTBLEND = ZERO;
		CULLMODE  = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);

		VertexShader = (branchVertexShaders_20[ select_vsIndex() ]);
		PixelShader  = (branchPixelShaders_20[ select_psIndex() ]);
	}
}
#else

LIGHTONLY_SKINNED_VSHADERS(branchVertexShaders_20, vs_2_0, vs_branches_20, false)
PixelShader branchPixelShaders_20[1] = { compile ps_2_0 ps_speedtree_lighting_20() };

#endif // SPTFX_ENABLE_NORMAL_MAPS

//----------------------------------------------------------------------------
// Shader Model 2 (no normal maps)
//----------------------------------------------------------------------------

technique branches_20
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		DESTBLEND = ZERO;
		CULLMODE  = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);
				
		VertexShader = (branchVertexShaders_20[lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights)]);
		PixelShader  = (branchPixelShaders_20[ 0 ]);
	}
}

//----------------------------------------------------------------------------
// Fixed Function
//----------------------------------------------------------------------------

OutputDiffuseLighting vs_branches_fixed(const VS_INPUT_BRANCHES input)
{
	OutputDiffuseLighting output = (OutputDiffuseLighting) 0;
	
	output.pos = branchesOutputPosition(input);	
	output.fog = vertexFog(output.pos.w, fogStart, fogEnd);
	output.tcDepth.xy  = input.texCoords;
		
	float3 normal = mul(input.normal, g_world);
	normal = normalize(normal);
	float ndotl = dot(normal, directionalLights[0].direction);
	float4 diffuse = g_material[0] * directionalLights[0].colour;
	float4 ambient = g_material[1] * ambientColour;
	output.sunlight.xyz = saturate(ndotl*diffuse + ambient);
	output.sunlight.w = 1.0f;
	
	return output;
};

technique branches_fixed
{
	pass Pass_0
	{
		BW_BLENDING_SOLID		
		BW_FOG		
		DESTBLEND        = ZERO;
		COLOROP[0]       = (g_texturedTrees ? 4 : 3);
		COLORARG1[0]     = TEXTURE;
		COLORARG2[0]     = DIFFUSE;
		ALPHAOP[0]       = SELECTARG1;
		ALPHAARG1[0]     = TEXTURE;
		ALPHAARG2[0]     = DIFFUSE;
		Texture[0]       = (g_diffuseMap);
		ADDRESSU[0]      = WRAP;
		ADDRESSV[0]      = WRAP;
		ADDRESSW[0]      = WRAP;
		MAGFILTER[0]     = LINEAR;
		MINFILTER[0]     = LINEAR;
		MIPFILTER[0]     = LINEAR;
		MAXMIPLEVEL[0]   = 0;
		MIPMAPLODBIAS[0] = 0;
		TexCoordIndex[0] = 0;
		
		BW_TEXTURESTAGE_TERMINATE(1)
		
		CULLMODE  = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);
				
		VertexShader = compile vs_1_1 vs_branches_fixed();
		PixelShader  = NULL;
	}
}
