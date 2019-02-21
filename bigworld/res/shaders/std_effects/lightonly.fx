#include "stdinclude.fxh"

#ifdef IN_GAME

#include "unskinned_effect_include.fxh"
#include "lightonly.fxh"


//--------------------------------------------------------------//
// Technique Section for shader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_VSHADERS( vertexShaders_2_0, vs_2_0, vs_main, vs_mainStaticLighting, false )

technique shader2
<
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{		
		BW_BLENDING_SOLID
		BW_FOG
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_2_0[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		PixelShader = compile ps_2_0 ps_main_2_0();
	}
}
#endif

//--------------------------------------------------------------//
// Technique Section for shader 1.1
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1)

LIGHTONLY_VSHADERS( vertexShaders_1_1, vs_1_1, vs_main, vs_mainStaticLighting, true )

technique shader1
<
	string label = "SHADER_MODEL_1";
>
{
	pass Pass_0
	{		
		BW_BLENDING_SOLID
		BW_FOG
		CULLMODE = (doubleSided ? 1 : 3);
		
		VertexShader = (vertexShaders_1_1[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		PixelShader = compile ps_1_1 ps_main_1_1();
	}
}
#endif


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_0)

LIGHTONLY_VSHADERS( vertexShaders_0_0, vs_2_0, vs_main, vs_mainStaticLighting, true )

technique standard
<
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
		
		VertexShader = (vertexShaders_0_0[nDirectionalLights + (nPointLights * 3) + (nSpotLights * 15) + int(staticLighting) * 45]);
		PixelShader = NULL;
	}
}
#endif

#else	//ifdef IN_GAME

#include "lightonly_max_preview.fxh"

//--------------------------------------------------------------//
// Technique Section for MAX
//--------------------------------------------------------------//
technique max_preview
{
	pass max_pass
	{
		BW_BLENDING_SOLID		
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		VertexShader = compile vs_2_0 vs_max();
		PixelShader = NULL;
	}
}

#endif	//ifdef IN_GAME