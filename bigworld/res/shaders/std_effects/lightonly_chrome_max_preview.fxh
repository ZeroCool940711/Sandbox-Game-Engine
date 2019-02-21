#include "max_preview_include.fxh"

texture otherMap
<
	bool artistEditable = true;
	string UIName = "Reflection Map";
	string UIDesc = "The reflection map for the material";
>;

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

float reflectionAmount
< 
	bool artistEditable = true;
	string UIName = "Reflection Amount";
	string UIDesc = "A scaling factor for the reflection";
	float UIMin = 0;
	float UIMax = 1.0;
	int UIDigits = 2;
> = 1.0;

BW_ARTIST_EDITABLE_FRESNEL
BW_ARTIST_EDITABLE_TEXTURE_OP

//Vertex Shader for lightonly_chrome
OutputDiffuseLighting3 vs_chrome( VertexXYZNUV input )
{
	OutputDiffuseLighting3 o = (OutputDiffuseLighting3)0;

	o.pos = mul(input.pos, worldViewProj);
	o.tc = o.tc3 = input.tc;
	float3 tc = float3(input.tc, 1);

	// calculate eye reflected around normal
	float3 cameraPos = worldViewInverse[3].xyz;
	float3 eye = normalize(cameraPos - input.pos);

	float d = dot( eye, input.normal );
	float3 eNormal = input.normal * d;
	float4 rEye = float4((eNormal - eye) + eNormal, 1);
	rEye.xyz = normalize( rEye.xyz );

	//transform eye reflected around normal to the coordinate system we want
	float4 ut = float4(mul( world, uTransform ).xyz, 1) * 0.5;
	float4 vt = float4(mul( world, -vTransform).xyz, 1) * 0.5;
	
	// output to extra texture coordinate
	o.tc2.x = dot( ut, rEye );
	o.tc2.y = dot( vt, rEye );
	
	o.sunlight.xyz =  float3(0.1, 0.1, 0.1) + selfIllumination;
	o.sunlight.a = fresnel( -normalize(eye), normalize(input.normal), fresnelExp, fresnelConstant ) * 2.0 * reflectionAmount;

	DirectionalLight dLight;
	dLight.colour = lightColour;
	dLight.direction = normalize(mul( lightDir.xyz, worldInverse ));

	o.sunlight.xyz += directionalLight( input.normal, dLight );

#ifdef MOD2X
	o.sunlight.xyz *= 0.5 * (1 + diffuseLightExtraModulation);
#endif
	return o;
}