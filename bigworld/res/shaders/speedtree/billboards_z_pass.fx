#include "speedtree.fxh"

float g_bbAlphaRef;

struct VS_INPUT_BB
{
    float4 pos         : POSITION;
    float3 lightNormal : NORMAL;
    float3 alphaNormal : TEXCOORD0;
    float2 texCoords   : TEXCOORD1;
	float3 binormal    : TEXCOORD2;
	float3 tangent     : TEXCOORD3;
};

struct SpeedTreeVertexOutput
{
	float4 pos		: POSITION;
	float4 tcDepth	: TEXCOORD0;
};

//----------------------------------------------------------------------------
// Pixel shaders
//----------------------------------------------------------------------------

BW_COLOUR_OUT ps20_billboards( SpeedTreeVertexOutput i ) : COLOR
{
	float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
	float4 colour = float4( 1, 0, 0, diffuseMap.w - i.tcDepth.w );
	BW_FINAL_COLOUR( i.tcDepth.z, colour )
//	return colour;
}

//----------------------------------------------------------------------------
// Billboards
//----------------------------------------------------------------------------

SpeedTreeVertexOutput vs_billboards(const VS_INPUT_BB i)
{
	SpeedTreeVertexOutput o = (SpeedTreeVertexOutput) 0;

	o.pos = mul(i.pos, g_worldViewProj);
	o.tcDepth.xy = i.texCoords.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)

	// view angle alpha
	float3 alphaNormal = mul(i.alphaNormal, g_world);
	alphaNormal = normalize(alphaNormal);
	float cameraDim = abs(dot(alphaNormal, g_cameraDir.xyz));
	o.tcDepth.w = 1.0f - ((1.0f-g_bbAlphaRef) * cameraDim);

	return o;
};

technique billboards
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		DESTBLEND = ZERO;
		CULLMODE = CW;

		VertexShader     = compile vs_2_0 vs_billboards();
		PixelShader      = compile ps_2_0 ps20_billboards();
	}
}


BillBoardOutputBumped vs_billboards_fixed(const VS_INPUT_BB v)
{
	BillBoardOutputBumped o = (BillBoardOutputBumped) 0;
	o.tcDepth.xy = v.texCoords.xy;
	
	o.pos = mul(v.pos, g_worldViewProj);

	float3 alphaNormal = mul(v.alphaNormal, g_world);
	alphaNormal = normalize(alphaNormal);
	float cameraDim = abs(dot(alphaNormal, g_cameraDir.xyz));
	o.sunlight.w = 1.0f - ((1.0f-g_bbAlphaRef)*cameraDim);

	return o;
};


technique billboards_fixed
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		COLOROP[0]       = SELECTARG2;
		COLORARG1[0]     = TEXTURE;
		COLORARG2[0]     = DIFFUSE;
		ALPHAOP[0]       = SUBTRACT;
		ALPHAARG1[0]     = TEXTURE;
		ALPHAARG2[0]     = DIFFUSE;
		Texture[0]       = (g_diffuseMap);
		ADDRESSU[0]      = WRAP;
		ADDRESSV[0]      = WRAP;
		ADDRESSW[0]      = WRAP;
		MAGFILTER[0]     = LINEAR;
		MINFILTER[0]     = LINEAR;
		MIPFILTER[0]     = LINEAR;
		MAXMIPLEVEL[0]   = 0;
		MIPMAPLODBIAS[0] = 0;
		TexCoordIndex[0] = 0;
			
		BW_TEXTURESTAGE_TERMINATE(1)		
		CULLMODE = CW;
		
		VertexShader = compile vs_1_1 vs_billboards_fixed();		
		PixelShader = NULL;
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
