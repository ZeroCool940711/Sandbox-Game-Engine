#ifdef IN_GAME
#include "stdinclude.fxh"
#include "mesh_particle_include.fxh"

BW_ARTIST_EDITABLE_ALPHA_BLEND

#include "shader_combination_helpers.fxh"

//--------------------------------------------------------------//
// Technique Section for shader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_SKINNED_VSHADERS( vertexShaders_2_0, vs_2_0, vs_main, false )

technique shader2
<
	string channel = "internalSorted";
	bool skinned = true;
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		BW_BLENDING_ALPHA
		BW_FOG		
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_2_0[lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, false)]);
		PixelShader = compile ps_2_0 ps_main();
	}
}

#endif

//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_1 || COMPILE_SHADER_MODEL_0)

LIGHTONLY_SKINNED_VSHADERS( vertexShaders_1_1, vs_1_1, vs_main, true )

technique standard
<
	string channel = "internalSorted";
	bool skinned = true;
	string label = "SHADER_MODEL_0";
>
{
	pass Pass_0
	{
		BW_BLENDING_ALPHA
		BW_FOG
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)				
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		ALPHAOP[0] = MODULATE;
		ALPHAARG1[0] = TEXTURE;
		ALPHAARG2[0] = DIFFUSE;
		
		VertexShader = (vertexShaders_1_1[lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, false)]);
		PixelShader = NULL;
	}
}

#endif

#else
#include "lightonly_alpha.fx"
#endif