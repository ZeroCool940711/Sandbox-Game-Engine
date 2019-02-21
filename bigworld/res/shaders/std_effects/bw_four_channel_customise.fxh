//BW_FOUR_CHANNEL_COLOURISE

#define BW_FOUR_CHANNEL_COLOURISE(pythonName1, uiName1, desc1, pythonName2, uiName2, desc2, pythonName3, uiName3, desc3, pythonName4, uiName4, desc4)\
texture maskMap\
<\
	bool artistEditable = true;\
	string UIName = "Mask Map";\
	string UIDesc = "The mask map to select colour customisation regions";\
>;\
\
float4 pythonName1\
<\
	bool artistEditable = true;\
	string UIWidget = "Color";\
	string UIName = uiName1;\
	string UIDesc = desc1;\
> = {1,1,1,1};\
\
float4 pythonName2\
<\
	bool artistEditable = true;\
	string UIWidget = "Color";\
	string UIName = uiName2;\
	string UIDesc = desc2;\
> = {1,1,1,1};\
\
float4 pythonName3\
<\
	bool artistEditable = true;\
	string UIWidget = "Color";\
	string UIName = uiName3;\
	string UIDesc = desc3;\
> = {1,1,1,1};\
\
float4 pythonName4\
<\
	bool artistEditable = true;\
	string UIWidget = "Color";\
	string UIName = uiName4;\
	string UIDesc = desc4;\
> = {1,1,1,1};\
\
sampler maskSampler = sampler_state\
{\
	Texture = (maskMap);\
	ADDRESSU = BW_TEX_ADDRESS_MODE;\
	ADDRESSV = BW_TEX_ADDRESS_MODE;\
	ADDRESSW = BW_TEX_ADDRESS_MODE;\
	MAGFILTER = POINT;\
	MINFILTER = LINEAR;\
	MIPFILTER = LINEAR;\
	MAXMIPLEVEL = 0;\
	MIPMAPLODBIAS = 0;\
};\
\
\
float4 colouriseDiffuseMap( sampler diffuseSampler, float2 diffuseTC, float2 maskTC )\
{\
	float4 diffuseMap = tex2D( diffuseSampler, diffuseTC );\
	float4 maskMap = tex2D( maskSampler, maskTC );\
\
	float3 customColour1 = maskMap.r * pythonName1.rgb;\
	float3 customColour2 = maskMap.g * pythonName2.rgb;\
	float3 customColour3 = maskMap.b * pythonName3.rgb;\
	float3 customColour4 = maskMap.a * pythonName4.rgb;\
	float3 colour = customColour1 + customColour2 + customColour3 + customColour4;\
\
	float colouriseAmount = dot( float4(1,1,1,1), maskMap );\
	float diffuseAmount = 1.0 - colouriseAmount;\
	diffuseMap.xyz = (diffuseAmount * diffuseMap.xyz) + (colouriseAmount * (colour.xyz * diffuseMap.xyz));\
	return diffuseMap;\
};\
float4 colouriseDiffuseMap_ps11( sampler diffuseSampler, float2 diffuseTC, float2 maskTC )\
{\
	float4 diffuse = tex2D( diffuseSampler, diffuseTC );\
	float4 mask = tex2D( maskSampler, maskTC );\
	float3 colouriseAmount = saturate(dot( float4(1,1,0,1), mask ));\
	float3 colour = mask.r * pythonName1.rgb;\
	colour += mask.g * pythonName2.rgb;\
	colour += mask.a * pythonName4.rgb;\
	float diffuseAmount = 1.0 - colouriseAmount;\
	diffuse.rgb = diffuseAmount + colour * colouriseAmount;\
	return diffuse;\
};
