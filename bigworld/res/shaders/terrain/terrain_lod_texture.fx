
#include "terrain_common.fxh"
#include "depth_helpers.fxh"

// 
bool hasHoles = false;

// We want to use a normal map
USE_TERRAIN_NORMAL_MAP

// We want to use a horizon map
USE_TERRAIN_HORIZON_MAP

// Lighting
BW_DIFFUSE_LIGHTING
BW_SPECULAR_LIGHTING
USE_TERRAIN_LIGHTING
BW_SKY_LIGHT_MAP
BW_FRESNEL
VERTEX_FOG

// Textures
USE_TERRAIN_BLEND_TEXTURE

// Tell shader to blend or not - there is no need to when terrain block is fully
// lod texture
bool useMultipassBlending = true;

// Need this for blending setup
BW_NON_EDITABLE_ALPHA_TEST

// Blend distances
float lodTextureStart 	= 200;	// this is where we start blending in lod texture
float lodTextureDistance= 100; 	// this is where lod texture is 100%

// The output from the vertex shader
struct TerrainVertexOutput
{
	//TODO: combine these interpolators............
	float4 position		: POSITION;
	float4 worldPosition: TEXCOORD0;
	float2 normalUV		: TEXCOORD1;
	float2 blendUV		: TEXCOORD2;
    float2 horizonUV    : TEXCOORD3;
	float2 skyLightUV	: TEXCOORD4;
	float4 alpha		: COLOR0;
	float3 diffuseLight	: COLOR1;		// used by 2.0 version for all non-primary lights
	float2 fogDepth		: TEXCOORD5;	// fogDepth.x = fog, fogDepth.y = zDepth
};

//----------------------------------------------------------------------------
// Vertex shader
//----------------------------------------------------------------------------

void vs_terrainCommon( in TerrainVertex vertex, inout TerrainVertexOutput oVertex)
{
	// Calculate the position of the vertex
	oVertex.worldPosition = terrainVertexPosition( vertex );
	oVertex.position = mul( oVertex.worldPosition, viewProj );
	
	// Calculate the texture coordinate for the normal map
	oVertex.normalUV = inclusiveTextureCoordinate( vertex, float2( normalMapSize, normalMapSize ) );
	// Calculate the texture coordinate for the horizon map
	oVertex.horizonUV = inclusiveTextureCoordinate( vertex, float2( horizonMapSize, horizonMapSize ) );
	
	// Calculate the texture coordinate for the blend map
	oVertex.blendUV = vertex.xz; 
	oVertex.blendUV.y = 1.0 - oVertex.blendUV.y;
	//inclusiveTextureCoordinate( vertex, float2( blendMapSize, blendMapSize ));
	
	// Calculate the texture coordinate for cloud shadows
	BW_SKY_MAP_COORDS( oVertex.skyLightUV, oVertex.worldPosition )
	
	// Do fogging
	oVertex.fogDepth.x = saturate(vertexFog( oVertex.position.w, fogStart, fogEnd ));
	BW_DEPTH(oVertex.fogDepth.y,oVertex.position.z)
  
	// Blend alpha in over a distance
	float len	= length(oVertex.worldPosition.xz - cameraPosition.xz );
	len 		= clamp( len, lodTextureStart, lodTextureStart + lodTextureDistance );
	float a		= abs(len - lodTextureStart) / lodTextureDistance;
	oVertex.alpha = float4( a, a, a, a );
};

void vs_terrainLighting( in TerrainVertex vertex, inout TerrainVertexOutput oVertex )
{
	float3 worldNormal= float3(0,1,0);
	float3 colour = float3(0,0,0);
	
	for ( int i = 1; i < nDirectionalLights; i++ )
	{
		colour += directionalLight( worldNormal, directionalLights[i] );
	}
	for ( int i = 0; i < nPointLights; i++ )
	{
		colour += pointLight( oVertex.worldPosition, worldNormal, pointLights[i] );
	}
	for ( int i = 0; i < nSpotLights; i++ )
	{
		colour += spotLight( oVertex.worldPosition, worldNormal, spotLights[i] );
	}
	
	oVertex.diffuseLight = colour;
};

//----------------------------------------------------------------------------
// Vertex shader 3.0 & 2.0
//----------------------------------------------------------------------------

