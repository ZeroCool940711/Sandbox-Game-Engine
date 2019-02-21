#ifndef UNSKINNED_PROJECTION_HELPERS_FXH
#define UNSKINNED_PROJECTION_HELPERS_FXH

float3 transformPos( float4x4 world, float4 pos )
{
	float3 ret = {	dot( world[0], pos ),
					dot( world[1], pos ),
					dot( world[2], pos ) };
	return ret;
}

float3 transformNormaliseVector( float4x4 world, float3 v )
{
	float3 ret;
	ret = mul( v, world );	
	return normalize( ret );
}

float3x3 worldSpaceTSMatrix( in float4x4 world, in float3 tangent, in float3 binormal, in float3 worldNormal )
{	
	float3 worldTangent = transformNormaliseVector( world, tangent);
	float3 worldBinormal = transformNormaliseVector( world, binormal );
	
	float3x3 tsMatrix = { worldTangent, worldBinormal, worldNormal };
	return tsMatrix;
}

float4 unskinnedTransform( in float4 pos, in float3 normal, in float4x4 world, in float4x4 viewProj, out float4 worldPos, out float3 worldNormal )
{	
	worldPos = mul(pos,world);
	worldNormal = transformNormaliseVector( world, normal );
	
	float4 projPos = mul(worldPos, viewProj);
	return projPos;
}

float4x4 world : World;
float4x4 viewProj : ViewProjection;
bool staticLighting : StaticLighting = false;

#define PROJECT_POSITION( o )\
float4 worldPos;\
float3 worldNormal;\
o = unskinnedTransform(i.pos, i.normal, world, viewProj, worldPos, worldNormal);

#define PROJECT_VECTOR3( ret, v )\
ret.x = dot( world[0].xyz, v  );\
ret.y = dot( world[1].xyz, v );\
ret.z = dot( world[2].xyz, v );

#define CALCULATE_TS_MATRIX\
float3x3 tsMatrix = worldSpaceTSMatrix(world, i.tangent, i.binormal, worldNormal);

#define VERTEX_FORMAT							VertexXYZNUV
#define STATIC_LIGHTING_VERTEX_FORMAT			VertexXYZNUV_D
#define BUMPED_VERTEX_FORMAT					VertexXYZNUVTB
#define BUMPED_STATIC_LIGHTING_VERTEX_FORMAT	VertexXYZNUVTB_D

#endif