// sub surface scattering!
#include "depth_helpers.fxh"

int graphicsSetting
<
	string label = "SKIN_RENDERING";
	string desc = "Skin Rendering";
>;

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_NORMAL_MAP
BW_ARTIST_EDITABLE_SPEC_MAP

//This changes the definition of some pixel shaders along the way...
#define USES_SPEC_MAP

texture subSurfaceMap
<
	bool artistEditable = true;
	string UIName = "Sub Surface Map";
	string UIDesc = "The sub-surface map for the material";
>;

float subSurfaceBlendPower
<
	string UIName = "Sub Surface Blend Power";
	string UIDesc = "The sub-surface blend power for the material";
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 1;
	bool artistEditable = true;
> = 1;

BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
float rimWidth 
< 
	string UIName = "Rim Width";
	bool artistEditable = true;
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 2; 
> = 0.5;

float rimStrength 
< 
	string UIName = "Rim Strength";
	bool artistEditable = true;
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 2; 
> = 0.2;

sampler diffuseSampler = BW_SAMPLER( diffuseMap, BW_TEX_ADDRESS_MODE )
sampler specularSampler = BW_SAMPLER( specularMap, BW_TEX_ADDRESS_MODE )
sampler normalSampler = BW_SAMPLER( normalMap, BW_TEX_ADDRESS_MODE )
sampler subSurfaceSampler = BW_SAMPLER( subSurfaceMap, BW_TEX_ADDRESS_MODE )

// Zero and one so we can saturate without saturate().
// See bug 22877
float zero = 0.0;
float one = 1.0;

BW_DIFFUSE_LIGHTING
BW_SPECULAR_LIGHTING
BW_SKY_LIGHT_MAP
VERTEX_FOG
BW_SKY_LIGHT_MAP_SAMPLER

struct Output
{
	float4 pos : POSITION;
	float4 worldPos : TEXCOORD0;
	float2 tc : TEXCOORD1;
	float3 tangent : TEXCOORD2;
	float3 binormal : TEXCOORD3;
	float3 normal : TEXCOORD4;
	float2 skyLightMap: TEXCOORD5;
	float fog : FOG;
};

#ifdef IN_GAME
Output vs_main_3_0( BUMPED_VERTEX_FORMAT i )
{
	Output o = (Output)0;
	
	PROJECT_POSITION( o.pos )
	o.tc = i.tc;
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )		
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);	
	o.worldPos.xyz = worldPos;
	BW_DEPTH(o.worldPos.w, o.pos.z)
	CALCULATE_TS_MATRIX	
	o.tangent = tsMatrix[0];
	o.binormal = tsMatrix[1];
	o.normal = worldNormal;
	
	return o;
}


BW_COLOUR_OUT ps_main_3_0( Output input )
{
	// Grab our maps
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, input.tc, input.tc );
#else
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );
#endif
	float3 specularMap = tex2D( specularSampler, input.tc );
	float3 normalMap = (tex2D( normalSampler, input.tc ).xyz * 2 ) - 1;
	float4 subSurfaceMap = tex2D( subSurfaceSampler, input.tc );
	float skyMap = BW_SAMPLE_SKY_MAP(input.skyLightMap)

	// construct texture space matrix and transform normal map normal to 
	// world space
	float3x3 tsMatrix = { input.tangent, input.binormal, input.normal };
	float3 normal = normalize( mul( normalMap, tsMatrix ) );
	
	// Lighting section
	
	// Start with the ambient colour
	float3 diffuse = ambientColour;
	
	// Do diffuse lighting
	if (nDirectionalLights > 0)	
	{
		diffuse += directionalLight( normal, directionalLights[0] ) * skyMap;
	}
	
	for (int i = 1; i < nDirectionalLights; i++)
	{
		diffuse += directionalLight( normal, directionalLights[i] );
	}

	for (int i = 0; i < nPointLights; i++)
	{
		diffuse += pointLight( input.worldPos.xyz, normal, pointLights[i] );
	}
	
	
	// grab the eye vector
	float3 eye = normalize(wsCameraPos - input.worldPos.xyz);
	
	// calculate specular lights
	float3 specular = float3(0,0,0);
	
	if (nSpecularDirectionalLights > 0)
	{
		specular += directionalSpecLight( normal, eye, specularDirectionalLights[0] ) * skyMap;
	}
	
	for (int i = 1; i < nSpecularDirectionalLights; i++)
	{
		specular += directionalSpecLight( normal, eye, specularDirectionalLights[i] );
	}

	for (int i = 0; i < nSpecularPointLights; i++)
	{
		specular += pointSpecLight( input.worldPos.xyz, normal, eye, specularPointLights[i] );
	}

	// Include spot lights	
	for (int i = 0; i < nSpotLights; i++)
	{
		float3 diff = float3(0,0,0);
		float3 spec = float3(0,0,0);
		spotLightBump(input.worldPos.xyz, normal, eye, spotLights[i], diff, spec);
		diffuse += diff;
		specular += spec;
	}
	
	// do the rim lighting
	half rimLight;
	if (nDirectionalLights > 0)
	{		
		half3 rLight = directionalLights[0].direction;
		rLight = -rLight;
		
		half attenuation = saturate(dot(rLight, normal ));
		rimLight = 1.0 - saturate( dot( eye, normal ) );
		rimLight = rimStrength * smoothstep( 1-rimWidth, 1, rimLight );
		rimLight = saturate( rimLight * attenuation );
	}
	else
	{
		rimLight = 0.0;
	}

	// calculate the proportien of diffuse to sub surface map to blend together and mul by the diffuse colour.
	float diffuseAmount = 1 - ( 1 - pow(saturate( dot(normal, eye) ), subSurfaceBlendPower ) ) * subSurfaceMap.w;
	diffuse = (diffuseMap.xyz * diffuseAmount + subSurfaceMap * (1-diffuseAmount)) * saturate( diffuse );
	
	// Calculate result colour
	float4 colour;
	colour.xyz = diffuse + (diffuseMap.xyz*rimLight) + saturate(specular) * specularMap;
