#ifndef NORMALMAP_2_0_FXH
#define NORMALMAP_2_0_FXH

#include "depth_helpers.fxh"

struct OutputNormalMap
{
	float4 pos:     POSITION;
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
#if SKY_LIGHT_MAP_ENABLE	
	float2 skyLightMap: TEXCOORD6;
#endif
	float4 diffuse: COLOR;
	float fog: FOG;
};


struct OutputNormalMapSpot
{
	float4 pos:     POSITION;
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
#if SKY_LIGHT_MAP_ENABLE	
	float2 skyLightMap: TEXCOORD6;
#endif
	float3 spotDir: TEXCOORD7;
	float4 diffuse: COLOR0;
	float fog: FOG;
};


struct OutputNormalMap2
{
	float4 pos:     POSITION;
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
#if SKY_LIGHT_MAP_ENABLE	
	float2 skyLightMap: TEXCOORD6;
#endif
	float3 eye:		TEXCOORD7;
	float4 diffuse: COLOR;
	float fog: FOG;
};


OutputNormalMap normalMap_vs2( BUMPED_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputNormalMap o = (OutputNormalMap)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tcDepth.xy = i.tc.xy;
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_CALC_LIGHT_COUNTS
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	
	o.diffuse = normalMapDiffuse( worldPos, worldNormal, ambientColour, selfIllumination, nDir, nPoint, nDirBump );
	o.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	o.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
	
	return o;
}


OutputNormalMap normalmapStatic_vs2( BUMPED_STATIC_LIGHTING_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputNormalMap o = (OutputNormalMap)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tcDepth.xy = i.tc.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_CALC_LIGHT_COUNTS
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);
	o.diffuse = normalMapDiffuse( worldPos, worldNormal, i.diffuse, selfIllumination, nDir, nPoint, nDirBump );			
	o.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	o.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
	return o;
}


OutputNormalMapSpot normalmapStaticSpot_vs2( BUMPED_STATIC_LIGHTING_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputNormalMapSpot o = (OutputNormalMapSpot)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tcDepth.xy = i.tc.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_CALC_LIGHT_COUNTS_SPOT
	
	o.diffuse = normalMapSpotDiffuse( worldPos, worldNormal, i.diffuse, nSpot, nDir, nPoint, nDirBump );
		
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	

	o.attenuation.y = normalMapSpotBumpDiffuse( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight2.xyz );
	o.attenuation.x = o.attenuation.z = spotSpecBumpLight( worldPos, worldNormal, eye, tsMatrix, spotLights[0], o.dLight1.xyz, o.sLight1.xyz, o.spotDir );
	o.attenuation.w = normalMapSpotSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight2.xyz );

	return o;
}


OutputNormalMapSpot normalMapSpot_vs2( BUMPED_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 	
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints	)
{
	OutputNormalMapSpot o = (OutputNormalMapSpot)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tcDepth.xy = i.tc.xy;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)		

	BW_CALC_LIGHT_COUNTS_SPOT
		
	o.diffuse = normalMapSpotDiffuse( worldPos, worldNormal, ambientColour, nSpot, nDir, nPoint, nDirBump );	
		
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	

	o.attenuation.y = normalMapSpotBumpDiffuse( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight2.xyz );
	o.attenuation.x = o.attenuation.z = spotSpecBumpLight( worldPos, worldNormal, eye, tsMatrix, spotLights[0], o.dLight1.xyz, o.sLight1.xyz, o.spotDir );
	o.attenuation.w = normalMapSpotSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight2.xyz );
	
	return o;
}


struct PS_INPUT
{
	float3 tcDepth:	TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation : TEXCOORD5;
	float4 skyLightMap : TEXCOORD6;
	float4 diffuse: COLOR;
};


#ifdef USES_SPEC_MAP
BW_COLOUR_OUT normalMap_ps2( PS_INPUT i )
{
	//  Output constant color:
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, i.tcDepth.xy, i.tcDepth.xy );
#else
	float4 diffuseMap = tex2D( diffuseSampler, i.tcDepth.xy );
