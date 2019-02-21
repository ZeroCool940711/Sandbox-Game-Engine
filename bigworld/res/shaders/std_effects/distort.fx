#include "stdinclude.fxh"

#ifdef IN_GAME

// Auto variables
float3 osCameraPos : CameraPosObjectSpace;
float4x4 worldViewProj : WorldViewProjection;
texture backBuffer : DistortionBuffer;

float4x4 world : World;
float4x4 viewProj : ViewProjection;

float time : Time;

texture normalisationMap : NormalisationMap;

VERTEX_FOG

// Exposed artist editable variables.
BW_ARTIST_EDITABLE_ALPHA_TEST
BW_ARTIST_EDITABLE_FRESNEL
BW_NON_EDITABLE_ADDITIVE_BLEND
BW_NON_EDITABLE_LIGHT_ENABLE
BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_REFLECTION_MAP
BW_ARTIST_EDITABLE_REFLECTION_AMOUNT
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

texture distortionMap 
< 
	bool artistEditable = true; 
	string UIName = "Distortion Map";
	string UIDesc = "The diffuse map for the material";
>;

bool useDiffuse
<
	bool artistEditable = true;
	string UIName = "Use Diffuse Map";
	string UIDesc = "Use Diffuse Map?";	
> = false;

bool useOverlay
<
	bool artistEditable = true;
	string UIName = "Use Overlayed Diffuse Map";
	string UIDesc = "Use Overlayed Diffuse Map?";	
> = false;

float scale
<
	bool artistEditable = true;
	string UIName = "Distortion Scale";
	string UIDesc = "Distortion Scale";
	float UIMin = 0;
	float UIMax = 1.0;
	int UIDigits = 5;
> = 0.01f;

bool useReflection
<
	bool artistEditable = true;
	string UIName = "Use Reflections";
	string UIDesc = "Use Reflection?";	
> = false;

float4 tint 
<
	bool artistEditable = true;
	string UIName = "tint";
	string UIDesc = "tint";
	string UIWidget = "Color";
> = {1,1,1,1};

float3 uTransform
<
	bool artistEditable = true;
	string UIName = "U Transform";
	string UIDesc = "The U-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0};

float3 vTransform
<
	bool artistEditable = true;
	string UIName = "V Transform";
	string UIDesc = "The V-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0};

sampler backbufferSampler = BW_SAMPLER(backBuffer, CLAMP)
sampler distortionSampler = BW_SAMPLER(distortionMap, BW_TEX_ADDRESS_MODE)
sampler diffuseSampler = BW_SAMPLER(diffuseMap, BW_TEX_ADDRESS_MODE)
samplerCUBE reflectionCubeSampler = BW_SAMPLER(reflectionMap, CLAMP)
sampler normalisationSampler = BW_SAMPLER( normalisationMap, CLAMP )

struct VSOutput
{
	float4 pos				: POSITION;
	float4 tc				: TEXCOORD0;
	float4 diffuse			: COLOR;
	float  fog				: FOG;
	float4 reflect_refract	: TEXCOORD1;
};

struct VSOutputReflect
{
	float4 pos				: POSITION;
	float4 tc				: TEXCOORD0;
	float4 diffuse			: COLOR;
	float  fog				: FOG;
	float4 reflect_refract	: TEXCOORD1;

	float4 tangentToCubeSpace0 : TEXCOORD2; //first row of the 3x3 transform from tangent to cube space
	float4 tangentToCubeSpace1 : TEXCOORD3; //second row of the 3x3 transform from tangent to cube space
	float4 tangentToCubeSpace2 : TEXCOORD4; //third row of the 3x3 transform from tangent to cube space	
};

#define CALCULATE_UVS( otc, itc )\
	float4 tc = float4(itc.xy, 1, 1);\
	otc.x = dot( tc, uTransform * float4(1,1,time,1) );\
	otc.y = dot( tc, vTransform * float4(1,1,time,1) );\
	otc.zw = o.tc.xy;

VSOutput vs_main( VertexXYZNUV input )
{
	VSOutput o = (VSOutput)0;

	CALCULATE_UVS(o.tc, input.tc);
	o.diffuse = float4(1,1,1,1);

	float4 projPos = mul( input.pos, worldViewProj );
	o.pos = projPos;
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	
	float2 tex;
	tex = (projPos.xy + projPos.w) * 0.5f;

	// Reflection transform
	o.reflect_refract.x = tex.x;
	o.reflect_refract.y = -tex.y;
	o.reflect_refract.z = projPos.w;
	o.reflect_refract.w = (1-projPos.z)*0.1;
	return o;
}

