#ifndef LIGHTONLY_OTHER_FXH
#define LIGHTONLY_OTHER_FXH

// Auto variables
float4x4 worldView : WorldView;
float time : Time;

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

// Project UVs for lightonly_other shader, this must be included before 
// lightonly_2uv.fxh because it is used in the vs_main defined there.
#define CALCULATE_UVS( o, itc, worldPos, worldNormal )\
{\
	o.tc = itc;\
	float4 ut = float4(mul(worldView, uTransform).xyz, 1) * 0.5;\
	float4 vt = float4(mul(worldView, -vTransform).xyz, 1) * 0.5;\
	o.tc2.x = dot( ut, float4(i.normal,1) );\
	o.tc2.y = dot( vt, float4(i.normal,1) );\
}

#endif