TerrainVertexOutput vs30_terrain( in TerrainVertex vertex )
{
	TerrainVertexOutput output = (TerrainVertexOutput)0;
	
	vs_terrainCommon( vertex, output );

	return output;
}

TerrainVertexOutput vs20_terrain( in TerrainVertex vertex )
{
	TerrainVertexOutput output = (TerrainVertexOutput)0;
	
	vs_terrainCommon( vertex, output );
	vs_terrainLighting( vertex, output );
	
	return output;
}


BW_SKY_LIGHT_MAP_SAMPLER

//----------------------------------------------------------------------------
// Pixel shader
//----------------------------------------------------------------------------

#ifdef USE_MOTION_BLUR
float4x4 lastVP : LastViewProjection;
float4 calcVelocity( float4 worldPos, float4 currentPos )
{
	float4 lastPos = mul( worldPos, lastVP );
	
	// do divide by W -> NDC coordinates
	currentPos.xyz = currentPos.xyz / currentPos.w;
	lastPos.xyz = lastPos.xyz / lastPos.w;
	float3 dP = (currentPos.xyz - lastPos.xyz);
	dP *= 0.5;
	return float4(dP.xyz, 1);
}
#endif //USE_MOTION_BLUR

/*
 * Shader body
 */
BW_COLOUR_OUT ps30_terrain( const TerrainVertexOutput terrainFragment, uniform bool simpleLighting  )
{
	// Get the diffuse colour
	float4 diffuseColour = tex2D(blendMapSampler, terrainFragment.blendUV);

	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV);

	// Calculate the horizon shadows
    float2 angles = tex2D( horizonMapSampler, terrainFragment.horizonUV ).xy;
	float2 angleDiff = (sunAngle - angles) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	float shade = angleDiff.x * angleDiff.y;

	// Calculate the cloud shadows
	float skyMap = BW_SAMPLE_SKY_MAP(terrainFragment.skyLightUV)
	shade = min( shade, skyMap );

	// Calculate the diffuse lighting
	float3 diffuse;
	if (simpleLighting)
	{
		diffuse = diffuseLightingSimple( terrainFragment.worldPosition, normal, shade );
	}
	else
	{
		diffuse = diffuseLighting( terrainFragment.worldPosition, normal, shade );
	}

	// Calculate specular lighting
	float3 eye		= normalize( cameraPosition - terrainFragment.worldPosition );
	float3 specular;
	if (simpleLighting)
	{
		specular = specularLightingSimple( terrainFragment.worldPosition, normal, eye, shade, 
											terrain2Specular.power, terrain2Specular.multiplier );
	}
	else
	{
		specular = specularLighting( terrainFragment.worldPosition, normal, eye, shade, 
											terrain2Specular.power, terrain2Specular.multiplier );
	}
	
	specular *= lerp( diffuseColour.xyz * diffuseColour.w, diffuseColour.w, specularBlend ) ;    

	// fresnel term
	float fres = fresnel( eye, normal, terrain2Specular.fresnelExp, terrain2Specular.fresnelConstant );
	specular *= fres;

	float4 colour = diffuseColour;
	colour.rgb	= (ambientColour.rgb + diffuse) * diffuseColour + specular;

	// Apply fog
	colour.rgb	= lerp( fogColour.rgb, colour.rgb, terrainFragment.fogDepth.x );

	// Blend alpha
	colour.a 	= terrainFragment.alpha.a;

#ifdef USE_MOTION_BLUR
	float2 motion;
	float4 wPos = terrainFragment.worldPosition;
	float4 oPos = mul( wPos, viewProj );
	float4 velocity = calcVelocity( wPos, oPos );
	motion = velocity.xy;

	BW_FINAL_COLOUR2( terrainFragment.fogDepth.y, colour, motion )
#else
	BW_FINAL_COLOUR( terrainFragment.fogDepth.y, colour )
#endif
};

