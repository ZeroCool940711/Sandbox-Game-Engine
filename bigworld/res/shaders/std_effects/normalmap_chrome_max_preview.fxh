#ifndef NORMALMAP_CHROME_MAX_PREVIEW_FXH
#define NORMALMAP_CHROME_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"

BW_ARTIST_EDITABLE_FRESNEL
BW_ARTIST_EDITABLE_NORMAL_MAP
BW_ARTIST_EDITABLE_GLOW_MAP
BW_ARTIST_EDITABLE_GLOW_FACTOR
BW_ARTIST_EDITABLE_MATERIAL_SPECULAR

float reflectionAmount
< 
	bool artistEditable = true;
	string UIName = "Reflection Amount";
	string UIDesc = "A scaling factor for the reflection";
	float UIMin = 0;
	float UIMax = 2.0;
	int UIDigits = 2;
> = 1.0;

texture reflectionMap
<
	bool artistEditable = true;
	string UIWidget = "CubeMap";
	string UIName = "Reflection Cube Map";
	string UIDesc = "The reflection map for the material";

	string type = "Cube";
>;

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
sampler normalSampler = BW_SAMPLER(normalMap, BW_TEX_ADDRESS_MODE)
sampler glowSampler = BW_SAMPLER(glowMap, BW_TEX_ADDRESS_MODE)
sampler reflectionSampler = BW_SAMPLER(reflectionMap, WRAP)
samplerCUBE reflectionCubeSampler = sampler_state
{
	Texture = (reflectionMap);
	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

struct OutputNormalMapEnv
{
	float4 pos:     POSITION;
	float4 tc:      TEXCOORD0;
	float4 dLight1: TEXCOORD1;
	float4 dLight2: TEXCOORD2;
	float4 sLight1: TEXCOORD3;
	float4 sLight2: TEXCOORD4;

	float4 tangentToCubeSpace0 : TEXCOORD5; //first row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace1 : TEXCOORD6; //second row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace2 : TEXCOORD7; //third row of the 3x3 transform from tangent to cube space	
	
	float4 diffuse: COLOR;
	float fog: FOG;
};

OutputNormalMapEnv normalMap_maxpreview_vs3( VertexXYZNUVTB i)
{
	OutputNormalMapEnv o = (OutputNormalMapEnv)0;
	DirectionalLight dl;
	dl.direction = normalize(mul( lightDir.xyz, worldInverse ));
	dl.colour = float4(0.5, 0.5, 0.5, 1);

	o.pos = mul(i.pos, worldViewProj);
	
	float3x3 tsMatrix = {i.tangent, i.binormal, i.normal };

	o.tc.xy = i.tc.xy;
	o.diffuse = float4(0.1, 0.1, 0.1, 1);

	o.dLight1.w = directionalBumpLight( tsMatrix, dl, o.dLight1.xyz );
	o.dLight2.w = 0;

	float3 camp = mul( viewInverse[3].xyz, worldInverse );
	float3 eye = normalize(camp - i.pos);	

	o.sLight1.w = directionalSpecBumpLight( eye, tsMatrix, dl, o.sLight1.xyz );;
	o.sLight2.w = 0;

	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = mul(tsMatrix, transpose(world)[0].xyz);
	o.tangentToCubeSpace1.xyz = mul(tsMatrix, transpose(world)[1].xyz);
	o.tangentToCubeSpace2.xyz = mul(tsMatrix, transpose(world)[2].xyz);

	float3 wsCameraPos = viewInverse[3].xyz;

	float4 worldPos = mul( i.pos, world );

    // Compute the eye vector (going from shaded point to eye) in cube space
	float3 eyeVector = wsCameraPos - worldPos;
    
    o.tangentToCubeSpace0.w = eyeVector.x;
    o.tangentToCubeSpace1.w = eyeVector.y;
    o.tangentToCubeSpace2.w = eyeVector.z;


	return o;
}

float4 normalMap_maxpreview_ps3( OutputNormalMapEnv i ) : COLOR0
{
	//  Output constant color:
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float3 normal = (tex2D( normalSampler, i.tc ) * 2 ) - 1;
	float3 glowMap = tex2D( glowSampler, i.tc );
	float3 diffuseColour = i.diffuse;
	
	// Calculate a reflection vector and fresnel term
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	float3 eyeVec = normalize( float3(i.tangentToCubeSpace0.w, i.tangentToCubeSpace1.w, i.tangentToCubeSpace2.w) );	
	float3 reflVec = reflect(eyeVec, envNormal);	
	reflVec.yz = reflVec.zy;
	reflVec.y = -reflVec.y;
	
	float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec);
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant);
	reflectionColour *= fresnelTerm;

	float3 diffuse1Colour = lightColour.xyz;
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = lightColour.xyz * materialSpecular;
	float3 specular2Colour = float3(0,0,0);
	float4 attenuation = float4(0,0,0,0);

	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) ) * i.dLight1.w;
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 ) * i.sLight1.w;

	diffuseColour +=  attenuation.x * diffuse1Colour;

	float3 specularColour = glowMap * glowFactor;
	specularColour += attenuation.z * specular1Colour;

	float4 colour;
	colour.xyz = (diffuseColour * diffuseMap) + specularColour;
	colour.xyz += reflectionColour * reflectionAmount * diffuseMap.aaa;
	colour.w = 1;

	return colour;
}

#endif