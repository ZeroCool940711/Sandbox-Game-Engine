#ifndef NORMALMAP_3_0_FXH
#define NORMALMAP_3_0_FXH

#include "depth_helpers.fxh"

//--------------------------------------------------------------//
// This file defines shaders and shader combination arrays for
// Normal Map Chrome pixel/vertexshader 3 stuff.
//--------------------------------------------------------------//

OutputNormalMapEnv normalMapEnv_vs3( BUMPED_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputNormalMapEnv o = (OutputNormalMapEnv)0;
	
	PROJECT_POSITION( o.pos )
	BW_SKY_MAP_COORDS_WORLD_SPACE_ENV( o, worldPos );
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc.xy = i.tc;
	
	CALCULATE_TS_MATRIX
	
	BW_CALC_LIGHT_COUNTS
	int ns = nSpotLights < 2 ? nSpotLights : 2;

	float4 attenuation;	
	float3 eye = normalize(wsCameraPos - worldPos);
	
	o.diffuse.rgb = normalMapDiffuse_vs1( worldPos, worldNormal, ambientColour, selfIllumination, nSpotLights, nDir, nPoint, nDirBump );
	attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
	
	o.dLight1.w = attenuation.x;
	o.dLight2.w = attenuation.y;
	o.sLight1.w = attenuation.z;
#if USE_MRT_DEPTH
	o.diffuse.a = attenuation.w;
	BW_DEPTH(o.sLight2.w, o.pos.z)
#else
	o.sLight2.w = attenuation.w;
#endif

	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = transpose(tsMatrix)[0].xyz;
	o.tangentToCubeSpace1.xyz = transpose(tsMatrix)[1].xyz;
	o.tangentToCubeSpace2.xyz = transpose(tsMatrix)[2].xyz;

	o.tangentToCubeSpace0.w = worldPos.x;
	o.tangentToCubeSpace1.w = worldPos.y;
	o.tangentToCubeSpace2.w = worldPos.z;

	return o;
}


OutputNormalMapEnv normalmapStaticEnv_vs3( BUMPED_STATIC_LIGHTING_VERTEX_FORMAT i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputNormalMapEnv o = (OutputNormalMapEnv)0;
	
	PROJECT_POSITION( o.pos )
	BW_SKY_MAP_COORDS_WORLD_SPACE_ENV( o, worldPos );
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc.xy = i.tc;
	
	CALCULATE_TS_MATRIX
				
	BW_CALC_LIGHT_COUNTS
	int ns = nSpotLights < 2 ? nSpotLights : 2;

	float4 attenuation;	
	float3 eye = normalize(wsCameraPos - worldPos);
	
	o.diffuse.rgb = normalMapDiffuse_vs1( worldPos, worldNormal, i.diffuse, selfIllumination, nSpotLights, nDir, nPoint, nDirBump );
	attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
	
	o.dLight1.w = attenuation.x;
	o.dLight2.w = attenuation.y;
	o.sLight1.w = attenuation.z;
#if USE_MRT_DEPTH
	o.diffuse.a = attenuation.w;
	BW_DEPTH(o.sLight2.w, o.pos.z)
#else
	o.sLight2.w = attenuation.w;
#endif
	
	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = transpose(tsMatrix)[0].xyz;
	o.tangentToCubeSpace1.xyz = transpose(tsMatrix)[1].xyz;
	o.tangentToCubeSpace2.xyz = transpose(tsMatrix)[2].xyz;

	
	o.tangentToCubeSpace0.w = worldPos.x;
	o.tangentToCubeSpace1.w = worldPos.y;
	o.tangentToCubeSpace2.w = worldPos.z;
    
	return o;
}


BW_COLOUR_OUT normalMapEnv_ps3( OutputNormalMapEnv i )
{
	//  Output constant color:
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float3 normal = (tex2D( normalSampler, i.tc ) * 2 ) - 1;
#ifdef NORMALMAP_GLOW_MAP
	float3 glowMap = tex2D( glowSampler, i.tc );
#endif
	float skyMap = BW_SAMPLE_SKY_MAP_ENV(i);
	float3 diffuseColour = i.diffuse;
#ifdef CHROME_SPEC_MAP
	float3 specularMap = tex2D( specularSampler, i.tc );
#endif
	float reflectionMask = tex2D( maskSampler, i.tc );

	// Calculate a reflection vector and fresnel term
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	float3 eyeVec = float3(	wsCameraPos.x - i.tangentToCubeSpace0.w,
							wsCameraPos.y - i.tangentToCubeSpace1.w, 
							wsCameraPos.z - i.tangentToCubeSpace2.w );
	eyeVec = normalize( eyeVec );
	float3 reflVec = reflect(eyeVec, envNormal);
	reflVec.y = -reflVec.y;

	float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec);
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant);
	reflectionColour *= fresnelTerm;

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
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) ) * i.dLight1.w;
	attenuation.y = saturate( dot( normalize(i.dLight2.xyz), normal ) ) * i.dLight2.w;
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 ) * i.sLight1.w;
#if USE_MRT_DEPTH
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 ) * i.diffuse.a;
#else
	attenuation.w = pow( saturate( dot( normalize(i.sLight2.xyz), normal ) ), 32 ) * i.sLight2.w;
#endif


	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;

#ifdef NORMALMAP_GLOW_MAP
	float3 specularColour = glowMap * glowFactor;
#else
	float3 specularColour = float3(0,0,0);
#endif
	
	specularColour += attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;

	float4 colour;
#ifdef MOD2X
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) );
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz);
#endif
	specularColour += diffuseColour * reflectionColour * reflectionAmount * reflectionMask;

#ifdef CHROME_SPEC_MAP
	specularColour *= specularMap;
#endif

	colour.xyz += specularColour;
	
	colour.xyz = lerp( fogColour.xyz, colour.xyz, saturate(i.fog) );

	colour.w = diffuseMap.a;

	BW_FINAL_COLOUR( i.sLight2.w, colour )
}

#include "normalmap_shader_combinations.fxh"


#define NORMALMAP_ENV_3_0_VSHADERS( arrayName, version )\
VertexShader arrayName[NORMALMAP_SHADER_COUNT + NORMALMAP_SHADER_COUNT] = {\
	NORMALMAP_VSHADER_COMBINATIONS( version, normalMapEnv_vs3 ),\
	NORMALMAP_VSHADER_COMBINATIONS( version, normalmapStaticEnv_vs3 )\
};

#define NORMALMAP_SKINNED_ENV_3_0_VSHADERS( arrayName, version )\
VertexShader arrayName[NORMALMAP_SHADER_COUNT] = {\
	NORMALMAP_VSHADER_COMBINATIONS( version, normalMapEnv_vs3 )\
};

int normalMapEnv_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int staticLighting )
{
	return (normalMap_vsIndex( nDir, nPoint, nSpecDir, nSpecPoint ) + int(staticLighting) * NORMALMAP_SHADER_COUNT);	
}

int normalMapSkinnedEnv_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint, int staticLighting )
{
	return (normalMapEnv_vsIndex(nDir, nPoint, nSpecDir, nSpecPoint, 0));
}


#endif