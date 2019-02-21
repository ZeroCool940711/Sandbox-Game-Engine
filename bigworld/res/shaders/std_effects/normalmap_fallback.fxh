//--------------------------------------------------------------//
// Fallback for software mode.
//--------------------------------------------------------------//
OutputDiffuseLighting2 diffuseOnlyFallback( VERTEX_FORMAT i)
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc = o.tc2 = i.tc;
	
	int nDirectionals = min( nDirectionalLights, 2 );
	int nPoints = min( nPointLights, 4 );
	int nSpots = min( nSpotLights, 2 );
		
	BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, worldPos, worldNormal, true );
	
	return o;
}

OutputDiffuseLighting2 diffuseOnlyFallbackStatic( STATIC_LIGHTING_VERTEX_FORMAT i)
{
	OutputDiffuseLighting2 o = (OutputDiffuseLighting2)0;

	PROJECT_POSITION( o.pos )
	o.fog = vertexFog( o.pos.w, fogStart, fogEnd );
	o.tc = o.tc2 = i.tc;
	
	int nDirectionals = min( nDirectionalLights, 2 );
	int nPoints = min( nPointLights, 4 );
	int nSpots = min( nSpotLights, 2 );
		
	BW_VERTEX_LIGHTING( o, i.diffuse, selfIllumination, worldPos, worldNormal, true );
	
	return o;
}

VertexShader fallbackShaders[2] =
{
	compile vs_2_0 diffuseOnlyFallback(),
	compile vs_2_0 diffuseOnlyFallbackStatic()
};
