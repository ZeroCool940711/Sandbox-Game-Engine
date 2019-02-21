#include "environment_helpers.fxh"

// This constant is read by the tools to know if it should setup a rendering
// environment that is appropriate for a skybox.
bool isBWSkyBox = true;

float4x4 environmentTransform : EnvironmentTransform;
float4x4 envShadowTransform : EnvironmentShadowTransform;
float4 cameraPos : CameraPos;
//control - (fogDensity, ignored, ignored, fade in/out)
float4 control : SkyBoxController = {1,1,1,1};
float4 fogColour : FogColour = {0,0,0,0};
//NOTE - this is set to true because currently ME hard-disables fogging
//bool fogEnabled : FogEnabled = true;
bool fogEnabled = true;
#include "unskinned_effect_include.fxh"

struct OutputVertex
{
	float4 pos:     POSITION;
	float2 tc1:     TEXCOORD0;
	float2 ntc:     TEXCOORD1;
	float2 tc2:     TEXCOORD2;
	float3 dLight1: TEXCOORD3;
	float3 normal:	TEXCOORD4;
	float4 diffuse: COLOR0;	
};

BW_ARTIST_EDITABLE_CLOUD_MAP( diffuseMap1, "Upper Diffuse Map" )
BW_ARTIST_EDITABLE_FOG_MAP( fogMap1, "Upper Fog Map" )
BW_ARTIST_EDITABLE_RIM_DETECT_WIDTH( rimDetectWidth1, "Upper Rim Detect Width" )
BW_ARTIST_EDITABLE_RIM_DETECT_POWER( rimDetectPower1, "Upper Rim Detect Power" )
BW_ARTIST_EDITABLE_RIM_POWER( rimPower1, "Upper Rim Power" )
BW_ARTIST_EDITABLE_RIM_STRENGTH( rimStrength1, "Upper Rim Strength" )
BW_ARTIST_EDITABLE_SCATTERING_POWER( scatteringPower1, "Upper Scattering Power" )
BW_ARTIST_EDITABLE_SCATTERING_STRENGTH( scatteringStrength1, "Upper Scattering Strength" )
BW_ARTIST_EDITABLE_WIND_SPEED( windSpeed1, "Upper Wind Speed" )
BW_ARTIST_EDITABLE_TEXTURE_TILE( textureTile1, "Upper Texture Tile" )
BW_ARTIST_EDITABLE_SUN_FLARE_OCCLUSION( sunFlareOcclusion1, "Upper Sun Flare Occlusion" )
BW_ARTIST_EDITABLE_PARALLAX( parallax1, "Upper Vertical Parallax", xzparallax1, "Upper Horizontal Parallax" )
BW_ARTIST_EDITABLE_SHADOW_CONTRAST( upperShadowContrast, "Upper Shadow Contrast" )

BW_ARTIST_EDITABLE_CLOUD_MAP( diffuseMap2, "Lower Diffuse Map" )
BW_ARTIST_EDITABLE_FOG_MAP( fogMap2, "Lower Fog Map" )
BW_ARTIST_EDITABLE_RIM_DETECT_WIDTH( rimDetectWidth2, "Lower Rim Detect Width" )
BW_ARTIST_EDITABLE_RIM_DETECT_POWER( rimDetectPower2, "Lower Rim Detect Power" )
BW_ARTIST_EDITABLE_RIM_POWER( rimPower2, "Lower Rim Power" )
BW_ARTIST_EDITABLE_RIM_STRENGTH( rimStrength2, "Lower Rim Strength" )
BW_ARTIST_EDITABLE_SCATTERING_POWER( scatteringPower2, "Lower Scattering Power" )
BW_ARTIST_EDITABLE_SCATTERING_STRENGTH( scatteringStrength2, "Lower Scattering Strength" )
BW_ARTIST_EDITABLE_WIND_SPEED( windSpeed2, "Lower Wind Speed" )
BW_ARTIST_EDITABLE_TEXTURE_TILE( textureTile2, "Lower Texture Tile" )
BW_ARTIST_EDITABLE_SUN_FLARE_OCCLUSION( sunFlareOcclusion2, "Lower Sun Flare Occlusion" )
BW_ARTIST_EDITABLE_PARALLAX( parallax2, "Lower Vertical Parallax", xzparallax2, "Lower Horizontal Parallax" )
BW_ARTIST_EDITABLE_SHADOW_CONTRAST( lowerShadowContrast, "Lower Shadow Contrast" )

BW_CLOUD_MAP_SAMPLER( diffuseSampler1, diffuseMap1, WRAP )
BW_FOG_MAP_SAMPLER( fogSampler1, fogMap1 )

BW_CLOUD_MAP_SAMPLER( diffuseSampler2, diffuseMap2, WRAP )
BW_FOG_MAP_SAMPLER( fogSampler2, fogMap2 )

//Note - yPoint used to created hacked normals,
//remove when normal modifier in max can be used instead
float yPoint = 0.0;

//This should be set to the max bounds of the object
float meshSize = 50.f;


