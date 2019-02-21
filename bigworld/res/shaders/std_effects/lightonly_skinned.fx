#include "stdinclude.fxh"

#ifdef IN_GAME
#include "skinned_effect_include.fxh"
#include "lightonly.fxh"

//--------------------------------------------------------------//
// Technique Section for shader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_SKINNED_VSHADERS( vertexShaders_2_0, vs_2_0, vs_main, false )

technique shader2
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
		
		VertexShader = (vertexShaders_2_0[ lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights) ]);
		PixelShader = compile ps_2_0 ps_main_2_0();
	}
}
#endif

//--------------------------------------------------------------//
// Technique Section for shader 1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1)

LIGHTONLY_SKINNED_VSHADERS_LIMITED( vertexShaders_1_1, vs_1_1, vs_main, true )

technique shader1
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
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		VertexShader = (vertexShaders_1_1[ lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights) ]);
		PixelShader = compile ps_1_1 ps_main_1_1();
	}
}
#endif

//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)

LIGHTONLY_SKINNED_VSHADERS( vertexShaders_0_0, vs_2_0, vs_main, true )

technique standard
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
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		VertexShader = (vertexShaders_0_0[ lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights) ]);
		PixelShader = NULL;
	}
}
#endif


#else	//ifdef IN_GAME

#include "lightonly.fx"

#endif	//ifdef IN_GAME
