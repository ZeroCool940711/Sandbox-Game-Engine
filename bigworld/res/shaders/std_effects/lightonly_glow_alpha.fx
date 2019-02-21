#include "stdinclude.fxh"
BW_ARTIST_EDITABLE_ALPHA_BLEND

#ifdef IN_GAME

#include "unskinned_effect_include.fxh"
#include "lightonly_glow.fxh"

//--------------------------------------------------------------//
// Technique Section for shader 2
//--------------------------------------------------------------//
#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

LIGHTONLY_VSHADERS( vertexShaders_2_0, vs_2_0, vs_main, vs_mainStaticLighting, false )

technique shader2
<	
	string channel = "internalSorted";
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{		
		BW_BLENDING_ALPHA
		BW_FOG
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_2_0[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		PixelShader = compile ps_2_0 ps_main();
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
	string channel = "internalSorted";
	string label = "SHADER_MODEL_1";
>
{
	pass Pass_0
	{		
		BW_BLENDING_ALPHA
		BW_FOG
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_1_1[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		PixelShader = compile ps_1_1 ps_main();
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
	string channel = "internalSorted";
	string label = "SHADER_MODEL_0";
>
{	
	pass Pass_0
	{
		BW_BLENDING_ALPHA
		BW_FOG								
		BW_CULL_DOUBLESIDED
		
		VertexShader = (vertexShaders_0_0[ lightonlyVShaderIndex(nDirectionalLights, nPointLights, nSpotLights, staticLighting) ]);
		
		TEXTUREFACTOR = (float4(glowFactor, glowFactor, glowFactor, glowFactor));
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_ADD(1, glowMap)
		BW_TEXTURESTAGE_TERMINATE(2)
		COLOROP[1] = MULTIPLYADD;
		COLORARG0[1] = CURRENT;
		COLORARG1[1] = TFACTOR;
		COLORARG2[1] = TEXTURE;		
		PixelShader = NULL;		
	}	
	
}
#endif


#else	//ifdef IN_GAME

# include "lightonly_max_preview.fxh"

DECLARE_OTHER_MAP( glowMap, glowSampler, "glow map", "The glow map for the material." )
BW_ARTIST_EDITABLE_GLOW_FACTOR

technique max_preview
{
	pass Pass_0
	{
		BW_BLENDING_ALPHA
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)		
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_2_0 vs_max();
		PixelShader = NULL;
	}
	
	pass Pass_1
	{		
		BW_TEXTURESTAGE_DIFFUSEONLY(0, glowMap)						
		BW_TEXTURESTAGE_DIFFUSEONLY(1, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(2)

		COLOROP[0] = MODULATE;
		COLORARG1[0] = TEXTURE;
		COLORARG2[0] = TFACTOR;
		
		COLOROP[1] = SELECTARG1;
		COLORARG1[1] = CURRENT;
		
		ALPHAOP[1] = SELECTARG1;
		ALPHAARG1[1] = TEXTURE;

		TEXTUREFACTOR = (float4(glowFactor, glowFactor, glowFactor, glowFactor));
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ONE;
		ZWRITEENABLE = FALSE;
		LIGHTING = FALSE;
		
		VertexShader = compile vs_2_0 vs_max();
		PixelShader = NULL;
	}
}

#endif	//ifdef IN_GAME