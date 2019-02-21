#include "stdinclude.fxh"

texture diffuseMap;
float scalePower = 8.0;

sampler diffuseSampler = sampler_state
{
	Texture = (diffuseMap);
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = NONE;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

struct PS_INPUT
{
	float2 tc0		: TEXCOORD0;
	float2 tc1		: TEXCOORD1;
	float2 tc2		: TEXCOORD2;
	float2 tc3		: TEXCOORD3;
};

float4 main( PS_INPUT v ) : COLOR
{
	float3 map0 = tex2D( diffuseSampler, v.tc0 );
	float3 map1 = tex2D( diffuseSampler, v.tc1 );
	float3 map2 = tex2D( diffuseSampler, v.tc2 );
	float3 map3 = tex2D( diffuseSampler, v.tc3 );

	float3 result = map0 + map1 + map2 + map3;
	result *= 0.25;
	return float4( result, 1 );
};


technique shader1
<
	string label = "SHADER_MODEL_1";
>
{
	pass Pass_0
	{
		ZENABLE = FALSE;
		ZWRITEENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		ALPHATESTENABLE = FALSE;
		CULLMODE = NONE;
		FOGENABLE = FALSE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		
		VertexShader = NULL;
		PixelShader = compile ps_1_1 main();
	}
}


BW_NULL_TECHNIQUE