#include "speedtree.fxh"

float4   g_bbAlphaRefs[64];
//float4   g_bbAlphaRefs[196];

#define g_bbAlphaRefs64 g_bbAlphaRefs
#define g_bbAlphaRefs196 g_bbAlphaRefs

//TODO: extend the maximum instance count...
//float4   g_bbAlphaRefs64[64];
//float4   g_bbAlphaRefs196[196];


float4x4 g_viewProj;
float2   g_UVScale;

float g_bias = 0.0;

sampler speedTreeDiffuseSamplerBiased = sampler_state
{
	Texture = (g_diffuseMap);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = (minMagFilter);
	MIPFILTER = (mipFilter);
	MAXANISOTROPY = (maxAnisotropy);

	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = (g_bias);
};

sampler bbNormalSamplerBiased = sampler_state
{
	Texture       = (g_normalMap);
	ADDRESSU      = WRAP;
	ADDRESSV      = WRAP;
	ADDRESSW      = WRAP;
	MAGFILTER     = POINT;
	MINFILTER     = POINT;
	MIPFILTER     = POINT;

	MAXMIPLEVEL   = 0;
	MIPMAPLODBIAS = (g_bias);
};

struct VS_INPUT_BB_OPT
{
    float4 pos            : POSITION;
    float3 lightNormal    : NORMAL;
    float3 alphaNormal    : TEXCOORD0;
    float2 texCoords      : TEXCOORD1;
    float  alphaIndex     : TEXCOORD2;
    float4 alphaMask      : TEXCOORD3;
	float3 binormal       : TEXCOORD4;
	float3 tangent        : TEXCOORD5;
	float4 diffuseNAdjust : TEXCOORD6;
	float3 ambient        : TEXCOORD7;
};

//----------------------------------------------------------------------------
// Shader Model 2
//----------------------------------------------------------------------------

BillBoardOutputBumped vs_billboards_opt_20_bumped(const VS_INPUT_BB_OPT v)
{
	BillBoardOutputBumped o = (BillBoardOutputBumped) 0;
	o.tcDepth.xy = v.texCoords.xy * g_UVScale.xy;

	// position
	o.pos = mul(v.pos, g_viewProj);
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	// fog
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
 
	// sky light map (clouds shadow)
	BW_SKY_MAP_COORDS( o.skyLightMap, v.pos )
	
	// sun will be calculated in ps
	o.sunlight = v.diffuseNAdjust;

	// ambient light
	o.diffuse.xyz = ambientColour.xyz * v.ambient.xyz;

	// view angle alpha
	float cameraDim = abs(dot(normalize(v.alphaNormal), g_cameraDir.xyz));
	float bbAlphaRef = dot(g_bbAlphaRefs196[v.alphaIndex], v.alphaMask);
	o.diffuse.w = 1.0f - ((1.0f-bbAlphaRef)*cameraDim);

	// normal mapping data
	float3 worldBinormal = v.binormal;
	float3 worldTangent  = v.tangent;
	float3 worldNormal = normalize(cross(worldTangent, worldBinormal));

	float3 dLight1_unused;
	float3x3 tsMatrix  = {worldBinormal, worldTangent, worldNormal};
	normalMapDiffuseBump( v.pos, tsMatrix, 1, 0, dLight1_unused, o.dLight2.xyz );

	return o;
};


BW_COLOUR_OUT ps_billboards_opt_20_bumped( const BillBoardOutputBumped i )
{
    float4 diffuseMap = tex2D( speedTreeDiffuseSamplerBiased, i.tcDepth.xy );
    float4 normalMap  = tex2D( bbNormalSamplerBiased, i.tcDepth.xy ) * 2 - 1;
	
	// normal mapping
	float attenuation    = max(dot( normalize(i.dLight2.xyz), normalMap.xyz ), i.sunlight.w);
    float3 diffuseColour = attenuation * directionalLights[0].colour.xyz * i.sunlight.xyz;
	
	// final colour
    float4 colour  = (0,0,0,1);
	
    float sunShade = BW_SAMPLE_SKY_MAP(i.skyLightMap)
	if (g_texturedTrees)
	{
		// i.diffuse.xyz is actually ambient light
		colour.xyz = saturate(diffuseMap.xyz * (sunShade*(diffuseColour) + i.diffuse.xyz));
	}
	else
	{
		// i.diffuse.xyz is actually ambient light
		colour.xyz = saturate((sunShade*(diffuseColour) + i.diffuse.xyz));
	}
	colour.w = diffuseMap.w - i.diffuse.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
};


#if SPTFX_ENABLE_NORMAL_MAPS
	technique billboards_opt_20_bumped
	<	
		bool bumpMapped = true;
	>
	{
		pass Pass_0
		{
			BW_FOG
			BW_BLENDING_SOLID

			DESTBLEND = ZERO;
			CULLMODE  = CW;
			
			VertexShader = compile vs_2_0 vs_billboards_opt_20_bumped();
			PixelShader = compile ps_2_0 ps_billboards_opt_20_bumped();
		}
	}
#endif // SPTFX_ENABLE_NORMAL_MAPS

//----------------------------------------------------------------------------
// Fixed Function
//----------------------------------------------------------------------------

BillBoardOutputBumped vs_billboards_opt_fixed(const VS_INPUT_BB_OPT v)
{
	BillBoardOutputBumped o = (BillBoardOutputBumped) 0;
	o.tcDepth.xy = v.texCoords.xy * g_UVScale.xy;
	
	o.pos = mul(v.pos, g_viewProj);
	
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);

	float ndotl = max(dot(v.lightNormal, directionalLights[0].direction.xyz), v.diffuseNAdjust.w);
	float3 diffuse = directionalLights[0].colour.xyz * v.diffuseNAdjust.xyz;
	float3 ambient = ambientColour * v.ambient.xyz;
	o.sunlight.xyz = saturate(ndotl * diffuse + ambient);

	// sky light map (clouds shadow)
	BW_SKY_MAP_COORDS( o.skyLightMap, v.pos )

	float cameraDim  = abs(dot(normalize(v.alphaNormal), g_cameraDir.xyz));
	float bbAlphaRef = dot(g_bbAlphaRefs64[v.alphaIndex], v.alphaMask);
	o.sunlight.w = 1.0f - ((1.0f-bbAlphaRef)*cameraDim);
	
	return o;
};

technique billboards_opt_fixed
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		BW_FOG
		COLOROP[0]       = (g_texturedTrees ? 4 : 3);
		COLORARG1[0]     = TEXTURE;
		COLORARG2[0]     = DIFFUSE;
		ALPHAOP[0]       = SUBTRACT;
		ALPHAARG1[0]     = TEXTURE;
		ALPHAARG2[0]     = DIFFUSE;
		Texture[0]       = (g_diffuseMap);
		ADDRESSU[0]      = CLAMP;
		ADDRESSV[0]      = CLAMP;
		ADDRESSW[0]      = CLAMP;
		MAGFILTER[0]     = LINEAR;
		MINFILTER[0]     = LINEAR;
		MIPFILTER[0]     = LINEAR;
		MAXMIPLEVEL[0]   = 0;
		MIPMAPLODBIAS[0] = 0;
		TexCoordIndex[0] = 0;
				
		BW_TEXTURESTAGE_CLOUDMAP(CLOUDMAP_STAGE)
		BW_TEXTURESTAGE_TERMINATE(CLOUDMAP_STAGE_PLUS1)		
		CULLMODE = CW;
		
		VertexShader = compile vs_1_1 vs_billboards_opt_fixed();		
		PixelShader = NULL;
	}
}
