#include "speedtree.fxh"

bool    g_cullEnabled   = true;

struct SpeedTreeVertexOutput
{
	float4 pos		: POSITION;
	float2 tcDepth	: TEXCOORD0;
};

//----------------------------------------------------------------------------
// Pixel shaders
//----------------------------------------------------------------------------

float4 ps20_branches( SpeedTreeVertexOutput i ) : COLOR
{
	float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth );
	return float4( 1, 0, 0, diffuseMap.w );
}

//----------------------------------------------------------------------------
// Branches
//----------------------------------------------------------------------------

SpeedTreeVertexOutput vs_branches(const VS_INPUT_BRANCHES i)
{
	SpeedTreeVertexOutput o = (SpeedTreeVertexOutput) 0;
	o.pos = branchesOutputPosition(i);
	o.tcDepth  = i.texCoords.xy;

	return o;
};

technique branches
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		DESTBLEND = ZERO;
		CULLMODE  = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);

		VertexShader     = compile vs_2_0 vs_branches();
		PixelShader      = compile ps_2_0 ps20_branches();
	}
}


//----------------------------------------------------------------------------
// Fixed Function
//----------------------------------------------------------------------------

OutputDiffuseLighting vs_branches_fixed(const VS_INPUT_BRANCHES input)
{
	OutputDiffuseLighting output = (OutputDiffuseLighting) 0;
	
	output.pos = branchesOutputPosition(input);	
	output.tcDepth.xy  = input.texCoords;
	return output;
};

technique branches_fixed
{
	pass Pass_0
	{
		BW_BLENDING_SOLID

		DESTBLEND = ZERO;
		CULLMODE  = (g_cullEnabled ? BW_CULL_CW : BW_CULL_NONE);


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
				
		VertexShader = compile vs_1_1 vs_branches();
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