VSOutputReflect vs_main_reflect( VertexXYZNUVTB input )
{
	VSOutputReflect o = (VSOutputReflect)0;

//#if SKY_LIGHT_MAP_ENABLE	
//	float4 skyLightMap;
//	skyLightMap.xyz = worldPos.xyz - wsCameraPos.xyz;
//	skyLightMap.w = 1.0;
//	o.tc.z = dot( skyLightMap, skyLightMapTransform[0] );
//	o.tc.w = dot( skyLightMap, skyLightMapTransform[1] );	
//#endif	

	CALCULATE_UVS(o.tc, input.tc);
	o.diffuse = float4(1,1,1,1);

	float4 worldPos = mul(input.pos,world);	
	float4 projPos = mul(worldPos, viewProj);

	o.pos = projPos;
	o.fog = vertexFog(o.pos.w, fogStart, fogEnd);
	
	float2 tex;
	tex = (projPos.xy + projPos.w) * 0.5f;

	// Reflection transform
	o.reflect_refract.x = tex.x;
	o.reflect_refract.y = -tex.y;
	o.reflect_refract.z = projPos.w;
	o.reflect_refract.w = (1-projPos.z)*0.1;
	
	float3x3 tsMatrix = { input.tangent, input.binormal, input.normal };

	// Compute the 3x3 transform from tangent space to cube space:
	o.tangentToCubeSpace0.xyz = transpose(tsMatrix)[0].xyz;
	o.tangentToCubeSpace1.xyz = transpose(tsMatrix)[1].xyz;
	o.tangentToCubeSpace2.xyz = transpose(tsMatrix)[2].xyz;

	o.tangentToCubeSpace0.w = worldPos.x;
	o.tangentToCubeSpace1.w = worldPos.y;
	o.tangentToCubeSpace2.w = worldPos.z;
	return o;
}


VSOutput vs_mask( VertexXYZNUV input )
{
	VSOutput o = (VSOutput)0;
	o.diffuse = float4(1,1,1,1);
	o.pos = mul( input.pos, worldViewProj );
	return o;
}


//TODO: screen edge fading...
float4 ps_main( VSOutput i ) : COLOR
{
	float4 distortion = 2*tex2D( distortionSampler, i.tc.xy ) - 1;
	
	half ooW = 1.0f / i.reflect_refract.z;

	half2 screenCoords = i.reflect_refract.xy * ooW;

	//TODO: Fade out the distortion offset at the screen borders to avoid artifacts.
//	half3 screenFade = tex2D( screenFadeSampler, screenCoords.xy  );	
//	half4 dependentTexCoords = vN * scale * screenFade.xxxx;

	screenCoords.y = screenCoords.y + 1;
	float4 colourOriginal = tex2D( backbufferSampler, screenCoords );
	
	screenCoords += distortion.xy*scale;
	float4 colourDistorted = tex2D( backbufferSampler, screenCoords );
	float3 finalColour =	(colourDistorted.rgb * colourDistorted.a) +
							((1 - colourDistorted.a) * colourOriginal.rgb);

	return float4(finalColour*tint,1);
	
}



float4 ps_mask( VSOutput i ) : COLOR
{
	return float4(1,0,0,1);
}

//TODO: add some chromatic abberation options??  e.g.
// IE: refract each channel with different indices.
//	    refractColor.x =  refract(incident, normal, IoR_Values.x)).x;
//	    refractColor.y =  refract(incident, normal, IoR_Values.y)).y;
//	    refractColor.z =  refract(incident, normal, IoR_Values.z)).z;
//		<IoR_Values x="1.14" y="1.12" z="1.10"
float4 ps_main_diffuse( VSOutput i ) : COLOR
{
	float4 distortion = 2*tex2D( distortionSampler, i.tc.xy ) - 1;
	float4 diffuse = tex2D( diffuseSampler, i.tc.xy );
	
	half ooW = 1.0f / i.reflect_refract.z;

	half2 screenCoords = i.reflect_refract.xy * ooW;

	// Fade out the distortion offset at the screen borders to avoid artifacts.
//	half3 screenFade = tex2D( screenFadeSampler, screenCoords.xy  );	
//	half4 dependentTexCoords = vN * scale * screenFade.xxxx;

	screenCoords.y = screenCoords.y + 1;
	float4 colourOriginal = tex2D( backbufferSampler, screenCoords );

	screenCoords += distortion.xy*scale;
	float4 colourDistorted = tex2D( backbufferSampler, screenCoords );
	float3 finalColour =	(colourDistorted.rgb * colourDistorted.a) +
							((1 - colourDistorted.a) * colourOriginal.rgb);

	return float4(finalColour*tint*diffuse.xyz, diffuse.a);
	
}

