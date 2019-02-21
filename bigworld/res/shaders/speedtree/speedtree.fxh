#include "stdinclude.fxh"
#include "unskinned_effect_include.fxh"
#include "shader_combination_helpers.fxh"
#include "normalmap_shader_combinations.fxh"

#define NUM_WIND_MATRICES 6

// Set SPTFX_ENABLE_MATRIX_OPT to 0 to disable the matrix array
// optimisaton. Don't forget to do the same in speedtree_renderer.cpp
#define SPTFX_ENABLE_MATRIX_OPT 1

// Set ENABLE_NORMAL_MAPS to 0 to disable normal mapping
#define SPTFX_ENABLE_NORMAL_MAPS 1

// Set SPTFX_ENABLE_SPECULAR to 0 if you want to use normal 
// maps but don't want specular reflection on the trunks
#define SPTFX_ENABLE_SPECULAR 1

// for optimisation sake, we're using an array of 
// matrices instead of uploading each matrix at a time.
#if SPTFX_ENABLE_MATRIX_OPT
	float4x4 g_matrices[5];
	#define  g_world                  g_matrices[0]
	#define  g_rotation               g_matrices[1]	
	#define  g_worldViewProj          g_matrices[2]
	#define  g_worldView              g_matrices[3]
	#define  g_rotationInverse        g_matrices[4]
	float4x4 g_projection;
#else // SPTFX_ENABLE_MATRIX_OPT
	float4x4 g_world;
	float4x4 g_rotation; 
	float4x4 g_projection;
	float4x4 g_worldViewProj;
	float4x4 g_worldView;
	float4x4 g_rotationInverse;
#endif // SPTFX_ENABLE_MATRIX_OPT

bool g_useNormalMap = true;

texture g_diffuseMap;
texture g_normalMap;

float4  g_material[2];
float4  g_sun[3];	//sun direction, sun colour, ambient colour

bool    g_texturedTrees = true;

float4  g_cameraDir;
float4  g_cameraPos;

float   fogStart      : FogStart = 0.0;
float   fogEnd        : FogEnd = 1.0;
float4  fogColour     : FogColour = {0,0,0,0};


// used by BW_BLENDING_SOLID

bool alphaTestEnable = true;
int  alphaReference  = 0;

BW_DIFFUSE_LIGHTING
BW_SPECULAR_LIGHTING
BW_SKY_LIGHT_MAP

// random value to get different wind for each instance of a tree
float    g_windMatrixOffset = 0.f;

float    g_treeScale;
float    g_LeafRockFar;
float4x4 g_windMatrices[NUM_WIND_MATRICES];
float4   g_leafAngles[64];
float4   g_leafAngleScalars;
float4   g_leafUnitSquare[5] =
	{                                   
		 float4(0.5f, 0.5f, 0.0f, 0.0f), 
		 float4(-0.5f, 0.5f, 0.0f, 0.0f), 
		 float4(-0.5f, -0.5f, 0.0f, 0.0f), 
		 float4(0.5f, -0.5f, 0.0f, 0.0f),
		 float4(0.0f, 0.0f, 0.0f, 0.0f)
	};

float g_leafLightAdj;
float g_billboardFudgeAmbLight;

struct SpeedtreeVertexOutput
{
	float4 pos         : POSITION;
	float3 tcDepth     : TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap : TEXCOORD1;
#endif // SKY_LIGHT_MAP_ENABLE
	float4 diffuse     : COLOR0;
	float4 sunlight    : COLOR1;
	float fog          : FOG;
};


struct SpeedtreeVertexOutputBumped
{
	float4 pos     : POSITION;
	float3 tcDepth : TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap : TEXCOORD1;
#endif // SKY_LIGHT_MAP_ENABLE
	float3 dLight1 : TEXCOORD2;
	float3 dLight2 : TEXCOORD3;
	float3 sLight1 : TEXCOORD4;
	float3 sLight2 : TEXCOORD5;
	float3 spotDir : TEXCOORD6;
	float4 attenuation : TEXCOORD7;
	float4 diffuse : COLOR0;
	float4 sunlight: COLOR1;
	float fog      : FOG;
};


struct BillBoardOutputBumped
{
	float4 pos         : POSITION;
	float3 tcDepth     : TEXCOORD0;
#if SKY_LIGHT_MAP_ENABLE
	float2 skyLightMap : TEXCOORD1;
#endif // SKY_LIGHT_MAP_ENABLE
	float3 dLight2     : TEXCOORD2;
	float4 sunlight    : COLOR;
	float4 diffuse     : COLOR1;
	float fog          : FOG;
};

