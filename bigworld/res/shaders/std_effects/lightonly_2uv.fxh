#ifndef LIGHTONLY_2UV_FXH
#define LIGHTONLY_2UV_FXH

#include "shader_combination_helpers.fxh"

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_TEXTURE_OP

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)

BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
BW_DIFFUSE_LIGHTING
VERTEX_FOG


OutputDiffuseLighting2 vs_main( VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	CALCULATE_UVS( o, i.tc, worldPos, worldNormal )
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )	

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
	CALCULATE_UVS( o, i.tc, worldPos, worldNormal )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	BW_VERTEX_LIGHTING( o, i.diffuse, selfIllumination, worldPos, worldNormal, combineLights )	

	return o;
}


//--------------------------------------------------------------//
// Pixel shader for 2 sets of uvs
//--------------------------------------------------------------//
float4 ps_main( OutputDiffuseLighting2 input, uniform sampler otherSampler ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );	
	float4 otherMap = tex2D( otherSampler, input.tc2 );
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );	
	float4 colour;
	colour = (input.sunlight * shade + input.diffuse) * diffuseMap;
	colour.xyz = bwTextureOp( (textureOperation), colour.xyz, diffuseMap.w, diffuseMap, otherMap );
	colour.w = diffuseMap.w;
	return colour;
}

#endif