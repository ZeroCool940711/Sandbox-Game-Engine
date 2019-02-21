#if USE_MRT_DEPTH
#include "depth_helpers.fxh"
#endif

// Exposed artist editable variables.
texture starMap
< 
bool artistEditable = true; 
string UIName = "Star Map";
string UIDesc = "The star map to use for the sky dome";
>;

texture mieMap
< 
bool artistEditable = true; 
string UIName = "MIE Gradient Map";
string UIDesc = "The MIE gradient map to use for the sky dome";
>;

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

// Manual variables
float4x4 environmentTransform : EnvironmentTransform;
texture RAYLEIGH_MAP;
float TIME_OF_DAY = 0.5;
float3 SUN_POSITION = {0,-1,0};
float4 SUN_COLOUR = {1,-1,1,1};
float FOG_AMOUNT = 1;
float MIE_EFFECT_AMOUNT = 0.3;
float TURBIDITY = 0.0;
float VERTEX_HEIGHT_EFFECT = 1.0;
float SUN_HEIGHT_EFFECT = 1.0;
float POWER = 4.0;


// Vertex Formats
struct VertexXYZNUV
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
};

struct OutputDiffuseLighting
{
	float4 pos:     POSITION;
	float3 tc0_Depth:      TEXCOORD0;
	float2 tc1:      TEXCOORD1;
	float2 tc2:      TEXCOORD2;
	float4 diffuse: COLOR;
	float fog: FOG;
};

OutputDiffuseLighting vs_main( VertexXYZNUV input )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;

	o.pos = mul(input.pos, environmentTransform).xyww;
#if USE_MRT_DEPTH
	o.tc0_Depth.z = 1.f;//farPlane.x * farPlane.y;
#endif
	o.tc0_Depth.xy = input.tc;
	
	//useful values
	float4 normVPos = normalize(input.pos);	
	float horizonVPos = 1.0 - abs(normVPos.y);
	float horizonSPos = 1.0 - abs(SUN_POSITION.y);
	
	//Rayliegh texture coordinates.	
	o.tc1.x = TIME_OF_DAY;
	o.tc1.y = saturate(1.0-normVPos.y);
	
	//Mie texture coordinates	
	float fwd_scattering_amount = saturate(dot(normVPos.xyz, SUN_POSITION));
	float mie_amount = (pow(fwd_scattering_amount,POWER));
	float sunHeightEffect = (SUN_POSITION.y * normVPos.y * SUN_HEIGHT_EFFECT);
	float vertexHeightEffect = (horizonVPos * horizonSPos * VERTEX_HEIGHT_EFFECT);	
	float additional = saturate(fwd_scattering_amount * (vertexHeightEffect + sunHeightEffect) + TURBIDITY);	
	o.tc2.xy = (mie_amount + additional) * MIE_EFFECT_AMOUNT;	
	
	o.fog = FOG_AMOUNT;
	o.diffuse = SUN_COLOUR;
	o.diffuse.w = mie_amount + FOG_AMOUNT;
	
	
	return o;
}

OutputDiffuseLighting vs_mx4( VertexXYZNUV input )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;

	o.pos = mul(input.pos, environmentTransform);
	o.tc0_Depth.xy = input.tc;
	
	//useful values
	float4 normVPos = normalize(input.pos);	
	float horizonVPos = 1.0 - abs(normVPos.y);
	float horizonSPos = 1.0 - abs(SUN_POSITION.y);
	
	//Rayliegh texture coordinates.	
	o.tc1.x = TIME_OF_DAY;
	o.tc1.y = saturate(1.0-normVPos.y);
	
	//Mie texture coordinates	
	float fwd_scattering_amount = saturate(dot(normVPos.xyz, SUN_POSITION));
	float mie_amount = (pow(fwd_scattering_amount,POWER));
	float sunHeightEffect = (SUN_POSITION.y * normVPos.y * SUN_HEIGHT_EFFECT);
	float vertexHeightEffect = (horizonVPos * horizonSPos * VERTEX_HEIGHT_EFFECT);	
	float additional = saturate(fwd_scattering_amount * (vertexHeightEffect + sunHeightEffect) + TURBIDITY);	
	o.tc2.xy = (mie_amount + additional) * MIE_EFFECT_AMOUNT;	
	
	o.fog = FOG_AMOUNT;
	o.diffuse = SUN_COLOUR;
	o.diffuse.w = mie_amount + FOG_AMOUNT;
	
	
	return o;
}


#if USE_MRT_DEPTH
#include "depth_helpers.fxh"

sampler starMapSampler = sampler_state
{
	Texture = (starMap);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};


sampler rayleighSampler = sampler_state
{
	Texture = (RAYLEIGH_MAP);
	ADDRESSU = WRAP;
	ADDRESSV = CLAMP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};


