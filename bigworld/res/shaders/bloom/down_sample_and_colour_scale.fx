#include "stdinclude.fxh"

texture diffuseMap;

float scalePower = 8.0;

struct PS_INPUT
{
	float2 tc0		: TEXCOORD0;
	float2 tc1		: TEXCOORD1;
	float2 tc2		: TEXCOORD2;
	float2 tc3		: TEXCOORD3;
};

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


float luminance( float3 input )
{
	return (0.3f*input.r + 0.59f*input.y + 0.11f*input.z);
}


float3 colourScale( float3 input )
{
	return pow( input, scalePower );
}


float4 main( PS_INPUT v ) : COLOR
{
	float3 map0 = tex2D( diffuseSampler, v.tc0 );
	float3 map1 = tex2D( diffuseSampler, v.tc1 );
	float3 map2 = tex2D( diffuseSampler, v.tc2 );
	float3 map3 = tex2D( diffuseSampler, v.tc3 );	
	

	float3 res = colourScale( map0 );
	res += colourScale( map1 );
	res += colourScale( map2 );
	res += colourScale( map3 );
	
	res = res / 4.f;
	
	return float4( res, 1 );
};

#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)

technique shader2
<
	string label = "SHADER_MODEL_2";
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
		PixelShader = compile ps_2_0 main();
	}
}

#else

BW_NULL_TECHNIQUE

#endif

BW_NULL_TECHNIQUE