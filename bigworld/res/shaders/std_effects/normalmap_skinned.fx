#ifdef IN_GAME
#include "stdinclude.fxh"
#include "normalmap.fxh"
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
>
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		BW_FOG
		ALPHABLENDENABLE = FALSE;
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
>
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;

		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;

		BW_FOG
		ALPHABLENDENABLE = FALSE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		BW_CULL_DOUBLESIDED
		
		VertexShader = (diffuseVertexShaders_vs1[diffuseOnlyVSIndex(nDirectionalLights, nPointLights, staticLighting)]);
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


//--------------------------------------------------------------//
// Technique section for software mode
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)
#include "normalmap_fallback.fxh"

technique software_fallback
<
	bool skinned = true;
	string label = "SHADER_MODEL_0";
>
{
	pass Pass_0
	{
		SPECULARENABLE = FALSE;
		BW_BLENDING_SOLID
		BW_FOG								
		BW_CULL_DOUBLESIDED

#ifdef NORMALMAP_GLOW_MAP
		TEXTUREFACTOR = (float4(glowFactor, glowFactor, glowFactor, glowFactor));
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_DIFFUSEONLY(1, glowMap)
		BW_TEXTURESTAGE_TERMINATE(2)
		COLOROP[1] = MULTIPLYADD;
		COLORARG0[1] = CURRENT;
		COLORARG1[1] = TFACTOR;
		COLORARG2[1] = TEXTURE;
#else
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
#endif

		VertexShader = compile vs_2_0 diffuseOnlyFallback();
		PixelShader = NULL;
	}
}
#endif


#else //IN_GAME
#include "normalmap_glow.fx"
#endif //IN_GAME