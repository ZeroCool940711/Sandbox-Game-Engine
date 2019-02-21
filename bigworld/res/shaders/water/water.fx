// ----------------------------------------------------------------------------
// Water shaders
// ----------------------------------------------------------------------------
// TODO: use normalization cube map?
// TODO: create a different texture parameter for the cube maps
// TODO: group all the common variables for the different water shaders.

#include "stdinclude.fxh"
#include "water_common.fxh"

#ifdef IN_GAME
float3		cameraPos : CameraPos;
VERTEX_FOG
#endif //IN_GAME

float4		scale					= {0.08,0.08,0.08,0.08};
float4		simulationTransformX	= {1,0,0,0};
float4		simulationTransformY	= {0,1,0,0};
float4		bumpTexCoordTransformX	= {1,0,0,0};
float4		bumpTexCoordTransformY	= {0,1,0,0};
float4		bumpTexCoordTransformX2 = {1,0,0,0};
float4		bumpTexCoordTransformY2 = {0,1,0,0};
float4		reflectionTint			= {1,1,1,1};
float4		refractionTint			= {0.1,0.9,1.0,1};
float4		foamColour				= {0.8, 0.8, 0.8, 1.0};

float4		deepColour				= {0, 0.21, 0.35, 1.0};

float4		screenOffset			= { 0,0,0,0 };

float		simulationTiling		= 1.f; //used to artificially increase the sim resolution for the rain
float		sunPower				= 32.0;
float		sunScale				= 1.0;
float		smoothness				= 0.0;

#define _VERTEX_ANIM
#ifdef _VERTEX_ANIM
float		time : Time;
float		texScale				= 0.0;
float		freqX					= 1.0;
float		freqZ					= 1.0;
float		maxDepth				= 100.f;
float		fadeDepth				= 10.f;
#endif //_VERTEX_ANIM

//configurable foam params
float softParticleContrast = 2.0;
float foamIntersectionFactor = 0.25;
float softIntersectionFactor = 1.0;
float foamMultiplier = 0.75;
float foamTiling = 1.0;

bool		useSimulation			= true;
bool		combineSimulation		= false;
bool		useRefraction			= true;
bool		highQuality				= true;
bool		useCubeMap				= false;
float		simpleReflection		= 0.0;

float4x4	worldViewProj;
float4x4	world;

BW_NON_EDITABLE_ALPHA_TEST
BW_NON_EDITABLE_ALPHA_BLEND
BW_FRESNEL
BW_SPECULAR_LIGHTING

// ----------------------------------------------------------------------------
// Section: Textures
// ----------------------------------------------------------------------------

texture		distortionBuffer : DistortionBuffer;
texture		reflectionMap;
texture		normalMap;
texture		foamMap;
texture		simulationMap;
texture		rainSimulationMap;
texture		screenFadeMap;
texture		reflectionCubeMap;

#include "depth_helpers.fxh"

float nearPlane : NearPlane;

