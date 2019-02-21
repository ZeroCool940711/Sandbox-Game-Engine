#ifdef IN_GAME

#include "stdinclude.fxh"
#ifdef NORMALMAP_ALPHA
BW_ARTIST_EDITABLE_ALPHA_BLEND
#endif
#include "normalmap_specmap.fxh"
#include "skinned_effect_include.fxh"


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)
#include "normalmap_2_0.fxh"

NORMALMAP_SKINNED_SEPARATE_SPOT_VSHADERS( vertexShaders_vs2, vs_2_0, normalMap_vs2, normalMapSpot_vs2 )

PixelShader pixelShaders_ps2[2] =
{
	compile ps_2_0 normalMap_ps2(),
	compile ps_2_0 normalMapSpot_ps2()
};

technique normalmap_shader2
<
	bool skinned = true;
	bool bumpMapped = true;
	string label = "SHADER_MODEL_2";
	string desc = "High";
#ifdef NORMALMAP_ALPHA
	string channel = "internalSorted";
#endif
>
{
	pass Pass_0
	{
#ifdef NORMALMAP_ALPHA
		BW_BLENDING_ALPHA
#else
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		ALPHABLENDENABLE = FALSE;
#endif //NORMALMAP_ALPHA

		BW_FOG
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_vs2[
			normalMapSkinnedSeparateSpot_vsIndex(
				nDirectionalLights,
				nPointLights,
				nSpecularDirectionalLights,
				nSpecularPointLights,
				nSpotLights)]);
		PixelShader = (pixelShaders_ps2[min(nSpotLights,1)]);
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1)

#ifdef COLOURISE_DIFFUSE_MAP

#include "shader_combination_helpers.fxh"

OutputDiffuseLighting2 vs_main( BUMPED_VERTEX_FORMAT i, 
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


#ifdef MOD2X
LIGHTONLY_SKINNED_VSHADERS_LIMITED_2( vertexShaders_11, vs_1_1, vs_main, true )
#else
LIGHTONLY_SKINNED_VSHADERS_LIMITED( vertexShaders_11, vs_1_1, vs_main, true )
#endif

technique shader_1_1
<
	bool skinned = true;
	string label = "SHADER_MODEL_1";
	string desc = "Medium";
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
#else

#include "normalmap_1_1.fxh"

NORMALMAP_SKINNED_DIFFUSEONLY_VSHADERS_LIMITED( diffuseVertexShaders_vs1, vs_1_1 )
NORMALMAP_SPECULARONLY_VSHADERS( specularVertexShaders_vs1, vs_1_1 )
NORMALMAP_DIFFUSEONLY_PSHADERS( diffusePixelShaders_ps1, ps_1_1 )
NORMALMAP_SPECULARONLY_PSHADERS( specularPixelShaders_ps1, ps_1_1 )

technique normalmap_shader1
<
	bool skinned = true;
	bool bumpMapped = true;
	string label = "SHADER_MODEL_1";
	string desc = "Medium";
#ifdef NORMALMAP_ALPHA
	string channel = "internalSorted";
#endif
>
{
	pass Pass_0
	{
#ifdef NORMALMAP_ALPHA
		BW_BLENDING_ALPHA
#else
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		ALPHABLENDENABLE = FALSE;
#endif //NORMALMAP_ALPHA

		BW_FOG
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		BW_CULL_DOUBLESIDED
		
		VertexShader = (diffuseVertexShaders_vs1[diffuseOnlyVSIndex(nDirectionalLights, nPointLights, 0)]);
		PixelShader = (diffusePixelShaders_ps1[min(nPointLights,2)]);
	}
	
	pass Pass_1
	{
		ZWRITEENABLE = FALSE;
		ZFUNC = EQUAL;

		FOGCOLOR = float4(0,0,0,0);

		ALPHABLENDENABLE = TRUE;
		DESTBLEND = ONE;
		SRCBLEND = ONE;
		
		VertexShader = (specularVertexShaders_vs1[specularOnlyVSIndex(nSpecularDirectionalLights, nSpecularPointLights)]);
		PixelShader = (specularPixelShaders_ps1[int(nSpecularPointLights>0)]);
	}	
}
#endif

#endif


//--------------------------------------------------------------//
// Technique section for software mode
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)
#include "normalmap_fallback.fxh"

technique software_fallback
<
	bool skinned = true;
	string label = "SHADER_MODEL_0";
	string desc = "Low";
#ifdef NORMALMAP_ALPHA
	string channel = "internalSorted";
#endif
>
{
	pass Pass_0
	{
#ifdef NORMALMAP_ALPHA
		BW_BLENDING_ALPHA
#else
		BW_BLENDING_SOLID
#endif
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;

		BW_FOG
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		
		VertexShader = (fallbackShaders[int(staticLighting)]);
		PixelShader = NULL;
	}
}
#endif


#else	//IN_GAME
#include "normalmap_specmap.fx"
#endif