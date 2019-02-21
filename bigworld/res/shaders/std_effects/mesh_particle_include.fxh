#ifndef MESH_PARTICLE_INCLUDE_FXH
#define MESH_PARTICLE_INCLUDE_FXH

#include "depth_helpers.fxh"

BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)

BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER
BW_DIFFUSE_LIGHTING
VERTEX_FOG

float4x4 viewProj : ViewProjection;
float4 world[45] : WorldPalette;		//15 is MAX_MESHES, and matrices are 3 float4s
float4 tint[15] : TintPalette;


//normal -> world normal and 1-bone skinning
float3 transformNormaliseVector( float4 world[45], float3 v, int index )
{
	float3 ret;
	ret.x = dot( world[index + 0].xyz, v  );
	ret.y = dot( world[index + 1].xyz, v );
	ret.z = dot( world[index + 2].xyz, v );
	return normalize( ret );
}

//pos -> world pos and 1-bone skinning
float3 transformPos( float4 world[45], float4 pos, int index )
{
	float3 ret = {	dot( world[index], pos ),
					dot( world[index + 1], pos ),
					dot( world[index + 2], pos ) };
	return ret;
}


float4 meshParticleTransform( in float4 pos, in float3 normal, in int index, float4 world[45], float4x4 viewProj, out float4 worldPos, out float3 worldNormal )
{
	worldPos =  float4( transformPos(world,pos,index), 1 );
	worldNormal = transformNormaliseVector( world, normal, index );
	
	float4 projPos = mul(worldPos, viewProj);
	return projPos;	
}


#define PROJECT_POSITION( o )\
float3 worldPos, worldNormal;\
o.pos = meshParticleTransform(i.pos, i.normal, i.index, world, viewProj, worldPos, worldNormal);


OutputDiffuseLighting vs_main( VertexXYZNUVI i, 
	uniform int nDirectionals, 
	uniform int nPoints, 
	uniform int nSpots,
	uniform bool combineLights )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;

	PROJECT_POSITION(o)
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	o.tcDepth.xy = i.tc;
	BW_DEPTH(o.tcDepth.z, o.pos.z)
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )	
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, combineLights );	
	o.sunlight *= tint[i.index/3];
	o.diffuse *= tint[i.index/3];

	return o;
}


BW_COLOUR_OUT ps_main( OutputDiffuseLighting input )
{
	float4 diffuseMap = tex2D( diffuseSampler, input.tcDepth.xy );	
	float shade = BW_SAMPLE_SKY_MAP( input.skyLightMap )	
	float4 colour;
	colour = (input.sunlight * shade + input.diffuse) * diffuseMap;
	colour.w = diffuseMap.w * input.sunlight.w;
	BW_FINAL_COLOUR( input.tcDepth.z, colour )
}


#endif