texture depthTex : DepthTex;
sampler depthSampler = sampler_state
{
	Texture = (depthTex);

	ADDRESSU = CLAMP;
	ADDRESSV = CLAMP;
	ADDRESSW = CLAMP;

	MAGFILTER = POINT;
	MINFILTER = POINT;
	MIPFILTER = POINT;

	MAXANISOTROPY = 1;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

// ----------------------------------------------------------------------------
// Section: Samplers
// ----------------------------------------------------------------------------

sampler simulationSampler		= BW_SAMPLER(simulationMap, CLAMP)
sampler rainSimulationSampler	= BW_SAMPLER(rainSimulationMap, WRAP)
sampler normalSampler			= BW_SAMPLER(normalMap, WRAP)
sampler screenFadeSampler		= BW_SAMPLER(screenFadeMap, WRAP)
sampler reflectionSampler		= BW_SAMPLER(reflectionMap, CLAMP)
sampler refractionSampler		= BW_SAMPLER(distortionBuffer, CLAMP)
samplerCUBE reflectionCubeSampler = BW_SAMPLER( reflectionCubeMap, CLAMP )

// ----------------------------------------------------------------------------
// Section: Vertex formats
// ----------------------------------------------------------------------------

struct VertexXYZDUV2
{
   float4 pos:				POSITION;
   float4 diffuse:			COLOR0;
   float4 tc:				TEXCOORD0;
};

// ----------------------------------------------------------------------------
// Section: Pixel shader input
// ----------------------------------------------------------------------------

struct PS_INPUT_RT
{
	float4 pos:				POSITION;
	float4 tc:				TEXCOORD0;
	float4 worldPos:		TEXCOORD1;
	float4 reflect_refract:	TEXCOORD2;
	float4 W_sim:			TEXCOORD3;
	float4 foam0:			TEXCOORD4;
	float4 alpha:			COLOR0;
	float fog:				FOG;	
};

// ----------------------------------------------------------------------------
// Section: General Functions
// ----------------------------------------------------------------------------

half3 generateSurfaceNormal( float4 tex, half3 simNormal, bool extraSample )
{
   	// Load normal and expand range
	half4 vNormalSample1 = tex2D( normalSampler, tex.xy );
	half3 vNormal1 = vNormalSample1 * 2.0 - 1.0;	// expand

	half4 vNormalSample2 = tex2D( normalSampler, tex.zw );
	half3 vNormal2 = vNormalSample2 * 2.0 - 1.0;	// expand
	
	half3 vNormal;
	if (extraSample)
	{
		//TODO: better sampling.	
		half4 vNormalSample3 = tex2D( normalSampler, tex.xy*0.1 + 5 );
		half3 vNormal3 = vNormalSample3 * 2.0 - 1.0;	// expand

		half4 vNormalSample4 = tex2D( normalSampler, tex.zw*7 - 2 );
		half3 vNormal4 = vNormalSample4 * 2.0 - 1.0;	// expand
	
		vNormal = normalize(vNormal1 + vNormal2 + vNormal3 + vNormal4);
	}
	else
		vNormal = normalize(vNormal1 + vNormal2);
	
	vNormal = lerp( vNormal, half3(0,0,1), smoothness);		
	//vNormal = normalize( vNormal + simNormal );

	//Note: changing this because it was smoothing the normal for
	// the variations that werent using the simNormal.... this isn't
	// mathematically correct but it makes it look better.
	vNormal.x += simNormal.x;
	vNormal.y += simNormal.y;

	vNormal = normalize( vNormal );
	return vNormal;
}

// ----------------------------------------------------------------------------
// Section: Vertex Shaders
// ----------------------------------------------------------------------------

#define cHalf	0.5f

PS_INPUT_RT vs_main ( VertexXYZDUV2 i, uniform bool modifyEdge )
{
	PS_INPUT_RT o = (PS_INPUT_RT)0;
	float4 inPos = i.pos;	
	float4 edge = i.diffuse;

#ifdef _VERTEX_ANIM
//	float depth = edge.w;
//	depth = depth * maxDepth;
//	float alpha = clamp(depth, 0, fadeDepth);
//	alpha = saturate(alpha / fadeDepth);
//	edge.w = alpha;

	float orig = inPos.y;
	if (modifyEdge)
	{
		//TODO: better anim
		float anim =   inPos.y - (1-sin((time+inPos.x)*freqX)*sin((time+inPos.z)*freqZ))*texScale;
		inPos.y = lerp(anim,orig,edge.w);

		//TODO: fade out the reflection when moving up+down
		//TODO: generate some tex coords from the x/z positions .. use them for a foam effect 
		// need a foam range.... alpha 0.5 - 1 maybe....... and then shift the alpha value below this so the foam is visible

	}
#endif // _VERTEX_ANIM	
		
	float4 projPos = mul( inPos, worldViewProj );
	o.pos = projPos;
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );

	// Transform bump coordinates
	o.tc.x = dot( float4(i.tc.xy,0,1), bumpTexCoordTransformX );
	o.tc.y = dot( float4(i.tc.xy,0,1), bumpTexCoordTransformY );

	o.tc.z = dot( float4(i.tc.xy,0,1), bumpTexCoordTransformX2 );
	o.tc.w = dot( float4(i.tc.xy,0,1), bumpTexCoordTransformY2 );

	o.W_sim.x = (i.tc.x + simulationTransformX.w)*simulationTransformX.x + cHalf;
	o.W_sim.y = (i.tc.y + simulationTransformY.w)*simulationTransformY.y + cHalf;

	// Map projected position to the reflection texture
	float2 reflectPos;
	reflectPos = (projPos.xy + projPos.w) * cHalf;

	// Map projected position to the refraction texture
	float2 refractPos;
	refractPos = (projPos.xy + projPos.w) * cHalf;

	// Reflection transform
	o.reflect_refract = float4( reflectPos.x, -reflectPos.y, -refractPos.y, refractPos.x );
	o.W_sim.z = projPos.w;

	float3 worldPos = mul(inPos, world);
	//TODO: clean up the water interpolators
    o.worldPos.xyz = worldPos;
	o.worldPos.w = o.pos.w;
    o.alpha = edge;

	//TODO: use the far plane here ....
	//o.W_sim.w = (1 - (projPos.z * 0.002));
	o.W_sim.w = projPos.z;
	// foam tex coords
	//TODO: better foam control
	//float foamAnim = frac( waterParam * wavesSpeed );
	float foamAnim = frac( bumpTexCoordTransformX.x  );
	o.foam0.xy = inPos.xz * float2( 0.02, 0.02 ) * 0.333 *16 * 3 - foamAnim;
	o.foam0.wz = inPos.zx * float2( 0.02, 0.02 ) * 0.333* 20 * 2.1 + foamAnim;

	o.foam0 *= foamTiling;
	return o;
};

