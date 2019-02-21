#include "stdinclude.fxh"

// Exposed artist editable variables.
texture diffuseMap
< 
bool artistEditable = true; 
string UIName = "Diffuse Map";
string UIDesc = "The texture map to use for the heavenly body";
>;

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

// Auto Variables
float4x4 environmentTransform : EnvironmentTransform;
float4x4 world;

// Vertex Formats
struct OUTPUT
{
	float4 pos: POSITION;
	float2 tc0: TEXCOORD0;	
};

BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_CLAMP)

float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};
bool fogEnabled : FogEnabled = true;
bool lightEnable = false;

OUTPUT vs_main( VertexXYZNUV input )
{
	OUTPUT o = (OUTPUT)0;

	float4 worldPos = mul(input.pos, world);
	o.pos = mul(worldPos, environmentTransform).xyww;
	o.tc0 = input.tc;
	
	return o;
}

sampler diffuseSampler = sampler_state
{
	Texture = (diffuseMap);
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};


float4 ps_main( OUTPUT i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc0 );
	
	//TODO : fog colour

	float4 colour;
	colour.xyz = diffuseMap.xyz;	
	colour.w = diffuseMap.w;

	return colour;
}

#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)
technique pixelShader2_0
{
   pass Pass_0
   {
      ALPHATESTENABLE = TRUE;
      ALPHAREF = 1;
      ZENABLE = TRUE;
      SRCBLEND = SRCALPHA;
      DESTBLEND = ONE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_main();
   }
}
#endif


#if (COMPILE_SHADER_MODEL_1)
technique pixelShader1_1
{
   pass Pass_0
   {
      ALPHATESTENABLE = TRUE;
      ALPHAREF = 1;
      ZENABLE = TRUE;
      SRCBLEND = SRCALPHA;
      DESTBLEND = ONE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;

      VertexShader = compile vs_1_1 vs_main();
      PixelShader = compile ps_1_1 ps_main();
   }
}
#endif


#if (COMPILE_SHADER_MODEL_0)
technique fixed_function
{
   pass Pass_0
   {
      ALPHATESTENABLE = TRUE;
      ALPHAREF = 1;
      ZENABLE = TRUE;
      SRCBLEND = SRCALPHA;
      DESTBLEND = ONE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
	  CULLMODE = NONE;
	  
	  BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
      BW_TEXTURESTAGE_TERMINATE(1)

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = NULL;            
   }
}
#endif