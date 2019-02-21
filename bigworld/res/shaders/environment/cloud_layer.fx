#include "environment_helpers.fxh"

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

float4x4 environmentTransform : EnvironmentTransform;
float4x4 envShadowTransform : EnvironmentShadowTransform;
float4 cameraPos : CameraPos;
//control - (fogDensity, windMultiplier, ignored, fade in/out)
float4 control : SkyBoxController = {1,1,1,1};

struct OutputNormalMap
{
	float4 pos:     POSITION;
	float2 tc:      TEXCOORD0;
	float2 ntc:     TEXCOORD1;
	float3 dLight1: TEXCOORD2;
	float3 normal:	TEXCOORD3;
	float4 diffuse: COLOR0;	
};

float4 fogColour : FogColour = {0,0,0,0};
//NOTE - this is set to true because currently ME hard-disables fogging
//bool fogEnabled : FogEnabled = true;
bool fogEnabled = true;

BW_ARTIST_EDITABLE_CLOUD_MAP( diffuseMap, "Diffuse Map" )
BW_ARTIST_EDITABLE_FOG_MAP( fogMap, "Fog Map" )
BW_ARTIST_EDITABLE_RIM_DETECT_WIDTH( rimDetectWidth, "Rim Detect Width" )
BW_ARTIST_EDITABLE_RIM_DETECT_POWER( rimDetectPower, "Rim Detect Power" )
BW_ARTIST_EDITABLE_RIM_POWER( rimPower, "Rim Power" )
BW_ARTIST_EDITABLE_RIM_STRENGTH( rimStrength, "Rim Strength" )
BW_ARTIST_EDITABLE_SCATTERING_POWER( scatteringPower, "Scattering Power" )
BW_ARTIST_EDITABLE_SCATTERING_STRENGTH( scatteringStrength, "Scattering Strength" )
BW_ARTIST_EDITABLE_WIND_SPEED( windSpeed, "Wind Speed" )
BW_ARTIST_EDITABLE_TEXTURE_TILE( textureTile, "Texture Tile" )
BW_ARTIST_EDITABLE_SUN_FLARE_OCCLUSION( sunFlareOcclusion, "Sun Flare Occlusion" )
BW_ARTIST_EDITABLE_PARALLAX( parallax, "Vertical Parallax", xzParallax, "Horizontal Parallax" )
BW_ARTIST_EDITABLE_SHADOW_CONTRAST( shadowContrast, "Shadow Contrast" )

/*float yPoint
<
	bool artistEditable = true;
	string UIName = "Curvature";
	string UIDesc = "Curvature of sky layer";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = 0;
	int UIDigits = 2;
> = 0.0;*/

//This should be set to the max bounds of the object
float meshSize = 50.f;

//Note - yPoint used to created hacked normals,
//remove when normal modifier in max can be used instead
float yPoint = 0.0;

#include "unskinned_effect_include.fxh"

BW_CLOUD_MAP_SAMPLER( diffuseSampler, diffuseMap, WRAP )
BW_FOG_MAP_SAMPLER( fogSampler, fogMap )


OutputNormalMap vs_main( VertexXYZNUVTB i )
{
	OutputNormalMap o = (OutputNormalMap)0;
	
	//bodgy a smooth normal over the surface.  remove
	//when we can export the normals directly from max
	o.normal = normalize(float3(0,-yPoint,0) - i.pos);	
	
	//adjust input y for parallax
	i.pos.y -= cameraPos.y * parallax;	
	
	//this is just so we get the world space TS matrix
	PROJECT_POSITION(o.pos)
	CALCULATE_TS_MATRIX
	
	o.pos = mul(i.pos, environmentTransform).xyww;
	
	o.normal = mul( tsMatrix, o.normal );
	
	directionalLights[0].direction = -directionalLights[0].direction;
	directionalBumpLight( tsMatrix, directionalLights[0], o.dLight1 );
	
	float4 tc = float4(i.tc, 1, 1);
	o.tc = adjustTexCoords( i.tc, xzParallax, cameraPos.x, cameraPos.z );
	o.tc = cloudLayerTexCoords( o.tc, textureTile, windSpeed );
	o.ntc = tc;	
	
	//o.diffuse = directionalLights[0].colour + ambientColour;	
	//o.diffuse = fogColour;
	
	//calculate current luminance of sun (move to effect constant, not per-vertex)
	o.diffuse = dot( float3(0.3,0.59,0.11), directionalLights[0].colour.r ) + ambientColour;
	
	return o;
}