// ----------------------------------------------------------------------------
// Section: Pixel Shaders
// ----------------------------------------------------------------------------


half4 computeDependentCoords_3_0(float4 reflect_refract, half2 offset, float w, float zScale,
							 out half2 oScreenCoords, out half4 origins  )
{
	// Perform division by W only once
	half ooW = 1.0f / w;
	// Vectorize the dependent UV calculations (reflect = .xy, refract = .wz)
	half4 vN = offset.xyxy;	
	half4 screenCoords = reflect_refract * ooW;
	screenCoords = screenCoords + screenOffset;

	oScreenCoords = screenCoords.wz;
	// Fade out the distortion offset at the screen borders to avoid artifacts.
	half3 screenFade = tex2D( screenFadeSampler, screenCoords.xy  );	
	half4 dependentTexCoords = vN * scale * screenFade.xxxx;

	origins = screenCoords;

	return dependentTexCoords;
}

half4 computeDependentCoords(float4 reflect_refract, half3 normal, float w, float zScale,
							 out half2 oScreenCoords  )
{
	// Perform division by W only once
	half ooW = 1.0f / w;
	// Vectorize the dependent UV calculations (reflect = .xy, refract = .wz)
	half4 vN = normal.xyxy;	
	half4 screenCoords = reflect_refract * ooW;
	screenCoords = screenCoords + screenOffset;

	oScreenCoords = screenCoords.wz;
	// Fade out the distortion offset at the screen borders to avoid artifacts.
	half3 screenFade = tex2D( screenFadeSampler, screenCoords.xy  );	
	half4 dependentTexCoords = vN * scale * screenFade.xxxx;
	dependentTexCoords += ( screenCoords );	
	return dependentTexCoords;
}

float3 calcReflection( half3 normal, float scale, half3 eye )
{
	float3 diff = (normal - float3(0,1,0)) * scale;
	float3 cubeNormal = float3(0,1,0) + diff;
	float3 reflVec = reflect(eye, cubeNormal);
	reflVec.y = -reflVec.y;
	return reflVec;
}

half3 calcMaskedReflection(float mask, float2 origTex, float2 distortedTex)
{
	half4 originalReflection = tex2D( reflectionSampler, origTex );
	half4 distortedReflection = tex2D( reflectionSampler, distortedTex );
	half3 reflectionColour = (1-mask) * originalReflection.rgb;
	reflectionColour = (mask) * distortedReflection.rgb + reflectionColour;
	return reflectionColour;
}

