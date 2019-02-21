
//Helpers for doing chrome mapping.

float2 chromeMapCoords(
		in float3 worldPos,
		in float3 worldNormal,
		in float3 cameraPos,		
		in float3 uTransform,
		in float3 vTransform,
		out float3 worldEye )
{
	float2 tc2;
	
	// calculate eye reflected around normal
	worldEye = cameraPos - worldPos;
	float d = dot( worldEye, worldNormal );
	float3 eNormal = worldNormal * d;
	float4 rEye = float4((eNormal - worldEye) + eNormal, 1);
	rEye.xyz = normalize( rEye.xyz );
	
	//transform eye reflected around normal to the coordinate system we want
	float4 ut = float4(uTransform.xyz,1) * 0.5;
	float4 vt = float4(-vTransform.xyz,1)* 0.5;

	// output to extra texture coordinate
	tc2.x = dot( ut, rEye );
	tc2.y = dot( vt, rEye );
	
	return tc2;
}