sampler speedTreeDiffuseSampler	= BW_SAMPLER(g_diffuseMap, WRAP)

sampler bbNormalSampler = sampler_state
{
	Texture       = (g_normalMap);
	ADDRESSU      = WRAP;
	ADDRESSV      = WRAP;
	ADDRESSW      = WRAP;
	MAGFILTER     = POINT;
	MINFILTER     = POINT;
	MIPFILTER     = POINT;
	MAXMIPLEVEL   = 0;
	MIPMAPLODBIAS = 0;
};

sampler speedTreeNormalSampler = BW_SAMPLER(g_normalMap, WRAP)

BW_SKY_LIGHT_MAP_SAMPLER

//--------------------------------------------------------------//
// Bumped Macros
//--------------------------------------------------------------//

#define SPT_WORLD_POS_NORMAL(input) 																																	\
	float4 worldPos = mul(input.pos, g_world);																															\
	float3 worldNormal = mul(input.normal, g_world);																													\
	worldNormal = normalize(worldNormal);

#define SPT_WORLD_POS_NORMAL_LEAF(input) 																																\
	float fDimming = input.extraInfo.z;																																	\
	float4 worldPos = mul(input.pos, g_world);																															\
	float3 worldNormal = mul(input.normal, g_world);																													\
	worldNormal = normalize(worldNormal) * fDimming;

#define SPT_FOG_SUNLIGHT(output, nSpots, lightAdjust) 																													\
	output.fog           = vertexFog(output.pos.w, fogStart, fogEnd);																									\
	output.sunlight      = speedtreeSunLight(worldNormal, lightAdjust, g_material[0], nDirectionals, nPoints, nSpots);													\

#define SPT_FOG_SUN_DIF(output, nSpots, lightAdjust) 																													\
	output.diffuse       = speedtreeDiffuse(worldNormal, worldPos, g_material[0], g_material[1], nDirectionals, nPoints, nSpots);										\
	SPT_FOG_SUNLIGHT(output, nSpots, lightAdjust)

#define SPT_FOG_SUN_DIF_ATT(output, lightAdjust) 																														\
	SPT_FOG_SUN_DIF(output, 1, lightAdjust)																																\
	float3 eye           = normalisedEyeVector(worldPos, g_cameraPos);																									\
	float3x3 tsMatrix    = worldSpaceTSMatrix(g_world, i.tangent, i.binormal, worldNormal);																				\
	output.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirectionals, nPoints, output.dLight1.xyz, output.dLight2.xyz );							\
	output.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, output.sLight1.xyz, output.sLight2.xyz );

#define SPT_FOG_SUN_DIF_ATT_SPOT(output, lightAdjust)																													\
	SPT_FOG_SUN_DIF(output, 1, lightAdjust)																																\
	float3 eye           = normalisedEyeVector(worldPos, g_cameraPos);																									\
	float3x3 tsMatrix    = worldSpaceTSMatrix(g_world, i.tangent, i.binormal, worldNormal);																				\
	output.attenuation.y = normalMapSpotBumpDiffuse( worldPos, tsMatrix, nDirectionals, nPoints, output.dLight2.xyz );											\
	output.attenuation.x = output.attenuation.z = spotSpecBumpLight( worldPos, worldNormal, eye, tsMatrix, spotLights[0], output.dLight1.xyz, output.sLight1.xyz, output.spotDir );

//--------------------------------------------------------------//
// Bumped vs
//--------------------------------------------------------------//

int vs_speedtree_lighting_20_bumped_Index( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int nSpot )
{
	int nSpecIndex = nSpecDir;
	if (nSpecPoint > 0)
	{
		if (nSpecPoint == 1)
		{
			
			if (nSpecDir == 0)
			{
				nSpecIndex = 3;
			}
			else
			{
				nSpecIndex = 4;
			}
		}
		else
		{
			nSpecIndex = 5;
		}
	}
	return min(nDir, 2) + (min(nPoint, 4) * 3) + (min(nSpecIndex, 6) * 15) + (min(nSpot,1)*90);
}

//--------------------------------------------------------------//
// Bumped PS
//--------------------------------------------------------------//



