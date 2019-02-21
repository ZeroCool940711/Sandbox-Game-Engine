#include "stdinclude.fxh"

// Auto variables
float4x4 viewProj : ViewProjection;
texture floraTexture : FloraTexture;
typedef float4 AnimationArray[64];
AnimationArray animationGrid : FloraAnimationGrid;

// Manual variables
float4 vertexToWorld;
float2 VISIBILITY;
float POS_MULTIPLIER = (1.0,1.0,1.0,1.0);
float FLEX_MULTIPLIER = 1.0 / 255.0;
float2 UV_MULTIPLIER = (1.0/16383.0, 1.0/16383.0);
float4 ambient;

float4x4 shadowProj;
float3 shadowDir : ShadowDir;

float shadowTexelSize;
float shadowIntensity = 1.f;
texture depthTexture;

float shadowDistance = 10.f;
float shadowFadeStart = 9.f;

int shadowQuality = 0;

// Vertex Formats
struct FloraVertex
{
   float4 pos:		POSITION;   
   float2 tc:		TEXCOORD0;
   float2 animation: BLENDWEIGHT;   
};


struct Output
{
	float4 pos : POSITION;
	float2 tc : TEXCOORD;
	float2 shadowUV : TEXCOORD1;
	float depth : TEXCOORD2;
	float fadeout : TEXCOORD3;
};


Output vs_main( FloraVertex input )
{
	Output o = (Output)0;
	
	//animate vertex position	
	float4 animatedPos = input.pos;
	float idx = input.animation[1];	
	animatedPos.xz = input.animation[0] * FLEX_MULTIPLIER * animationGrid[idx].xz + animatedPos.xz;	
	float4 animatedWorldPos = animatedPos + vertexToWorld;
	o.pos = mul(animatedWorldPos, viewProj);
	
	float4 shadow = mul( animatedWorldPos, shadowProj );
	o.shadowUV = shadow.xy;
	o.depth = (1.f - shadow.z) * 1.006;	

	//tc is the flora texture
	o.tc.xy = input.tc.xy * UV_MULTIPLIER;	
	
	float sf = shadowFadeStart;
	
	if (shadowDistance < (shadowFadeStart + 0.2))
		sf = shadowDistance - 0.2;
		
	o.fadeout = ( shadow.z - 1.f ) / ((sf - shadowDistance) / shadowDistance );
	
	return o;
}

sampler shadowSampler = sampler_state
{
	Texture = <depthTexture>;
	MIPFILTER = POINT;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
};

sampler diffuseSampler = sampler_state
{
	Texture = <floraTexture>;
	MIPFILTER = LINEAR;
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
	float4 colour = tex2D( diffuseSampler, i.tc );	
	i.depth = max( i.depth, 0.01f );
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn *= saturate(i.fadeout) * shadowIntensity;
	return float4( depthOn, depthOn, depthOn, colour.w );			
}

float4 ps_detail1( Output i ) : COLOR
{
	float4 colour = tex2D( diffuseSampler, i.tc );
	i.depth = max( i.depth, 0.01f );
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset1 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset2 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset2 ).r > i.depth;	
	depthOn *= saturate(i.fadeout) * shadowIntensity * (1.f / 4.f);
	return float4( depthOn, depthOn, depthOn, colour.w );
}

float4 ps_detail2( Output i ) : COLOR
{
	float4 colour = tex2D( diffuseSampler, i.tc );
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
	depthOn *= saturate(i.fadeout) * shadowIntensity * (1.f / 14.f);
	return float4( depthOn, depthOn, depthOn, colour.w );
}

PixelShader pixelShaders[3] =
{
	compile ps_2_0 ps_detail0(),
	compile ps_2_0 ps_detail1(),
	compile ps_2_0 ps_detail2()
};


technique standard
{
   pass Pass_0
   {
      ALPHABLENDENABLE = TRUE;
      SRCBLEND = ZERO;
	  DESTBLEND = INVSRCCOLOR;
      ZWRITEENABLE = FALSE;
      ALPHATESTENABLE = TRUE;
      ALPHAFUNC = GREATER;
      ZENABLE = TRUE;
      CULLMODE = NONE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      LIGHTING = FALSE;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = (pixelShaders[ min(2,max(shadowQuality, 0) ) ]  );
   }
}
BW_NULL_TECHNIQUE