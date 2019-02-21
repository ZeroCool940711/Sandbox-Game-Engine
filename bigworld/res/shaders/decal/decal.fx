#include "stdinclude.fxh"

// Auto variables
float4x4 view : View;
float4x4 projection : Projection;
float4x4 viewProjection : ViewProjection;


// Exposed artist editable variables.
texture decalMap;


float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};

float4x4 identity = { 
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1 };
			

#define BW_TEXTURESTAGE_DECAL(stage, inTexture)\
COLOROP[stage] = SELECTARG1;\
COLORARG1[stage] = TEXTURE;\
COLORARG2[stage] = DIFFUSE;\
ALPHAOP[stage] = SELECTARG1;\
ALPHAARG1[stage] = TEXTURE;\
ALPHAARG2[stage] = DIFFUSE;\
Texture[stage] = (inTexture);\
ADDRESSU[stage] = WRAP;\
ADDRESSV[stage] = WRAP;\
ADDRESSW[stage] = WRAP;\
MAGFILTER[stage] = LINEAR;\
MINFILTER[stage] = LINEAR;\
MIPFILTER[stage] = LINEAR;\
MAXMIPLEVEL[stage] = 0;\
MIPMAPLODBIAS[stage] = 0;\
TexCoordIndex[stage] = stage;

sampler decalSampler = sampler_state
{
	Texture = (decalMap);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

struct VertexXYZUV
{
   float4 pos:		POSITION;
   float2 tc:		TEXCOORD0;
};

struct DecalOut
{
	float4 pos:	POSITION;
	float2 tc: TEXCOORD0;
	float3 fog : TEXCOORD1;
};

DecalOut decalVS ( VertexXYZUV i )
{
	DecalOut o = (DecalOut)0;
	o.pos = mul( i.pos, viewProjection );
	float2 fogging = float2((-1.0 / (fogEnd - fogStart)), (fogEnd / (fogEnd - fogStart)));
	o.fog.xyz = o.pos.w * fogging.x + fogging.y;
	o.tc = i.tc;
	return o;
};

float4 decalPS( DecalOut i ) : COLOR0
{
	float4 tex = tex2D( decalSampler, i.tc );
	i.fog = saturate( i.fog );
	return float4( (tex.xyz*i.fog + ((1-i.fog)*fogColour.xyz)), tex.w);
};


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique shaderDecal
{
	pass Pass_0
	{
		LIGHTING = FALSE;
		ALPHATESTENABLE = FALSE;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = INVSRCALPHA;
		DESTBLEND = SRCALPHA;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		BW_TEXTURESTAGE_DECAL(0, decalMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		CULLMODE = CCW;
		VIEWTRANSFORM = <view>;
		PROJECTIONTRANSFORM = <projection>;
		WORLDTRANSFORM[0] = <identity>;
				
		FOGENABLE = FALSE; //doing the fogging in the shader
		
		VertexShader = compile vs_2_0 decalVS();
		PixelShader = compile ps_2_0 decalPS();
	}
}

//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique fixedFunctionDecal
{
	pass Pass_0
	{
		FOGENABLE = TRUE;
		FOGSTART = <fogStart>;
		FOGEND = <fogEnd>;
		FOGCOLOR = <fogColour>;
		FOGTABLEMODE = NONE;
		FOGVERTEXMODE = LINEAR;

		LIGHTING = FALSE;
		ALPHATESTENABLE = FALSE;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = INVSRCALPHA;
		DESTBLEND = SRCALPHA;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		BW_TEXTURESTAGE_DECAL(0, decalMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		CULLMODE = CCW;
		VIEWTRANSFORM = <view>;
		PROJECTIONTRANSFORM = <projection>;
		WORLDTRANSFORM[0] = <identity>;
		VertexShader = NULL;
		PixelShader = NULL;
	}
}