half3 calcFinalColour_3_0(half4 dependentTexCoords, half3 normal, half3 worldPos,
					  half2 originalTexCoords, int nDirectionals, float depth, float height)
{
	half3 eye = normalize(cameraPos - worldPos);

	// Sample reflection and refraction
	half4 originalRefraction = tex2D( refractionSampler, originalTexCoords );
	half4 distortedRefraction = tex2D( refractionSampler, dependentTexCoords.wz );
	half4 distortedRefraction2 = tex2D( refractionSampler, dependentTexCoords.xy );
	half3 refractionColour = distortedRefraction.rgb * distortedRefraction.a +
						(1 - distortedRefraction.a) * originalRefraction.rgb;
	refractionColour *= refractionTint;

	half3 reflectionColour;
	
	if ( simpleReflection == 0.0 )
	{
		if (useCubeMap)
		{
			float3 reflVec = calcReflection(normal, scale.x, eye);
			reflectionColour.rgb = texCUBE(reflectionCubeSampler, reflVec).rgb;
		}
		else
		{
			reflectionColour = calcMaskedReflection(distortedRefraction2.a, originalTexCoords, dependentTexCoords.xy);
		}
	}
	else
	{
		if ( simpleReflection == 1.0 )
		{
			float3 reflVec = calcReflection(normal, scale.x, eye);
			reflectionColour.rgb = texCUBE(reflectionCubeSampler, reflVec).rgb;
		}
		else
		{
			half3 c = calcMaskedReflection(distortedRefraction2.a, originalTexCoords, dependentTexCoords.xy);
			
			if ( simpleReflection <= 0.0 )
			{
				reflectionColour = c;
			}
			else 
			{
				float3 reflVec = calcReflection(normal, scale.x, eye);
				float3 env = texCUBE(reflectionCubeSampler, reflVec).rgb;
				reflectionColour = lerp( c, env, simpleReflection );
			}
		}
	}
	
	reflectionColour *= reflectionTint;

	//deepening effect
	float deepening = min(depth, maxDepth);
	deepening = max(deepening - fadeDepth, 0);
	deepening = (deepening / (maxDepth-fadeDepth));	
	refractionColour =  lerp(refractionColour, deepColour.rgb, min(deepening,0.95) );
	
	///////
	// sim foaming:
	//float height = simSample.a;
	float4 foam = foamColour;
	foam *= (normal.z*normal.z);
	float foamAmount = clamp(abs(height),0,0.4);
	
	refractionColour = lerp(refractionColour, foam, foamAmount );
	///////
		
   // Combine reflection / refraction
	half fresnel = fresnel( eye, normal, fresnelExp, fresnelConstant );
	half3 finalColour = lerp( refractionColour, reflectionColour, fresnel );

	// Specular light
	if (nDirectionals > 0)
	{
		half3 halfAngle = normalize(specularDirectionalLights[0].direction + eye);
		half specular = sunScale*pow( saturate( dot( halfAngle, normal ) ), sunPower );
		finalColour += (specular * specularDirectionalLights[0].colour);
	}

	return finalColour;
}

void calcCubeReflections( half3 eye, half3 normal, half2 tex,
						 out half3 refr,
						 out half3 refl )
{
	refr = tex2D( refractionSampler, tex );
	float3 cubeNormal = float3(0,1-scale.x,0) + normal*scale.x;
	float3 reflVec = reflect(eye, cubeNormal);
	reflVec.y = -reflVec.y;
	refl = texCUBE(reflectionCubeSampler, reflVec).rgb;
}


half3 calcFinalColour(half4 dependentTexCoords, half3 normal, half3 worldPos,
						  half height, half2 tc, half2 originalTexCoords,
						  int nDirectionals, bool bUseCube, bool useFoam)
{
	float4 foam = foamColour;
	half3 eye = normalize(cameraPos - worldPos);

	half3 finalColour;
	half3 refractionColour;
	half3 reflectionColour;
	if (bUseCube)
	{
		// Sample reflection and refraction
		calcCubeReflections( eye, normal, dependentTexCoords.wz, 
							refractionColour, reflectionColour );
	}
	else
	{
		half2 screenTexCoord = dependentTexCoords.wz;
		// Sample reflection and refraction
		half4 originalRefraction = tex2D( refractionSampler, originalTexCoords );
		half4 distortedRefraction = tex2D( refractionSampler, screenTexCoord );
		refractionColour = distortedRefraction.rgb * distortedRefraction.a +
							(1 - distortedRefraction.a) * originalRefraction.rgb;
		if (useFoam)
			refractionColour = lerp( refractionColour, foam, height );

		half4 originalReflection = tex2D( reflectionSampler, originalTexCoords );
		half4 distortedReflection = tex2D( reflectionSampler, screenTexCoord );
		reflectionColour = (distortedRefraction.a) * distortedReflection.rgb;
		half3 reflectionColour2 = (1-distortedRefraction.a) * originalReflection.rgb;
		reflectionColour += reflectionColour2;
	}

	refractionColour *= refractionTint;
	reflectionColour *= reflectionTint;

	// Combine reflection / refraction
	half fresnel = fresnel( eye, normal, fresnelExp, fresnelConstant );
	finalColour = lerp( refractionColour, reflectionColour, fresnel );

	if (nDirectionals > 0)
	{
		half3 halfAngle = normalize(specularDirectionalLights[0].direction + eye);
		half specular = sunScale*pow( saturate( dot( halfAngle, normal ) ), sunPower );
		finalColour += (specular * specularDirectionalLights[0].colour);
	}
	return finalColour;
}

