#ifndef LIGHTONLY_NORMAL_MAX_PREVIEW_FXH
#define LIGHTONLY_NORMAL_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"

DECLARE_OTHER_MAP( otherMap, otherSampler, "other map", "The texture map that will drawn using camera-normal projection" )

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

BW_ARTIST_EDITABLE_TEXTURE_OP

OutputDiffuseLighting2 vs_max( VertexXYZNUV input )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	o.pos = mul(input.pos, worldViewProj);
	o.tc = input.tc;
	float3 tc = float3(input.tc, 1);
	
	//transform normal to the coordinate system we want
	float4 ut = float4(mul( worldView, uTransform).xyz, 1) * 0.5;
	float4 vt = float4(mul( worldView, -vTransform).xyz, 1) * 0.5;
	
	// output to second texture coordinate
	o.tc2.x = dot( ut, float4(input.normal,1) );
	o.tc2.y = dot( vt, float4(input.normal,1) );
	
	float4 diffuse = float4(0.1, 0.1, 0.1, 1) + selfIllumination;
	
	DirectionalLight dLight;
	dLight.colour = lightColour;
	dLight.direction = normalize(mul( lightDir.xyz, worldInverse ));
	diffuse.xyz += directionalLight( input.normal, dLight );
	
	o.sunlight = diffuse;
	return o;
}

#endif