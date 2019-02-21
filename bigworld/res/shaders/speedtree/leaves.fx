#include "speedtree.fxh"

#if SPTFX_ENABLE_NORMAL_MAPS
//TODO: use the COMPILE_SHADER_MODEL_3 stuff

#define SPT_FOG(output, nSpots, lightAdjust) 																													\
	output.fog           = vertexFog(output.pos.w, fogStart, fogEnd);																									

#define SPT_FOG_SUN_DIF2(output, nSpots, lightAdjust) 																													\
	output.diffuse       = speedtreeDiffuse(worldNormal, worldPos, g_material[0], g_material[1], nDirectionals, nPoints, nSpots);										\
	SPT_FOG(output, nSpots, lightAdjust)

#define SPT_FOG_SUN_DIF_ATT2(output, lightAdjust) 																														\
	SPT_FOG_SUN_DIF2(output, 1, lightAdjust)																																\
	float3 eye           = normalisedEyeVector(worldPos, g_cameraPos);																									\
	float3x3 tsMatrix    = worldSpaceTSMatrix(g_world, i.tangent, i.binormal, worldNormal);																				\
	output.eye = mul( tsMatrix, eye );\
	output.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirectionals, nPoints, output.dLight1.xyz, output.dLight2.xyz );							\
	output.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, output.sLight1.xyz, output.sLight2.xyz );

//----------------------------------------------------------------------------
// Shader Model 3
//----------------------------------------------------------------------------

struct SpeedtreeLeafVertexOutput_3_0
{
	float4 pos			: POSITION;
	float4 tcDepth		: TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap	: TEXCOORD1;
#endif // SKY_LIGHT_MAP_ENABLE
	float3 tangent		: TEXCOORD2;
	float3 binormal		: TEXCOORD3;
	float3 normal		: TEXCOORD4;
	float4 worldPos		: TEXCOORD5;
	float fog			: FOG;
};

SpeedtreeLeafVertexOutput_3_0 vs_leaves_30( const VS_INPUT_LEAF i  )
{
	SpeedtreeLeafVertexOutput_3_0 o = (SpeedtreeLeafVertexOutput_3_0)0;
	
	o.tcDepth.xy  = i.texCoords.xy;
	float4 outPosition = calcLeafVertex2(i);
	o.pos = mul(outPosition, g_projection);
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	o.tcDepth.w = i.extraInfo.z;

	SPT_WORLD_POS_NORMAL_LEAF(i)

	float3x3 tsMatrix    = worldSpaceTSMatrix(g_world, i.tangent, i.binormal, worldNormal);	

	o.tangent = tsMatrix[0];
	o.binormal = tsMatrix[1];
	o.normal = tsMatrix[2];

	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	o.worldPos.xyz = worldPos;
	
	return o;
};