half4 calcFinalColourAlpha(half4 dependentTexCoords, half3 normal, half3 worldPos, int nDirectionals)
{
	half3 eye = normalize(cameraPos - worldPos);	
	half2 screenTexCoord = dependentTexCoords.xy;

	// Sample reflection and use alpha for transparency
	half4 reflectionColour = tex2D( reflectionSampler, screenTexCoord ) * reflectionTint;	
	half4 refractionColour = refractionTint;
	half fresnel = fresnel( eye, normal, fresnelExp, fresnelConstant );
	half4 finalColour = reflectionColour*fresnel;
	
	// Specular
	if (nDirectionals > 0)
	{
		half3 halfAngle = normalize(specularDirectionalLights[0].direction + eye);
		half specular = sunScale*pow( saturate( dot( halfAngle, normal ) ), sunPower );	
		half4 specCol = specular * specularDirectionalLights[0].colour;
		finalColour.xyz += saturate(specCol);
		finalColour.xyz += saturate(specCol * (1-fresnel));
		fresnel= saturate(fresnel + (1-fresnel)*specular);
	}
	return float4( finalColour.xyz, fresnel );
}

#define WATER_NORMAL_3_0(tc, simNormal) \
	half3 vNormal = generateSurfaceNormal( tc, simNormal, true );\
	half edging = i.alpha.w;

#define WATER_NORMAL(tc, simNormal) \
	half3 vNormal = generateSurfaceNormal( tc, simNormal, false );\
	half edging = i.alpha.w;

#define WATER_TEX_COORDS \
	half2 screenCoords;\
	/* Compute coordinates for sampling Reflection/Refraction*/\
	half4 dependentTexCoords = computeDependentCoords(i.reflect_refract, vNormal, i.W_sim.z, i.W_sim.w, screenCoords);

#define WATER_TEX_COORDS_3_0 \
	half2 screenCoords;\
	half4 origin;\
	/* Compute coordinates for sampling Reflection/Refraction*/\
	half4 dependentTexCoords = computeDependentCoords_3_0(i.reflect_refract, vNormal, i.W_sim.z, i.W_sim.w, screenCoords, origin);\
	float waterDepth = i.W_sim.w;\
	dependentTexCoords += ( origin );\
	float objID = 0.0;\
	float sceneDepth = decodeDepthWithAlpha( depthSampler, screenCoords, objID );\
	float4 col = float4(0,0,0,0);\
	float z = sceneDepth;\
	float zdiff = (z - waterDepth);\
	float c = Contrast(zdiff, softParticleContrast);\
    if( (zdiff * c) <= EPSILONZ )\
    {\
        discard;\
        return col;\
    }\
	float depth = max( zdiff, 0 );\
	float sceneDepth2 = decodeDepth( depthSampler, dependentTexCoords.wz );\
	float zdiff2 = (sceneDepth2 - waterDepth);\
	float depth2 = max( zdiff2, 0 );\
	depth = max(depth2,depth);\
	float softIntersect1 = saturate((i.worldPos.w - nearPlane)  * 50);\
	float softIntersect = saturate( softIntersectionFactor * depth);

float Contrast(float Input, float ContrastPower)
{
#if 1
     //piecewise contrast function
     bool IsAboveHalf = Input > 0.5 ;
     float ToRaise = saturate(2*(IsAboveHalf ? 1-Input : Input));
     float Output = 0.5*pow(ToRaise, ContrastPower); 
     Output = IsAboveHalf ? 1-Output : Output;
     return Output;
#else
    // another solution to create a kind of contrast function
    return 1.0 - exp2(-2*pow(2.0*saturate(Input), ContrastPower));
#endif
}

