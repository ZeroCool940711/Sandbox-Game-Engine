#define ADDITIVE_EFFECT 1
#include "stdinclude.fxh"

// Auto variables
float4x4 viewProj : ViewProjection;
float4x4 world : World;
float4x4 view : View;
float3 cameraPos : CameraPos;

// Exposed artist editable variables.

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_ADDITIVE_BLEND
BW_NON_EDITABLE_ADDRESS_MODE(BW_WRAP)

bool lightEnable = false;

#ifdef IN_GAME

VERTEX_FOG

OutputDiffuseLighting vs_main( VertexXYZNUV input)
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;
	
	float4x4 w = world;
	
	float3 up = w[1].xyz;
	float3 ahead = normalize(w[3].xyz - cameraPos);
	float3 side = normalize(cross( up, ahead));
	ahead = normalize(cross( side, up ));
	
	float aheadScale = length(w[2].xyz);
	float sideScale = length(w[0].xyz);
	
	float4x4 newWorld = float4x4( float4(side * sideScale,0), float4(up,0), float4(ahead * aheadScale,0), float4(w[3].xyz,1) );

	float3 worldPos = mul( input.pos, newWorld );
	
	
	o.pos = mul(float4(worldPos,1), viewProj);
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	o.tcDepth.xy = input.tc;
	
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )
	
	o.sunlight = float4(1,1,1,1);	

	return o;
}

//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique standard
<
	string channel = "sorted";
	string label = "SHADER_MODEL_0";
>
{
	pass Pass_0
	{
		BW_BLENDING_ADD
		BW_FOG_ADD
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		ADDRESSU[0] = CLAMP;
		ADDRESSV[0] = CLAMP;
		ADDRESSW[0] = CLAMP;
		
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = NULL;
	}
}

#else

float4x4 viewInverse : ViewI;
float4x4 projection : Projection;

OutputDiffuseLighting vs_max( VertexXYZNUV input )
{
	OutputDiffuseLighting o = (OutputDiffuseLighting)0;
	
	float4x4 w = world;
	
	float3 up = w[2].xyz;
	float3 ahead = normalize(w[3].xyz - viewInverse[3].xyz);
	float3 side = normalize(cross( ahead, up));
	ahead = normalize(cross( up, side ));
	
	float aheadScale = length(w[2].xyz);
	float sideScale = length(w[0].xyz);
	
	float4x4 newWorld = float4x4( float4(side * sideScale,0), float4(ahead * aheadScale,0), float4(up,0), float4(w[3].xyz,1) );

	float3 pos = mul( input.pos, newWorld );
	
	
	o.pos = mul(float4(pos,1), mul(view, projection));
	o.tcDepth.xy = input.tc;
	
	o.diffuse = float4(1,1,1,1);

	return o;
}

technique max_preview
{
	pass max_pass
	{
		BW_BLENDING_ADD		
		BW_TEXTURESTAGE_DIFFUSEONLY(0, diffuseMap)
		BW_TEXTURESTAGE_TERMINATE(1)
		BW_CULL_DOUBLESIDED
		VertexShader = compile vs_2_0 vs_max();
		PixelShader = NULL;
	}
}

#endif

#undef ADDITIVE_EFFECT