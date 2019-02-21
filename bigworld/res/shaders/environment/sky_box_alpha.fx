#include "environment_helpers.fxh"

// Exposed artist editable variables.
BW_ARTIST_EDITABLE_CLOUD_MAP( diffuseMap, "Cloud Map" )
BW_ARTIST_EDITABLE_FOG_MAP( fogMap, "Fog Mask" )

BW_CLOUD_MAP_SAMPLER( diffuseSampler, diffuseMap, CLAMP )
BW_FOG_MAP_SAMPLER( fogMapSampler, fogMap )

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

// Auto Variables
float4x4 environmentTransform : EnvironmentTransform;


struct Output
{
	float4 pos: POSITION;
	float2 tc0: TEXCOORD0;
	float2 tc1: TEXCOORD1;	
};


float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};
bool fogEnabled : FogEnabled = true;

Output vs_main( VertexXYZNUV input )
{
	Output o = (Output)0;

	o.pos = mul(input.pos, environmentTransform).xyww;
	o.tc0 = input.tc;
	o.tc1 = input.tc;
	
	return o;
}


//blend between the skybox map and the fog colour based on the fog gradient map
float4 ps_main( Output i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc0 );
	float4 fogAmount;

	if (fogEnabled)
		fogAmount = tex2D( fogMapSampler, i.tc1 );
	else
		fogAmount = float4(0,0,0,0);

	float4 colour;
	colour.xyz = (float3(1,1,1) - fogAmount.xyz) * diffuseMap.xyz;
	colour.xyz += (fogColour.xyz * fogAmount.xyz);
	colour.w = diffuseMap.w;

	return colour;
}

//occlusion test shader
float4 ps_occlusion( Output i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc0 );	
	return diffuseMap;
}


//Since sky_box.fx is pixel shader 1.1 or Fixed Function only,
//it does not support sky shadowing.
float4 ps_shadow( Output i ) : COLOR0
{
	float4 colour = (0,0,0,0);
	return colour;	
}


PixelShader pixelShaders[3] = 
{
	compile ps_2_0 ps_main(),
	compile ps_2_0 ps_occlusion(),
	compile ps_2_0 ps_shadow()
};


//The pixel shader version turns off fogging with the engine.
technique pixelShader1_1
{
   pass Pass_0
   {
      ALPHATESTENABLE = (occlusionTest);      
      ALPHAREF = alphaReference();      
      ZENABLE = enableZ();
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = (pixelShaders[pixelShaderIndex()]);
   }
}


//The fixed function version cannot turn off fogging.
technique fixedFunction
{
   pass Pass_0
   {
      ALPHATESTENABLE = (occlusionTest);
      ALPHAREF = alphaReference();      
      ZENABLE = enableZ();
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;
      TEXTUREFACTOR = <fogColour>;
      
      //stage 0 - diffuse
      ColorOp[0] = SELECTARG1;
      ColorArg1[0] = TEXTURE;
      ColorArg2[0] = CURRENT;
      AlphaOp[0] = SELECTARG1;
      AlphaArg1[0] = TEXTURE;
      AlphaArg2[0] = CURRENT;
      Texture[0] = <diffuseMap>;
      ADDRESSU[0] = CLAMP;
      ADDRESSV[0] = CLAMP;
      ADDRESSW[0] = CLAMP;
      
      //stage 1 - fog
      ColorOp[1] = LERP;      
      ColorArg1[1] = TFACTOR;
      ColorArg2[1] = CURRENT;
      ColorArg0[1] = TEXTURE;
      AlphaOp[1] = SELECTARG1;
      AlphaArg1[1] = CURRENT;
      AlphaArg2[1] = CURRENT;
      Texture[1] = <fogMap>;
      
      ColorOp[2] = DISABLE;
      AlphaOp[2] = DISABLE;

      VertexShader = compile vs_1_1 vs_main();
      PixelShader = NULL;
   }
}