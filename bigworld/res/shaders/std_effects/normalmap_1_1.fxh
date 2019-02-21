#ifndef NORMALMAP_1_1_FXH
#define NORMALMAP_1_1_FXH

texture normalisationMap : NormalisationMap;
sampler normalisationSampler = BW_SAMPLER( normalisationMap, CLAMP )

struct OutputDiffuseNormalMap
{
	float4 pos:     		POSITION;
	float2 tc:      		TEXCOORD0;
	float2 ntc:		TEXCOORD1;
	float3 dLight1: 	TEXCOORD2;
	float3 dLight2: 	TEXCOORD3;
	float4 diffuse: 		COLOR0;
	float4 attenuation: 	COLOR1;
	float fog: FOG;
};


struct OutputSpecularNormalMap
{
	float4 pos:     		POSITION;
	float2 tc:			TEXCOORD0;
	float2 ntc:			TEXCOORD1;
	float3 sLight: 		TEXCOORD2;
	float4 attenuation:	COLOR0;
	float fog: FOG;
};


struct OutputSpecularNormalMapAlpha
{
	float4 pos:     		POSITION;
	float2 tc:			TEXCOORD0;
	float2 ntc:			TEXCOORD1;
	float3 sLight: 		TEXCOORD2;
	float2 atc:			TEXCOORD3;	
	float4 attenuation:	COLOR0;
	float fog: FOG;
};


OutputDiffuseNormalMap diffuseNormalMap_vs1( BUMPED_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints )
{
	OutputDiffuseNormalMap o = (OutputDiffuseNormalMap)0;

	PROJECT_POSITION( o.pos )

	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tc = i.tc;
	o.ntc = i.tc;

	BW_CALC_LIGHT_COUNTS
	
	o.diffuse = normalMapDiffuse_vs1(worldPos, worldNormal, ambientColour, selfIllumination, nSpotLights, nDir, nPoint, nDirBump);
	CALCULATE_TS_MATRIX
	o.attenuation = normalMapDiffuseBump_vs1(worldPos, tsMatrix, nPointBump, nDirBump, o.dLight1.xyz, o.dLight2.xyz );
	
	return o;
}


OutputDiffuseNormalMap diffuseStaticNormalMap_vs1( BUMPED_STATIC_LIGHTING_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints )
{
	OutputDiffuseNormalMap o = (OutputDiffuseNormalMap)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc = i.tc;
	o.ntc = i.tc;

	BW_CALC_LIGHT_COUNTS
	
	o.diffuse = normalMapDiffuse_vs1( worldPos, worldNormal, i.diffuse, selfIllumination, nSpotLights, nDir, nPoint, nDirBump );		
	CALCULATE_TS_MATRIX
	o.attenuation = normalMapDiffuseBump_vs1(worldPos, tsMatrix, nPointBump, nDirBump, o.dLight1.xyz, o.dLight2.xyz);
	
	return o;
}


OutputSpecularNormalMapAlpha specularNormalMap_vs1( BUMPED_VERTEX_FORMAT i,
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputSpecularNormalMapAlpha o = (OutputSpecularNormalMapAlpha)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	

	o.ntc = i.tc;
	o.tc = i.tc;
	o.atc = i.tc;	

	BW_CALC_SPEC_LIGHT_COUNTS_VS1
	
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	

	o.attenuation.xyz = normalMapSpecularBump_vs1(worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight.xyz );
	o.attenuation.w = 0;
	
	return o;
}


float4 diffuseNormalMap_ps1( OutputDiffuseNormalMap i,
							uniform int nBumpPointLights) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float3 normal = (tex2D( normalSampler, i.ntc ) * 2 ) - 1;
	float3 diffuseDir1 = (texCUBE( normalisationSampler, i.dLight1 ) * 2) - 1;
	float3 diffuseDir2 = (texCUBE( normalisationSampler, i.dLight2 ) * 2) - 1;
	float4 diffuse = i.diffuse;
	
	float3 d2Colour;
	float3 d1Colour;
	if (nBumpPointLights < 2)
		d2Colour = directionalLights[0].colour;
	else
		d2Colour = pointLights[1].colour;
	if (nBumpPointLights < 1)
		d1Colour = directionalLights[1].colour;
	else
		d1Colour = pointLights[0].colour;

	diffuse.xyz += saturate(dot(normal, diffuseDir1)) * i.attenuation.xyz * d1Colour;
	diffuse.xyz += saturate(dot(normal, diffuseDir2)) * i.attenuation.w * d2Colour;
	
