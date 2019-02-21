/**
 *	This include file has functions to help with calculating fog
 */

#define VERTEX_FOG \
float fogStart : FogStart = 0.0; \
float fogEnd : FogEnd = 1.0; \
float4 fogColour : FogColour = {0,0,0,0};

float vertexFog( in float wPos, in float fogStart, in float fogEnd )
{
	float2 fogging = float2((-1.0 / (fogEnd - fogStart)), (fogEnd / (fogEnd - fogStart)));
	return wPos * fogging.x + fogging.y;
}