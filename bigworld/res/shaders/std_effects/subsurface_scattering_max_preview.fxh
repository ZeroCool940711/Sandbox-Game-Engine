#ifndef SUBSURFACE_SCATTERING_MAX_PREVIEW_FXH
#define SUBSURFACE_SCATTERING_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"

BW_ARTIST_EDITABLE_NORMAL_MAP
BW_ARTIST_EDITABLE_SPEC_MAP

texture subSurfaceMap
<
	bool artistEditable = true;
	string UIName = "Sub Surface Map";
	string UIDesc = "The sub-surface map for the material";
>;

float subSurfaceBlendPower
<
	string UIName = "Sub Surface Blend Power";
	string UIDesc = "The sub-surface blend power for the material";
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 1;
	bool artistEditable = true;
> = 1;

float rimWidth 
< 
	string UIName = "Rim Width";
	bool artistEditable = true;
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 2; 
> = 0.5;

float rimStrength 
< 
	string UIName = "Rim Strength";
	bool artistEditable = true;
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 2; 
> = 0.2;

sampler diffuseSampler = BW_SAMPLER( diffuseMap, BW_TEX_ADDRESS_MODE )
sampler specularSampler = BW_SAMPLER( specularMap, BW_TEX_ADDRESS_MODE )
sampler normalSampler = BW_SAMPLER( normalMap, BW_TEX_ADDRESS_MODE )
sampler subSurfaceSampler = BW_SAMPLER( subSurfaceMap, BW_TEX_ADDRESS_MODE )

struct Output
{
	float4 pos : POSITION;
	float3 worldPos : TEXCOORD0;
	float2 tc : TEXCOORD1;
	float3 tangent : TEXCOORD2;
	float3 binormal : TEXCOORD3;
	float3 normal : TEXCOORD4;
	float2 skyLightMap: TEXCOORD5;
	float fog : FOG;
};

Output subsurface_maxpreview_vs3( VertexXYZNUVTB input)
{
	Output output = (Output)0;

	output.pos = mul(input.pos, worldViewProj);

	// transform the vertex position to world space
	float4 worldPos = mul(input.pos, world);
	
	// Output the position and the world position
	output.worldPos = input.pos;

	// output the normal, tangent and binormal for use in the pixel shader
	output.normal = normalize( input.normal );
	output.tangent = normalize( input.tangent );
	output.binormal = normalize( input.binormal );
	
	// tex coord
	output.tc = input.tc;	
	return output;
}

float4 subsurface_maxpreview_ps3( Output input ) : COLOR0
{
	// Grab our maps	
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, input.tc, input.tc );
#else
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );
#endif
	float3 specularMap = tex2D( specularSampler, input.tc );
	float3 normalMap = (tex2D( normalSampler, input.tc ).xyz * 2 ) - 1;
	float4 subSurfaceMap = tex2D( subSurfaceSampler, input.tc );

	// construct texture space matrix and transform normal map normal to 
	// world space
	float3x3 tsMatrix = { input.tangent, input.binormal, input.normal };
	float3 normal = normalize( mul( normalMap, tsMatrix ) );
	
	// Lighting section

	DirectionalLight dl;
	dl.direction = normalize(mul( lightDir.xyz, worldInverse ));
	dl.colour = float4(0.5, 0.5, 0.5, 1);
	
	// Start with the ambient colour
	float3 diffuse = float3(0.1, 0.1, 0.1) + selfIllumination;
	
	// Do diffuse lighting
	diffuse += directionalLight( normal, dl );

	float3 cameraPos = worldViewInverse[3].xyz;

	float3 eye = normalize(cameraPos - input.worldPos);
	
	// calculate specular lights
	float3 specular = float3(0,0,0);
	
	specular += directionalSpecLight( normal, eye, dl );

	// do the rim lighting
	half rimLight;

	half3 rLight = dl.direction;
	rLight = -rLight;
		
	half attenuation = saturate(dot(rLight, normal ));
	rimLight = 1.0 - saturate( dot( eye, normal ) );
	rimLight = rimStrength * smoothstep( 1-rimWidth, 1, rimLight );
	rimLight = saturate( rimLight * attenuation );

	// calculate the proportion of diffuse to sub surface map to blend together and mul by the diffuse colour.
	float diffuseAmount = 1 - ( 1 - pow(saturate( dot(normal, eye) ), subSurfaceBlendPower ) ) * subSurfaceMap.w;
	diffuse = (diffuseMap.xyz * diffuseAmount + subSurfaceMap * (1-diffuseAmount)) * saturate( diffuse );

	// Calculate result colour
	float4 colour;
	colour.xyz = diffuse + (diffuseMap.xyz*rimLight) + saturate(specular) * specularMap;
#ifdef MOD2X
	colour.xyz *= (1 + diffuseLightExtraModulation);
#endif
	colour.w = diffuseMap.w;	

	return colour;
}

#endif