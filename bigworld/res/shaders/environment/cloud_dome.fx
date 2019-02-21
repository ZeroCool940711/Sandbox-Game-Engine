#include "stdinclude.fxh"

float4 lower_uTransform
<
	bool artistEditable = true;
	string UIName = "Lower U Transform";
	string UIDesc = "The U-transform vector for the lower cloud layer";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0.0075,0};

float4 lower_vTransform
<
	bool artistEditable = true;
	string UIName = "Lower V Transform";
	string UIDesc = "The V-transform vector for the lower cloud layer";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0.005,0};

float4 upper_uTransform
<
	bool artistEditable = true;
	string UIName = "Upper U Transform";
	string UIDesc = "The U-transform vector for the upper cloud layer";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0.005,0};

float4 upper_vTransform
<
	bool artistEditable = true;
	string UIName = "Upper V Transform";
	string UIDesc = "The V-transform vector for the upper cloud layer";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0.0025,0};

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

// Auto Variables
float4x4 environmentTransform : EnvironmentTransform;
int occlusionTest : EnvironmentOcclusionTest;
int occlusionAlphaRef : EnvironmentOcclusionAlphaRef;
float time : Time;

// Engine-driven variables.
texture cloudMap1 : CloudTexture1;
texture cloudMap2 : CloudTexture2;
texture cloudMap3 : CloudTexture3;
float4 blendAmount : CloudsBlendAmount;
int bUseBlend : CloudsUseBlend;

BW_DIFFUSE_LIGHTING

struct OutputLighting
{
	float4 pos: POSITION;
	float2 tc0: TEXCOORD0;
	float2 tc1: TEXCOORD1;
	float2 tc2: TEXCOORD2;
	float2 tc3: TEXCOORD3;
	float4 col: COLOR;
	float fog: FOG;
};


float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};
bool fogEnabled : FogEnabled = true;

OutputLighting vs_main( VertexXYZNUV input )
{
	OutputLighting o = (OutputLighting)0;

	o.pos = mul(input.pos, environmentTransform).xyww;
	
	float4 tc = float4(input.tc, 1, 1);
	o.tc0.x = dot( tc, lower_uTransform * float4(1,1,time,1) );
	o.tc0.y = dot( tc, lower_vTransform * float4(1,1,time,1) );	
	
	o.tc1.x = dot( tc, upper_uTransform * float4(1,1,time,1) );
	o.tc1.y = dot( tc, upper_vTransform * float4(1,1,time,1) );
	
	o.tc2 = o.tc0;
	o.tc3 = o.tc1;
	
	//use height of vertex for fog
	float2 fogging = float2((-1.0 / (fogEnd - fogStart)), (fogEnd / (fogEnd - fogStart)));
	o.fog = input.pos.y/3.0;
	
	o.col = directionalLights[0].colour + ambientColour;	
	
	return o;
}

sampler cloudMap1Sampler = sampler_state
{
	Texture = (cloudMap1);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler cloudMap2Sampler = sampler_state
{
	Texture = (cloudMap2);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler cloudMap3Sampler = sampler_state
{
	Texture = (cloudMap3);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

/**
 *	This pixel shader is used when there is no transition
 *	required for the clouds.  It saves fillrate by only drawing
 *	the two cloud layers
 */
float4 ps_main( OutputLighting i ) : COLOR0
{	
	float4 clouds0 = tex2D( cloudMap1Sampler, i.tc0 );
	float4 clouds1 = tex2D( cloudMap2Sampler, i.tc1 );
	
	float4 clouds;
	clouds.xyz = clouds0.w * clouds0.xyz + (1.0 - clouds0.w) * clouds1.xyz;
	clouds.w = max(clouds0.w, clouds1.w);
	
	//light
	clouds *= i.col;	
	
	return clouds;
}


/**
 *	This pixel shader is used when there is a transition
 *	occuring for the clouds.  It blends between the current
 *	and the next cloud texture map based on blend factors.
 */
float4 ps_blend( OutputLighting i ) : COLOR0
{
	float4 clouds0 = tex2D( cloudMap1Sampler, i.tc0 );
	float4 clouds1 = tex2D( cloudMap2Sampler, i.tc1 );
	float4 clouds2 = tex2D( cloudMap3Sampler, i.tc2 );
	float4 clouds3 = tex2D( cloudMap3Sampler, i.tc3 );
	
	float4 system[2];
	system[0] = blendAmount[0] * clouds0 + blendAmount[1] * clouds2;
	system[1] = blendAmount[2] * clouds1 + blendAmount[3] * clouds3;
	
	float4 clouds;
	clouds.xyz = system[0].w * system[0].xyz + (1.0 - system[0].w) * system[1].xyz;
	clouds.w = max(system[0].w, system[1].w);	
	
	//light
	clouds *= i.col;	
	
	return clouds;
}


PixelShader pixelShaders[2] =
{	
	compile ps_2_0 ps_main(),
	compile ps_2_0 ps_blend()
};


//return the alpha reference for the technique.
int alphaReference()
{
	if (!occlusionTest)
		return 0;		
	else
		return occlusionAlphaRef;
};

technique pixelShader1_1
{
   pass Pass_0
   {
      ALPHATESTENABLE = (occlusionTest);      
      ALPHAREF = alphaReference();      
      ZENABLE = (1-occlusionTest);
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;

      VertexShader = compile vs_2_0 vs_main();      
      //Note - no need for writing occlusion shaders,
      //as there is no benefit to changing it (no
      //instructions will be saved)
      PixelShader = (pixelShaders[bUseBlend]);
   }
}
BW_NULL_TECHNIQUE