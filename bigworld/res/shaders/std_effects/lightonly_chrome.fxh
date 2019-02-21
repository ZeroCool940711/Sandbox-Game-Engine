#ifndef LIGHTONLY_CHROME_FXH
#define LIGHTONLY_CHROME_FXH

#include "shader_combination_helpers.fxh"

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_DOUBLE_SIDED

texture otherMap
<
	bool artistEditable = true;
	string UIName = "Reflection Map";
	string UIDesc = "The reflection map for the material";
>;

float3 uTransform
<
	bool artistEditable = true;
	string UIName = "U Transform";
	string UIDesc = "The U-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0};

float3 vTransform
<
	bool artistEditable = true;
	string UIName = "V Transform";
	string UIDesc = "The V-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0};

float reflectionAmount
< 
	bool artistEditable = true;
	string UIName = "Reflection Amount";
	string UIDesc = "A scaling factor for the reflection";
	float UIMin = 0;
	float UIMax = 1.0;
	int UIDigits = 2;
> = 1.0;

BW_ARTIST_EDITABLE_FRESNEL
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_TEXTURE_OP
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

DECLARE_OTHER_MAP( reflectionMask, maskSampler, "Reflection Mask", "mask of the reflective area" )

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
sampler otherSampler = BW_SAMPLER(otherMap, BW_TEX_ADDRESS_MODE)

// Define the lighting type used in this shader
BW_DIFFUSE_LIGHTING
BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
VERTEX_FOG


#include "chrome.fxh"


OutputDiffuseLighting2 vs_main( VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	o.tc = i.tc;	
	
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	float3 eye;
	o.tc2 = chromeMapCoords( worldPos, worldNormal, wsCameraPos, uTransform, vTransform, eye );	

	return o;
}


OutputDiffuseLighting2 vs_mainStaticLighting( STATIC_LIGHTING_VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	o.tc = i.tc;	
	
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)	
	BW_VERTEX_LIGHTING( o, i.diffuse, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	float3 eye;	
	o.tc2 = chromeMapCoords( worldPos, worldNormal, wsCameraPos, uTransform, vTransform, eye );	

	return o;
}


OutputDiffuseLighting3 vs_mainFresnel( VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting3 o = (OutputDiffuseLighting3)0;

	PROJECT_POSITION( o.pos )
	o.tc = o.tc3 = i.tc;	
	
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	float3 eye;	
	o.tc2 = chromeMapCoords( worldPos, worldNormal, wsCameraPos, uTransform, vTransform, eye );
	
	o.diffuse.a = fresnel( -normalize(eye), normalize(worldNormal), fresnelExp, fresnelConstant ) * 2.0 * reflectionAmount;	

	return o;
}


OutputDiffuseLighting3 vs_mainStaticLightingFresnel( STATIC_LIGHTING_VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting3 o = (OutputDiffuseLighting3)0;

	PROJECT_POSITION( o.pos )
	o.tc = o.tc3 = i.tc;	
	
	BW_SKY_MAP_COORDS(o.skyLightMap, worldPos)	
	BW_VERTEX_LIGHTING( o, i.diffuse, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	float3 eye;
	o.tc2 = chromeMapCoords( worldPos, worldNormal, wsCameraPos, uTransform, vTransform, eye );
	
	o.diffuse.a = fresnel( -normalize(eye), normalize(worldNormal), fresnelExp, fresnelConstant ) * 2.0 * reflectionAmount;	

	return o;
}


//--------------------------------------------------------------//
// Pixel shader for shader 2
//--------------------------------------------------------------//
float4 ps_main( OutputDiffuseLighting3 input ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );
	float4 otherMap = tex2D( otherSampler, input.tc2 );
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );
	float alpha = diffuseMap.w * input.diffuse.w * input.diffuse.w;
	float reflectiveMask = tex2D( maskSampler, input.tc );
	float4 colour = (input.sunlight * shade + input.diffuse) * diffuseMap;
	colour.xyz = bwTextureOp( (textureOperation), colour.xyz, alpha, diffuseMap, otherMap * reflectiveMask );
	colour.w = diffuseMap.w;
	return colour;
}

#endif