OutputVertex vs_main( VertexXYZNUVTB i )
{
	OutputVertex o = (OutputVertex)0;
	
	//bodgy a smooth normal over the surface.  remove
	//when we can export the normals directly from max
	o.normal = normalize(float3(0,-yPoint,0) - i.pos);	
	
	//adjust input y for parallax
	i.pos.y -= cameraPos.y * parallax1;	
	
	//this is just so we get the world space TS matrix
	PROJECT_POSITION(o.pos)
	CALCULATE_TS_MATRIX
	
	o.pos = mul(i.pos, environmentTransform).xyww;
	
	o.normal = mul( tsMatrix, o.normal );
	
	directionalLights[0].direction = -directionalLights[0].direction;
	directionalBumpLight( tsMatrix, directionalLights[0], o.dLight1 );
		
	o.tc1 = adjustTexCoords( i.tc, xzparallax1, cameraPos.x, cameraPos.z );
	o.tc2 = adjustTexCoords( i.tc, xzparallax2, cameraPos.x, cameraPos.z );
	o.tc1 = cloudLayerTexCoords( o.tc1, textureTile1, windSpeed1 );
	o.tc2 = cloudLayerTexCoords( o.tc2, textureTile2, windSpeed2 );
	o.ntc = i.tc;	
	
	//o.diffuse = directionalLights[0].colour + ambientColour;	
	//o.diffuse = fogColour;
	
	//calculate current luminance of sun (move to effect constant, not per-vertex)
	o.diffuse = dot( float3(0.3,0.59,0.11), directionalLights[0].colour.r ) + ambientColour;
	
	return o;
}


OutputVertex vs_shadows( VertexXYZNUVTB i )
{
	OutputVertex o = (OutputVertex)0;
	
	o.normal = (0,-1,0);	
	i.pos.xz = worldVertexPosition( i.pos.xz, meshSize );
	
	//this is just so we get the world space TS matrix
	PROJECT_POSITION(o.pos)
	CALCULATE_TS_MATRIX
	
	o.pos = mul(i.pos, envShadowTransform);
	o.pos.z = o.pos.w;	
	o.tc1 = adjustTexCoords( i.tc, xzparallax1, cameraPos.x, cameraPos.z );
	o.tc2 = adjustTexCoords( i.tc, xzparallax2, cameraPos.x, cameraPos.z );	
	o.tc1 = cloudLayerTexCoords( o.tc1, textureTile1, windSpeed1 );
	o.tc2 = cloudLayerTexCoords( o.tc2, textureTile2, windSpeed2 );
	
	o.ntc = i.tc;
	o.diffuse = (1,1,1,1);
	o.dLight1 = (0,0,0,0);
	
	return o;
}


float4 ps_main( OutputVertex i ) : COLOR0
{	
	float4 diffuseMap1 = tex2D( diffuseSampler1, i.tc1 );
	float4 diffuseMap2 = tex2D( diffuseSampler2, i.tc2 );
	float4 fogAmount1 = tex2D( fogSampler1, i.ntc );	
	float4 fogAmount2 = tex2D( fogSampler2, i.ntc );	
	float3 normal = i.normal;
			
	fogAmount1 = saturate( fogAmount1 + control.xxxx );			
	fogAmount2 = saturate( fogAmount1 + control.xxxx );
	
	float3 light = saturate( dot( normalize(i.dLight1.xyz), normal ) );	
	float4 colour1 = cloudLighting( light, i.diffuse, diffuseMap1, rimDetectWidth1, rimDetectPower1, scatteringPower1, scatteringStrength1, rimPower1, rimStrength1, fogColour, fogAmount1 );
	float4 colour2 = cloudLighting( light, i.diffuse, diffuseMap2, rimDetectWidth2, rimDetectPower2, scatteringPower2, scatteringStrength2, rimPower2, rimStrength2, fogColour, fogAmount2 );
	
	float4 colour = lerp( colour1, colour2, colour2.w );
	
	colour.w = saturate( colour.w + fogAmount2 * fogAmount2 );
	colour.w *= control.w;
			
	return colour;
}


float4 ps_simple( OutputVertex i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler1, i.tc1 );
	float4 fogAmount = tex2D( fogSampler1, i.ntc );
	float4 colour = i.diffuse * diffuseMap;
	colour.w = diffuseMap.w;
	colour.xyz = lerp(colour.xyz, fogColour.xyz, fogAmount);
	colour.w *= control.w;
	return colour;
}


float4 ps_occlusion( OutputVertex i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler1, i.tc1 );
	float4 diffuseMap2 = tex2D( diffuseSampler2, i.tc2 );
	float4 colour = lerp( diffuseMap, diffuseMap2, diffuseMap2.w );
	colour.w *= control.w;
	return colour;
}


float4 ps_shadowDraw( OutputVertex i ) : COLOR0
{
	float4 diffuseMap = tex2D( diffuseSampler1, i.tc1 );
	float4 diffuseMap2 = tex2D( diffuseSampler2, i.tc2 );
	diffuseMap.w *= diffuseMap.w;
	diffuseMap.w *= upperShadowContrast;
	diffuseMap2.w *= diffuseMap2.w;
	diffuseMap2.w *= lowerShadowContrast;
	
	float4 colour = (1,1,1,0);
	colour.w = diffuseMap.w + diffuseMap2.w;
	colour.w *= control.w;
	return colour;
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
      ALPHATESTENABLE = TRUE;
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
      ALPHATESTENABLE = TRUE;
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
      ALPHATESTENABLE = TRUE;
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
      TEXTUREFACTOR = <fogColour>;
      
	  COLOROP[0] = MODULATE;
	  COLORARG1[0] = TEXTURE;
	  COLORARG2[0] = DIFFUSE;
	  ALPHAOP[0] = SELECTARG1;
	  ALPHAARG1[0] = TEXTURE;
	  ALPHAARG2[0] = DIFFUSE;
	  Texture[0] = (diffuseMap1);
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
	  Texture[1] = (fogMap1);
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
      ALPHATESTENABLE = TRUE;
      ALPHAREF = (sunFlareOcclusion1);
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

      VertexShader = compile vs_2_0 vs_main();
      PixelShader = compile ps_2_0 ps_shadowDraw();
   }
}