#ifdef MOD2X
	colour.xyz *= (1 + diffuseLightExtraModulation);
#endif
	colour.w = diffuseMap.w;

	// Avoid using saturate as this generate broken code in shader model 3 (bug 22877)
//	colour.xyz = lerp( fogColour.xyz, colour.xyz, saturate(input.fog) );
	float fogSat = max( zero, input.fog );
	fogSat = min( one, fogSat );
	colour.xyz = lerp( fogColour.xyz, colour.xyz, fogSat );

	BW_FINAL_COLOUR( input.worldPos.w, colour )	
}


struct Output_2_0
{
	float4 pos : POSITION;
	float4 worldPos : TEXCOORD0;
	float2 tc : TEXCOORD1;
	float3 tangent : TEXCOORD2;
	float3 binormal : TEXCOORD3;
	float3 normal : TEXCOORD4;
	float2 skyLightMap: TEXCOORD5;
	float3 diffuse : COLOR;
	float fog : FOG;
};

Output_2_0 vs_main_2_0( BUMPED_VERTEX_FORMAT i,
					uniform int nDirectionals,
					uniform int nPoints )
{
	Output_2_0 o = (Output_2_0)0;
	
	PROJECT_POSITION( o.pos )
	o.tc = i.tc;
	BW_SKY_MAP_COORDS( o.skyLightMap, worldPos )		
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);	
	o.worldPos.xyz = worldPos;		
	CALCULATE_TS_MATRIX	
	o.tangent = tsMatrix[0];
	o.binormal = tsMatrix[1];
	o.normal = worldNormal;
	
	o.diffuse = ambientColour;
	
	int startDir = nDirectionals > 0 ? 1 : 0;

	for (int i = startDir; i < nDirectionals; i++)
	{
		o.diffuse += directionalLight( o.normal, directionalLights[i] );
	}

	int startPoint = nDirectionals > 0 ? 0 : 1;
	for (int i = startPoint; i < nPoints; i++)
	{
		o.diffuse += pointLight( worldPos.xyz, o.normal, pointLights[i] );
	}

	int ns = nSpotLights < 2 ? nSpotLights : 2;
	for (int l = 0; l < ns; l++)
	{
		o.diffuse += spotLight( worldPos.xyz, o.normal, spotLights[l] );
	}	
	
	return o;
}