BW_COLOUR_OUT ps_speedtree_lighting_20_bumped(const SpeedtreeVertexOutputBumped i, 
											uniform bool leaf)
{
	float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
	float4 normalMapSample = tex2D( speedTreeNormalSampler, i.tcDepth.xy );
	float3 normalMap = normalMapSample.xyz * 2 - 1;

	float3 diffuse1Colour  = float3(0,0,0);
	float3 diffuse2Colour  = float3(0,0,0);

	float sunShade = BW_SAMPLE_SKY_MAP(i.skyLightMap)

	if (nDirectionalLights > 1)
	{
		diffuse1Colour = directionalLights[1].colour.xyz;
	} 
	else if (nPointLights > 0)
	{
		diffuse1Colour = pointLights[0].colour.xyz;
	}

	if (nDirectionalLights > 0)
	{
		diffuse2Colour = directionalLights[0].colour.xyz * sunShade;
	} 
	else if (nPointLights > 1)
	{
		diffuse2Colour = pointLights[1].colour.xyz;
	}

	#if SPTFX_ENABLE_SPECULAR
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);
	if (!leaf)
	{
		if (nSpecularPointLights > 0)
		{
			specular1Colour = specularPointLights[0].colour.xyz;
		}
		else
		{
			specular1Colour = specularDirectionalLights[1].colour.xyz;
		}

		if (nSpecularPointLights > 1)
		{
			specular2Colour = specularPointLights[1].colour.xyz;
		}
		else
		{
			specular2Colour = specularDirectionalLights[0].colour.xyz * sunShade;
		}
	}
	#endif // SPTFX_ENABLE_SPECULAR
		
	float4 attenuation;
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normalize(normalMap.xyz) ) );
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normalize(normalMap.xyz) ) );
	if (!leaf)
	{
		attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normalize(normalMap.xyz) ) ), 128 );
		attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normalize(normalMap.xyz) ) ), 128 );
	}
	else
	{
		attenuation.z = 0;
		attenuation.w = 0;
	}
	attenuation  *= i.attenuation;

	float3 diffuseColour = i.diffuse.xyz;
	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;

	float3 specularColour = (0,0,0);
	#if SPTFX_ENABLE_SPECULAR
	if (!leaf)
	{
		specularColour += attenuation.z * specular1Colour;
		specularColour += attenuation.w * specular2Colour;
	}
	#endif // SPTFX_ENABLE_SPECULAR

	float4 colour  = (0, 0, 0, 1);
	if (g_texturedTrees)
	{
		colour.xyz = diffuseMap.xyz * saturate(diffuseColour) + saturate(specularColour) * normalMapSample.w;
	}
	else
	{
		colour.xyz = saturate(diffuseColour) + saturate(specularColour) * normalMapSample.w;	
	}
	colour.w = i.diffuse.w * diffuseMap.w;
	
	BW_FINAL_COLOUR( i.tcDepth.z, colour )
};


BW_COLOUR_OUT ps_speedtree_lighting_20_bumped_spot(const SpeedtreeVertexOutputBumped i,
							uniform bool leaf)
{
	float4 diffuseMap    = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
	float4 normalMap     = tex2D( speedTreeNormalSampler, i.tcDepth.xy ) * 2 - 1;
	float3 diffuseColour = i.diffuse.xyz;

	float3 diffuse1Colour  = float3(0,0,0);
	float3 diffuse2Colour  = float3(0,0,0);

	bool d2 = true;

	diffuse1Colour = spotLights[0].colour.xyz;
	
	float sunShade = BW_SAMPLE_SKY_MAP(i.skyLightMap)
	if (nDirectionalLights > 0)
	{
		diffuse2Colour = directionalLights[0].colour.xyz * sunShade;
	}
	else if (nPointLights > 0)
	{
		diffuse2Colour = pointLights[0].colour.xyz;
	}
	else
	{
		d2 = false;
	}
	
	#if SPTFX_ENABLE_SPECULAR
	bool s2 = true;
	
	float3 specular1Colour = spotLights[0].colour.xyz;
	float3 specular2Colour = float3(0,0,0);
	
	if (!leaf)
	{
		if (nSpecularDirectionalLights > 0)
		{
			specular2Colour = specularDirectionalLights[0].colour.xyz * sunShade;
		}
		else if (nSpecularPointLights > 0)
		{
			specular2Colour = specularPointLights[0].colour.xyz;
		}
		else
		{
			s2 = false;
		}
	}
	#endif // SPTFX_ENABLE_SPECULAR
		
	float4 attenuation = (0,0,0,0);
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normalize(normalMap.xyz) ) );
	if (d2)
	{
		attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normalize(normalMap.xyz) ) );
	}

	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normalize(normalMap.xyz) ) ), 32 );
	
	#if SPTFX_ENABLE_SPECULAR
		if (s2 && !leaf)
		{
			attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normalize(normalMap.xyz) ) ), 32 );
		}
	#endif // SPTFX_ENABLE_SPECULAR
	
	float spotEffect = 
		saturate( dot( normalize(i.spotDir), spotLights[0].direction.xyz  ) - 
		spotLights[0].attenuation.z) /  ( 1 - spotLights[0].attenuation.z );
		
	attenuation.xz  *= spotEffect;
	attenuation     *= i.attenuation;

	diffuseColour += attenuation.x * diffuse1Colour;
	if (d2)
	{
		diffuseColour +=  attenuation.y * diffuse2Colour;
	}

	float3 specularColour = (0, 0, 0);
	#if SPTFX_ENABLE_SPECULAR
	if (!leaf)
	{
		specularColour += attenuation.z * specular1Colour;
		if (s2)
		{
			specularColour += attenuation.w * specular2Colour;
		}
	}
	#endif // SPTFX_ENABLE_SPECULAR

	float4 colour  = (0, 0, 0, 1);
	if (g_texturedTrees)
	{
		colour.xyz = diffuseMap.xyz * saturate(diffuseColour) + saturate(specularColour) * normalMap.w;
	}
	else
	{
		colour.xyz = diffuseColour + saturate(specularColour) * normalMap.w;
	}
	colour.w = i.diffuse.w * diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
};

