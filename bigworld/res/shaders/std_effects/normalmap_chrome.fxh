#ifndef NORMALMAP_CHROME
#define NORMALMAP_CHROME

struct OutputNormalMapEnv
{
	float4 pos:     POSITION;
	float4 tc:      TEXCOORD0;
	float4 dLight1: TEXCOORD1;
	float4 dLight2: TEXCOORD2;
	float4 sLight1: TEXCOORD3;
	float4 sLight2: TEXCOORD4;

	float4 tangentToCubeSpace0 : TEXCOORD5; //first row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace1 : TEXCOORD6; //second row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace2 : TEXCOORD7; //third row of the 3x3 transform from tangent to cube space	
	
	float4 diffuse: COLOR;
	float fog: FOG;
};

BW_ARTIST_EDITABLE_FRESNEL
BW_NON_EDITABLE_LIGHT_ENABLE

int graphicsSetting
<
	string label = "REFLECTION_RENDERING";
	string desc = "Reflection Rendering";
>;

BW_ARTIST_EDITABLE_REFLECTION_AMOUNT
BW_ARTIST_EDITABLE_MATERIAL_SPECULAR
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_NORMAL_MAP

#ifdef CHROME_GLOW_MAP
BW_ARTIST_EDITABLE_GLOW_FACTOR
BW_ARTIST_EDITABLE_GLOW_MAP
#define NORMALMAP_GLOW_MAP
#endif

#ifdef CHROME_SPEC_MAP
BW_ARTIST_EDITABLE_SPEC_MAP
#endif

BW_ARTIST_EDITABLE_REFLECTION_MAP
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

#ifdef IN_GAME

VERTEX_FOG
BW_SKY_LIGHT_MAP
BW_SKY_LIGHT_MAP_SAMPLER

#endif //IN_GAME

DECLARE_OTHER_MAP( reflectionMask, maskSampler, "Reflection Mask", "mask of the reflective area" )

sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
sampler normalSampler = BW_SAMPLER(normalMap, BW_TEX_ADDRESS_MODE)
sampler reflectionSampler = BW_SAMPLER(reflectionMap, WRAP)
samplerCUBE reflectionCubeSampler = BW_SAMPLER( reflectionMap, CLAMP )

#ifdef CHROME_GLOW_MAP
sampler glowSampler = BW_SAMPLER(glowMap, BW_TEX_ADDRESS_MODE)
#endif
#ifdef CHROME_SPEC_MAP
sampler specularSampler = BW_SAMPLER(specularMap, BW_TEX_ADDRESS_MODE)
#endif

#if SKY_LIGHT_MAP_ENABLE
#define BW_SKY_MAP_COORDS_WORLD_SPACE_ENV( output, worldPos )\
	float4 skyLightMap;\
	skyLightMap.xyz = worldPos.xyz - wsCameraPos.xyz;\
	skyLightMap.w = 1.0;\
	output.tc.z = dot( skyLightMap, skyLightMapTransform[0] );\
	output.tc.w = dot( skyLightMap, skyLightMapTransform[1] );
#else // SKY_LIGHT_MAP_ENABLE
#define BW_SKY_MAP_COORDS_WORLD_SPACE_ENV( output, worldPos )
#endif // SKY_LIGHT_MAP_ENABLE

#if SKY_LIGHT_MAP_ENABLE
#define BW_SAMPLE_SKY_MAP_ENV(input) 1.0 - tex2D( skyLightMapSampler, input.tc.zw ).w;
#else
#define BW_SAMPLE_SKY_MAP_ENV(input) 1.0;
#endif //SKY_LIGHT_MAP_ENABLE

BW_DIFFUSE_LIGHTING
BW_SPECULAR_LIGHTING

void calculateLightingEnv_vs3(in float3 worldPos,
							in float3 worldNormal,
							in float3 tangent,
							in float3 binormal,
							in float4 diffuse,
							in float3x3 tsMatrix,
							in int nDirectionals, 
							in int nPoints, 
							in int nSpecularDirectionals, 
							in int nSpecularPoints,
							out OutputNormalMapEnv o )
{
	BW_CALC_LIGHT_COUNTS
	int ns = nSpotLights < 2 ? nSpotLights : 2;

	float4 attenuation;	
	float3 eye = normalize(wsCameraPos - worldPos);
	
	o.diffuse = normalMapDiffuse_vs1( worldPos, worldNormal, diffuse, selfIllumination, nSpotLights, nDir, nPoint, nDirBump );
	attenuation.xy = normalMapDiffuseBump( worldPos, tsMatrix, nDirBump, nPointBump, o.dLight1.xyz, o.dLight2.xyz );
	attenuation.zw = normalMapSpecularBump( worldPos, eye, tsMatrix, nSpecularPoints, nSpecularDirectionals, o.sLight1.xyz, o.sLight2.xyz );
	
	o.dLight1.w = attenuation.x;
	o.dLight2.w = attenuation.y;
	o.sLight1.w = attenuation.z;
	o.sLight2.w = attenuation.w;		
	
	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = transpose(tsMatrix)[0].xyz;
	o.tangentToCubeSpace1.xyz = transpose(tsMatrix)[1].xyz;
	o.tangentToCubeSpace2.xyz = transpose(tsMatrix)[2].xyz;

    o.tangentToCubeSpace0.w = eye.x;
    o.tangentToCubeSpace1.w = eye.y;
    o.tangentToCubeSpace2.w = eye.z;
}

struct OutputReflection
{
	float4 pos:     	POSITION;
	float2 tc:			TEXCOORD0;
	float3 tc2:			TEXCOORD1;
	float4 fresnel:		COLOR0;
	float fog: FOG;
};

#include "chrome.fxh"


