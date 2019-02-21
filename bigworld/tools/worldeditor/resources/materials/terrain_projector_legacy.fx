#include "stdinclude.fxh"

// variables
texture diffuse;
bool alphaTestEnable = false;
int alphaReference = 0;
float4x4 world;
float4x4 view;
float4x4 viewProj;
float4x4 proj;
float4x4 projtextransform;
float4x4 fftextransform;
int wrap = BW_WRAP;	//D3DTADDRESS_WRAP
BW_NON_EDITABLE_ADDITIVE_BLEND

// Define the lighting type used in this shader
BW_DIFFUSE_LIGHTING

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
    float2 t0			: TEXCOORD0;
    float2 t1			: TEXCOORD1;
    float2 t2			: TEXCOORD2;
    float2 t3			: TEXCOORD3;
    float4 diffuse		: COLOR0;
    float4 blend		: COLOR1;
    float3 reflection	: TEXCOORD4;
    float3 view     	: TEXCOORD5;
    float fog			: FOG;
};

struct VS_OUTPUT11
{
    float4 pos			: POSITION;
    float3 reflection	: TEXCOORD0;
    float3 view     	: TEXCOORD1;    
};


//----------------------------------------------------------------------------
// Shader body 
//----------------------------------------------------------------------------

//This shader sets up the standard diffuse pixel shader
VS_OUTPUT vs_main(const VS_INPUT v)
{
	VS_OUTPUT o = (VS_OUTPUT) 0;

	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	float2 tc;
	tc.x = dot( v.pos, projtextransform[0] );
	tc.y = dot( v.pos, projtextransform[2] );
	o.t0.xy=tc.xy;	
	o.diffuse.xyzw = (1.0,1.0,1.0,1.0);	
	return o;
};

sampler diffuseSampler = sampler_state
{
	Texture = (diffuse);
	ADDRESSU = (wrap);
	ADDRESSV = (wrap);
	ADDRESSW = (wrap);
	BORDERCOLOR = 0;
	MAGFILTER = LINEAR;
	MINFILTER = ANISOTROPIC;
	MIPFILTER = LINEAR;
	MAXANISOTROPY = 16;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};


float4 ps_main( const VS_OUTPUT input ) : COLOR0
{	
	float4 diffuse = tex2D( diffuseSampler, input.t0 );
	return diffuse;
};


technique shader_version
{
	pass Pass_0
	{
		BW_BLENDING_ADD
		FOGENABLE = FALSE;
		SPECULARENABLE = TRUE;
		CULLMODE = CCW;
		
		VertexShader = compile vs_1_1 vs_main();
		PixelShader = compile ps_1_1 ps_main();
	}
}

technique fixedFunction
{
	pass Pass_0
	{
		VertexShader = NULL;
		PixelShader = NULL;
		
		ALPHATESTENABLE = FALSE;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ONE;		
		WORLDTRANSFORM[0] = <world>;
		VIEWTRANSFORM = <view>;
		PROJECTIONTRANSFORM = <proj>;
		SPECULARENABLE = FALSE;				
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		ZENABLE = TRUE;
		LIGHTING = FALSE;		
		
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TEXTURE;
		COLORARG2[0] = DIFFUSE;
		ALPHAOP[0] = DISABLE;		
		TEXTURE[0] = <diffuse>;		
		TEXCOORDINDEX[0] = CAMERASPACEPOSITION;
		TEXTURETRANSFORM[0] = <fftextransform>;
		TEXTURETRANSFORMFLAGS[0] = COUNT2;
		ADDRESSU[0] = (wrap);
		ADDRESSV[0] = (wrap);
		ADDRESSW[0] = (wrap);
		MAGFILTER[0] = LINEAR;
		MINFILTER[0] = LINEAR;
		MIPFILTER[0] = NONE;
		MAXMIPLEVEL[0] = 0;
		MIPMAPLODBIAS[0] = 0;		
		
		COLOROP[1] = DISABLE;				
	}
}