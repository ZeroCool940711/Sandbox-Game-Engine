#include "stdinclude.fxh"

// Auto variables
float4x4 viewProj : ViewProjection;
float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};
texture lightMap : FloraLightMap;
texture floraTexture : FloraTexture;
typedef float4 AnimationArray[64];
AnimationArray animationGrid : FloraAnimationGrid;

BW_SKY_LIGHT_MAP

// Manual variables
float4 vertexToWorld;
float2 VISIBILITY;
matrix LIGHT_MAP_PROJECTION;
//float POS_MULTIPLIER = (200.0/32767.0,200.0/32767.0,200.0/32767.0,1.0);
float POS_MULTIPLIER = (1.0,1.0,1.0,1.0);
float FLEX_MULTIPLIER = 1.0 / 255.0;
float2 UV_MULTIPLIER = (1.0/16383.0, 1.0/16383.0);
float4 ambient;
float4 lightMapTransform[2] : FloraLightMapTransform;



// Vertex Formats
struct FloraVertex
{
   float4 pos:		POSITION;   
   float2 tc:		TEXCOORD0;
   float2 animation: BLENDWEIGHT;   
};

struct OutputVertex
{
	float4 pos: POSITION;
	float2 tc0: TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap: TEXCOORD1;
	float2 tc2: TEXCOORD2;	
#else
	float2 tc2: TEXCOORD1;
#endif
	float4 col: COLOR;
	float fog: FOG;
};


OutputVertex vs_main( FloraVertex input )
{
	OutputVertex o = (OutputVertex)0;
	
	//animate vertex position	
	/*float4 animatedPos = input.pos * POS_MULTIPLIER;
	animatedPos.w = 1.0;*/
	float4 animatedPos = input.pos;
	float idx = input.animation[1];	
	animatedPos.xz = input.animation[0] * FLEX_MULTIPLIER * animationGrid[idx].xz + animatedPos.xz;	
	float4 animatedWorldPos = animatedPos + vertexToWorld;
	o.pos = mul(animatedWorldPos, viewProj);
	o.tc2.xy = input.tc.xy * UV_MULTIPLIER;
	
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	
	//fade out based on distance
	float dist = length(o.pos.xyz) * VISIBILITY.x + VISIBILITY.y;	
	o.col.w = 1.0 - saturate(dist);	
	
	//tc0 is the terrain light map (diffuse lighting + terrain shadows)	
	o.tc0.x = dot( animatedWorldPos, lightMapTransform[0] );
	o.tc0.y = dot( animatedWorldPos, lightMapTransform[1] );
	
	//tc1 is the sky light map (cloud shadows)		
	BW_SKY_MAP_COORDS(o.skyLightMap,animatedWorldPos)
	
	return o;
}


technique standard
{
   pass Pass_0
   {
	  //The following states are setup in code
	  //This is due to the 2 passes - alpha test pass and
	  //alpha blend pass
      /*ALPHABLENDENABLE = TRUE;
      ALPHAREF = 0x00000001;
      SRCBLEND = SRC_ALPHA;
      DESTBLEND = INV_SRC_ALPHA;
      ZWRITEENABLE = FALSE;*/
      
      ALPHATESTENABLE = TRUE;
      ZENABLE = TRUE;
      CULLMODE = NONE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = TRUE; 
      LIGHTING = FALSE;     
      
      TEXTUREFACTOR = <ambient>;
      
      //stage 0 - light map
      ColorOp[0] = ADD;
      ColorArg1[0] = TEXTURE;
      ColorArg2[0] = TFACTOR;
      AlphaOp[0] = SELECTARG2;
      AlphaArg1[0] = TEXTURE;
      AlphaArg2[0] = CURRENT;
      ADDRESSU[0] = CLAMP;
      ADDRESSV[0] = CLAMP;
      Texture[0] = (lightMap);
      TexCoordIndex[0] = 0;
      
#if SKY_LIGHT_MAP_ENABLE
      //stage 1 - sky light map
      ColorOp[1] = MODULATEINVALPHA_ADDCOLOR;
      ColorArg1[1] = TEXTURE;
      ColorArg2[1] = CURRENT;
      AlphaOp[1] = SELECTARG1;
      AlphaArg1[1] = CURRENT;
      AlphaArg2[1] = CURRENT;
      Texture[1] = (skyLightMap);
      ADDRESSU[1] = WRAP;
      ADDRESSV[1] = CLAMP;
      TexCoordIndex[1] = 1;
#endif
      
      //stage 2 - diffuse
      ColorOp[CLOUDMAP_STAGE_PLUS1] = MODULATE;
      ColorArg1[CLOUDMAP_STAGE_PLUS1] = TEXTURE;
      ColorArg2[CLOUDMAP_STAGE_PLUS1] = CURRENT;      
      AlphaOp[CLOUDMAP_STAGE_PLUS1] = MODULATE;
      AlphaArg1[CLOUDMAP_STAGE_PLUS1] = CURRENT;
      AlphaArg2[CLOUDMAP_STAGE_PLUS1] = TEXTURE;
      Texture[CLOUDMAP_STAGE_PLUS1] = (floraTexture);      
      TexCoordIndex[CLOUDMAP_STAGE_PLUS1] = CLOUDMAP_STAGE_PLUS1;
      
      ColorOp[CLOUDMAP_STAGE_PLUS2] = DISABLE;
      AlphaOp[CLOUDMAP_STAGE_PLUS2] = DISABLE;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = NULL;      
   }
}