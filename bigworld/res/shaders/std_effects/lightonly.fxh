#ifndef LIGHTONLY_FXH
#define LIGHTONLY_FXH

#include "shader_combination_helpers.fxh"
#include "depth_helpers.fxh"

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)

BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
BW_DIFFUSE_LIGHTING
VERTEX_FOG


OutputDiffuseLighting vs_main( VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;

	PROJECT_POSITION( o.pos )
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )	

	return o;
}


OutputDiffuseLighting vs_mainStaticLighting( STATIC_LIGHTING_VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;

	PROJECT_POSITION( o.pos )
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );	
	// Pass in static lighting as self illumination as ambient gets affected by cloud shadows
	BW_VERTEX_LIGHTING( o, float4(0,0,0,0), i.diffuse + selfIllumination, worldPos, worldNormal, combineLights )	

	return o;
}


BW_COLOUR_OUT ps_main_2_0( OutputDiffuseLighting input )
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tcDepth.xy );	
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );	
	float4 colour;
#ifdef MOD2X
	colour = 2.0 * (input.sunlight * shade + input.diffuse) * diffuseMap;
#else
	colour = (input.sunlight * shade + input.diffuse) * diffuseMap;
#endif
	colour.w = diffuseMap.w;

#ifdef USE_MOTION_BLUR
	float2 motion;
	float4 objPos = input.currentPos;
	float4 oPos = mul(objPos,WVP);
	float4 velocity = calcVelocity( objPos, oPos );
	motion = velocity.xy;
	BW_FINAL_COLOUR2( input.tcDepth.z, colour, motion )
#else
	BW_FINAL_COLOUR( input.tcDepth.z, colour )
#endif
}


float4 ps_main_1_1( OutputDiffuseLighting input ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tcDepth.xy );	
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );	
	float4 colour;
#ifdef MOD2X
	colour = 2.0 * (input.sunlight * shade + input.diffuse) * diffuseMap;
#else
	colour = (input.sunlight * shade + input.diffuse) * diffuseMap;
#endif
	colour.w = diffuseMap.w;
	return colour;
}

#endif