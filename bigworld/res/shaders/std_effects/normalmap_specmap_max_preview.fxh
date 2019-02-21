#ifndef NORMALMAP_SPECMAP_MAX_PREVIEW_FXH
#define NORMALMAP_SPECMAP_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"

BW_ARTIST_EDITABLE_NORMAL_MAP
BW_ARTIST_EDITABLE_SPEC_MAP

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
sampler normalSampler = BW_SAMPLER(normalMap, BW_TEX_ADDRESS_MODE)
sampler specularSampler = BW_SAMPLER(specularMap, BW_TEX_ADDRESS_MODE)

struct OutputNormalMap
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
	float3 dLight1: TEXCOORD1;
	float3 dLight2: TEXCOORD2;
	float3 sLight1: TEXCOORD3;
	float3 sLight2: TEXCOORD4;
	float4 attenuation: TEXCOORD5;
	float4 diffuse: COLOR;
	float fog: FOG;
};


OutputNormalMap normalMapSpecmap_maxpreview_vs2( VertexXYZNUVTB i)
{
	OutputNormalMap o = (OutputNormalMap)0;
	DirectionalLight dl;
	dl.direction = normalize(mul( lightDir.xyz, worldInverse ));
	dl.colour = float4(0.5, 0.5, 0.5, 1);

	o.pos = mul(i.pos, worldViewProj);
	
	float3x3 tsMatrix = {i.tangent, i.binormal, i.normal };

	o.tc = i.tc;
	o.diffuse = float4(0.1, 0.1, 0.1, 1);

	o.attenuation.x = directionalBumpLight( tsMatrix, dl, o.dLight1.xyz );
	o.attenuation.y = 0;

	float3 camp = mul( viewInverse[3].xyz, worldInverse );
	float3 eye = normalize(camp - i.pos);

	o.attenuation.z = directionalSpecBumpLight( eye, tsMatrix, dl, o.sLight1.xyz );
	o.attenuation.w = 0;
	return o;
}

float4 normalMapSpecmap_maxpreview_ps2( OutputNormalMap i ) : COLOR0
{
	//  Output constant color:
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, i.tc, i.tc );
#else
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
#endif	
	float3 normal = (tex2D( normalSampler, i.tc ) * 2 ) - 1;
	float3 specularMap = tex2D( specularSampler, i.tc );
	float3 diffuseColour = i.diffuse;

	float3 diffuse1Colour = lightColour.xyz;
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = lightColour.xyz;
	float3 specular2Colour = float3(0,0,0);
	float4 attenuation = float4(0,0,0,0);

	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) );
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 );
	attenuation *= i.attenuation;

	diffuseColour +=  attenuation.x * diffuse1Colour;

	float3 specularColour = attenuation.z * specular1Colour * specularMap;

	float4 colour;
	colour.xyz = (diffuseColour * diffuseMap.xyz) + specularColour;
	colour.w = diffuseMap.w;

	return colour;
}

#endif