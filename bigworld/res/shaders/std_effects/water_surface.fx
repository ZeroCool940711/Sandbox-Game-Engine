//--------------------------------------------------------------//
// General purpose water shader.
//
//TODO: 
//- second wave texture
//- specular
//- reflection tint
//- alpha control??? (provided map or something?)
//- backfaces shouldnt be fresnel'd??
//- clean up
//--------------------------------------------------------------//

#include "stdinclude.fxh"

#ifdef IN_GAME

float3 cameraPos : CameraPos;
float3 osCameraPos : CameraPosObjectSpace;
float4x3 objToCubeSpace : World; // for env. mapping only
float3   eyePositionEnv : CameraPos; // in cube space; for env. mapping only
float    BumpScaleEnv;   // for env. mapping only
float time : Time;

float vertexExtrusion
< 
	bool artistEditable = true; 
	string UIName = "Extrusion amount";
	string UIDesc = "The scalar factor to shift the mesh vertex along the normal";
	float UIMin = -1;
	float UIMax = 1;
	int UIDigits = 6;
> = 0.00;


float rippleScale
< 
	bool artistEditable = true; 
	string UIName = "Ripple scale";
	string UIDesc = "The scalar factor to scale the amount of distortion in the reflection";
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 2;
> = 1.0;

float4 waveAnimation
< 
	bool artistEditable = true; 
	string UIName = "Wave Animation";
	string UIDesc = "Movement of the wave on the surface";
	float UIMin = -10;
	float UIMax = 10;
	int UIDigits = 4;
> = 0.1;

float waveTiling
< 
	bool artistEditable = true; 
	string UIName = "Tiling";
	string UIDesc = "Tiling of the wave textures";
	float UIMin = -100;
	float UIMax = 100;
	int UIDigits = 4;
> = 1.0;

float smoothness
< 
	bool artistEditable = true; 
	string UIName = "Smoothness";
	string UIDesc = "Smoothness of the surface";
	float UIMin = 0;
	float UIMax = 1;
	int UIDigits = 2;
> = 0.0;

bool useRefraction = true;
bool useSmoothEdges = false;

float4 simulationTransformX = {1,0,0,0};
float4 simulationTransformY = {0,1,0,0};

//TODO: these are dodgy values put in to test
//float4 reflectionTint = {1,1,1,1};
float4 reflectionTint
< 
	bool artistEditable = true; 
	string UIName = "Reflection Tint";
	string UIDesc = "Reflection Tint";
	string UIWidget = "Color";
	float UIMin = 0;
	float UIMax = 2;
	int UIDigits = 1;

> = {1,1,1,1};

float sunPower
< 
	bool artistEditable = true; 
	string UIName = "Specular";
	string UIDesc = "Specular";
	float UIMin = 0;
	float UIMax = 128;
	int UIDigits = 3;
> = 32.0;

BW_ARTIST_EDITABLE_DOUBLE_SIDED

//float sunPower = 32.0;
float sunScale = 1.0;

float4x4 worldViewProj : WorldViewProjection;
float4x4 world;
float4x4 tex;
float4x4 trans;

//TODO: create a different texture parameter for the cube maps
texture reflectionMap
<
	bool artistEditable = true;
	string UIWidget = "CubeMap";
	string UIName = "Reflection Cube Map";
	string UIDesc = "The reflection map for the material";
>;
BW_ARTIST_EDITABLE_NORMAL_MAP

texture simulationMap;

BW_NON_EDITABLE_ALPHA_BLEND

int nSpecularDirectionalLights : SpecularDirectionalLightCount;
int nSpecularPointLights : SpecularPointLightCount;
DirectionalLight osSpecularDirectionalLights[2] : SpecularDirectionalLights;
PointLight osSpecularPointLights[2] : SpecularPointLights;

VERTEX_FOG

bool alphaTestEnable = false;
int alphaReference = 0;


BW_ARTIST_EDITABLE_FRESNEL


bool invertFresnel
<
	bool artistEditable = true;
	string UIName = "Invert fresnel";
	string UIDesc = "Invert the fresnel falloff";\
	int UIMin = 1;\
	int UIMax = 15;\
	int UIDigits = 2;\
> = false;

float fresnelScale
< 
	bool artistEditable = true; 
	string UIName = "Fresnel scale";
	string UIDesc = "Fresnel scale";
	float UIMin = 0;
	float UIMax = 5;
	int UIDigits = 3;
> = 1.0;

float reflectionScale
< 
	bool artistEditable = true; 
	string UIName = "Reflection scale";
	string UIDesc = "Reflection scale";
	float UIMin = 0;
	float UIMax = 5;
	int UIDigits = 3;
> = 1.0;

