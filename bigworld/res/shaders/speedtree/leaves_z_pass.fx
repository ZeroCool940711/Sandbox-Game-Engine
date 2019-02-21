#include "speedtree.fxh"

struct SpeedTreeVertexOutput
{
	float4 pos		: POSITION;
	float2 tcDepth	: TEXCOORD0;
};

//----------------------------------------------------------------------------
// Pixel shaders
//----------------------------------------------------------------------------

float4 ps20_leaves( SpeedTreeVertexOutput i ) : COLOR
{
	float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth );
	return float4( 1, 0, 0, diffuseMap.w );
}

//----------------------------------------------------------------------------
// Leaves
//----------------------------------------------------------------------------

SpeedTreeVertexOutput vs_leaves(const VS_INPUT_LEAF i)
{
	SpeedTreeVertexOutput o = (SpeedTreeVertexOutput) 0;
	float4 outPosition = calcLeafVertex2(i);
	o.pos = mul(outPosition, g_projection);
	o.tcDepth  = i.texCoords.xy;

	return o;
};

technique leaves
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		DESTBLEND = ZERO;
		CULLMODE = NONE;

		VertexShader     = compile vs_2_0 vs_leaves();
		PixelShader      = compile ps_2_0 ps20_leaves();
	}
}


OutputDiffuseLighting vs_leaves_fixed( const VS_INPUT_LEAF input )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;
	
	float4 outPosition = calcLeafVertex2VS11(input);
	o.pos = mul(outPosition, g_projection);
	o.tcDepth.xy  = input.texCoords.xy;
	return o;
}

technique leaves_fixed
{
	pass Pass_0
	{
		BW_BLENDING_SOLID		
		DESTBLEND=ZERO;	
		
		BW_FOG		
		COLOROP[0]       = SELECTARG2;
		COLORARG1[0]     = TEXTURE;
		COLORARG2[0]     = DIFFUSE;
		ALPHAOP[0]       = SELECTARG1;
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
		
		CULLMODE = NONE;
				
		VertexShader = compile vs_1_1 vs_leaves_fixed();
		PixelShader  = NULL;
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
