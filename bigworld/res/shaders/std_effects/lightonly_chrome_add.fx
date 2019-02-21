#define ADDITIVE_EFFECT 1
#include "stdinclude.fxh"
BW_ARTIST_EDITABLE_ADDITIVE_BLEND

#ifdef IN_GAME

#include "unskinned_effect_include.fxh"
#include "lightonly_chrome.fxh"

//--------------------------------------------------------------//
// Technique Section for shader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_VSHADERS( vertexShaders_2_0, vs_2_0, vs_mainFresnel, vs_mainStaticLightingFresnel, false )

technique shader2
<
	string channel = "sorted";
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{		
		BW_BLENDING_ADD
		BW_FOG_ADD
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_2_0[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		PixelShader = compile ps_2_0 ps_main();
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for shader 1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1)

LIGHTONLY_VSHADERS( vertexShaders_1_1, vs_1_1, vs_main, vs_mainStaticLighting, true )

technique shader1
<
	string channel = "sorted";
	string label = "SHADER_MODEL_1";
>
{
	pass Pass_0
	{		
		BW_BLENDING_ADD
		BW_FOG_ADD
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_1_1[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TEXTUREOP(1, otherMap)
		BW_TEXTURESTAGE_TERMINATE(2)
		PixelShader = NULL;
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)

LIGHTONLY_VSHADERS( vertexShaders_0_0, vs_2_0, vs_mainFresnel, vs_mainStaticLightingFresnel, true )

technique standard
<
	string channel = "sorted";
	string label = "SHADER_MODEL_0";
>
{
	pass Pass_0
	{
		BW_BLENDING_ADD
		BW_FOG_ADD
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_0_0[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TEXTUREOP(1, otherMap)
		BW_TEXTURESTAGE_TERMINATE(2)
		PixelShader = NULL;
	}
}
#endif


#else	//ifdef IN_GAME

#include "lightonly_chrome_max_preview.fxh"

//--------------------------------------------------------------//
// Technique Section for Max
//--------------------------------------------------------------//
technique standard
{
	pass Pass_0
	{
		BW_BLENDING_ADD
		BW_TEXTURESTAGE_DIFFUSEONLY_ALPHA(0, diffuseMap)
		BW_TEXTURESTAGE_TEXTUREOP(1, otherMap)
		BW_TEXTURESTAGE_ALPHAONLY(2, diffuseMap)	
		BW_TEXTURESTAGE_TERMINATE(3)
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_2_0 vs_chrome();
		PixelShader = NULL;
	}
}

#endif	//ifdef IN_GAME