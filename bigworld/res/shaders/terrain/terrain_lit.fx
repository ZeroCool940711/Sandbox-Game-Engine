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
TERRAIN_TEXTURE( layer0 )
TERRAIN_TEXTURE( layer1 )
TERRAIN_TEXTURE( layer2 )
TERRAIN_TEXTURE( layer3 )

bool useMultipassBlending = false;

// Need this for blending setup
BW_NON_EDITABLE_ALPHA_TEST

// The mask used to set which layers are in use
float4 layerMask = float4( 1, 1, 1, 1 );

// The output from the vertex shader. Note UV's are packed into semantics to 
// allow Shader 2.0 versions to work (otherwise we use too many TEXCOORD's).
struct TerrainVertexOutput
{
	float4 position			: POSITION;
	float4 worldPosition	: TEXCOORD0;
	float4 normalBlendUV	: TEXCOORD1;	// normal.uv = xy, blend.uv = zw
    float4 horizonSkyLightUV: TEXCOORD2;	// horizon.uv = xy,skyLight.uv = zw
	float4 layer01UV		: TEXCOORD3;	// layer0.uv = xy, layer1.uv = zw
	float4 layer23UV		: TEXCOORD4;	// layer2.uv = xy, layer3.uv = zw
	float3 diffuseLight		: COLOR;		// used by 2.0 version for all non-primary lights
	float2 fogDepth			: TEXCOORD5;	// fogDepth.x = fog, fogDepth.y = zDepth
};

//----------------------------------------------------------------------------
// Helper methods
//----------------------------------------------------------------------------
float4 getBlendedColour( const TerrainVertexOutput fragment, out float blendSum )
{
	float4 blend = tex2D( blendMapSampler, fragment.normalBlendUV.zw );
	blend *= layerMask;
	float4 ret = tex2D( layer0Sampler, fragment.layer01UV.xy ) * blend.x;
	ret += tex2D( layer1Sampler, fragment.layer01UV.zw ) * blend.y;
	ret += tex2D( layer2Sampler, fragment.layer23UV.xy ) * blend.z;
	ret += tex2D( layer3Sampler, fragment.layer23UV.zw ) * blend.w;
	blendSum = dot( blend, layerMask ); 
    
	return ret;
}

BW_SKY_LIGHT_MAP_SAMPLER

void vs_terrainCommon( in TerrainVertex vertex, inout TerrainVertexOutput oVertex )
{	
	// Calculate the position of the vertex
	oVertex.worldPosition = terrainVertexPosition( vertex );
	oVertex.position = mul( oVertex.worldPosition, viewProj );
	
	// Calculate the texture coordinate for the normal map
	oVertex.normalBlendUV.xy = inclusiveTextureCoordinate( vertex, float2( normalMapSize, normalMapSize ) );
	
	// Calculate the texture coordinate for the blend map
	oVertex.normalBlendUV.zw = inclusiveTextureCoordinate( vertex, float2( blendMapSize, blendMapSize ));
	
	// Calculate the texture coordinates for our texture layers
	oVertex.layer01UV.xy = float2( dot( layer0UProjection, oVertex.worldPosition ),
							dot( layer0VProjection, oVertex.worldPosition ) );
	oVertex.layer01UV.zw = float2( dot( layer1UProjection, oVertex.worldPosition ),
							dot( layer1VProjection, oVertex.worldPosition ) );
	oVertex.layer23UV.xy = float2( dot( layer2UProjection, oVertex.worldPosition ),
							dot( layer2VProjection, oVertex.worldPosition ) );
	oVertex.layer23UV.zw = float2( dot( layer3UProjection, oVertex.worldPosition ),
							dot( layer3VProjection, oVertex.worldPosition ) );
	
	// Calculate the texture coordinate for the horizon map
	oVertex.horizonSkyLightUV.xy = inclusiveTextureCoordinate( vertex, float2( horizonMapSize, horizonMapSize ) );
		
	// Calculate the texture coordinate for cloud shadows
	BW_SKY_MAP_COORDS( oVertex.horizonSkyLightUV.zw, oVertex.worldPosition )
	
	// Do fogging
	oVertex.fogDepth.x = saturate(vertexFog( oVertex.position.w, fogStart, fogEnd ));
#if USE_MRT_DEPTH
	//TODO: disable the depth output on secondary passes
	if (useMultipassBlending)
		oVertex.fogDepth.y = 0.f;
	else
		BW_DEPTH(oVertex.fogDepth.y, oVertex.position.z)
#endif
}

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

//----------------------------------------------------------------------------
// Pixel shader 3.0
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
#endif

// Shader body
BW_COLOUR_OUT ps30_terrain( const TerrainVertexOutput terrainFragment, uniform bool simpleLighting )
{
    float blendSum;

	// Get the blended diffuse colour
	float4 diffuseColour = getBlendedColour( terrainFragment, blendSum );
	
	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalBlendUV.xy);
	
	// Calculate the horizon shadows
    float2 angles = tex2D( horizonMapSampler, terrainFragment.horizonSkyLightUV.xy ).xy;
	float2 angleDiff = (sunAngle - angles) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	float shade = angleDiff.x * angleDiff.y;
	
	// Calculate the cloud shadows
	float skyMap = BW_SAMPLE_SKY_MAP( terrainFragment.horizonSkyLightUV.zw );
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
	colour.xyz = (ambientColour.xyz + diffuse) * diffuseColour + specular;
    
	// Apply fog
	colour.xyz = lerp( fogColour.xyz * blendSum, colour.xyz, terrainFragment.fogDepth.x );
	
	// debug
