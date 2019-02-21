#include "speedtree.fxh"

//----------------------------------------------------------------------------
// Common
//----------------------------------------------------------------------------

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

//----------------------------------------------------------------------------
// Shader 2
//----------------------------------------------------------------------------

BillBoardOutputBumped vs_billboards_bumped(const VS_INPUT_BB v)
{
	BillBoardOutputBumped o = (BillBoardOutputBumped) 0;
	o.tcDepth.xy = v.texCoords.xy;
	
	o.pos = mul(v.pos, g_worldViewProj);
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	
	// fog
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
 
	// sky light map (clouds shadow)
	float4 worldPos = mul(v.pos, g_world);
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )
	
	// sun will be calculated in ps
	o.sunlight= float4(0, 0, 0, 1);

	// ambient light
	o.diffuse = g_sun[2] * g_material[1];
	
	// view angle alpha
	float3 alphaNormal = mul(v.alphaNormal, g_world);
	alphaNormal = normalize(alphaNormal);
	float cameraDim = abs(dot(alphaNormal, g_cameraDir.xyz));
	o.diffuse.w = 1.0f - ((1.0f-g_bbAlphaRef) * cameraDim);
	
	// normal mapping data
	float3 tangent  = v.tangent;
	float3 binormal = v.binormal;

	float3 worldBinormal = mul(binormal, g_world);
	float3 worldTangent  = mul(tangent, g_world);
	float3 worldNormal   = normalize(cross(worldTangent, worldBinormal));

	float3 dLight1_unused;
	float3x3 tsMatrix  = {worldBinormal, worldTangent, worldNormal};
	normalMapDiffuseBump( worldPos, tsMatrix, 1, 0, dLight1_unused, o.dLight2.xyz );
		
	return o;
};


BW_COLOUR_OUT ps20_lighting_bumped( const BillBoardOutputBumped i )
{
    float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
    float4 normalMap  = tex2D( bbNormalSampler, i.tcDepth.xy ) * 2 - 1;

	// normal mapping
	float attenuation    = max(dot( normalize(i.dLight2.xyz), normalMap.xyz ), g_leafLightAdj);
    float3 diffuseColour = attenuation * directionalLights[0].colour.xyz * g_material[0];

	// final colour
    float4 colour  = (0,0,0,1);
    float sunShade = BW_SAMPLE_SKY_MAP(i.skyLightMap)
	if (g_texturedTrees)
	{
		colour.xyz = saturate(diffuseMap.xyz * (sunShade*(i.sunlight.xyz + diffuseColour) + i.diffuse.xyz));
	}
	else
	{
		colour.xyz = saturate((sunShade*(i.sunlight.xyz + diffuseColour) + i.diffuse.xyz));
	}
	colour.w = diffuseMap.w - i.diffuse.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
};

#if SPTFX_ENABLE_NORMAL_MAPS
	technique billboards_2
	{
		pass Pass_0
		{
			BW_FOG
			BW_BLENDING_SOLID
			DESTBLEND = ZERO;
			CULLMODE = CW;
			
			VertexShader = compile vs_2_0 vs_billboards_bumped();		
			PixelShader = compile ps_2_0 ps20_lighting_bumped();
		}
	}
#endif // SPTFX_ENABLE_NORMAL_MAPS

//----------------------------------------------------------------------------
// Fixed Function
//----------------------------------------------------------------------------

BillBoardOutputBumped vs_billboards_fixed(const VS_INPUT_BB v)
{
	BillBoardOutputBumped o = (BillBoardOutputBumped) 0;
	o.tcDepth.xy = v.texCoords.xy;
	
	o.pos = mul(v.pos, g_worldViewProj);
	
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
 
	float3 lightNormal = mul(v.lightNormal, g_world);
	lightNormal = normalize(lightNormal);
	float ndotl = dot(lightNormal, g_sun[0].xyz);
	float4 diffuse = g_sun[1] * g_material[0];
	float4 ambient = g_sun[2] * g_material[1];
	o.sunlight.xyz = saturate(ndotl * diffuse + ambient);
	o.diffuse.xyz  = float3(0, 0, 0);

	// sky light map (clouds shadow)
	float4 worldPos = mul(v.pos, g_world);
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )

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
		COLOROP[0]       = (g_texturedTrees ? 4 : 3);
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
			
		BW_TEXTURESTAGE_CLOUDMAP(CLOUDMAP_STAGE)
		BW_TEXTURESTAGE_TERMINATE(CLOUDMAP_STAGE_PLUS1)		
		CULLMODE = CW;
		
		VertexShader = compile vs_1_1 vs_billboards_fixed();		
		PixelShader = NULL;
	}
}
