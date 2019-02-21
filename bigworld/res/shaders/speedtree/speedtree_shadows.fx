#include "speedtree.fxh"

bool     g_cullEnabled = true;
float4x4 g_invView;

// Shadow specific constants
int      shadowQuality = 0;
float3   shadowDir : ShadowDir;

float4x4 shadowProj;

float    shadowTexelSize;
float    shadowIntensity = 1.f;
texture  depthTexture;

float    shadowDistance  = 10.f;
float    shadowFadeStart = 9.f;

struct ShadowOutput
{
	float4 pos : POSITION;
	float2 shadowUV : TEXCOORD;
	float depth : TEXCOORD1;
	float fadeout : TEXCOORD2;
	float3 worldNormal : TEXCOORD3;
};

//----------------------------------------------------------------------------
// Pixel shaders
//----------------------------------------------------------------------------

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
	
float4 ps_detail0( ShadowOutput i ) : COLOR
{
	i.depth = max( i.depth, 0.01f );
	
	float fadeout = saturate(-dot(normalize(i.worldNormal), normalize(shadowDir)));
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn *= saturate(i.fadeout) * shadowIntensity * fadeout;
	return float4(depthOn, depthOn, depthOn, depthOn );
}

float4 ps_detail1( ShadowOutput i ) : COLOR
{
	i.depth = max( i.depth, 0.01f );
	
	float depthOn = tex2D( shadowSampler, i.shadowUV ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset1 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV + uvOffset2 ).r > i.depth;
	depthOn += tex2D( shadowSampler, i.shadowUV - uvOffset2 ).r > i.depth;
	float fadeout = saturate(-dot(normalize(i.worldNormal), normalize(shadowDir)));
	depthOn *= saturate(i.fadeout) * shadowIntensity * (1.f / 4.f) * fadeout;
	return float4(depthOn, depthOn, depthOn, depthOn );
}

float4 ps_detail2( ShadowOutput i ) : COLOR
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
	return float4(depthOn, depthOn, depthOn, depthOn );
}

PixelShader pixelShaders[3] =
{
	compile ps_2_0 ps_detail0(),
	compile ps_2_0 ps_detail1(),
	compile ps_2_0 ps_detail2()
};

//----------------------------------------------------------------------------
// Branches
//----------------------------------------------------------------------------

ShadowOutput vs_branches(const VS_INPUT_BRANCHES input)
{
	ShadowOutput output = (ShadowOutput) 0;
	output.pos = branchesOutputPosition(input);
	
	SPT_WORLD_POS_NORMAL(input)	
	output.worldNormal = worldNormal;
	
	float4 shadow   = mul(worldPos, shadowProj);
	output.shadowUV = shadow.xy;
	output.depth    = (1.f - shadow.z) * 1.01;

	float sf = shadowFadeStart;
	if (shadowDistance < (shadowFadeStart + 0.2))
	{
		sf = shadowDistance - 0.2;
	}
		
	output.fadeout = (shadow.z-1.f) / ((sf-shadowDistance) / shadowDistance);

	return output;
};

technique branches
{
	pass Pass_0
	{
		ALPHATESTENABLE  = FALSE;
		ALPHAFUNC        = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND         = ZERO;
		DESTBLEND        = INVSRCCOLOR;
		ZENABLE          = TRUE;
		ZWRITEENABLE     = FALSE;
		ZFUNC            = LESSEQUAL;
		CULLMODE         = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);
		FOGENABLE        = FALSE;
		VertexShader     = compile vs_2_0 vs_branches();
		PixelShader      = (pixelShaders[min(2,max(shadowQuality, 0))]);
	}
}

//----------------------------------------------------------------------------
// Leaves
//----------------------------------------------------------------------------

ShadowOutput vs_leaves(const VS_INPUT_LEAF input)
{
	ShadowOutput output = (ShadowOutput) 0;
	
	float4 outPosition = calcLeafVertex2(input);
	output.pos = mul(outPosition, g_projection);

	SPT_WORLD_POS_NORMAL(input)	
	output.worldNormal = worldNormal;

	float4 shadow   = mul(worldPos, shadowProj);
	output.shadowUV = shadow.xy;
	output.depth    = (1.f - shadow.z) * 1.01;
	
	float sf = shadowFadeStart;
	if (shadowDistance < (shadowFadeStart + 0.2))
	{
		sf = shadowDistance - 0.2;
	}
		
	output.fadeout = (shadow.z-1.f) / ((sf-shadowDistance) / shadowDistance);

	return output;
};

technique leaves
{
	pass Pass_0
	{
		ALPHATESTENABLE  = FALSE;
		ALPHAFUNC        = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND         = ZERO;
		DESTBLEND        = INVSRCCOLOR;
		ZENABLE          = TRUE;
		ZWRITEENABLE     = FALSE;
		ZFUNC            = EQUAL;
		CULLMODE         = CW;
		FOGENABLE        = FALSE;
		VertexShader     = compile vs_2_0 vs_leaves();
		PixelShader      = (pixelShaders[min(2,max(shadowQuality, 0))]);
	}
}

// Null technique to get rid of warnings when changing shader model caps
technique null
{
   pass Pass_0
   {
      VertexShader = NULL;
      PixelShader = NULL;
   }
}