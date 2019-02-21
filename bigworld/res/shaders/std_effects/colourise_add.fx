#include "stdinclude.fxh"

// Auto variables
float4x4 worldViewProj : WorldViewProjection;

// Exposed artist editable variables.

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

float4 colour
<
	bool artistEditable = true;
	string UIWidget = "Color";
	string UIName = "Colour";
	string UIDesc = "The colour tint of the colorisation";
> = {1,1,1,1};

BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_NON_EDITABLE_ADDITIVE_BLEND
BW_NON_EDITABLE_LIGHT_ENABLE

#ifdef IN_GAME

struct OutputVertex
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
	float fog: 	FOG;
};

VERTEX_FOG

OutputVertex vs_main( VertexXYZNUV input )
{
	OutputVertex o = (OutputVertex)0;

	o.pos = mul(input.pos, worldViewProj);
	o.tc = input.tc;
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);

	return o;
}


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
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
		TEXTUREFACTOR = (colour);
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		COLOROP[0] = MODULATE;
		COLORARG1[0] = TEXTURE;
		COLORARG2[0] = TFACTOR;
		ALPHAOP[0] = MODULATE;
		ALPHAARG1[0] = TEXTURE;
		ALPHAARG2[0] = TFACTOR;
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_1_1 vs_main();
		PixelShader = NULL;
	}
}

#else

struct OutputVertex
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
};

OutputVertex vs_max( VertexXYZNUV input )
{
	OutputVertex o = (OutputVertex)0;

	o.pos = mul(input.pos, worldViewProj);
	o.tc = input.tc;
	return o;
}

technique max_preview
{
	pass Pass_0
	{
		BW_BLENDING_ADD
		TEXTUREFACTOR = (colour);
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		COLOROP[0] = MODULATE;
		COLORARG1[0] = TEXTURE;
		COLORARG2[0] = TFACTOR;
		ALPHAOP[0] = MODULATE;
		ALPHAARG1[0] = TEXTURE;
		ALPHAARG2[0] = TFACTOR;
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_1_1 vs_max();
		PixelShader = NULL;
	}
}

#endif