BW_COLOUR_OUT ps_leaves_30( const SpeedtreeLeafVertexOutput_3_0 i,
							uniform bool simpleLighting, uniform bool useDiffuse )
{
	float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
	float4 normalMapSample  = tex2D( speedTreeNormalSampler, i.tcDepth.xy );
	float3 normalMap = normalMapSample.xyz * 2 - 1;
	float3x3 tsMatrix = { i.tangent, i.binormal, i.normal };
	float3 normal = normalize( mul( normalMap, tsMatrix ) );
	float3 worldPos = i.worldPos.xyz;

	// grab the eye vector
	float3 eye = normalize(wsCameraPos - worldPos);

	float3 diffuse = ambientColour * g_material[0];
	float u = 0;
	float fog = saturate(i.fog);

	float skyMap = BW_SAMPLE_SKY_MAP(i.skyLightMap);

	float depth = i.tcDepth.z;
	float leafDimming = i.tcDepth.w;
	if (simpleLighting)
	{
		diffuse += directionalLight( normal, directionalLights[0] ) * skyMap;
		u = saturate( -dot( eye, directionalLights[0].direction ) );
	}
	else
	{
		// Do diffuse lighting
		if (nDirectionalLights > 0)	
		{
			diffuse += directionalLight( normal, directionalLights[0] ) * skyMap;

			// two-sided lighting contribution (sun only)
			u = saturate( -dot( eye, directionalLights[0].direction ) );
		}
		
		for (int i = 1; i < nDirectionalLights; i++)
		{
			diffuse += directionalLight( normal, directionalLights[i] );
		}

		for (int i = 0; i < nPointLights; i++)
		{
			diffuse += pointLight( worldPos, normal, pointLights[i] );
		}
	}
	
	// calculate specular lights
	float3 specular = float3(0,0,0);

	#if SPTFX_ENABLE_SPECULAR

	if (simpleLighting)
	{
		specular = directionalSpecLight( normal, eye, specularDirectionalLights[0] ) * skyMap;		
	}
	else
	{
		if (nSpecularDirectionalLights > 0)
		{
			specular += directionalSpecLight( normal, eye, specularDirectionalLights[0] ) * skyMap;
		}
		
		for (int i = 1; i < nSpecularDirectionalLights; i++)
		{
			specular += directionalSpecLight( normal, eye, specularDirectionalLights[i] );
		}

		for (int i = 0; i < nSpecularPointLights; i++)
		{
			specular += pointSpecLight( worldPos, normal, eye, specularPointLights[i] );
		}

		// Include spot lights	
		for (int i = 0; i < nSpotLights; i++)
		{
			float3 diff = float3(0,0,0);
			float3 spec = float3(0,0,0);
			spotLightBump(worldPos, normal, eye, spotLights[i], diff, spec);
			diffuse += diff;
			specular += spec;
		}
	}

	#endif // SPTFX_ENABLE_SPECULAR

	// Fade out the specular with distance from the camera.
	const float invFadeDistance = 1.0/500.0;
	specular *= (1 - saturate( depth * farPlane.x * invFadeDistance ));

	float4 colour  = (0, 0, 0, 1);
	
	// Turn off the specular for the backs of the leaves.
	specular *= (1-saturate(u));
	
	diffuse *= g_material[1];

	if (useDiffuse)
	{
		colour.xyz = diffuseMap.xyz * diffuse + saturate(specular) * normalMapSample.w;
	#ifdef MOD2X
		colour.xyz *= (1 + diffuseLightExtraModulation);
	#endif
		colour.w = diffuseMap.w;
		
		// Lerp to a yellow-ish leaf colour for the back sides
		// of the leaves (based on the sunlight position)
		float3 colour2 = float3( colour.r * 0.9, colour.g * 1.0, colour.b * 0.2);
		colour.xyz = lerp( colour.xyz, colour2.xyz, saturate(u) );
	}
	else
	{
		colour.xyz = diffuse + saturate(specular) * normalMapSample.w;
	}
	colour.xyz *= leafDimming;
	colour.xyz = lerp( fogColour.xyz, colour.xyz, fog );
	colour.w = diffuseMap.w;	
	BW_FINAL_COLOUR( depth, colour )
};


PixelShader ps3_shaders[4] = {
	compile ps_3_0 ps_leaves_30(false, false),
	compile ps_3_0 ps_leaves_30(false, true),
	compile ps_3_0 ps_leaves_30(true, false),
	compile ps_3_0 ps_leaves_30(true, true)
};

int ps_30_selector()
{
	int ret = 0;
	if (nDirectionalLights != 1 ||
		nPointLights > 0 ||
		nSpotLights > 0 ||
		nSpecularDirectionalLights != 1 ||
		nSpecularPointLights > 0)
		ret = 1;
	return (ret * 2) + (int)g_texturedTrees;
}

technique leaves_30
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		DESTBLEND = ZERO;
		CULLMODE = NONE;
		VertexShader = compile vs_3_0 vs_leaves_30();
		PixelShader = (ps3_shaders[(ps_30_selector())]);
	}
}

#endif //SPTFX_ENABLE_NORMAL_MAPS

//----------------------------------------------------------------------------
// Shader Model 2 (no normal maps)
//----------------------------------------------------------------------------

SpeedtreeVertexOutput vs_leaves_20( 
		const VS_INPUT_LEAF input,
		uniform int  nDirectionals, 
		uniform int  nPoints, 
		uniform int  nSpots,
		uniform bool combineLights )
{
	SpeedtreeVertexOutput o = (SpeedtreeVertexOutput)0;
	
	float4 outPosition = calcLeafVertex2(input);
	o.pos = mul(outPosition, g_projection);
	o.tcDepth.xy  = input.texCoords.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)

	SPT_WORLD_POS_NORMAL_LEAF(input)	
	SPT_FOG_SUN_DIF(o, nSpots, g_leafLightAdj)
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	
	return o;
}