//--------------------------------------------------------------//
// Non bumped PS
//--------------------------------------------------------------//

BW_COLOUR_OUT ps_speedtree_lighting_20( const SpeedtreeVertexOutput i )
{
    float4 colour = (0,0,0,1);
    float4 diffuseMap = tex2D( speedTreeDiffuseSampler, i.tcDepth.xy );
    float shade = BW_SAMPLE_SKY_MAP(i.skyLightMap)
    
    if (g_texturedTrees)
	{
		colour.xyz = diffuseMap.xyz * saturate(i.sunlight * shade + i.diffuse);
	}
	else
	{
		colour.xyz = saturate(i.sunlight * shade + i.diffuse);
	}
	colour.w = diffuseMap.w;
	BW_FINAL_COLOUR( i.tcDepth.z, colour )
};

//--------------------------------------------------------------//
// Utility functions
//--------------------------------------------------------------//

float3x3 calcRotationMatrixZ(float fAngle)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);    
    return float3x3(vSinCos.y, -vSinCos.x, 0.0f, 
                    vSinCos.x, vSinCos.y, 0.0f, 
                    0.0f, 0.0f, 1.0f);
}

float3x3 calcRotationMatrixY(float fAngle)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    return float3x3(vSinCos.y, 0.0f, vSinCos.x,
                    0.0f, 1.0f, 0.0f,
                    -vSinCos.x, 0.0f, vSinCos.y);
}

float3x3 calcRotationMatrixX(float fAngle)
{
    float2 vSinCos;
    sincos(fAngle, vSinCos.x, vSinCos.y);
    return float3x3(1.0f, 0.0f, 0.0f,
                    0.0f, vSinCos.y, -vSinCos.x,
                    0.0f, vSinCos.x, vSinCos.y);
}

float3 directionalLightMinAngle( in float3 normal, in DirectionalLight light, in float minAngle )
{
	float ndotl  = max(dot(light.direction, normal), minAngle);
	return saturate(ndotl) * light.colour.xyz;

// a better version:
//	float ndotl  = max(dot(light.direction, normal), 0.f);
//	ndotl = lerp(minAngle, 1.0f, ndotl);
//	return ndotl * light.colour.xyz;
}

float4 speedtreeSunLight(
	float3 worldNormal, 
	float leafLightAdj, 
	float4 ambientMaterial,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots )
{
	float3 sunlight = (0,0,0);
	if (nDirectionals > 0)
	{
		sunlight.xyz = directionalLightMinAngle( worldNormal, directionalLights[0], leafLightAdj );
	}
	
	float4 result;
	result.xyz = saturate(sunlight.xyz * ambientMaterial);
	result.w   = 1.0f;	
	return result;
}
	