OutputReflection reflection_vs1( BUMPED_VERTEX_FORMAT i )
{
	OutputReflection o = (OutputReflection)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc = i.tc;
	
	//TODO : work out how to stick this back in.  Also the fact that this
	//needed to be removed probably explains why lightonly_chrome maps are
	//upside down and offset a bit.
	//float4 ut = float4(mul( world, float3(1,0,0) ).xyz, 1) * 0.5;
	//float4 vt = float4(mul( world, float3(0,-1,0) ).xyz, 1) * 0.5;

	float3 worldEye;
	o.tc2.xy = chromeMapCoords( worldPos, worldNormal, wsCameraPos, float3(1,0,0), float3(0,-1,0), worldEye );
	o.tc2.z = 0;
	
	o.fresnel.x = fresnel(-worldEye, worldNormal, fresnelExp, fresnelConstant) * reflectionAmount;	

	return o;
}


float4 reflection_ps1( OutputReflection i ) : COLOR0
{
	float4 diffuseColour = tex2D( diffuseSampler, i.tc );
	float3 reflectionColour = tex2D( reflectionSampler, i.tc2 );	
	return float4( i.fresnel.xxx * reflectionColour, diffuseColour.a );
}

#ifndef IN_GAME

/*
 * Things for the 3ds max technique
 */
float4 lightDir : Direction 
<
string UIName = "Light Direction";
string Object = "TargetLight";
int RefID = 0;
> = {-0.577, -0.577, 0.577,1.0};

float4 lightColour : LightColor 
<
//string UIName = "Diffuse";
int LightRef = 0;
> = float4( 1.0f, 1.0f, 1.0f, 1.0f );    // diffuse


float4x4 worldInverse : WorldI;
float4x4 viewInverse  : ViewI;

OutputNormalMapEnv normalMap_maxpreview_vs3( VertexXYZNUVTB i)
{
	OutputNormalMapEnv o = (OutputNormalMapEnv)0;
	DirectionalLight dl;
	dl.direction = normalize(mul( lightDir.xyz, worldInverse ));
	dl.colour = float4(0.5, 0.5, 0.5, 1);

	o.pos = mul(i.pos, worldViewProj);
	
	float3x3 tsMatrix = {i.tangent, i.binormal, i.normal };

	o.tc.xy = i.tc.xy;
	o.diffuse = float4(0.1, 0.1, 0.1, 1);

	o.dLight1.w = directionalBumpLight( tsMatrix, dl, o.dLight1.xyz );
	o.dLight2.w = 0;

	float3 camp = mul( viewInverse[3].xyz, worldInverse );
	float3 eye = normalize(camp - i.pos);	

	o.sLight1.w = directionalSpecBumpLight( eye, tsMatrix, dl, o.sLight1.xyz );;
	o.sLight2.w = 0;

	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = mul(tsMatrix, transpose(world)[0].xyz);
	o.tangentToCubeSpace1.xyz = mul(tsMatrix, transpose(world)[1].xyz);
	o.tangentToCubeSpace2.xyz = mul(tsMatrix, transpose(world)[2].xyz);

	float3 wsCameraPos = viewInverse[3].xyz;

	float4 worldPos = mul( i.pos, world );

    // Compute the eye vector (going from shaded point to eye) in cube space
	float3 eyeVector = wsCameraPos - worldPos;
    
    o.tangentToCubeSpace0.w = eyeVector.x;
    o.tangentToCubeSpace1.w = eyeVector.y;
    o.tangentToCubeSpace2.w = eyeVector.z;


	return o;
}

float4 normalMap_maxpreview_ps3( OutputNormalMapEnv i ) : COLOR0
{
	//  Output constant color:
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float3 normal = (tex2D( normalSampler, i.tc ) * 2 ) - 1;
#ifdef CHROME_GLOW_MAP
	float3 glowMap = tex2D( glowSampler, i.tc );
#endif
	float3 diffuseColour = i.diffuse;
	
	// Calculate a reflection vector and fresnel term
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	float3 eyeVec = normalize( float3(i.tangentToCubeSpace0.w, i.tangentToCubeSpace1.w, i.tangentToCubeSpace2.w) );	
	float3 reflVec = reflect(eyeVec, envNormal);	
	reflVec.yz = reflVec.zy;
	reflVec.y = -reflVec.y;
	
	float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec);
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant);
	reflectionColour *= fresnelTerm;

	float3 diffuse1Colour = lightColour.xyz;
	float3 diffuse2Colour = float3(0,0,0);
	float3 specular1Colour = lightColour.xyz * materialSpecular;
	float3 specular2Colour = float3(0,0,0);
	float4 attenuation = float4(0,0,0,0);

	attenuation.x = saturate( dot( normalize(i.dLight1.xyz), normal ) ) * i.dLight1.w;
	attenuation.z = pow( saturate( dot( normalize(i.sLight1.xyz), normal ) ), 32 ) * i.sLight1.w;

	diffuseColour +=  attenuation.x * diffuse1Colour;

#ifdef CHROME_GLOW_MAP
	float3 specularColour = glowMap * glowFactor;
	specularColour += attenuation.z * specular1Colour;
#else
	float3 specularColour = attenuation.z * specular1Colour;
#endif

	float4 colour;
	colour.xyz = (diffuseColour * diffuseMap) + specularColour;
	colour.xyz += reflectionColour * reflectionAmount * diffuseMap.aaa;
	colour.w = 1;

	return colour;
}

technique max_preview
{
	pass Pass_0
	{
		ZEnable = true;
		ZWriteEnable = true;
		BW_CULL_DOUBLESIDED
		
		VertexShader = compile vs_3_0 normalMap_maxpreview_vs3();
		PixelShader = compile ps_3_0 normalMap_maxpreview_ps3();
	}
}

#endif 

#endif //NORMALMAP_CHROME