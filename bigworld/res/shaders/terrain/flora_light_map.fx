#include "stdinclude.fxh"

// Auto variables
float fogStart : FogStart = 0.0;
float fogEnd : FogEnd = 1.0;
float4 fogColour : FogColour = {0,0,0,0};
float4 rcpPenumbra : PenumbraSize = {0.1,0.1,0.1,0.1};
float4 sunAngle : SunAngle = {0.5,0.5,0.5,0.5};
float4 textureTransform[2] : TerrainTextureTransform;

// variables
texture Layer0;
texture Layer1;
texture Layer2;
texture Layer3;
float3 axis0 = {0.0, 0.0, 1.0};
float3 axis1 = {0.0, 1.0, 0.0};
bool alphaTestEnable = false;
int alphaReference = 0;
matrix lightMapProjection;
matrix lightMapWorld;
matrix lightMapView;
matrix lightMapProj;
float4x4 textransform;
float	specMultiplier = 10.0;
float3 luminance = {0.3,0.59,0.11};

// this variable can be set at runtime, render/terrain/...
float specularDiffuseAmount = 0.2;

// Define the lighting type used in this shader
BW_DIFFUSE_LIGHTING

struct VS_INPUT
{
    float4 pos		: POSITION;
    float3 normal	: NORMAL;
    float4 blend	: COLOR0;
    float4 shadow	: COLOR1;
};

struct VS_OUTPUT
{
    float4 pos		: POSITION;
    float2 t0		: TEXCOORD0;
    float2 t1		: TEXCOORD1;
    float2 t2		: TEXCOORD2;
    float2 t3		: TEXCOORD3;
    float4 diffuse	: COLOR0;
    float4 blend	: COLOR1;    
};

//----------------------------------------------------------------------------
// Shader body 
//----------------------------------------------------------------------------

VS_OUTPUT vs_main(const VS_INPUT v)
{
	VS_OUTPUT o = (VS_OUTPUT) 0;
	
	o.pos = mul( v.pos, lightMapProjection );
	o.blend = v.blend;	
	
	float2 tc;
	tc.x = dot( v.pos, textureTransform[0] );
	tc.y = dot( v.pos, textureTransform[1] );
	o.t0.xy=tc.xy;
	o.t1.xy=tc.xy;
	o.t2.xy=tc.xy;
	o.t3.xy=tc.xy;
	
	o.diffuse.xyz = directionalLight( v.normal, directionalLights[0] );
	float2 angleDiff = (sunAngle - v.shadow) * rcpPenumbra;
	angleDiff = saturate( float2( angleDiff.x, -angleDiff.y ) );
	o.diffuse.xyz = ( o.diffuse.xyz * angleDiff.x * angleDiff.y )/* + ambientColour.xyz*/;
	o.diffuse.w = v.blend.x * 2 - 1;		
	
	return o;
};

