#include "stdinclude.fxh"
#ifdef IN_GAME

#include "unskinned_effect_include.fxh"
#include "parallax_lighting_specmap.fxh"


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)
#include "normalmap_2_0.fxh"

NORMALMAP_SEPARATE_SPOT_VSHADERS( vertexShaders_vs2, vs_2_0, parallaxLighting_vs2, parallaxLightingSpot_vs2, parallaxStaticLighting_vs2, parallaxStaticLightingSpot_vs2 )

PixelShader pixelShaders[2] =
{
	compile ps_2_0 parallaxLighting_ps2(),
	compile ps_2_0 parallaxLightingSpot_ps2()
};


technique parallax_lighting_shader2
<
	string label = "SHADER_MODEL_2";
	string desc = "High";
	bool bumpMapped = true;
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
		
		VertexShader = (vertexShaders_vs2[normalMapSeparateSpot_vsIndex(
			nDirectionalLights,
			nPointLights,
			nSpecularDirectionalLights,
			nSpecularPointLights,
			nSpotLights,
			staticLighting)]);
		PixelShader = (pixelShaders[min(nSpotLights,1)]);
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1)
#include "normalmap_1_1.fxh"

NORMALMAP_DIFFUSEONLY_VSHADERS( diffuseVertexShaders_vs1, vs_1_1 )
NORMALMAP_SPECULARONLY_VSHADERS( specularVertexShaders_vs1, vs_1_1 )
NORMALMAP_DIFFUSEONLY_PSHADERS( diffusePixelShaders_ps1, ps_1_1 )
NORMALMAP_SPECULARONLY_PSHADERS( specularPixelShaders_ps1, ps_1_1 )


technique parallax_lighting_shader1
< 
	string label = "SHADER_MODEL_1";
	string desc = "Medium";
	bool bumpMapped = true;
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
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		BW_FOG
		ALPHABLENDENABLE = TRUE;
		DESTBLEND = ONE;
		SRCBLEND = ONE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		BW_CULL_DOUBLESIDED
		
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
	string label = "SHADER_MODEL_0";
	string desc = "Low";
>
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
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

#include "parallax_lighting_specmap_max_preview.fxh"

technique max_preview
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;

		ALPHABLENDENABLE = FALSE;
		BW_CULL_DOUBLESIDED
		TEXCOORDINDEX[0] = 0;
		TEXCOORDINDEX[1] = 1;
		TEXCOORDINDEX[2] = 2;
		TEXCOORDINDEX[3] = 3;
		TEXCOORDINDEX[4] = 4;
		TEXCOORDINDEX[5] = 5;
		TEXCOORDINDEX[6] = 6;
		TEXCOORDINDEX[7] = 7;
		
		VertexShader = compile vs_2_0 parallaxLighting_maxpreview_vs2();
		PixelShader = compile ps_2_0 parallaxLighting_maxpreview_ps2();
	}
}
#endif