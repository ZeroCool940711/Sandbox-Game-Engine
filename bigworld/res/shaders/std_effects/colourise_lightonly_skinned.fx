#include "stdinclude.fxh"
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)
#include "bw_four_channel_customise.fxh"
#include "shader_combination_helpers.fxh"

BW_FOUR_CHANNEL_COLOURISE( skinColour, "Skin Colour", "Custom colour for skin",
							hairColour, "Hair Colour", "Custom colour for hair",
							clothesColour1, "Clothes Colour", "Custom colour for misc. clothes 1",
							clothesColour2, "Clothes Colour 2", "Custom colour for misc. clothes 2" )
							
#ifdef IN_GAME						
							
BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_MOD2X							
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_SELF_ILLUMINATION

sampler diffuseSampler = BW_SAMPLER( diffuseMap, BW_TEX_ADDRESS_MODE )
BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
BW_DIFFUSE_LIGHTING
VERTEX_FOG

#include "skinned_effect_include.fxh"

OutputDiffuseLighting2 vs_main( VertexXYZNUVIIIWW i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;
	
	PROJECT_POSITION( o.pos )
	o.tc = o.tc2 = i.tc;
	
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos );
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	
	return o;
}

//This pixel shader coloursies the diffuse map, using the mask map
//to select between 4 user-defined colours.  The alpha channel of
//the diffuse map is output to the alpha channel to be used for
//alpha test.
float4 ps_main( OutputDiffuseLighting2 input ) : COLOR0
{
	//get a customised / colourised version of diffuse map
	float4 result = colouriseDiffuseMap( diffuseSampler, input.tc, input.tc2 );
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );

#ifdef MOD2X
	result.rgb = 2.0 * (input.sunlight.rgb * shade + input.diffuse.rgb) * result.rgb;
#else
	result.rgb = (input.sunlight.rgb * shade + input.diffuse.rgb) * result.rgb;
#endif
	return result;
}

float4 ps_11_colourise( OutputDiffuseLighting2 input ) : COLOR0
{
	//get a customised / colourised version of diffuse map
	float4 result = colouriseDiffuseMap_ps11( diffuseSampler, input.tc, input.tc2 );
	return result;
}

float4 ps_11_diffuse( OutputDiffuseLighting2 input ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );
	float4 result;
	result.rgb = input.sunlight.rgb * diffuseMap.rgb;
	result.a = diffuseMap.a;
	return result;
}


//--------------------------------------------------------------//
// Technique Section for shader 2.0
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_SKINNED_VSHADERS( vertexShaders_20, vs_2_0, vs_main, false )

technique shader_2_0
<
	bool skinned = true;
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		BW_FOG
		BW_CULL_DOUBLESIDED
		VertexShader = (vertexShaders_20[lightonlySkinnedVShaderIndex( nDirectionalLights, nPointLights, nSpotLights)]);
		PixelShader = compile ps_2_0 ps_main();
	}
}
#endif


#if (COMPILE_SHADER_MODEL_1)

#ifdef MOD2X
LIGHTONLY_SKINNED_VSHADERS_LIMITED_2( vertexShaders_11, vs_1_1, vs_main, true )
#else
LIGHTONLY_SKINNED_VSHADERS_LIMITED( vertexShaders_11, vs_1_1, vs_main, true )
#endif

technique shader_1_1
<
	bool skinned = true;
	string label = "SHADER_MODEL_1";
>
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		BW_FOG
		BW_CULL_DOUBLESIDED
		VertexShader = (vertexShaders_11[lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights)]);
		PixelShader = compile ps_1_1 ps_11_colourise();
	}
	
	pass Pass_1
	{
		ALPHABLENDENABLE = TRUE;
		DESTBLEND = SRCCOLOR;
		SRCBLEND = ZERO;

		VertexShader = (vertexShaders_11[nDirectionalLights + (nPointLights * 3) + (nSpotLights * 15) ]);
		PixelShader = compile ps_1_1 ps_11_diffuse();
	}
}
#endif


#else	//ifdef IN_GAME
#define COLOURISE_DIFFUSE_MAP
#include "max_preview_include.fxh"

sampler diffuseSampler = BW_SAMPLER( diffuseMap, BW_TEX_ADDRESS_MODE )

OutputDiffuseLighting vs_max( VertexXYZNUV i )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;
	o.pos = mul( i.pos, worldViewProj );
	o.tcDepth.xy = i.tc;
	
	float3 lDir = normalize(mul( lightDir, worldInverse ));
	
#ifdef MOD2X
	o.diffuse = saturate(dot( lDir, i.normal )) * lightColour * 0.5 * (1 + diffuseLightExtraModulation);
#else
	o.diffuse = saturate(dot( lDir, i.normal )) * lightColour;
#endif
	o.diffuse.w = 1;
	return o;
}

float4 ps_max( OutputDiffuseLighting input ) : COLOR0
{
	//get a customised / colourised version of diffuse map
	float4 result = colouriseDiffuseMap( diffuseSampler, input.tcDepth.xy, input.tcDepth.xy );
	
	//and do normal lighting.	
	result.xyz = result.xyz * input.diffuse.xyz;	
	return result;
}

technique max_preview
{
	pass max_pass
	{
		BW_BLENDING_SOLID
		BW_CULL_DOUBLESIDED
		VertexShader = compile vs_2_0 vs_max();
		PixelShader = compile ps_2_0 ps_max();
	}
}
#endif	//ifdef IN_GAME
