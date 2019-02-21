#include "stipple_include.fxh"
float4x4 worldViewProj : WorldViewProjection;
float4 screen : Screen;
texture stippleMap : StippleMap;

struct VertexXYZ
{
	float4 pos : POSITION;
};

struct Output
{
	float4 pos : POSITION;
	float4 screenPos : TEXCOORD;
};

Output vs_main( VertexXYZ input )
{
	Output o = (Output)0;

	o.pos = mul(input.pos, worldViewProj);
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