float4 speedtreeDiffuse(
	float3 worldNormal, 
	float4 worldPos, 
	float4 ambientMaterial, 
	float4 diffuseMaterial,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots )
{
	float3 diffuse = (0,0,0);
	for (int i = 1; i < nDirectionals; i++)
	{
		diffuse.xyz += directionalLight( worldNormal, directionalLights[i] );
	}

	for (int i = 0; i < nPoints; i++)
	{
		diffuse.xyz += pointLight( worldPos, worldNormal, pointLights[i] );
	}
	
	for (int i = 0; i < nSpots; i++)
	{
		if (nSpotLights > 0)
		{
			diffuse.xyz += spotLight( worldPos, worldNormal, spotLights[i] );
		}
	}
	
	float4 result;
	result.xyz  = saturate(diffuse.xyz * diffuseMaterial + ambientColour * ambientMaterial);
	result.w = 1.0f;
	return result;
}

//----------------------------------------------------------------------------
// Branches
//----------------------------------------------------------------------------



///////////////////////////////////////////////////////////////////////  
//  WindEffect
//
//  New with 4.0 is a two-weight wind system that allows the tree model
//  to bend at more than one branch level.
//
//  In order to keep the vertex size small, the wind parameters have been
//  compressed as detailed here:
//
//      vWindInfo.x = (wind_matrix_index1 * 10.0) / NUM_WIND_MATRICES  + wind_weight1
//      vWindInfo.y = (wind_matrix_index2 * 10.0) / NUM_WIND_MATRICES  + wind_weight2
//
//  * Note: NUM_WIND_MATRICES cannot be larger than 10 in this case
//
//  * Caution: Negative wind weights will not work with this scheme.  We rely on the
//             fact that the SpeedTreeRT library clamps wind weights to [0.0, 1.0]

float3 WindEffect( float3 vPosition, float2 vWindInfo )
{
	// decode both wind weights and matrix indices at the same time in order to save
	// vertex instructions
	vWindInfo.xy += g_windMatrixOffset.xx;
	float2 vWeights = frac( vWindInfo.xy );
	float2 vIndices = (vWindInfo - vWeights) * 0.05f * NUM_WIND_MATRICES;

	// first-level wind effect - interpolate between static position and fully-blown
	// wind position by the wind weight value
	float3 vWindEffect = lerp( vPosition.xyz, mul( vPosition, g_windMatrices[int(vIndices.x)] ), vWeights.x );

	// second-level wind effect - interpolate between first-level wind position and 
	// the fully-blown wind position by the second wind weight value
	return lerp(vWindEffect, mul(vWindEffect, g_windMatrices[int(vIndices.y)]), vWeights.y);
}

struct VS_INPUT_BRANCHES
{
    float4 pos       : POSITION;
    float3 normal    : NORMAL;
    float2 texCoords : TEXCOORD0;
    float2 windInfo  : TEXCOORD1;
	float3 binormal  : TEXCOORD2;
	float3 tangent   : TEXCOORD3;
};

float4 branchesOutputPosition(const VS_INPUT_BRANCHES i)
{
	float4 position     = mul(i.pos, g_rotation);
	float2 vWindParams = float2(i.windInfo.xy);
	position.xyz = WindEffect( position.xyz, vWindParams );
	position            = mul(position, g_rotationInverse);
	position            = mul(position, g_worldViewProj);
	return position;
};

//----------------------------------------------------------------------------
// Leaves
//----------------------------------------------------------------------------

struct VS_INPUT_LEAF
{
    float4 pos       : POSITION;
    float3 normal    : NORMAL;
    float2 texCoords : TEXCOORD0;
    float2 windInfo  : TEXCOORD1;
    float2 rotInfo   : TEXCOORD2;
    float3 extraInfo : TEXCOORD3;
    float2 geomInfo  : TEXCOORD4;
    float2 pivotInfo : TEXCOORD5;
	float3 binormal  : TEXCOORD6;
	float3 tangent   : TEXCOORD7;
};	

