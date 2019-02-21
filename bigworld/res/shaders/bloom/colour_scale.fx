#include "stdinclude.fxh"

texture diffuseMap;

struct PS_INPUT
{
	float2 tc0		: TEXCOORD0;
};

sampler diffuseSampler = sampler_state
{
	Texture = (diffuseMap);
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;
	MAGFILTER = POINT;
	MINFILTER = POINT;
	MIPFILTER = NONE;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

float4 main( PS_INPUT v ) : COLOR
{
	float3 map0 = tex2D( diffuseSampler, v.tc0 );
	float3 res = map0 * map0;
    res = res * res;
    res = res * res;
	return float4( res, 1 );
};

technique standard
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