#define EPSILONZ 0.0

sampler2D foamSampler = sampler_state
{
  Texture = (foamMap);
  MinFilter = LINEAR;
  MagFilter = LINEAR;
  MipFilter = LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
};

// A special case to draw the water in the project view thumbnails.
float4 ps_main_proj_view( PS_INPUT_RT i ) : COLOR0
{
	WATER_NORMAL(i.tc, half3(0,0,1))
	
	half2 screenCoords;
	// Compute coordinates for sampling Reflection/Refraction
	half4 dependentTexCoords = computeDependentCoords(i.reflect_refract, vNormal, i.W_sim.z, i.W_sim.w, screenCoords);
			
	half3 eye = half3(0,1,0);
	half2 screenTexCoord = dependentTexCoords.xy;

	// Sample reflection and use alpha for transparency
	half4 reflectionColour = tex2D( reflectionSampler, screenTexCoord ) * reflectionTint;	
	half4 refractionColour = refractionTint;
	half fresnel = fresnel( eye, vNormal.xzy, fresnelExp, fresnelConstant );
	half4 finalColour = reflectionColour*fresnel;
	
	// Specular
	if (nSpecularDirectionalLights > 0)
	{
		half3 halfAngle = normalize(specularDirectionalLights[0].direction + eye);
		half specular = sunScale*pow( saturate( dot( halfAngle, vNormal.xzy ) ), sunPower );	
		half4 specCol = specular * specularDirectionalLights[0].colour;
		finalColour.xyz += saturate(specCol);
		finalColour.xyz += saturate(specCol * (1-fresnel));
		fresnel= saturate(fresnel + (1-fresnel)*specular);
	}
	return float4( finalColour.xyz, fresnel*edging );

};

float2 calculateSimTex( float2 inputCoord )
{
	const float val = 1 - (2*SIM_BORDER_SIZE*c_pixelSize);
	float2 simTex = inputCoord;
	simTex = ((simTex - 0.5) * val) + 0.5;
	return simTex;
}

// The high quality water shader. (Shader Model 3)
float4 ps_main_3_0( PS_INPUT_RT i ) : COLOR0
{
	half3 simNormal = half3(0,0,1);
	float height = 0.f;

	if (useSimulation)
	{
		if (simulationTiling == 1.0)
		{
			half4 simSample = tex2D(simulationSampler, calculateSimTex(i.W_sim.xy));
			simNormal = simSample.xzy;
			height = simSample.a;
		}
		else if (combineSimulation) // sim and rain
		{
			half4 simSample = tex2D(simulationSampler, calculateSimTex(i.W_sim.xy));
			half3 rainSample = tex2D(rainSimulationSampler, calculateSimTex(i.W_sim.xy * simulationTiling));
			simSample.xyz = (simSample.xyz + rainSample.xyz) * 0.5;
			simNormal = simSample.xzy;
			height = simSample.a;
		}
		else // just rain
		{
			half4 simSample = tex2D(rainSimulationSampler, calculateSimTex(i.W_sim.xy * simulationTiling));
			simNormal = simSample.xzy;
		}
	}
	WATER_NORMAL_3_0( i.tc, simNormal )
	WATER_TEX_COORDS_3_0

	half4 finalColour = half4(0,0,0,0);
 
	//if (useRefraction)
		finalColour.rgb = calcFinalColour_3_0(dependentTexCoords, vNormal.xzy, i.worldPos.xyz, screenCoords, nSpecularDirectionalLights, depth, height );
	//else
	//	finalColour = calcFinalColourAlpha( dependentTexCoords, vNormal.xzy, i.worldPos.xyz, nSpecularDirectionalLights );

	/*foam*/
	half3 cFoam = tex2D(foamSampler, i.foam0.wz + vNormal.xy*0.075).xyz;
	cFoam += tex2D(foamSampler, i.foam0.xy + vNormal.xy*0.075).xyz;
	half fFoamLuminance = dot( cFoam.xyz, 0.333) - 1;
	fFoamLuminance = saturate( fFoamLuminance );
	half foamSoftIntersect = saturate(foamIntersectionFactor * (0.2 / fFoamLuminance) * (depth) );
	/* procedural foam*/
	half foam = 1-foamSoftIntersect;
	half foamMask = smoothstep(0.2, 0.7, saturate( softIntersect ) * foam );
	foam *= foamMask;
	half3 cFoamFinal = foam * fFoamLuminance;
	finalColour.xyz += cFoamFinal *foamMultiplier * (1-objID);
	finalColour.xyz = lerp( fogColour.xyz, finalColour.xyz, saturate(i.fog) );
	return float4( finalColour.xyz, /*softIntersect1*/ softIntersect* c);
};

