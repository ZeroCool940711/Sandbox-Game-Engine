#include "stdinclude.fxh"

#ifdef IN_GAME
#include "unskinned_effect_include.fxh"
#include "normalmap_chrome.fxh"


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 3
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_3)
#include "normalmap_3_0.fxh"

NORMALMAP_ENV_3_0_VSHADERS( vertexShaders_vs3, vs_3_0 )

technique normalmap_shader3
<
	bool bumpMapped = true;
	string label = "SHADER_MODEL_3";
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
		
		VertexShader = (vertexShaders_vs3[normalMapEnv_vsIndex(nDirectionalLights, nPointLights, nSpecularDirectionalLights, nSpecularPointLights, staticLighting)]);
		PixelShader = compile ps_3_0 normalMapEnv_ps3();
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for for pixel/vertexshader 1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2) || (COMPILE_SHADER_MODEL_1)
#include "normalmap_1_1.fxh"

NORMALMAP_DIFFUSEONLY_VSHADERS( diffuseVertexShaders_vs1, vs_1_1 )
NORMALMAP_SPECULARONLY_VSHADERS( specularVertexShaders_vs1, vs_1_1 )
NORMALMAP_DIFFUSEONLY_PSHADERS( diffusePixelShaders_ps1, ps_1_1 )
NORMALMAP_SPECULARONLY_PSHADERS( specularPixelShaders_ps1, ps_1_1 )

technique normalmap_shader1
<
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


	// no reflection pass, shader will require a re-think if this is absolutely necessary
}
#endif


//--------------------------------------------------------------//
// Shaders section for software mode
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)
#include "normalmap_fallback.fxh"

//--------------------------------------------------------------//
// Technique section for software mode
//--------------------------------------------------------------//

technique software_fallback
<
	string label = "SHADER_MODEL_0";
	string desc = "Low";
>
{
	pass Pass_0
	{
#ifdef CHROME_GLOW_MAP
		SPECULARENABLE = FALSE;
		BW_BLENDING_SOLID
		BW_FOG								
		BW_CULL_DOUBLESIDED
		
		TEXTUREFACTOR = (float4(glowFactor, glowFactor, glowFactor, glowFactor));
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_DIFFUSEONLY(1, glowMap)
		BW_TEXTURESTAGE_TERMINATE(2)
		COLOROP[1] = MULTIPLYADD;
		COLORARG0[1] = CURRENT;
		COLORARG1[1] = TFACTOR;
		COLORARG2[1] = TEXTURE;		
#else
		BW_BLENDING_SOLID
		BW_FOG
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
#endif
	    
		VertexShader = (fallbackShaders[int(staticLighting)]);
		PixelShader = NULL;		
	}
}
#endif


#else //IN_GAME

#include "normalmap_chrome_max_preview.fxh"

technique max_preview
{
	pass Pass_0
	{
		ZEnable = true;
		ZWriteEnable = true;
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_3_0 normalMap_maxpreview_vs3();
		PixelShader = compile ps_3_0 normalMap_maxpreview_ps3();
	}
}

#endif