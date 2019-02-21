#include "max_preview_include.fxh"

//Vertex Shader for lightonly
OutputDiffuseLighting2 vs_max( VertexXYZNUV i )
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;
	o.pos = mul( i.pos, worldViewProj );
	o.tc = i.tc;
	o.tc2 = i.tc;

	o.sunlight.xyz = float3(0.1, 0.1, 0.1) + selfIllumination;
	
	float3 lDir = normalize(mul( lightDir, worldInverse ));
	
#ifdef MOD2X
	o.sunlight.xyz += saturate(dot( lDir, i.normal )) * lightColour * 0.5 * (1 + diffuseLightExtraModulation);
#else
	o.sunlight += saturate(dot( lDir, i.normal )) * lightColour;
#endif
	o.sunlight.w = 1;
	return o;
}