// The medium quality water shader. (Shader Model 2)
float4 ps_main_2_0( PS_INPUT_RT i,
				/*uniform bool bUseRefraction,*/
				uniform bool bUseCubeMap,
				uniform bool bUseSim,
				uniform bool bUseRain,
				uniform int nDirectionals ) : COLOR0
{
	half3 simNormal;
	float height = 0.f;
	if (bUseSim)
	{
		half4 simSample = tex2D(simulationSampler, i.W_sim.xy);
		height = simSample.a;
		if (bUseRain)
		{
			half3 rainSample = tex2D(rainSimulationSampler, i.W_sim.xy * simulationTiling);
			simSample.xyz = (simSample.xyz + rainSample.xyz) * 0.5;
			simNormal = simSample.xzy;
		}
		else
		{
			simNormal = simSample.xzy;
		}
	} 
	else if (bUseRain)
	{
		half4 simSample = tex2D(rainSimulationSampler, i.W_sim.xy * simulationTiling);
		simNormal = simSample.xzy;
	}
	else
		simNormal = half3(0,0,1);

	WATER_NORMAL(i.tc, simNormal)
	WATER_TEX_COORDS
	//if (bUseRefraction)
	{
		half3 finalColour = calcFinalColour( dependentTexCoords, 
			vNormal.xzy, i.worldPos.xyz, height, i.tc.xy, screenCoords, nDirectionals,
			bUseCubeMap, bUseSim);
		return float4( finalColour, edging );
	}
	//else
	//{
	//	half4 finalColour = calcFinalColourAlpha( dependentTexCoords,
	//	vNormal.xzy, i.worldPos.xyz, nDirectionals);
	//	return float4( finalColour.xyz, finalColour.w*edging);
	//}
}

//--------------------------------------------------------------
// Technique Section
//--------------------------------------------------------------

PixelShader rt_pshaders[17] = 
{
	//TODO: branch in the shaders for HQ mode..
	compile ps_3_0 ps_main_3_0( ),

	// (bUseRefraction not used
	// (bUseCubeMap, bUseSim, bUseRain, nDirectionals)
	compile ps_2_0 ps_main_2_0( false, false, false, 0 ),
	compile ps_2_0 ps_main_2_0( false, true, false, 0 ),
	compile ps_2_0 ps_main_2_0( false, true, true, 0 ),
	compile ps_2_0 ps_main_2_0( false, false, true, 0 ),
	
	compile ps_2_0 ps_main_2_0( true, false, false, 0 ),
	compile ps_2_0 ps_main_2_0( true, true, false, 0 ),
	compile ps_2_0 ps_main_2_0( true, true, true, 0 ),
	compile ps_2_0 ps_main_2_0( true, false, true, 0 ),

	compile ps_2_0 ps_main_2_0( false, false, false, 1 ),
	compile ps_2_0 ps_main_2_0( false, true, false, 1 ),
	compile ps_2_0 ps_main_2_0( false, true, true, 1 ),
	compile ps_2_0 ps_main_2_0( false, false, true, 1 ),
	
	compile ps_2_0 ps_main_2_0( true, false, false, 1 ),
	compile ps_2_0 ps_main_2_0( true, true, false, 1 ),
	compile ps_2_0 ps_main_2_0( true, true, true, 1 ),
	compile ps_2_0 ps_main_2_0( true, false, true, 1 ),
};


