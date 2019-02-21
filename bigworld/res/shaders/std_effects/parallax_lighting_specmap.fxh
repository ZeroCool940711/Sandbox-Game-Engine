#ifndef PARALLAX_LIGHTING_SPECMAP_FXH
#define PARALLAX_LIGHTING_SPECMAP_FXH

#include "depth_helpers.fxh"

#ifdef IN_GAME
//Expose and declare the standard characater lighting specmap variables
#include "normalmap_specmap.fxh"
#endif

float parallaxScale
<
	bool artistEditable = true;
	string UIName = "Parallax Scale";
	string UIDesc = "The parallax scale factor for the material";
	float UIMin = 0;
	float UIMax = 1;
	int UIDigits = 3;
> = 0.042;

float parallaxAspectRatio
<
	bool artistEditable = true;
	string UIName = "Parallax Aspect Ratio";
	string UIDesc = "The parallax aspect ratio for the material";
	float UIMin = 1;
	float UIMax = 2;
	int UIDigits = 1;
> = 1.0;

float2 parallax( float2 uv, float3 eye, float height, float disp)
{
	float3 eyeVect = normalize(eye.xyz);
	float offset = (height*disp)-(disp/2);
	return uv+float2(eyeVect.x * offset, eyeVect.y * (-offset) * parallaxAspectRatio);
}

struct OutputParallaxLighting
{
	float4 pos:     POSITION;
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
	float3 eye: TEXCOORD6;
#if SKY_LIGHT_MAP_ENABLE	
	float2 skyLightMap: TEXCOORD7;
#endif
	float4 diffuse: COLOR;
	float fog: FOG;
};


#ifdef IN_GAME
struct OutputParallaxLightingSpot
{
	float4 pos:     POSITION;
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
	float3 eye: TEXCOORD6;
#if SKY_LIGHT_MAP_ENABLE	
	float2 skyLightMap: TEXCOORD7;
#endif
	float4 spotDir : COLOR1;
	float4 diffuse: COLOR0;
	float fog: FOG;
};


OutputParallaxLighting parallaxLighting_vs2( VertexXYZNUVTB i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputParallaxLighting o = (OutputParallaxLighting)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )		

	BW_CALC_LIGHT_COUNTS
	
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	
	o.diffuse = normalMapDiffuse( worldPos, worldNormal, ambientColour, selfIllumination, nDir, nPoint, nDirBump );
	o.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );	
	o.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
		
	o.eye = mul( tsMatrix, eye );
	
	return o;
}


OutputParallaxLighting parallaxStaticLighting_vs2( VertexXYZNUVTB_D i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputParallaxLighting o = (OutputParallaxLighting)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_CALC_LIGHT_COUNTS
	
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);
	o.diffuse = normalMapDiffuse( worldPos, worldNormal, i.diffuse, selfIllumination, nDir, nPoint, nDirBump );			
	o.attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	o.attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
		
	o.eye = mul( tsMatrix, eye );
		
	return o;
}


OutputParallaxLightingSpot parallaxLightingSpot_vs2( VertexXYZNUVTB i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputParallaxLightingSpot o = (OutputParallaxLightingSpot)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )

	BW_CALC_LIGHT_COUNTS_SPOT
	o.diffuse = normalMapSpotDiffuse( worldPos, worldNormal, ambientColour, nSpot, nDir, nPoint, nDirBump );
		
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	

	o.attenuation.y = normalMapSpotBumpDiffuse( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight2.xyz );
	o.attenuation.x = o.attenuation.z = spotSpecBumpLight( worldPos, worldNormal, eye, tsMatrix, spotLights[0], o.dLight1.xyz, o.sLight1.xyz, o.spotDir.xyz );
	o.attenuation.w = normalMapSpotSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight2.xyz );
	o.spotDir.xyz  = (normalize(o.spotDir.xyz) + float3(1,1,1))/2.0;

	o.eye = mul( tsMatrix, eye );
	
	return o;
}


OutputParallaxLightingSpot parallaxStaticLightingSpot_vs2( VertexXYZNUVTB_D i,
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpecularDirectionals, 
	uniform int nSpecularPoints )
{
	OutputParallaxLightingSpot o = (OutputParallaxLightingSpot)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_CALC_LIGHT_COUNTS_SPOT
	o.diffuse = normalMapSpotDiffuse( worldPos, worldNormal, i.diffuse, nSpot, nDir, nPoint, nDirBump );
		
	CALCULATE_TS_MATRIX
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);	

	o.attenuation.y = normalMapSpotBumpDiffuse( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight2.xyz );
	o.attenuation.x = o.attenuation.z = spotSpecBumpLight( worldPos, worldNormal, eye, tsMatrix, spotLights[0], o.dLight1.xyz, o.sLight1.xyz, o.spotDir.xyz );
	o.attenuation.w = normalMapSpotSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight2.xyz );
	o.spotDir.xyz  = (normalize(o.spotDir.xyz) + float3(1,1,1))/2.0;
		
	o.eye = mul( tsMatrix, eye );
	
	return o;
}


struct PARALLAX_PS_INPUT
{
	float3 tcDepth: TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation : TEXCOORD5;
	float3 eye : TEXCOORD6;
	float4 skyLightMap : TEXCOORD7;
	float4 diffuse: COLOR;
};


BW_COLOUR_OUT parallaxLighting_ps2( PARALLAX_PS_INPUT i )
{
	float height = tex2D( normalSampler, i.tcDepth.xy ).a;
	float2 tc = parallax( i.tcDepth.xy, i.eye, height, parallaxScale );
	float4 diffuseMap = tex2D( diffuseSampler, tc );
	float3 normal = (tex2D( normalSampler, tc ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, tc );
	float skyMap = BW_SAMPLE_SKY_MAP(i.skyLightMap);
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

BW_COLOUR_OUT parallaxLightingSpot_ps2( OutputParallaxLightingSpot i )
{
	float height = tex2D( normalSampler, i.tcDepth.xy ).a;
	float2 tc = parallax( i.tcDepth.xy, i.eye, height, parallaxScale );
	float4 diffuseMap = tex2D( diffuseSampler, tc );
	float3 normal = (tex2D( normalSampler, tc ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, tc );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = float3(0,0,0);
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);
	float3 specular2Colour = float3(0,0,0);

	diffuse1Colour = spotLights[0].colour.xyz;

	if (nPointLights > 0 && nDirectionalLights < 1)
		diffuse2Colour = pointLights[0].colour.xyz;
	else
		diffuse2Colour = directionalLights[0].colour.xyz;

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

	// NOTE: the only free input register was COLOR1, this made precision a problem for the spotlight
	// but its only really noticable for really really small cone angles
	float spotEffect = saturate( dot( normalize((i.spotDir*2.0)-float3(1,1,1)), spotLights[0].direction.xyz  ) - spotLights[0].attenuation.z) /  ( 1 - spotLights[0].attenuation.z );
	
	attenuation.xz *= spotEffect;
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;
	diffuseColour +=  attenuation.y * diffuse2Colour;

	float3 specularColour = attenuation.z * specular1Colour;
	specularColour += attenuation.w * specular2Colour;
	specularColour *= specularMap;

	float skyMap = BW_SAMPLE_SKY_MAP(i.skyLightMap)
	float4 colour;
#ifdef MOD2X	
	colour.xyz = (skyMap * diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (skyMap * diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	BW_FINAL_COLOUR( i.tcDepth.z, colour )
}

#endif	//ifdef IN_GAME

#endif	//ifdef PARALLAX_LIGHTING_SPECMAP_FXH