// Includes 2 leaf influences and better rock/rustle
float4 calcLeafVertex2( const VS_INPUT_LEAF input )
{
	float4 centerPoint = mul( input.pos, g_rotation );
	centerPoint.xyz = WindEffect( centerPoint.xyz, input.windInfo.xy );
	centerPoint = mul(centerPoint, g_rotationInverse);

	float3 corner = g_leafUnitSquare[input.extraInfo.y].xyz ;
	corner.xy += input.pivotInfo.xy;
	corner *= g_treeScale;

	// adjust by pivot point so rotation occurs around the correct point
	corner = corner * input.geomInfo.xyx;


    // rock & rustling on the card corner
    float fRotAngleX = input.rotInfo.x;         // angle offset for leaf rocking (helps make it distinct)
    float fRotAngleY = input.rotInfo.y;         // angle offset for leaf rustling (helps make it distinct)
	//x is rock
	//y is rustle

	float2 leafRockAndRustle = g_leafAngleScalars.xy * g_leafAngles[input.extraInfo.x].xy;
	//float3x3 matRotation = calcRotationMatrixY(fRotAngleX);
	float3x3 matRotation = calcRotationMatrixX(fRotAngleY);
	//matRotation = mul(matRotation, calcRotationMatrixY(-leafRockAndRustle.y));
	matRotation = mul(matRotation, calcRotationMatrixY(fRotAngleX-leafRockAndRustle.y));	
	matRotation = mul(matRotation, calcRotationMatrixZ(leafRockAndRustle.x));

	corner = mul(matRotation, corner);
	centerPoint        = mul(centerPoint, g_worldView);
	centerPoint.xyz += corner;
	return centerPoint;
}


// Includes 2 leaf influences and better rock/rustle, 
//some SM1 card only support 96 instructions
float4 calcLeafVertex2VS11( const VS_INPUT_LEAF input )
{
	float4 centerPoint = mul( input.pos, g_rotation );
	centerPoint.xyz = WindEffect( centerPoint.xyz, input.windInfo.xy );
	centerPoint = mul(centerPoint, g_rotationInverse);

	float3 corner = g_leafUnitSquare[input.extraInfo.y].xyz ;
	corner.xy += input.pivotInfo.xy;
	corner *= g_treeScale;

	// adjust by pivot point so rotation occurs around the correct point
	corner = corner * input.geomInfo.xyx;


    // rock & rustling on the card corner
    float fRotAngleX = input.rotInfo.x;         // angle offset for leaf rocking (helps make it distinct)
    float fRotAngleY = input.rotInfo.y;         // angle offset for leaf rustling (helps make it distinct)
	//x is rock
	//y is rustle

	float2 leafRockAndRustle = g_leafAngleScalars.xy * g_leafAngles[input.extraInfo.x].xy;
	//float3x3 matRotation = calcRotationMatrixY(fRotAngleX);
	float3x3 matRotation = calcRotationMatrixX(fRotAngleY);
	corner = mul(matRotation, corner);
	centerPoint        = mul(centerPoint, g_worldView);
	centerPoint.xyz += corner;
	return centerPoint;
}

//----------------------------------------------------------------------------
// Light Combination Macros
//----------------------------------------------------------------------------

#define SPT_LIGHTING_SEPARATE_SPOT_VSHADERS( arrayName, version, vs, vs_spot )   \
	VertexShader arrayName[NORMALMAP_SHADER_COUNT + NORMALMAP_SHADER_COUNT] = {\
		NORMALMAP_VSHADER_COMBINATIONS( version, vs ),      \
		NORMALMAP_VSHADER_COMBINATIONS( version, vs_spot )  \
	};

#define ST_SHADER_COUNT	(	LIGHTONLY_SHADER_COUNT + \
							NORMALMAP_SHADER_COUNT + \
							NORMALMAP_SHADER_COUNT )

#define SPT_LIGHTING_SEPARATE_SPOT_VSHADERS_BUMP( arrayName, version, vs, vs_bump, vs_spot_bump )\
	VertexShader arrayName[ST_SHADER_COUNT] = {					\
		LIGHTONLY_VSHADER_COMBINATIONS( version, vs, false ),   \
		NORMALMAP_VSHADER_COMBINATIONS( version, vs_bump ),     \
		NORMALMAP_VSHADER_COMBINATIONS( version, vs_spot_bump )	\
	};

int speedTreeLightingSeparateSpot_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int nSpot )
{
	return normalMap_vsIndex(nDir,nPoint,nSpecDir,nSpecPoint) + (min(nSpot,1)*90);
}

int select_vsIndex()
{
	int offset = 0;
	if (g_useNormalMap)
	{
		offset = LIGHTONLY_SHADER_COUNT;
	}
	return offset  + speedTreeLightingSeparateSpot_vsIndex(
						nDirectionalLights,
						nPointLights,
						nSpecularDirectionalLights,
						nSpecularPointLights,
						nSpotLights);
}

int select_psIndex()
{
	return g_useNormalMap ? (1 + min(nSpotLights,1)) : 0;
}

