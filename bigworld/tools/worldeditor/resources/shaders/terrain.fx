#include "stdinclude.fxh"

float4x4 world;
float4x4 viewProj;
float4x4 view;
float4x4 proj;
float4x4 textransform;

struct VS_INPUT
{
    float4 pos		: POSITION;
    float3 normal		: NORMAL;
    float4 blend		: COLOR0;
    float4 shadow		: COLOR1;
};

struct VS_OUTPUT
{
    float4 pos			: POSITION;
};

struct VS_OUTPUT11
{
    float4 pos			: POSITION;
};

struct VS_OUTPUT11WITHFIX
{
    float4 pos			: POSITION;
};

//----------------------------------------------------------------------------
// Shader body 
//----------------------------------------------------------------------------

VS_OUTPUT vs_spec(const VS_INPUT v)
{
	VS_OUTPUT o = (VS_OUTPUT) 0;
	
	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	return o;
};

//This shader sets up the specular components for the
//second pass of the 1.1 shader version.
VS_OUTPUT11WITHFIX vs_spec_11(const VS_INPUT v)
{
	VS_OUTPUT11WITHFIX o = (VS_OUTPUT11WITHFIX) 0;
	
	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	return o;
};

//This shader is a hack to clear the alpha channel.  Should
//be replaced once a better way of doing spec on ps1.1 cards is found.
VS_OUTPUT11 vs_fixup(const VS_INPUT v)
{
	VS_OUTPUT11 o = (VS_OUTPUT11) 0;
	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	return o;
};

//This shader sets up the standard diffuse pixel shader
VS_OUTPUT vs_main(const VS_INPUT v)
{
	VS_OUTPUT o = (VS_OUTPUT) 0;

	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	return o;
};

//--------------------------------------------------------------//
// Technique Sections for shader hardware
//--------------------------------------------------------------//
technique shader_version_2_0
{
	pass Pass_0
	{
		ALPHATESTENABLE = FALSE;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		CULLMODE = NONE;
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TFACTOR;
		ALPHAOP[0] = SELECTARG1;
		ALPHAARG1[0] = TFACTOR;
		COLOROP[1] = DISABLE;
		ALPHAOP[1] = DISABLE;
		FOGENABLE = FALSE;
		STENCILENABLE = FALSE;
		COLORWRITEENABLE = RED | GREEN | BLUE | ALPHA;

		VertexShader = compile vs_2_0 vs_spec();
		PixelShader = NULL;
	}
}

technique shader_version_1_1
{
	pass Pass_Diffuse
	{
		ALPHATESTENABLE = FALSE;		
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		CULLMODE = NONE;
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TFACTOR;
		ALPHAOP[0] = SELECTARG1;
		ALPHAARG1[0] = TFACTOR;
		COLOROP[1] = DISABLE;
		ALPHAOP[1] = DISABLE;
		FOGENABLE = FALSE;
		STENCILENABLE = FALSE;
		COLORWRITEENABLE = RED | GREEN | BLUE | ALPHA;

		VertexShader = compile vs_1_1 vs_main();
		PixelShader = NULL;
	}
}


//--------------------------------------------------------------//
// Technique Section for fixed function hardware.
//--------------------------------------------------------------//
technique fixedFunction
{
	pass Pass_TextureLayer3
	{
		VertexShader = NULL;
		PixelShader = NULL;
		
		ALPHATESTENABLE = FALSE;		
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		CULLMODE = CCW;
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TFACTOR;
		ALPHAOP[0] = SELECTARG1;
		ALPHAARG1[0] = TFACTOR;
		COLOROP[1] = DISABLE;
		ALPHAOP[1] = DISABLE;
		FOGENABLE = FALSE;
		STENCILENABLE = FALSE;
		COLORWRITEENABLE = RED | GREEN | BLUE | ALPHA;
	}
}

