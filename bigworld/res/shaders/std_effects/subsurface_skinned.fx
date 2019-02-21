// sub surface scattering!
#include "stdinclude.fxh"

#ifdef IN_GAME

#ifndef COLOURISE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)
#endif

#include "skinned_effect_include.fxh"
#include "subsurface_scattering.fxh"

//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 3
//--------------------------------------------------------------//
technique subsurface_skinned_shader3
<
	bool skinned = true;
	bool bumpMapped = true;
	string label = "SHADER_MODEL_3";
	string desc = "Very High";
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
		
		VertexShader = compile vs_3_0 vs_main_3_0();
		PixelShader = compile ps_3_0 ps_main_3_0();
	}

}


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 2
//--------------------------------------------------------------//
technique subsurface_skinned_shader2
<
	bool skinned = true;
	bool bumpMapped = true;
	string label = "SHADER_MODEL_2";
	string desc = "High";
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
		
		VertexShader = (vertexShaders_2_0[vs_2_0Selector()]);
		PixelShader = (pixelShaders_2_0[ps_2_0Selector()]);
	}

}


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 1
//--------------------------------------------------------------//
#include "normalmap_1_1.fxh"

NORMALMAP_SKINNED_DIFFUSEONLY_VSHADERS_LIMITED( diffuseVertexShaders_vs1, vs_1_1 )
NORMALMAP_SPECULARONLY_VSHADERS( specularVertexShaders_vs1, vs_1_1 )
NORMALMAP_DIFFUSEONLY_PSHADERS( diffusePixelShaders_ps1, ps_1_1 )
NORMALMAP_SPECULARONLY_PSHADERS( specularPixelShaders_ps1, ps_1_1 )

technique subsurface_skinned_shader1
<
	bool skinned = true;
	bool bumpMapped = true;
	string label = "SHADER_MODEL_1";
	string desc = "Medium";
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
		
		VertexShader = (diffuseVertexShaders_vs1[diffuseOnlyVSIndex(nDirectionalLights, nPointLights, 0)]);
#ifdef MOD2X		
		PixelShader = (diffusePixelShaders_ps1[min(nPointLights,2)]);
#else
		PixelShader = (diffusePixelShaders_ps1[min(nPointLights,2) ]);
#endif
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
#ifdef MOD2X		
		PixelShader = (specularPixelShaders_ps1[int(nSpecularPointLights>0)]);
#else
		PixelShader = (specularPixelShaders_ps1[int(nSpecularPointLights>0)]);
#endif
	}
	
}

//--------------------------------------------------------------//
// Fallback for software mode.
//--------------------------------------------------------------//
#define lightEnable 1

#include "normalmap_fallback.fxh"

technique software_fallback
<
	bool skinned = true;
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
		
		VertexShader = compile vs_2_0 diffuseOnlyFallback();
		PixelShader = NULL;
	}
}

#else	//ifdef IN_GAME

#include "subsurface_scattering_max_preview.fxh"

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
		
		VertexShader = compile vs_3_0 subsurface_maxpreview_vs3();
		PixelShader = compile ps_3_0 subsurface_maxpreview_ps3();
	}
}

#endif	//ifdef IN_GAME