//TODO: needs some lighting of the overlay material
float4 ps_main_diffuse_overlay( VSOutput i ) : COLOR
{
	float4 distortion = 2*tex2D( distortionSampler, i.tc.xy ) - 1;
	float4 diffuse = tex2D( diffuseSampler, i.tc.xy );
	
	half ooW = 1.0f / i.reflect_refract.z;

	half2 screenCoords = i.reflect_refract.xy * ooW;

	// Fade out the distortion offset at the screen borders to avoid artifacts.
//	half3 screenFade = tex2D( screenFadeSampler, screenCoords.xy  );	
//	half4 dependentTexCoords = vN * scale * screenFade.xxxx;

	screenCoords.y = screenCoords.y + 1;
	float4 colourOriginal = tex2D( backbufferSampler, screenCoords );

	screenCoords += distortion.xy*scale;
	float4 colourDistorted = tex2D( backbufferSampler, screenCoords );
	float3 finalColour =	(colourDistorted.rgb * colourDistorted.a) +
							((1 - colourDistorted.a) * colourOriginal.rgb);

	finalColour = lerp(finalColour*tint, diffuse.xyz, diffuse.a);

	return float4(finalColour, 1);	
}

float4 ps_main_reflect( VSOutputReflect i ) : COLOR
{
	float4 normal = 2*tex2D( distortionSampler, i.tc.xy ) - 1;

	// Perform division by W only once
	half ooW = 1.0f / i.reflect_refract.z;
	half2 screenCoords = i.reflect_refract.xy * ooW;

	screenCoords.y = screenCoords.y + 1;
	float4 colourOriginal = tex2D( backbufferSampler, screenCoords );

	
	screenCoords += normal.xy*scale;
	float4 colourDistorted = tex2D( backbufferSampler, screenCoords );
	float3 distColour =	(colourDistorted.rgb * colourDistorted.a) +
							((1 - colourDistorted.a) * colourOriginal.rgb);

	float4 colour = float4(distColour, 1);	

	// Calculate a reflection vector and fresnel term
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	envNormal = (texCUBE( normalisationSampler, envNormal ) * 2) - 1;
	float3 eyeVec = float3(	wsCameraPos.x - i.tangentToCubeSpace0.w,
							wsCameraPos.y - i.tangentToCubeSpace1.w, 
							wsCameraPos.z - i.tangentToCubeSpace2.w );
	eyeVec = (texCUBE( normalisationSampler, eyeVec ) * 2) - 1;
	float3 reflVec = reflect(eyeVec, envNormal);
	reflVec.y = -reflVec.y;

	float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec);
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant);
	fresnelTerm *= reflectionAmount;
	half3 finalColour = lerp( colour, reflectionColour, saturate(fresnelTerm) );
	return float4(finalColour*tint, 1);
}

