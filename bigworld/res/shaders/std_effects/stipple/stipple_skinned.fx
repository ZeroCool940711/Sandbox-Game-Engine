#include "stipple_include.fxh"
#include "skinned_effect_include.fxh"

float4 screen : Screen;
texture stippleMap : StippleMap;

struct VertexXYZIIIWW
{
	float4 pos : POSITION;
	float3 indices:	BLENDINDICES;
	float2 weights:	BLENDWEIGHT;
};

struct Output
{
	float4 pos : POSITION;
	float4 screenPos : TEXCOORD;
};

Output vs_main( VertexXYZIIIWW input )
{
	Output o = (Output)0;

	int indices[3];
    calculateSkinningIndices( input.indices, indices );
							
	float weights[3] = { input.weights.x, input.weights.y, 1 - input.weights.y - input.weights.x };
	
	float4 worldPos = transformPos( world, input.pos, weights, indices );
	
	o.pos = mul(worldPos, viewProj);
	o.screenPos = o.pos;

	return o;
}

STIPPLE_SAMPLER_DECLARE

float4 ps_main( Output i ) : COLOR
{
	float2 uv;
	STIPPLE_UV_COORDS( i.screenPos, uv );	
	return tex2D( stippleSampler, uv );	
}


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique standard
<
	bool skinned = true;
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		ALPHATESTENABLE = TRUE;		
		ALPHAREF = 127;
		ALPHABLENDENABLE = FALSE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		CULLMODE = NONE;
		FOGENABLE = FALSE;
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps_main();
	}
}
