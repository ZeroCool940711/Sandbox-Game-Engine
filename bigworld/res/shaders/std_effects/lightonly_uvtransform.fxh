#ifndef LIGHTONLY_UVTRANSFORM_FXH
#define LIGHTONLY_UVTRANSFORM_FXH

// Auto variables
float time : Time;

#ifdef TRANSFORM_DIFFUSE_UV
float4 uTransform_diffuse
<
	bool artistEditable = true;
	string UIName = "U Transform Diffuse";
	string UIDesc = "The U-transform vector for the diffuse material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0,0};

float4 vTransform_diffuse
<
	bool artistEditable = true;
	string UIName = "V Transform Diffuse";
	string UIDesc = "The V-transform vector for the diffuse material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0,0};
#endif

float4 uTransform
<
	bool artistEditable = true;
	string UIName = "U Transform";
	string UIDesc = "The U-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {1,0,0,0};

float4 vTransform
<
	bool artistEditable = true;
	string UIName = "V Transform";
	string UIDesc = "The V-transform vector for the material";
	string UIWidget = "Spinner";
	float UIMax = 100;
	float UIMin = -100;
> = {0,1,0,0};

BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)

// Project UVs for lightonly_projection shader, this must be included before 
// lightonly_2uv.fxh because it is used in the vs_main defined there.

#ifdef TRANSFORM_DIFFUSE_UV
#define CALCULATE_UVS( o, itc, worldPos, worldNormal )\
{\
	float4 tc = float4(itc, 1, 1);\
	o.tc2.x = dot( tc, uTransform * float4(1,1,time,1) );\
	o.tc2.y = dot( tc, vTransform * float4(1,1,time,1) );\
	o.tc.x = dot( tc, uTransform_diffuse * float4(1,1,time,1) );\
	o.tc.y = dot( tc, vTransform_diffuse * float4(1,1,time,1) );\
}
#else
#define CALCULATE_UVS( o, itc, worldPos, worldNormal )\
{\
	o.tc = itc;\
	float4 tc = float4(itc, 1, 1);\
	o.tc2.x = dot( tc, uTransform * float4(1,1,time,1) );\
	o.tc2.y = dot( tc, vTransform * float4(1,1,time,1) );\
}
#endif

DECLARE_OTHER_MAP( otherMap, otherSampler, "Other Map", "The other map for the material, to which the uv transform will be applied." )

#endif