//	colour.xyz = normal.xzy * 0.5 + float3(0.5, 0.5, 0.5);
    
//	float index = saturate(normal.y) * 2.0;
//  if (index > 1.0)
//  {
//      colour.xyz = lerp( float3(1, 0, 0), float3(0, 1, 0), index - 1.0 );
//  }
//  else
//  {
//      colour.xyz = lerp( float3(0, 0, 1), float3(1, 0, 0), index );
//  }

//  float distance = length( terrainFragment.worldPosition.xz - cameraPosition.xz );
//	float lod = saturate( (distance - lodStart) / (lodEnd - lodStart) );
//  colour.xyz = float3( lod, lod, lod );

#ifdef USE_MOTION_BLUR
	float2 motion = float2(0.5,0.5);
	if (!useMultipassBlending)
	{
		//float4 objPos = terrainFragment.worldPosition;
		float4 wPos = terrainFragment.worldPosition;
		float4 oPos = mul( wPos, viewProj );
		float4 velocity = calcVelocity( wPos, oPos );
		motion = velocity.xy;
	}
	BW_FINAL_COLOUR2( terrainFragment.fogDepth.y, colour, motion )
#else
	BW_FINAL_COLOUR( terrainFragment.fogDepth.y, colour )
#endif //USE_MOTION_BLUR
};

//----------------------------------------------------------------------------
// Pixel shader 2.0 - currently 8/32 texture, 64/64 arithmetic instructions.
//----------------------------------------------------------------------------
//SM2.0 doesn't support MRT due to instruction number limitation. Otherwise, this FX file can't be compiled under PC with SM3.0 card
float4 ps20_terrain( const TerrainVertexOutput terrainFragment ) :COLOR0
{
	float4 blendSum;

	// Get the blended diffuse colour
	float4 diffuseColour = getBlendedColour( terrainFragment, blendSum );
	
	// Get the normal from the terrain normal map
	float3 normal = terrainNormal( normalMapSampler, terrainFragment.normalBlendUV.xy );
	
	// Calculate the horizon shadows
    float2 angles = tex2D( horizonMapSampler, terrainFragment.horizonSkyLightUV.xy ).xy;
	float2 angleDiff = (sunAngle - angles) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	float shade = angleDiff.x * angleDiff.y;

	// Calculate the cloud shadows
	float skyMap = BW_SAMPLE_SKY_MAP( terrainFragment.horizonSkyLightUV.zw );
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
	colour.rgb = lerp( fogColour.rgb * blendSum, colour.rgb, terrainFragment.fogDepth.x );

	return colour;
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
<
	string label = "SHADER_MODEL_3";
>
{
	pass lighting_only_pass
	{
        // Turn on alpha blend and set dest blend to 1 if using
        // multiple passes
        ALPHABLENDENABLE = (useMultipassBlending ? 1 : 0);
        SRCBLEND = ONE;
        DESTBLEND = (useMultipassBlending ? BW_BLEND_ONE : BW_BLEND_ZERO);
        ALPHATESTENABLE = FALSE;
        ZWRITEENABLE = (hasHoles || useMultipassBlending ? 0 : 1);
        ZFUNC = (hasHoles || useMultipassBlending ? BW_CMP_EQUAL : BW_CMP_LESSEQUAL);
        ZENABLE = TRUE;
		CULLMODE = BW_CULL_CCW;
        
        VertexShader = compile vs_3_0 vs30_terrain();
		PixelShader = (ps3_shaders[(ps_30_selector())]);
	}
}


//--------------------------------------------------------------//
// Technique Sections for shader model 2_0
//--------------------------------------------------------------//
technique four_layer_shader_2_0
<
	string label = "SHADER_MODEL_2";
>
{
	pass lighting_only_pass
	{
        // Turn on alpha blend and set dest blend to 1 if using
        // multiple passes
        ALPHABLENDENABLE = (useMultipassBlending ? 1 : 0);
        SRCBLEND = ONE;
        DESTBLEND = (useMultipassBlending ? BW_BLEND_ONE : BW_BLEND_ZERO);
        ALPHATESTENABLE = FALSE;
        ZWRITEENABLE = (hasHoles || useMultipassBlending ? 0 : 1);
        ZFUNC = (hasHoles || useMultipassBlending ? BW_CMP_EQUAL : BW_CMP_LESSEQUAL);
        ZENABLE = TRUE;
		CULLMODE = BW_CULL_CCW;
        FOGENABLE = FALSE;
		
        VertexShader = compile vs_2_0 vs20_terrain(); // re-use vertex shader for both profiles
		PixelShader = compile ps_2_0 ps20_terrain();
	}
}