OutputNormalMap vs_shadows( VertexXYZNUVTB i )
{
	OutputNormalMap o = (OutputNormalMap)0;
		
	o.normal = (0,-1,0);	
	i.pos.xz = worldVertexPosition( i.pos.xz, meshSize );
	
	//this is just so we get the world space TS matrix
	PROJECT_POSITION(o.pos)
	CALCULATE_TS_MATRIX
	
	o.pos = mul(i.pos, envShadowTransform);
	o.pos.z = o.pos.w;		
	o.tc = adjustTexCoords( i.tc, xzParallax, cameraPos.x, cameraPos.z );
	o.tc = cloudLayerTexCoords( o.tc, textureTile, windSpeed );
	o.ntc = i.tc;	
	o.diffuse = (1,1,1,1);
	o.dLight1 = (0,0,0,0);
	
	return o;
}


float4 ps_main( OutputNormalMap i ) : COLOR0
{	
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float4 fogAmount;
	if (fogEnabled)
		fogAmount = tex2D( fogSampler, i.ntc );
	else
		fogAmount = float4(0,0,0,0);
	float3 normal = i.normal;	
	
	fogAmount = saturate( fogAmount + control.xxxx );			
			
	float3 light = saturate( dot( normalize(i.dLight1.xyz), normal ) );	
	float4 colour = cloudLighting(
							light,
							i.diffuse,
							diffuseMap,
							rimDetectWidth,
							rimDetectPower,
							scatteringPower,
							scatteringStrength,
							rimPower,
							rimStrength,
							fogColour,
							fogAmount );								
	colour.w *= control.w;
	return colour;
}


float4 ps_simple( OutputNormalMap i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	float4 fogAmount = tex2D( fogSampler, i.ntc );
	float4 colour = i.diffuse * diffuseMap;
	colour.w = diffuseMap.w;
	colour.xyz = lerp(colour.xyz, fogColour.xyz, fogAmount);
	colour.w *= control.w;
	return colour;
}


float4 ps_occlusion( OutputNormalMap i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	diffuseMap.xyz = 1.0;
	diffuseMap.w *= control.w;
	return diffuseMap;
}


float4 ps_shadowDraw( OutputNormalMap i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler, i.tc );
	diffuseMap.xyz = 0.0;
	diffuseMap.w *= diffuseMap.w;
	diffuseMap.w *= shadowContrast;
	diffuseMap.w *= control.w;	
	return diffuseMap;
}


#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)
PixelShader pixelShaders2[3] = 
{
	compile ps_2_0 ps_main(),
	compile ps_2_0 ps_occlusion(),
	compile ps_2_0 ps_shadowDraw()
};

VertexShader vertexShaders2[2] =
{
	compile vs_2_0 vs_main(),
	compile vs_2_0 vs_shadows(),
};

technique pixelShader2
<
	string label = "SHADER_MODEL_2";
>
{
   pass Pass_0
   {
      ALPHATESTENABLE = enableAlphaTest();
      ALPHAREF = (sunFlareOcclusion);
      ZENABLE = enableZ();
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      TexCoordIndex[0] = 0;
      TexCoordIndex[1] = 1;
      TexCoordIndex[2] = 2;
      TexCoordIndex[3] = 3;

      VertexShader = (vertexShaders2[vertexShaderIndex()]);
      PixelShader = (pixelShaders2[pixelShaderIndex()]);
   }
}
#endif


#if (COMPILE_SHADER_MODEL_1)
PixelShader pixelShaders1[3] = 
{
	compile ps_1_1 ps_simple(),
	compile ps_1_1 ps_occlusion(),
	compile ps_1_1 ps_shadowDraw()	
};

VertexShader vertexShaders1[2] =
{
	compile vs_1_1 vs_main(),
	compile vs_1_1 vs_shadows(),
};