#endif
	float3 normal = (tex2D( normalSampler, i.tcDepth.xy ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, i.tcDepth.xy );
	float skyMap = BW_SAMPLE_SKY_MAP( i.skyLightMap );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = float3(0,0,0);
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);

	if (nPointLights > 0)
		diffuse1Colour = pointLights[0].colour.xyz;
	else
		diffuse1Colour = directionalLights[1].colour.xyz;

	if (nPointLights > 1 && nDirectionalLights < 1)
		diffuse2Colour = pointLights[1].colour.xyz;
	else
		diffuse2Colour = directionalLights[0].colour.xyz * skyMap;

	if (nSpecularPointLights > 0)
		specular1Colour = specularPointLights[0].colour.xyz;
	else
		specular1Colour = specularDirectionalLights[1].colour.xyz;

	if (nSpecularPointLights > 1 && nSpecularDirectionalLights < 1)
		specular2Colour = specularPointLights[1].colour.xyz;
	else
		specular2Colour = specularDirectionalLights[0].colour.xyz * skyMap;

	float4 attenuation;
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 );
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;
	diffuseColour = saturate(diffuseColour);

	float3 specularColour = attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;
	specularColour *= specularMap;

	float4 colour;
#ifdef MOD2X
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
}


BW_COLOUR_OUT normalMapSpot_ps2( OutputNormalMapSpot i )
{
	//  Output constant color:
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, i.tcDepth.xy, i.tcDepth.xy );
#else
	float4 diffuseMap = tex2D( diffuseSampler, i.tcDepth.xy );
#endif
	float3 normal = (tex2D( normalSampler, i.tcDepth.xy ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, i.tcDepth.xy );
	float skyMap = BW_SAMPLE_SKY_MAP( i.skyLightMap );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = float3(0,0,0);
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);

	diffuse1Colour = spotLights[0].colour.xyz;

	if (nPointLights > 0 && nDirectionalLights < 1)
		diffuse2Colour = pointLights[0].colour.xyz;
	else
		diffuse2Colour = directionalLights[0].colour.xyz * skyMap;

	specular1Colour = spotLights[0].colour.xyz;

	if (nSpecularPointLights > 0 && nSpecularDirectionalLights < 1)
		specular2Colour = specularPointLights[0].colour.xyz;
	else
		specular2Colour = specularDirectionalLights[0].colour.xyz;
		
	float4 attenuation;
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 );

	float spotEffect = saturate( dot( normalize(i.spotDir), spotLights[0].direction.xyz  ) - spotLights[0].attenuation.z) /  ( 1 - spotLights[0].attenuation.z );
	attenuation.xz *= spotEffect;
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;
	diffuseColour = saturate(diffuseColour);

	float3 specularColour = attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;
	specularColour *= specularMap;
		
	float4 colour;
#ifdef MOD2X
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
}
#else
BW_COLOUR_OUT normalMap_ps2( PS_INPUT i )
{
	//  Output constant color:
	float4 diffuseMap = tex2D( diffuseSampler, i.tcDepth.xy );
	float3 normal = (tex2D( normalSampler, i.tcDepth.xy ) * 2 ) - 1;
#ifdef NORMALMAP_GLOW_MAP
	float3 glowMap = tex2D( glowSampler, i.tcDepth.xy );
#endif
	float3 diffuseColour = i.diffuse;
	float skyMap = BW_SAMPLE_SKY_MAP( i.skyLightMap );

	float3 diffuse1Colour = float3(0,0,0);
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);

	if (nPointLights > 0)
		diffuse1Colour = pointLights[0].colour.xyz;
	else
		diffuse1Colour = directionalLights[1].colour.xyz;

	if (nPointLights > 1 && nDirectionalLights < 1)
		diffuse2Colour = pointLights[1].colour.xyz;
	else
		diffuse2Colour = directionalLights[0].colour.xyz * skyMap;

	if (nSpecularPointLights > 0)
		specular1Colour = specularPointLights[0].colour.xyz * materialSpecular.xyz;
	else
		specular1Colour = specularDirectionalLights[1].colour.xyz * materialSpecular.xyz;

	if (nSpecularPointLights > 1 && nSpecularDirectionalLights < 1)
		specular2Colour = specularPointLights[1].colour.xyz * materialSpecular.xyz;
	else
		specular2Colour = specularDirectionalLights[0].colour.xyz * materialSpecular.xyz * skyMap;
		
	float4 attenuation;
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 );
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;
	diffuseColour = saturate(diffuseColour);