float4 ps_main_2_0( Output_2_0 input, 
				uniform int nDirectionals, 
				uniform int nPoints,
				uniform int nSpecDirectionals,
				uniform int nSpecPoints ) : COLOR0
{
	// Grab our maps
#ifdef COLOURISE_DIFFUSE_MAP
	float4 diffuseMap = colouriseDiffuseMap( diffuseSampler, input.tc, input.tc );
#else
	float4 diffuseMap = tex2D( diffuseSampler, input.tc );
#endif
	float3 specularMap = tex2D( specularSampler, input.tc );
	float3 normalMap = (tex2D( normalSampler, input.tc ).xyz * 2 ) - 1;
	float4 subSurfaceMap = tex2D( subSurfaceSampler, input.tc );

	// construct texture space matrix and transform normal map normal to 
	// world space
	float3x3 tsMatrix = { input.tangent, input.binormal, input.normal };
	float3 normal = normalize( mul( normalMap, tsMatrix ) );
	
	// Lighting section
	
	// Start with the ambient colour
	float3 diffuse = input.diffuse;
	
	// Do diffuse lighting
	for (int i = 0; i < nDirectionals; i++)
	{
		diffuse += directionalLight( normal, directionalLights[i] );
	}

	for (int i = 0; i < nPoints; i++)
	{
		diffuse += pointLight( input.worldPos.xyz, normal, pointLights[i] );
	}
	
	
	// grab the eye vector
	float3 eye = normalize(wsCameraPos - input.worldPos.xyz);
	
	// calculate specular lights
	float3 specular = float3(0,0,0);
	
	for (int i = 0; i < nSpecDirectionals; i++)
	{
		specular += directionalSpecLight( normal, eye, specularDirectionalLights[i] );
	}

	for (int i = 0; i < nSpecPoints; i++)
	{
		specular += pointSpecLight( input.worldPos.xyz, normal, eye, specularPointLights[i] );
	}
	
	// do the rim lighting
	float rimLight;
	if (nDirectionals > 0)
	{
		half3 rLight = directionalLights[0].direction;
		rLight = -rLight;
		float attenuation = saturate(dot(rLight, normal ));
		rimLight = 1.0 - saturate( dot( eye, normal ) );
		rimLight = rimStrength * smoothstep( 1-rimWidth, 1, rimLight );
		rimLight = saturate( rimLight * attenuation );
	}
	else
	{
		rimLight = 0.0;
	}

	// calculate the proportien of diffuse to sub surface map to blend together and mul by the diffuse colour.
	float diffuseAmount = 1 - ( 1 - pow(saturate( dot(normal, eye) ), subSurfaceBlendPower ) ) * subSurfaceMap.w;
	diffuse = (diffuseMap.xyz * diffuseAmount + subSurfaceMap * (1-diffuseAmount)) * saturate( diffuse );

	// Sample sky map
	float skyMap = BW_SAMPLE_SKY_MAP(input.skyLightMap)
	
	// Calculate result colour
	float4 colour;
	colour.xyz = skyMap * diffuse + (rimLight*diffuseMap.xyz) + saturate(specular) * specularMap;
#ifdef MOD2X
	colour.xyz *= (1 + diffuseLightExtraModulation);
#endif
	colour.w = diffuseMap.w;

	return colour;
}

VertexShader vertexShaders_2_0[9] =
{
	compile vs_2_0 vs_main_2_0(0,0),
	compile vs_2_0 vs_main_2_0(0,1),
	compile vs_2_0 vs_main_2_0(0,2),
	compile vs_2_0 vs_main_2_0(0,3),
	compile vs_2_0 vs_main_2_0(1,0),
	compile vs_2_0 vs_main_2_0(1,1),
	compile vs_2_0 vs_main_2_0(1,2),
	compile vs_2_0 vs_main_2_0(1,3),
	compile vs_2_0 vs_main_2_0(2,4)
};

int vs_2_0Selector()
{
	int res = 0;
	if (nDirectionalLights > 1 || nPointLights >3)
	{
		res = 8;
	}
	else
	{
		res = nDirectionalLights *4 + nPointLights;
	}
	return res;
}

PixelShader pixelShaders_2_0[11] = 
{
	compile ps_2_0 ps_main_2_0(0,0,0,0),
	compile ps_2_0 ps_main_2_0(1,0,0,0),
	compile ps_2_0 ps_main_2_0(0,1,0,0),
	compile ps_2_0 ps_main_2_0(1,0,0,0),
	compile ps_2_0 ps_main_2_0(0,0,1,0),
	compile ps_2_0 ps_main_2_0(1,0,1,0),
	compile ps_2_0 ps_main_2_0(0,1,1,0),
	compile ps_2_0 ps_main_2_0(1,0,1,0),
	compile ps_2_0 ps_main_2_0(0,0,0,1),
	compile ps_2_0 ps_main_2_0(1,0,0,1),
	compile ps_2_0 ps_main_2_0(0,1,0,1)
};

int ps_2_0Selector()
{
	int dir = nDirectionalLights > 0;
	int isPoint = dir ? 0 : nPointLights > 0;
	int specDir = nSpecularDirectionalLights > 0;
	int specPoint = specDir ? 0 : nSpecularPointLights > 0;
	return specPoint * 8 + specDir * 4 + isPoint *2 + dir;
}


#endif