technique pixelShader1
<
	string label = "SHADER_MODEL_1";
>
{
   pass Pass_0
   {
      ALPHATESTENABLE = enableAlphaTest();
      ALPHAREF = alphaReference();
      ZENABLE = enableZ();
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      TexCoordIndex[0] = 0;
      TexCoordIndex[1] = 1;
      TexCoordIndex[2] = 2;
      TexCoordIndex[3] = 3;

      VertexShader = (vertexShaders1[vertexShaderIndex()]);
      PixelShader = (pixelShaders1[pixelShaderIndex()]);
   }
}
#endif


technique pixelShader0
<
	string label = "SHADER_MODEL_0";
>
{
   pass Pass_0
   {
      ALPHATESTENABLE = enableAlphaTest();
      ALPHAREF = alphaReference();
      ZENABLE = TRUE;
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      TEXTUREFACTOR = <fogColour>;
      
	  COLOROP[0] = MODULATE;
	  COLORARG1[0] = TEXTURE;
	  COLORARG2[0] = DIFFUSE;
	  ALPHAOP[0] = SELECTARG1;
	  ALPHAARG1[0] = TEXTURE;
	  ALPHAARG2[0] = DIFFUSE;
	  Texture[0] = (diffuseMap);
	  ADDRESSU[0] = WRAP;
	  ADDRESSV[0] = WRAP;
	  ADDRESSW[0] = CLAMP;
	  MAGFILTER[0] = LINEAR;
	  MINFILTER[0] = LINEAR;
	  MIPFILTER[0] = LINEAR;
	  MAXMIPLEVEL[0] = 0;
	  MIPMAPLODBIAS[0] = 0;
	  TexCoordIndex[0] = 0;
	  
	  COLOROP[1] = LERP;
	  COLORARG0[1] = TEXTURE;
	  COLORARG1[1] = TFACTOR;
	  COLORARG2[1] = DIFFUSE;
	  ALPHAOP[1] = SELECTARG1;
	  ALPHAARG1[1] = CURRENT;
	  ALPHAARG2[1] = DIFFUSE;
	  Texture[1] = (fogMap);
	  ADDRESSU[1] = CLAMP;
	  ADDRESSV[1] = CLAMP;
	  ADDRESSW[1] = CLAMP;
	  MAGFILTER[1] = LINEAR;
	  MINFILTER[1] = LINEAR;
	  MIPFILTER[1] = LINEAR;
	  MAXMIPLEVEL[1] = 0;
	  MIPMAPLODBIAS[1] = 0;
	  TexCoordIndex[1] = 1;
	  
	  BW_TEXTURESTAGE_TERMINATE(2)

      VertexShader = (vertexShaders2[vertexShaderIndex()]);
      PixelShader = NULL;
   }
}


technique occlusionTestViz
<
	string label = "SHADER_MODEL_2";
>
{
   pass Pass_0
   {
      ALPHATESTENABLE = enableAlphaTest();
      ALPHAREF = (sunFlareOcclusion);
      ZENABLE = TRUE;
      SRCBLEND = ONE;
      DESTBLEND = ZERO;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      TexCoordIndex[0] = 0;
      TexCoordIndex[1] = 1;
      TexCoordIndex[2] = 2;
      TexCoordIndex[3] = 3;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_occlusion();
   }
}


technique shadowTestViz
<
	string label = "SHADER_MODEL_2";
>
{
   pass Pass_0
   {
      ALPHATESTENABLE = enableAlphaTest();
      ALPHAREF = 0;
      ZENABLE = FALSE;
      SRCBLEND = SRCALPHA;
      DESTBLEND = INVSRCALPHA;
      ZWRITEENABLE = FALSE;
      ZFUNC = LESSEQUAL;
      FOGENABLE = FALSE;
      ALPHABLENDENABLE = TRUE;      
      POINTSPRITEENABLE = FALSE;
      STENCILENABLE = FALSE;
      CULLMODE = NONE;
      
      TexCoordIndex[0] = 0;
      TexCoordIndex[1] = 1;
      TexCoordIndex[2] = 2;
      TexCoordIndex[3] = 3;

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_shadowDraw();
   }
}