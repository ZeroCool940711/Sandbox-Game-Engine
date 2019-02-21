#include "shader_combination_helpers.fxh"

// Exposed artist editable variables.
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)
BW_ARTIST_EDITABLE_DIFFUSE_MAP
DECLARE_OTHER_MAP( glowMap, glowSampler, "glow map", "The glow map for the material." )
BW_ARTIST_EDITABLE_GLOW_FACTOR
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_MOD2X

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)

// Define the lighting type used in this shader
BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
BW_DIFFUSE_LIGHTING
VERTEX_FOG

#include "shader_combination_helpers.fxh"

OutputDiffuseLighting2 vs_main( VERTEX_FORMAT i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	o.tc = o.tc2 = i.tc;	
	
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );

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
	o.tc = o.tc2 = i.tc;
	
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	BW_VERTEX_LIGHTING( o, i.diffuse, selfIllumination, worldPos, worldNormal, combineLights )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );

	return o;
}


//--------------------------------------------------------------//
// Pixel shader for shader 2
//--------------------------------------------------------------//

float4 ps_main( OutputDiffuseLighting2 input ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );	
	float4 glowMap = tex2D( glowSampler, input.tc2 );
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap );
	float4 colour;
	colour = (input.sunlight * shade + input.diffuse) * diffuseMap + (glowMap * (glowFactor));
	colour.w = diffuseMap.w;
	return colour;
}