sampler layer0Sampler = sampler_state
{
	Texture = (Layer0);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler layer1Sampler = sampler_state
{
	Texture = (Layer1);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler layer2Sampler = sampler_state
{
	Texture = (Layer2);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

sampler layer3Sampler = sampler_state
{
	Texture = (Layer3);
	ADDRESSU = WRAP;
	ADDRESSV = WRAP;
	ADDRESSW = WRAP;
	MAGFILTER = LINEAR;
	MINFILTER = LINEAR;
	MIPFILTER = LINEAR;
	MAXMIPLEVEL = 0;
	MIPMAPLODBIAS = 0;
};

/**
 * This pixel shader outputs :
 * RGB	- diffuse lighting only, no texture
 * A	- blended specular map incl. diffuse specular
 *
 * The resultant texture map will be used to light the
 * flora using RGB as terrain lighting, and the ALPHA
 * as a modulating source for the flora's vertex specular.
 */
float4 ps20_main( const VS_OUTPUT input ) : COLOR0
{	
	float4 layer0Map = tex2D( layer0Sampler, input.t0 );
	float4 layer1Map = tex2D( layer1Sampler, input.t1 );
	float4 layer2Map = tex2D( layer2Sampler, input.t2 );
	float4 layer3Map = tex2D( layer3Sampler, input.t3 );		
	float3 bx2Blend = 2.0*(input.blend.xyz-0.5);
	float blend0 = saturate(dot(bx2Blend,axis0));
	float blend1 = saturate(dot(bx2Blend,axis1));
	float4 colour;
	colour = blend0 * layer0Map;	
	colour += blend1 * layer1Map;
	colour += input.diffuse.a * layer2Map;
	colour += input.blend.a * layer3Map;
	float3 specDiffuse = specularDiffuseAmount * colour.xyz;
	float3 specLight = directionalLights[0].colour;
	colour.xyz += specMultiplier * colour.a * (specDiffuse+specLight);
	colour.a = saturate(dot(luminance,colour.xyz));
	colour.xyz = input.diffuse;	
	return colour;	
};

/**
 * This pixel shader outputs :
 * RGB	- diffuse lighting only, no texture
 * A	- blended specular map not incl. diffuse specular
 *
 * The resultant texture map will be used to light the
 * flora using RGB as terrain lighting, and the ALPHA
 * as a modulating source for the flora's vertex specular.
 *
 * As the ps1.1 terrain does not include a diffuse specular
 * component, we do not include it here either.
 */
float4 ps11_main( const VS_OUTPUT input ) : COLOR0
{	
	float4 layer0Map = tex2D( layer0Sampler, input.t0 );
	float4 layer1Map = tex2D( layer1Sampler, input.t1 );
	float4 layer2Map = tex2D( layer2Sampler, input.t2 );
	float4 layer3Map = tex2D( layer3Sampler, input.t3 );		
	float3 bx2Blend = 2.0*(input.blend.xyz-0.5);
	float blend0 = saturate(dot(bx2Blend,axis0));
	float blend1 = saturate(dot(bx2Blend,axis1));	
	float4 colour;
	colour.xyz = input.diffuse;
	colour = blend0 * layer0Map;
	colour += blend1 * layer1Map;
	colour += input.diffuse.a * layer2Map;
	colour += input.blend.a * layer3Map;
	colour.a = saturate(dot(luminance,colour.xyz));
	colour.xyz = input.diffuse;
	return colour;
};


//--------------------------------------------------------------//
// Technique Sections for shader hardware
//--------------------------------------------------------------//
technique shader_version_2_0
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		FOGENABLE = FALSE;
		SPECULARENABLE = TRUE;
		CULLMODE = NONE;
		ZENABLE = FALSE;
		COLORWRITEENABLE = RED | BLUE | GREEN | ALPHA;
		
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps20_main();
	}
}

technique shader_version_1_1
{
	pass Pass_0
	{
		BW_BLENDING_SOLID
		FOGENABLE = FALSE;
		SPECULARENABLE = TRUE;
		CULLMODE = NONE;
		ZENABLE = FALSE;
		COLORWRITEENABLE = RED | BLUE | GREEN | ALPHA;
		
		VertexShader = compile vs_1_1 vs_main();
		PixelShader = compile ps_1_1 ps11_main();
	}	
}


//--------------------------------------------------------------//
// Technique Section for fixed function hardware.
//--------------------------------------------------------------//
technique fixedFunction
{
	pass Pass_0
	{
		VertexShader = NULL;
		PixelShader = NULL;
		
		ALPHATESTENABLE = FALSE;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;		
		WORLDTRANSFORM[0] = <lightMapWorld>;
		VIEWTRANSFORM = <lightMapView>;
		PROJECTIONTRANSFORM = <lightMapProj>;
		SPECULARENABLE = FALSE;
		ZENABLE = FALSE;
		LIGHTING = TRUE;
		CULLMODE = NONE;
				
		/*LightType[0] = DIRECTIONAL;
		LIGHTATTENUATION0[0] = 1.0;
		LIGHTATTENUATION1[0] = 0.0;
		LIGHTATTENUATION2[0] = 0.0;
		LIGHTDIRECTION[0] = <-directionalLights[0].direction>;
		LIGHTENABLE[0] = TRUE;
		LIGHTDIFFUSE[0] = <directionalLights[0].colour>;
		LIGHTENABLE[1] = FALSE;*/
		
		DIFFUSEMATERIALSOURCE = MATERIAL;
		MATERIALEMISSIVE = <ambientColour>;
		MATERIALDIFFUSE = (1,1,1,1);
				
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = DIFFUSE;
		COLORARG2[0] = TFACTOR;
		COLOROP[1] = DISABLE;
		ALPHAOP[1] = DISABLE;		
	}
}