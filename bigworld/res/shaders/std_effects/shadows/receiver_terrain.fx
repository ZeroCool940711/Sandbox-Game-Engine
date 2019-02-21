#include "stdinclude.fxh"
// variables
texture Layer0;
texture Layer1;
texture Layer2;
texture Layer3;
float3 axis0 = {0.0, 0.0, 1.0};
float3 axis1 = {0.0, 1.0, 0.0};
bool alphaTestEnable = false;
int alphaReference = 0;
float4x4 world;
float4x4 view;
float4x4 viewProj;
float4x4 proj;
float4x4 textransform;
int shadowQuality = 0;
float3 shadowDir : ShadowDir;

// these variables can be set at runtime, render/terrain/...
float specularDiffuseAmount = 0.5;
float specularPower = 4.0;

float4x4 shadowProj;

float shadowTexelSize;
float shadowIntensity = 1.f;
texture depthTexture;

float shadowDistance = 10.f;
float shadowFadeStart = 9.f;

struct VS_INPUT
{
    float4 pos		: POSITION;
    float3 normal		: NORMAL;
};

struct Output
{
	float4 pos : POSITION;
	float2 shadowUV : TEXCOORD;
	float depth : TEXCOORD1;
	float fadeout : TEXCOORD2;
	float3 worldNormal : TEXCOORD3;
};


//----------------------------------------------------------------------------
// Shader body 
//----------------------------------------------------------------------------

Output vs_main(const VS_INPUT v)
{
	Output o = (Output) 0;
	
	float4 worldPos = mul( v.pos, world );
	o.pos = mul( worldPos, viewProj );
	o.worldNormal = v.normal;

	float4 shadow = mul( worldPos, shadowProj );

	o.shadowUV = shadow.xy;
	o.depth = (1.f - shadow.z) * 1.01;

	float sf = shadowFadeStart;
	
	if (shadowDistance < (shadowFadeStart + 0.2))
		sf = shadowDistance - 0.2;
		
	o.fadeout = ( shadow.z - 1.f ) / ((sf - shadowDistance) / shadowDistance );
	return o;
};

sampler shadowSampler = sampler_state
{
	Texture = <depthTexture>;
	MIPFILTER = POINT;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
};

float2 uvOffset1 = float2( ((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 0.5f) );
float2 uvOffset2 = float2( -((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 0.5f) );

float2 uvOffset3 = float2( ((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 0.5f) );
float2 uvOffset4 = float2( ((1.f/1024.f) * 1.5f), -((1.f/1024.f) * 0.5f) );
float2 uvOffset5 = float2( ((1.f/1024.f) * 0.5f), ((1.f/1024.f) * 1.5f) );
float2 uvOffset6 = float2( ((1.f/1024.f) * 0.5f), -((1.f/1024.f) * 1.5f) );
	
float2 uvOffset7 = float2( ((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 1.5f) );
float2 uvOffset8 = float2( -((1.f/1024.f) * 1.5f), ((1.f/1024.f) * 1.5f) );
	
float4 ps_detail0( Output i ) : COLOR
{
	i.depth = max( i.depth, 0.01f );
	
	float fadeout = saturate(-dot(normalize(i.worldNormal), normalize(shadowDir)));
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn *= saturate(i.fadeout) * shadowIntensity * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}

float4 ps_detail1( Output i ) : COLOR
{
	i.depth = max( i.depth, 0.01f );
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset1 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset2 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset2 ).r > i.depth;
	float fadeout = saturate(-dot(normalize(i.worldNormal), normalize(shadowDir)));
	depthOn *= saturate(i.fadeout) * shadowIntensity * (1.f / 4.f) * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}

float4 ps_detail2( Output i ) : COLOR
{
	i.depth = max( i.depth, 0.01f );
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset1 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset2 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset2 ).r > i.depth;
	depthOn *= 1.5f;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset3 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset3 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset4 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset4 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset5 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset5 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset6 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset6 ).r > i.depth;
	float fadeout = saturate(-dot(normalize(i.worldNormal), normalize(shadowDir)));
	depthOn *= saturate(i.fadeout) * shadowIntensity * (1.f / 14.f) * fadeout;
	return float4( depthOn, depthOn, depthOn, depthOn );
}

PixelShader pixelShaders[3] =
{
	compile ps_2_0 ps_detail0(),
	compile ps_2_0 ps_detail1(),
	compile ps_2_0 ps_detail2()
};

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
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ZERO;
		DESTBLEND = INVSRCCOLOR;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		CULLMODE = CCW;
		FOGENABLE=FALSE;
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = (pixelShaders[ min(2,max(shadowQuality, 0) ) ]  );
	}
}
BW_NULL_TECHNIQUE