sampler mieSampler = sampler_state
{
	Texture = (mieMap);
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

// Pixel shader implementation of the fixed function version...
// I think it's correct!
BW_COLOUR_OUT ps_main( OutputDiffuseLighting i )
{
	float4 colour;

	//stage 0 - stars
	float4 star = tex2D( starMapSampler, i.tc0_Depth.xy );
	float4 ray = tex2D( rayleighSampler, i.tc1 );
	float4 mie = tex2D( mieSampler, i.tc2 );

	colour.a = i.diffuse.a;
	
	//stage 1 - rayleigh
	colour.rgb = ray * ray.a + star * (1-ray.a);
 
	//stage 2 - mie
	colour.rgb = mie * colour.rgb + colour.rgb;

	BW_FINAL_COLOUR( i.tc0_Depth.z, colour )
}
technique standard2
{
   pass Pass_0
   {
      ALPHATESTENABLE = FALSE;
      SRCBLEND = ONE;
      DESTBLEND = ZERO;
      ZENABLE = TRUE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE;
	  FOGSTART = 1.0;
	  FOGEND = 0.0;
	  FOGTABLEMODE = NONE;
	  FOGVERTEXMODE = LINEAR; 
      ALPHABLENDENABLE = FALSE;
      COLORWRITEENABLE = RED | GREEN | BLUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
	  VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_main();
   }
}

#endif //USE_MRT_DEPTH

technique standard
{
   pass Pass_0
   {
      ALPHATESTENABLE = FALSE;
      SRCBLEND = ONE;
      DESTBLEND = ZERO;
      ZENABLE = TRUE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE;
      FOGSTART = 1.0;
	  FOGEND = 0.0;
	  FOGTABLEMODE = NONE;
	  FOGVERTEXMODE = LINEAR; 
      ALPHABLENDENABLE = FALSE;
      COLORWRITEENABLE = RED | GREEN | BLUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      //stage 0 - stars
      ColorOp[0] = SELECTARG1;
      ColorArg1[0] = TEXTURE;
      ColorArg2[0] = CURRENT;
      AlphaOp[0] = SELECTARG1;
      AlphaArg1[0] = DIFFUSE;
      AlphaArg2[0] = CURRENT;
      Texture[0] = <starMap>;
      
      //stage 1 - rayleigh
      ColorOp[1] = BLENDTEXTUREALPHA;
      ColorArg1[1] = TEXTURE;
      ColorArg2[1] = CURRENT;
      AlphaOp[1] = DISABLE;
      Texture[1] = <RAYLEIGH_MAP>;
      ADDRESSU[1] = WRAP;
      ADDRESSV[1] = CLAMP;      
      
      //stage 2 - mie
      ColorOp[2] = MULTIPLYADD;      
      ColorArg1[2] = TEXTURE;
      ColorArg2[2] = CURRENT;
      Texture[2] = <mieMap>;
      ADDRESSV[2] = CLAMP;
      ADDRESSU[2] = CLAMP;
      
      //stage 3 - disable
      ColorOp[3] = DISABLE;
      AlphaOp[3] = DISABLE;

      VertexShader = compile vs_1_1 vs_main();
      PixelShader = NULL;
   }
}


technique mx400
{
   pass Pass_0
   {
      ALPHATESTENABLE = FALSE;
      SRCBLEND = ONE;
      DESTBLEND = ZERO;
      ZENABLE = TRUE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE;
      FOGSTART = 1.0;
	  FOGEND = 0.0;
	  FOGTABLEMODE = NONE;
	  FOGVERTEXMODE = LINEAR; 
      ALPHABLENDENABLE = FALSE;
      COLORWRITEENABLE = RED | GREEN | BLUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      //stage 0 - stars
      ColorOp[0] = SELECTARG1;
      ColorArg1[0] = TEXTURE;
      ColorArg2[0] = CURRENT;
      AlphaOp[0] = SELECTARG1;
      AlphaArg1[0] = DIFFUSE;
      AlphaArg2[0] = CURRENT;
      Texture[0] = <starMap>;
      
      //stage 1 - rayleigh
      ColorOp[1] = BLENDTEXTUREALPHA;
      ColorArg1[1] = TEXTURE;
      ColorArg2[1] = CURRENT;
      AlphaOp[1] = DISABLE;
      Texture[1] = <RAYLEIGH_MAP>;
      ADDRESSU[1] = WRAP;
      ADDRESSV[1] = CLAMP;      
      
      //stage 2 - disable on MX400, only 2 texture stages at once.
      ColorOp[2] = DISABLE;
      AlphaOp[2] = DISABLE;
      
      VertexShader = compile vs_2_0 vs_main();
      PixelShader = NULL;
   }
}

technique mx4
{
   pass Pass_0
   {
      ALPHATESTENABLE = FALSE;
      SRCBLEND = ONE;
      DESTBLEND = ZERO;
      ZENABLE = FALSE;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE;
      FOGSTART = 1.0;
	  FOGEND = 0.0;
	  FOGTABLEMODE = NONE;
	  FOGVERTEXMODE = LINEAR; 
      ALPHABLENDENABLE = FALSE;
      COLORWRITEENABLE = RED | GREEN | BLUE;
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      //stage 0 - stars
      ColorOp[0] = SELECTARG1;
      ColorArg1[0] = TEXTURE;
      ColorArg2[0] = CURRENT;
      AlphaOp[0] = SELECTARG1;
      AlphaArg1[0] = DIFFUSE;
      AlphaArg2[0] = CURRENT;
      Texture[0] = <starMap>;
      
      //stage 1 - rayleigh
      ColorOp[1] = BLENDTEXTUREALPHA;
      ColorArg1[1] = TEXTURE;
      ColorArg2[1] = CURRENT;
      AlphaOp[1] = DISABLE;
      Texture[1] = <RAYLEIGH_MAP>;
      ADDRESSU[1] = WRAP;
      ADDRESSV[1] = CLAMP;      
      
      //stage 2 - disable on MX4, only 2 texture stages at once.
      ColorOp[2] = DISABLE;
      AlphaOp[2] = DISABLE;
      
      VertexShader = compile vs_2_0 vs_mx4();
      PixelShader = NULL;
   }
}