int psIndex( int nDirectionals )
{
	if (highQuality)
		return 0;
	else
	{
		int index=1;
		if (useSimulation)
		{
			if (simulationTiling == 1.0)
				index = 2;
			else if (combineSimulation)//raining
				index = 3;
			else
				index = 4;
		}

		if (useCubeMap)
			index += 4;
		index += (8*nDirectionals);	

		return index;
	}
}

VertexShader rt_vshaders[3] =
{
	compile vs_3_0 vs_main(false),
	compile vs_3_0 vs_main(true),

	compile vs_2_0 vs_main(false)
};

int vsIndex()
{
	if (highQuality)
	{
#ifdef _VERTEX_ANIM
		if (texScale > 0)
			return 1;
		else
#endif
			return 0;
	}
	else
		return 2;
}

// Standard water render technique:
technique water_rt
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = <srcBlend>;
		DESTBLEND = <destBlend>;
		ZENABLE = TRUE;
		ZWRITEENABLE = <useRefraction>;
		ZFUNC = LESSEQUAL;		
		BW_FOG
		CULLMODE = NONE;
			
		VertexShader = (rt_vshaders[vsIndex()]);
		PixelShader  = (rt_pshaders[psIndex(nSpecularDirectionalLights)]);
	}
}

// Project view technique:
technique water_proj
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = <srcBlend>;
		DESTBLEND = <destBlend>;
		ZENABLE = TRUE;
		ZWRITEENABLE = <useRefraction>;
		ZFUNC = LESSEQUAL;		
		BW_FOG
		CULLMODE = NONE;
			
		VertexShader = (rt_vshaders[2]);
		PixelShader = compile ps_2_0 ps_main_proj_view();
	}
}


//TODO: better fallbacks
float4 ps_main_SM1( PS_INPUT_RT i ) : COLOR0
{
	// TODO: need to use the normalization cube map for SM1
	// Sample reflection and use alpha for transparency
	half4 reflectionColour = half4(0.8,0.8,1,1);	
	half4 refractionColour = refractionTint;

   	// Load normal and expand range
	half4 vNormalSample1 = tex2D( normalSampler, i.tc.xy );
	half3 vNormal1 = vNormalSample1 * 2.0 - 1.0;	// expand

	half3 vNormal = vNormal1;	

	//half fresnel = fresnel( eye, vNormal, fresnelExp, fresnelConstant );
	half fresnel = vNormal.z * 0.75;
	half4 finalColour = reflectionColour*fresnel;
	half edging = i.alpha.w;

	return float4( finalColour.xyz, fresnel*edging );
};
technique water_SM1
{
	pass Pass_0
	{
		ALPHATESTENABLE = <alphaTestEnable>;
		ALPHAREF = <alphaReference>;
		ALPHAFUNC = GREATER;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = <srcBlend>;
		DESTBLEND = <destBlend>;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;		
		BW_FOG
		CULLMODE = NONE;

		VertexShader = compile vs_1_1 vs_main(false);
		PixelShader = compile ps_1_1 ps_main_SM1();
	}
}


technique water_SM0
{
	// Please note: this looks bad, 
	// it's just here to placehold the fixed function fallback
	pass Pass_0
	{
		SPECULARENABLE = FALSE;
		TEXTUREFACTOR = (float4(0.5, 0.8, 1.0, 0.4));
		BW_FOG
		COLOROP[0] = ( 4 );
		COLORARG1[0] = TEXTURE;
		COLORARG2[0] = TFACTOR;
		ALPHAOP[0] = SELECTARG2;
		ALPHAARG1[0] = TEXTURE;
		ALPHAARG2[0] = TFACTOR;
		Texture[0] = (normalMap);
		ADDRESSU[0] = WRAP;
		ADDRESSV[0] = WRAP;
		ADDRESSW[0] = WRAP;
		MAGFILTER[0] = LINEAR;
		MINFILTER[0] = LINEAR;
		MIPFILTER[0] = LINEAR;
		MAXMIPLEVEL[0] = 0;
		MIPMAPLODBIAS[0] = 0;
		TexCoordIndex[0] = 0;

		BW_TEXTURESTAGE_TERMINATE(1)
		CULLMODE = NONE;

		ALPHATESTENABLE = FALSE;
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = <srcBlend>;
		DESTBLEND = <destBlend>;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;		

		VertexShader = compile vs_1_1 vs_main(false);
		PixelShader = NULL;
	}
}