#ifdef MOD2X
	return float4(diffuse.xyz * diffuseMap.xyz + (diffuse.xyz * diffuseMap.xyz * diffuseLightExtraModulation), diffuseMap.w);
#else
	return float4(diffuse.xyz * diffuseMap.xyz, diffuseMap.w);
#endif
}


#ifdef USES_SPEC_MAP
float4 specularNormalMap_ps1( OutputSpecularNormalMapAlpha i, uniform bool isPoint ) : COLOR0
{
	float3 normal = (tex2D( normalSampler, i.ntc ) * 2 ) - 1;
	float3 specularDir = (texCUBE( normalisationSampler, i.sLight ) * 2) - 1;
	float3 specularMap = (tex2D( specularSampler, i.tc ));
	float4 specular;
	specular.w = dot( normal, specularDir ), 32;
	specular.w = pow(specular.w, 32);
	float3 specularColour;
	if (isPoint)
		specularColour = specularPointLights[0].colour * specularMap;
	else
		specularColour = specularDirectionalLights[0].colour * specularMap;
	float3 colour = specularColour * i.attenuation.xyz;
		
	float alpha = tex2D(diffuseSampler, i.atc).a;
	return float4( specular.w * colour, alpha );
}
#else
float4 specularNormalMap_ps1( OutputSpecularNormalMapAlpha i, uniform bool isPoint ) : COLOR0
{
	float3 normal = (tex2D( normalSampler, i.ntc ) * 2 ) - 1;
	float3 specularDir = (texCUBE( normalisationSampler, i.sLight ) * 2) - 1;
#ifdef NORMALMAP_GLOW_MAP
	float3 glow = (tex2D( glowSampler, i.tc )) * glowFactor;
#endif
	float4 specular;
	specular.w = saturate(dot( normal, specularDir ));
	specular.w = pow(specular.w, 32);
	float3 specularColour;
	if (isPoint)
		specularColour = specularPointLights[0].colour * materialSpecular;
	else
		specularColour = specularDirectionalLights[0].colour * materialSpecular;
	float3 colour = specularColour * i.attenuation.xyz;
		
	float alpha = tex2D(diffuseSampler, i.atc).a;	
#ifdef NORMALMAP_GLOW_MAP
	return float4( specular.w * colour + glow, alpha );
#else
	return float4( specular.w * colour, alpha );
#endif
	
}
#endif


#include "normalmap_shader_combinations.fxh"


#define NORMALMAP_DIFFUSEONLY_VSHADERS( arrayName, version )\
VertexShader arrayName[30] = {\
	NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS( version, diffuseNormalMap_vs1 ),\
	NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS( version, diffuseStaticNormalMap_vs1 )\
};

#define NORMALMAP_SKINNED_DIFFUSEONLY_VSHADERS( arrayName, version )\
VertexShader arrayName[15] = {\
	NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS( version, diffuseNormalMap_vs1 )\
};

#define NORMALMAP_SKINNED_DIFFUSEONLY_VSHADERS_LIMITED( arrayName, version )\
VertexShader arrayName[15] = {\
	NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS_LIMITED( version, diffuseNormalMap_vs1 )\
};

#define NORMALMAP_SPECULARONLY_VSHADERS( arrayName, version )\
VertexShader arrayName[3] = {\
	compile version specularNormalMap_vs1(0,0),\
	compile version specularNormalMap_vs1(1,0),\
	compile version specularNormalMap_vs1(0,1)\
};

#define NORMALMAP_DIFFUSEONLY_PSHADERS( arrayName, version )\
PixelShader arrayName[3] = {\
	compile version diffuseNormalMap_ps1(0),\
	compile version diffuseNormalMap_ps1(1),\
	compile version diffuseNormalMap_ps1(2)\
};

#define NORMALMAP_SPECULARONLY_PSHADERS( arrayName, version )\
PixelShader arrayName[2] = {\
	compile version specularNormalMap_ps1(false),\
	compile version specularNormalMap_ps1(true)\
};

int specularOnlyVSIndex( int nSpecDir, int nSpecPoint )
{
	int index = 0;
	if (nSpecDir >0)
		index = 1;
	else if (nSpecPoint > 0)
		index = 2;

	return index;
}

#endif