//----------------------------------------------------------------------------
// Shader Model 2 (with normal maps)
//----------------------------------------------------------------------------

#if SPTFX_ENABLE_NORMAL_MAPS

SpeedtreeVertexOutputBumped vs_leaves_20_bumped(
		const VS_INPUT_LEAF i,
		uniform int nDirectionals, 
		uniform int nPoints, 
		uniform int nSpecularDirectionals, 
		uniform int nSpecularPoints )
{
	SpeedtreeVertexOutputBumped o = (SpeedtreeVertexOutputBumped)0;
	
	o.tcDepth.xy  = i.texCoords.xy;
	float4 outPosition = calcLeafVertex2(i);
	o.pos = mul(outPosition, g_projection);
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	SPT_WORLD_POS_NORMAL_LEAF(i)	
	SPT_FOG_SUN_DIF_ATT(o, g_leafLightAdj) 
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	
	return o;
};

SpeedtreeVertexOutputBumped vs_leaves_20_bumped_spot(
		const VS_INPUT_LEAF i,
		uniform int nDirectionals, 
		uniform int nPoints, 
		uniform int nSpecularDirectionals, 
		uniform int nSpecularPoints )
{
	SpeedtreeVertexOutputBumped o = (SpeedtreeVertexOutputBumped)0;
	
	float4 outPosition = calcLeafVertex2(i);
	o.pos = mul(outPosition, g_projection);
	o.tcDepth.xy  = i.texCoords.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	SPT_WORLD_POS_NORMAL_LEAF(i)	
	SPT_FOG_SUN_DIF_ATT_SPOT(o, g_leafLightAdj)
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)	
	
	return o;
};

SPT_LIGHTING_SEPARATE_SPOT_VSHADERS_BUMP(
	leavesVertexShaders_20, vs_2_0,
	vs_leaves_20,
	vs_leaves_20_bumped, vs_leaves_20_bumped_spot)

PixelShader leavesPixelShaders_20[3] =
{
	compile ps_2_0 ps_speedtree_lighting_20(),
	compile ps_2_0 ps_speedtree_lighting_20_bumped(true),
	compile ps_2_0 ps_speedtree_lighting_20_bumped_spot(true)
};


technique leaves_20_bumped
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		DESTBLEND = ZERO;
		CULLMODE = NONE;
				
		VertexShader = (leavesVertexShaders_20[ select_vsIndex() ]);
		PixelShader  = (leavesPixelShaders_20[ select_psIndex() ]);
	}
}

#else

LIGHTONLY_SKINNED_VSHADERS(leavesVertexShaders_20, vs_2_0, vs_leaves_20, false)

PixelShader leavesPixelShaders_20[1] = { compile ps_2_0 ps_speedtree_lighting_20() };

#endif // SPTFX_ENABLE_NORMAL_MAPS


technique leaves_20
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		DESTBLEND=ZERO;
		BW_FOG
		CULLMODE = NONE;
				
		VertexShader = (leavesVertexShaders_20[lightonlySkinnedVShaderIndex(nDirectionalLights, nPointLights, nSpotLights)]);
		PixelShader = (leavesPixelShaders_20[0]);
	}
}

//----------------------------------------------------------------------------
// Fixed Function
//----------------------------------------------------------------------------

OutputDiffuseLighting vs_leaves_fixed( const VS_INPUT_LEAF input )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;
	
	float4 outPosition = calcLeafVertex2VS11(input);
	o.pos = mul(outPosition, g_projection);
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	o.tcDepth.xy  = input.texCoords.xy;
	
	float3 normal = mul(input.normal, g_world);
	normal = normalize(normal);
	float ndotl  = max(dot(normal, directionalLights[0].direction), g_leafLightAdj);
	float4 diffuse = g_material[0] * directionalLights[0].colour;
	float4 ambient = g_material[1] * ambientColour;
	o.sunlight.xyz = saturate(ndotl*diffuse + ambient);
	o.sunlight.w = 1.0f;

	return o;
}

technique leaves_fixed
{
	pass Pass_0
	{
		BW_BLENDING_SOLID		
		DESTBLEND=ZERO;	
		
		BW_FOG		
		COLOROP[0]       = (g_texturedTrees ? 4 : 3);
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
