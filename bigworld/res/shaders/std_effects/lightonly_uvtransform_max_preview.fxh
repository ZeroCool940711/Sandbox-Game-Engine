#ifndef LIGHTONLY_UVTRANSFORM_MAX_PREVIEW_FXH
#define LIGHTONLY_UVTRANSFORM_MAX_PREVIEW_FXH

#include "max_preview_include.fxh"

DECLARE_OTHER_MAP( otherMap, otherSampler, "other map", "The texture map that will be transformed" )

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

float time : Time;

OutputDiffuseLighting2 vs_max( VertexXYZNUV input )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	o.pos = mul(input.pos, worldViewProj);
	o.tc = input.tc;

	float4 tc = float4(input.tc, 1, 1);
	o.tc2.x = dot( tc, uTransform * float4(1,1,time,1) );
	o.tc2.y = dot( tc, vTransform * float4(1,1,time,1) );
	
	float4 diffuse = float4(0.1, 0.1, 0.1, 1) + selfIllumination;
	
	DirectionalLight dLight;
	dLight.colour = lightColour;	
	dLight.direction = normalize(mul( lightDir.xyz, worldInverse ));
	diffuse.xyz += directionalLight( input.normal, dLight );
	
#ifdef MOD2X
	diffuse.xyz *= 0.5 * (1 + diffuseLightExtraModulation);
#endif
	
	o.sunlight = diffuse;
	return o;
}

#endif