#ifdef NORMALMAP_GLOW_MAP
	float3 specularColour = glowMap * glowFactor;
#else
	float3 specularColour = float3(0,0,0);
#endif
	specularColour += attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;
		
	float4 colour;
#ifdef MOD2X
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
}


BW_COLOUR_OUT normalMapSpot_ps2( OutputNormalMapSpot i )
{
	//  Output constant color:
	float4 diffuseMap = tex2D( diffuseSampler, i.tcDepth.xy );
	float3 normal = (tex2D( normalSampler, i.tcDepth.xy ) * 2 ) - 1;
#ifdef NORMALMAP_GLOW_MAP
	float3 glowMap = tex2D( glowSampler, i.tcDepth.xy );
#endif
	float skyMap = BW_SAMPLE_SKY_MAP( i.skyLightMap );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = float3(0,0,0);
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);

	diffuse1Colour = spotLights[0].colour.xyz;

	if ( nPointLights > 0 && nDirectionalLights < 1)
		diffuse2Colour = pointLights[0].colour.xyz;				
	else
		diffuse2Colour = directionalLights[0].colour.xyz * skyMap;

	specular1Colour = spotLights[0].colour.xyz * materialSpecular.xyz;	

	if ( nSpecularPointLights > 0 && nSpecularDirectionalLights < 1)
		specular2Colour = specularPointLights[0].colour.xyz * materialSpecular.xyz;
	else
		specular2Colour = specularDirectionalLights[0].colour.xyz * materialSpecular.xyz * skyMap;
		
	float4 attenuation;
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 );

	float spotEffect = saturate( dot( normalize(i.spotDir), spotLights[0].direction.xyz  ) - spotLights[0].attenuation.z) /  ( 1 - spotLights[0].attenuation.z );
	attenuation.xz *= spotEffect;
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;
	diffuseColour = saturate(diffuseColour);

#ifdef NORMALMAP_GLOW_MAP
	float3 specularColour = glowMap * glowFactor;
#else
	float3 specularColour = float3(0,0,0);
#endif
	specularColour += attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;
		
	float4 colour;
#ifdef MOD2X
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
}
#endif //#USES_SPEC_MAP


#include "normalmap_shader_combinations.fxh"


#define NORMALMAP_SEPARATE_SPOT_PSHADER_COMBINATIONS( arrayName, version, ps, ps_spot )\
PixelShader arrayName[2] =\
{\
	compile version ps(),\
	compile version ps_spot()\
};


#define NORMALMAP_SEPARATE_SPOT_VSHADERS( arrayName, version, vs, vs_spot, vs_staticLighting, vs_staticLighting_spot )\
VertexShader arrayName[360] = {\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs ),\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs_spot ),\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs_staticLighting ),\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs_staticLighting_spot )\
};


#define NORMALMAP_SKINNED_SEPARATE_SPOT_VSHADERS( arrayName, version, vs, vs_spot )\
VertexShader arrayName[180] = {\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs ),\
	NORMALMAP_VSHADER_COMBINATIONS( version, vs_spot )\
};


int normalMapSeparateSpot_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int nSpot, int staticLighting )
{	
	return normalMap_vsIndex(nDir,nPoint,nSpecDir,nSpecPoint) + (min(nSpot,1)*90) + int(staticLighting) * 180;
}


int normalMapSkinnedSeparateSpot_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int nSpot )
{
	return normalMapSeparateSpot_vsIndex( nDir, nPoint, nSpecDir, nSpecPoint, nSpot, 0 );
}


#endif