sampler simulationSampler = sampler_state
{
	Texture = (simulationMap);

	ADDRESSU = BORDER;
	ADDRESSV = BORDER;
	ADDRESSW = BORDER;
	
	BORDERCOLOR = 0x0000ff00;

	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;

	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler normalSampler = sampler_state
{
	Texture = (normalMap);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

samplerCUBE reflectionCubeSampler = sampler_state
{
	Texture = (reflectionMap);

	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;

	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

struct PS_INPUT
{
	float4 pos:				POSITION;
	float4 tc:				TEXCOORD0;
	//float3 reflectionVec:	TEXCOORD1;
	//float3 normal:			TEXCOORD2;	
	//float3 eyeVec:			TEXCOORD3;
    float4 tangentToCubeSpace0 : TEXCOORD1; //first row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace1 : TEXCOORD2; //second row of the 3x3 transform from tangent to cube space
    float4 tangentToCubeSpace2 : TEXCOORD3; //third row of the 3x3 transform from tangent to cube space	
	float3 tsEyeVec : TEXCOORD4;
	float4 alpha:			COLOR0;
	float fog:				FOG;
};

#define cHalf	0.5f

//TODO: need tangent space



PS_INPUT vs_main_envmap( VertexXYZNUVTB i )
{
	PS_INPUT o = (PS_INPUT)0;
	
	float4 offsetPos = i.pos;
	offsetPos.xyz += i.normal * vertexExtrusion;
	
	float4 projPos = mul( offsetPos, worldViewProj );
	o.pos = projPos;
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	
	float4 rippleTransformX;
	float4 rippleTransformY;

	rippleTransformX = float4(waveTiling,0,0, waveAnimation.x * time);
	rippleTransformY = float4(0,waveTiling,0, waveAnimation.y * time);

	// Transform bump coordinates
	o.tc.x = dot( float4(i.tc.xy,0,1), rippleTransformX );
	o.tc.y = dot(  float4(i.tc.xy,0,1), rippleTransformY );
		
	float3 worldPos = mul(i.pos, world);    
    
    float3x3 tsMatrix = {i.tangent, i.binormal, i.normal };
    
	float3 eye = normalize(osCameraPos - i.pos);
    
	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = transpose(tsMatrix)[0].xyz;
	o.tangentToCubeSpace1.xyz = transpose(tsMatrix)[1].xyz;
	o.tangentToCubeSpace2.xyz = transpose(tsMatrix)[2].xyz;
    
    o.tangentToCubeSpace0.w = i.pos.x;
	o.tangentToCubeSpace1.w = i.pos.y;
	o.tangentToCubeSpace2.w = i.pos.z;

	float3 h = osSpecularDirectionalLights[0].direction + eye;
	o.tsEyeVec = mul( tsMatrix, h );
    
	return o;
};

float4 ps_main_envmap( PS_INPUT i ) : COLOR0
{
   	// Load normal and expand range
	float4 normalSample1 = tex2D( normalSampler, i.tc.xy );
	float3 normal = normalSample1 * 2.0 - 1.0;	// expand
	
	normal = normalize(lerp(normal, float3(0,0,1), smoothness));
	
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	float3 eyeVec = float3(	osCameraPos.x - i.tangentToCubeSpace0.w,
							osCameraPos.y - i.tangentToCubeSpace1.w, 
							osCameraPos.z - i.tangentToCubeSpace2.w );
	eyeVec = normalize( eyeVec );
	envNormal = normalize(envNormal);
	float3 reflVec = reflect(eyeVec, envNormal);
	reflVec.y = -reflVec.y;
	
    float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec) * reflectionTint * reflectionScale;
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant) * fresnelScale;

	if (invertFresnel)
		fresnelTerm = 1 - saturate(fresnelTerm);
	//fresnelTerm = -(fresnelTerm - invertFresnel);


//TODO: Specular
//	half3 halfAngle = i.tsEyeVec;
//
//	half specular = sunScale*pow( saturate( dot( halfAngle, vNormal ) ), sunPower );
//	reflectionColour += saturate(specular * osSpecularDirectionalLights[0].colour);


	return float4(reflectionColour.xyz, fresnelTerm);
};


//--------------------------------------------------------------//
// Technique Section
//--------------------------------------------------------------//

// Uses a cube map for reflections with the reflection vector perturbed
// by the combination of 2 normal maps. Alpha transparency is used for refraction.
// Fresnel term is included.
// The 2 normal maps are animated through the bumpTexCoordTransformXX parameters
// TODO: refraction tinting (how to do this when using alpha?)
technique water_envmap_alpha
<
	bool bumpMapped = true;
	string channel = "internalSorted";
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		ALPHATESTENABLE = FALSE;
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		ZFUNC = LESSEQUAL;
		BW_FOG

		ALPHABLENDENABLE = TRUE;
		SRCBLEND = SRCALPHA;
		DESTBLEND = INVSRCALPHA;
		
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		BW_CULL_DOUBLESIDED

		VertexShader = compile vs_2_0 vs_main_envmap();
		PixelShader = compile ps_2_0 ps_main_envmap();
	}
}

//TODO: fallback

#else	//ifdef IN_GAME

#include "lightonly.fx"

#endif	//endif