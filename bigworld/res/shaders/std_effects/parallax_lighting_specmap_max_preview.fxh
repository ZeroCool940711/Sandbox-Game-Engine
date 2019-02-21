#ifndef PARALLAX_LIGHTING_SPECMAP_MAX_PREVIEW_FXH
#define PARALLAX_LIGHTING_SPECMAP_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"
#include "parallax_lighting_specmap.fxh"

BW_ARTIST_EDITABLE_NORMAL_MAP
BW_ARTIST_EDITABLE_SPEC_MAP
sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
sampler normalSampler = BW_SAMPLER(normalMap, BW_TEX_ADDRESS_MODE)
sampler specularSampler = BW_SAMPLER(specularMap, BW_TEX_ADDRESS_MODE)

float2 maxparallax( float2 uv, float3 eye, float height, float disp)
{
	float3 eyeVect = normalize(eye.xyz);
	float offset = (height*disp)-(disp/2);
	return uv.xy+float2(eyeVect.y * offset, -eyeVect.x * (-offset) * parallaxAspectRatio);
}

OutputParallaxLighting parallaxLighting_maxpreview_vs2( VertexXYZNUVTB i )
{
	OutputParallaxLighting o = (OutputParallaxLighting)0;
	DirectionalLight dl;
	dl.direction = normalize(mul( lightDir.xyz, worldInverse ));
	dl.colour = lightColour;

	PROJECT_POSITION( o.pos )
	o.pos = mul( i.pos, worldViewProj );

	o.tcDepth.xy = i.tc;
	o.diffuse = float4(0.1, 0.1, 0.1, 1) + selfIllumination;

	CALCULATE_TS_MATRIX
	float3 wsCameraPos = viewInverse[3].xyz;
	float3 eye = normalisedEyeVector(worldPos, wsCameraPos);
	o.eye = mul( tsMatrix, eye );

	o.attenuation.x = directionalBumpLight( tsMatrix, dl, o.dLight1.xyz );
	o.attenuation.y = 0;

	o.attenuation.z = directionalSpecBumpLight( eye, tsMatrix, dl, o.sLight1.xyz );
	o.attenuation.w = 0;


	return o;
}


float4 parallaxLighting_maxpreview_ps2( OutputParallaxLighting i ) : COLOR0
{
	float height = tex2D( normalSampler, i.tcDepth.xy ).a;
	float2 tc = maxparallax( i.tcDepth.xy, i.eye, height, parallaxScale );
	float4 diffuseMap = tex2D( diffuseSampler, tc );
	float3 normal = (tex2D( normalSampler, tc ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, tc );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = float3(0,0,0);
	float3 specular1Colour = float3(0,0,0);

	diffuse1Colour = lightColour.xyz;

	specular1Colour = lightColour.xyz;
		
	float4 attenuation = float4(0,0,0,0);
	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;

	float3 specularColour = attenuation.z * specular1Colour;
	specularColour *= specularMap;

	float4 colour;
#ifdef MOD2X	
	colour.xyz = (diffuseColour * diffuseMap.xyz * (1 + diffuseLightExtraModulation) ) + specularColour;
#else
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
#endif
	colour.w = diffuseMap.w;

	return colour;
}

#endif