//----------------------------------------------------------------------------
// Pixel shader 2.0 - currently 4/32 texture, 48/64 arithmetic instructions.
//----------------------------------------------------------------------------
BW_COLOUR_OUT ps20_terrain( const TerrainVertexOutput terrainFragment )
{
	// Get the diffuse colour
	float4 diffuseColour = tex2D( blendMapSampler, terrainFragment.blendUV );
	
	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalUV );
	
	// Calculate the horizon shadows
    float2 angles = tex2D( horizonMapSampler, terrainFragment.horizonUV ).xy;
	float2 angleDiff = (sunAngle - angles) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	float shade = angleDiff.x * angleDiff.y;

	// Calculate the cloud shadows
	float skyMap = BW_SAMPLE_SKY_MAP( terrainFragment.skyLightUV );
	shade = min( shade, skyMap );
	
	// Calculate diffuse lighting component, only using primary light source
	float3 diffuse = directionalLight( normal, directionalLights[0] ) * shade;
	
	// Add per vertex diffuse light
	diffuse += terrainFragment.diffuseLight;
	
	// Calculate specular lighting component
	float3 eye		= normalize( cameraPosition - terrainFragment.worldPosition );
	float3 specular = terrainDirectionalSpecLight( normal, eye, specularDirectionalLights[0], 
									terrain2Specular.power, terrain2Specular.multiplier );
	specular *= shade;									
	specular *= lerp( diffuseColour.xyz * diffuseColour.w, diffuseColour.w, specularBlend ) ;    
										
	// fresnel term - PS2.0 version has no constant term due to instruction limitations.

	float fres = fresnelNoConstant( eye, normal, terrain2Specular.fresnelExp );	
	specular *= fres;

	// (almost) final colour

	float4 colour = float4(0,0,0,1);
	colour.rgb = (ambientColour.rgb + diffuse) * diffuseColour + specular;
	
	// apply fog
	colour.rgb = lerp( fogColour.rgb, colour.rgb, terrainFragment.fogDepth.x );

	// Blend alpha
	colour.a 	= terrainFragment.alpha.a;

#ifdef USE_MOTION_BLUR
	float2 motion;
	float4 wPos = terrainFragment.worldPosition;
	float4 oPos = mul( wPos, viewProj );
	float4 velocity = calcVelocity( wPos, oPos );
	motion = velocity.xy;
	BW_FINAL_COLOUR2( terrainFragment.fogDepth.y, colour, motion )
#else
	BW_FINAL_COLOUR( terrainFragment.fogDepth.y, colour )
#endif
};


PixelShader ps3_shaders[2] = {
	compile ps_3_0 ps30_terrain(true),
	compile ps_3_0 ps30_terrain(false)
};

int ps_30_selector()
{
	int ret = 0;
	if (nDirectionalLights != 1 ||
		nPointLights > 0 ||
		nSpotLights > 0 ||
		nSpecularDirectionalLights != 1 ||
		nSpecularPointLights > 0)
		ret = 1;
	return ret;
}

//--------------------------------------------------------------//
// Technique Sections for shader model 3
//--------------------------------------------------------------//
technique four_layer_shader_3_0
{
	pass lighting_only_pass
	{
        ALPHABLENDENABLE = (useMultipassBlending ? 1 : 0);
        SRCBLEND = SRCALPHA;
        DESTBLEND = INVSRCALPHA;
        ALPHATESTENABLE = FALSE;
        ZWRITEENABLE = (hasHoles ? 0 : 1);
        ZFUNC = (hasHoles ? BW_CMP_EQUAL : BW_CMP_LESSEQUAL);
        ZENABLE = TRUE;
		CULLMODE = BW_CULL_CCW;
        
        VertexShader = compile vs_3_0 vs30_terrain();
		PixelShader = (ps3_shaders[(ps_30_selector())]);
	}
}


//--------------------------------------------------------------//
// Technique Sections for shader model 2
//--------------------------------------------------------------//
technique four_layer_shader_2_0
{
	pass lighting_only_pass
	{
        ALPHABLENDENABLE = (useMultipassBlending ? 1 : 0);
        SRCBLEND = SRCALPHA;
        DESTBLEND = INVSRCALPHA;
        ALPHATESTENABLE = FALSE;
        ZWRITEENABLE = (hasHoles ? 0 : 1);
        ZFUNC = (hasHoles ? BW_CMP_EQUAL : BW_CMP_LESSEQUAL);
        ZENABLE = TRUE;
		CULLMODE = BW_CULL_CCW;
        
        VertexShader = compile vs_2_0 vs20_terrain();
		PixelShader = compile ps_2_0 ps20_terrain();
	}
}