float4 ps_main_reflect_diffuse( VSOutputReflect i ) : COLOR
{
	float4 normal = 2*tex2D( distortionSampler, i.tc.xy ) - 1;
	float4 diffuse = tex2D( diffuseSampler, i.tc.xy );

	// Perform division by W only once
	half ooW = 1.0f / i.reflect_refract.z;
	half2 screenCoords = i.reflect_refract.xy * ooW;

	screenCoords.y = screenCoords.y + 1;
	float4 colourOriginal = tex2D( backbufferSampler, screenCoords );
	
	screenCoords += normal.xy*scale;
	float4 colourDistorted = tex2D( backbufferSampler, screenCoords );
	float3 distColour =	(colourDistorted.rgb * colourDistorted.a) +
							((1 - colourDistorted.a) * colourOriginal.rgb);

	float4 colour = float4(distColour,1);

	// Calculate a reflection vector and fresnel term
    float3 envNormal;
	envNormal.x = dot(i.tangentToCubeSpace0.xyz, normal );
	envNormal.y = dot(i.tangentToCubeSpace1.xyz, normal );
	envNormal.z = dot(i.tangentToCubeSpace2.xyz, normal );
	envNormal = (texCUBE( normalisationSampler, envNormal ) * 2) - 1;
	float3 eyeVec = float3(	wsCameraPos.x - i.tangentToCubeSpace0.w,
							wsCameraPos.y - i.tangentToCubeSpace1.w, 
							wsCameraPos.z - i.tangentToCubeSpace2.w );
	eyeVec = (texCUBE( normalisationSampler, eyeVec ) * 2) - 1;
	float3 reflVec = reflect(eyeVec, envNormal);
	reflVec.y = -reflVec.y;

	float4 reflectionColour = texCUBE(reflectionCubeSampler, reflVec);
	half fresnelTerm = fresnel(eyeVec, envNormal, fresnelExp, fresnelConstant);
	fresnelTerm *= reflectionAmount * diffuse.a;
	half3 finalColour = lerp( colour, reflectionColour, saturate(fresnelTerm) );
	return float4(finalColour*tint*diffuse.xyz, diffuse.a);
}

VertexShader vertexShaders_vs2[3] =
{
	compile vs_2_0 vs_main(),
	compile vs_2_0 vs_main_reflect(),
	compile vs_2_0 vs_mask()
};

PixelShader pixelShaders_ps2[6] =
{
	compile ps_2_0 ps_main(),
	compile ps_2_0 ps_main_diffuse(),
	compile ps_2_0 ps_main_diffuse_overlay(),
	compile ps_2_0 ps_main_reflect(),
	compile ps_2_0 ps_main_reflect_diffuse(),
	compile ps_2_0 ps_mask()
};

int selectPixelShader()
{
	if (useOverlay)
	{
		return 2;
	}
	else
	{
		int index = (useReflection ? 3 : 0);
		return (useDiffuse ? index+1 : index );
	}
}

//--------------------------------------------------------------//
// Technique Section for SHADER_MODEL_2
//--------------------------------------------------------------//
technique standard2
<
	bool bumpMapped = true;
	string channel = "distortion";
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		BW_FOG
		BW_BLENDING_SOLID
		BW_CULL_DOUBLESIDED

		VertexShader = (vertexShaders_vs2[ useReflection ? 1 : 0 ]);
		PixelShader = (pixelShaders_ps2[selectPixelShader()]);
	}

	// DISORTION MASKING PASS: not draw to the main scene.
	pass Pass_1
	{
		FOGENABLE = FALSE;
		ALPHATESTENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		BW_CULL_DOUBLESIDED

		VertexShader = (vertexShaders_vs2[ 2 ]);
		PixelShader = (pixelShaders_ps2[ 5 ]);
	}
}
//--------------------------------------------------------------//
// Technique Section for SHADER_MODEL_1
//--------------------------------------------------------------//


//~ VertexShader vertexShaders_vs1[2] =
//~ {
	//~ compile vs_1_4 vs_main_sm1()
//~ };

//~ PixelShader pixelShaders_ps1[5] =
//~ {
	//~ compile ps_1_4 ps_main_sm1(),
	//~ compile ps_1_4 ps_mask_sm1()
//~ };

//~ int selectPixelShader_SM1()
//~ {
	//~ return (maskPass == true ? 1 : 0);
//~ }

//~ technique standard_SM1
//~ <
	//~ string channel = "distortion";
	//~ string label = "SHADER_MODEL_1";
//~ >
//~ {
	//~ pass Pass_0
	//~ {
		//~ ALPHATESTENABLE = <alphaTestEnable>;
		//~ ALPHAREF = <alphaReference>;
		//~ ALPHAFUNC = GREATER;
		//~ ALPHABLENDENABLE = FALSE;
		//~ SRCBLEND = ONE;
		//~ ZENABLE = TRUE;
		//~ ZWRITEENABLE = (maskPass ? 0 : 1);
		//~ ZFUNC = LESSEQUAL;
		//~ BW_CULL_DOUBLESIDED

		//~ VertexShader = (vertexShaders_vs1[ useReflection ? 1 : 0 ]);
		//~ PixelShader = (pixelShaders_ps1[ selectPixelShader_SM1() ]);
	//~ }
//~ }
#else
#include "lightonly_add.fx"
#endif //IN_GAME

