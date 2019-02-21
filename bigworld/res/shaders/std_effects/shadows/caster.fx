#include "stdinclude.fxh"

float4x4 world : World;
float4x4 viewProj : ViewProjection;

struct VertexXYZ
{
	float4 pos : POSITION;
};

struct Output
{
	float4 pos : POSITION;
	float depth : TEXCOORD;
};

Output vs_main( VertexXYZ input )
{
	Output o = (Output)0;

	float4 worldPos = mul(input.pos, world);
	o.pos = mul(worldPos, viewProj);
	o.depth = 1.f - o.pos.z;

	return o;
}

float4 ps_main( Output i ) : COLOR
{
	return float4( i.depth, i.depth, i.depth, i.depth );
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
		ALPHATESTENABLE = FALSE;
		ALPHAFUNC = GREATER;
		ALPHABLENDENABLE = FALSE;
		SRCBLEND = ONE;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		CULLMODE = CCW;
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps_main();
	}
}
BW_NULL_TECHNIQUE
