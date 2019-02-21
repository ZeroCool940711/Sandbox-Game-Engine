#ifndef LIGHTING_HELPERS_FXH
#define LIGHTING_HELPERS_FXH

/**
 *	This include file contains methods to help light vertices and pixels.
 */
 
//-----------------------------------------------
//	Lighting structures
//-----------------------------------------------
 
struct DirectionalLight
{
	float3 direction;
	float4 colour;
};

struct PointLight
{
	float3 position;
	float4 colour;
	float2 attenuation;
};

struct SpotLight
{
	float3 position;
	float4 colour;
	float3 attenuation;
	float3 direction;
};

//-----------------------------------------------
// Lighting defines
//-----------------------------------------------
#define BW_DIFFUSE_LIGHTING \
float4 ambientColour : Ambient; \
int nDirectionalLights : DirectionalLightCount; \
int nPointLights : PointLightCount; \
int nSpotLights : SpotLightCount;
DirectionalLight directionalLights[2] : DirectionalLights; \
PointLight pointLights[4] : PointLights; \
SpotLight spotLights[2] : SpotLights;

#define BW_SPECULAR_LIGHTING \
int nSpecularDirectionalLights : SpecularDirectionalLightCount; \
int nSpecularPointLights : SpecularPointLightCount;
DirectionalLight specularDirectionalLights[2] : SpecularDirectionalLights; \
PointLight specularPointLights[2] : SpecularPointLights;


//Lighting priority macros:
//	The lighting priority is setup so the per pixel light selection will
//	grab the primary directional light source (sunlight) first and then
//	any spot lights and points lights (in that order).
// 	There is a maximum of 2 diffuse and 2 specular pp lights.	
#define BW_CALC_LIGHT_COUNTS \
	int nPointBump; \
	if (nDirectionals < 1) \
		nPointBump = min( nPoints, 2 ); \
	else \
		nPointBump = min( nPoints, 1 ); \
	int nDirBump = min(2 - nPointBump, min(2, nDirectionals) ); \
	int nPoint = min( max(nPoints - nPointBump, 0), 4); \
	int nDir = min(max(nDirectionals - nDirBump, 0), 2 );
	

#define BW_CALC_LIGHT_COUNTS_SPOT \
	int nPointBump; \
	if (nDirectionals < 1) \
		nPointBump = min( nPoints, 1 ); \
	else \
		nPointBump = 0; \
	int nDirBump = min(1 - nPointBump, min(1, nDirectionals) ); \
	int nPoint = min( max(nPoints - nPointBump, 0), 3); \
	int nDir = min(max(nDirectionals - nDirBump, 0), 2 ); \
	int nSpot = min( max(nSpotLights - 1, 0), 2 );


#define BW_CALC_SPEC_LIGHT_COUNTS \
	int nPointSpecBump; \
	if (nSpecularDirectionals<1) \
		nPointSpecBump = min( nSpecularPoints, 2 ); \
	else \
		nPointSpecBump = min( nSpecularPoints, 1 ); \
	int nDirSpecBump = min(2 - nPointSpecBump, min(2, nSpecularDirectionals) );

#define BW_CALC_SPEC_LIGHT_COUNTS_VS1 \
	int nPointSpecBump; \
	if (nSpecularDirectionals<1) \
		nPointSpecBump = min( nSpecularPoints, 1 ); \
	else \
		nPointSpecBump = 0; \
	int nDirSpecBump = min(1 - nPointSpecBump, min(1, nSpecularDirectionals) );

float3 normalisedEyeVector( in float3 pos, in float3 cameraPos )
{
	return normalize(cameraPos - pos);
}


//-----------------------------------------------
// Lighting Code
//-----------------------------------------------

// For using Mod2X lighting
float3 mod2LightAdjust( float4 colour, float diffuseLightExtraModulation )
{
#ifdef MOD2X
	return colour.xyz * 0.5 * (1 + diffuseLightExtraModulation);
#else
	return colour.xyz;
#endif
}


// Directional lights
float3 directionalLight( in float3 normal, in DirectionalLight light )
{
	return saturate(dot(light.direction, normal )) * light.colour.xyz;
}


float directionalBumpLight( in float3x3 tsMatrix, in DirectionalLight light, out float3 tsLightDir )
{
	tsLightDir = mul( tsMatrix, light.direction ); // transform light direction to texture space
	return 1; // return attenuation, always 1 for directional lights
}


float directionalSpecBumpLight( in float3 eye, in float3x3 tsMatrix, in DirectionalLight light, out float3 tsLightDir )
{
	float3 h = light.direction + eye;
	tsLightDir = mul( tsMatrix, h );
	return 1;
}


float3 directionalSpecLight( in float3 normal, in float3 eye, in DirectionalLight light )
{
	float3 h = normalize(eye + light.direction);
	float att = saturate(dot(normal, h));
	att = pow(att,32);
	return att * light.colour;
}


// Point lights
float3 pointLight( in float3 position, in float3 normal, in PointLight light )
{
	float3 lDir = normalize( light.position - position );
	float	distance = dot( light.position - position, lDir );
	float2 att = {(-distance + light.attenuation.x) * light.attenuation.y, dot( lDir, normal )};
	att = saturate(att);
	return att.x * att.y * light.colour;
}


float pointBumpLight( in float3 position, in float3x3 tsMatrix, in PointLight light, out float3 tsLightDir )
{
	float3 dir = light.position - position;
	tsLightDir = mul( tsMatrix, dir );
	
	return saturate((-length( dir ) + light.attenuation.x) * light.attenuation.y);
}


float pointOrDirBumpLight( in float3 position,	in float3x3 tsMatrix, 
	in PointLight pLight, in DirectionalLight dLight, out float3 tsLightDir,
	int pointLight )
{
	float att;
	float3 dir;

	dir = (pLight.position - position) * pointLight;
	att = saturate((-length( dir ) + pLight.attenuation.x) * pLight.attenuation.y) * pointLight;

	dir += dLight.direction * (1 - pointLight);
	att += 1 - pointLight;

	tsLightDir = mul( tsMatrix, dir );
	return att;
}


float pointSpecBumpLight( in float3 position, in float3 eye, in float3x3 tsMatrix, in PointLight light, out float3 tsLightDir )
{
	float3 lightVector = light.position - position;
	float lightLen = length( lightVector );
	lightVector /= lightLen;
	
	float3 h = lightVector + eye;
	
	tsLightDir = mul( tsMatrix, h );
	
	return saturate((-lightLen + light.attenuation.x) * light.attenuation.y);
}


float pointOrDirSpecBumpLight( in float3 position, in float3 eye, 
	in float3x3 tsMatrix, in PointLight pLight, 
	in DirectionalLight dLight, out float3 tsLightDir,
	int pointLight )
{
	float3	lDir;
	float	lAtt;
	float3 l = pLight.position - position;
	lDir = normalize(l) * pointLight;
	float lightLen = dot( l, lDir );
	lAtt = saturate((-lightLen + pLight.attenuation.x) * pLight.attenuation.y) * pointLight;
	lDir += dLight.direction * (1 - pointLight);
	lAtt += 1-pointLight;

	float3 h = lDir + eye;
	tsLightDir = mul( tsMatrix, h );
	return lAtt;
}


float3 pointSpecLight( in float3 position, in float3 normal, in float3 eye,
	in PointLight light )
{
	float3 lightDir = light.position - position;
	float lightLen = length( lightDir );
	lightDir /= lightLen;

	float3 h = normalize(eye + lightDir);
	float att = saturate(dot(normal, h));
	att = pow(att,32);
	return saturate((-lightLen + light.attenuation.x) * light.attenuation.y) * att * light.colour;
	
}


// Spot lights
float3 spotLight( in float3 position, in float3 normal, in SpotLight light )
{
	float3 lDir = normalize( light.position - position );
	float	distance = dot( light.position - position, lDir );
	
	float3 att = {(-distance + light.attenuation.x) * light.attenuation.y, 
				dot( light.direction, normal ),  
				(dot( light.direction, lDir ) -light.attenuation.z) / (1 - light.attenuation.z) };
	att = saturate(att);
	return att.x * att.y * att.z * light.colour.xyz;
}


float spotBumpLight( in float3 position, in float3 normal, in float3x3 tsMatrix, in SpotLight light, out float3 tsLightDir )
{
	float3 lDir = normalize( light.position - position );
	float	distance = dot( light.position - position, lDir );
	
	float3 att = {(-distance + light.attenuation.x) * light.attenuation.y, 
				/*dot( light.direction, normal )*/ 0,  
				(dot( light.direction, lDir ) -light.attenuation.z) / (1 - light.attenuation.z) };
	
	att = saturate(att);	
	
	float3 dir = light.position - position;
	tsLightDir = mul( tsMatrix, dir ); // transform light direction to texture space
	
	return att.x * att.z;
}


float spotSpecBumpLight( in float3 position, in float3 normal, in float3 eye,
						 in float3x3 tsMatrix, in SpotLight light,
						 out float3 tsLightDir, out float3 tsHalfVec, out float3 spotDir )
{
	float3 dir = light.position - position;
	float3 lDir = normalize( dir );
	float	distance = dot( dir, lDir );

	float3 h = lDir + eye;
	
	tsHalfVec = mul( tsMatrix, h );
	tsLightDir = mul( tsMatrix, dir ); // transform light direction to texture space
	
	spotDir = dir;
	
	float att = (-distance + light.attenuation.x) * light.attenuation.y;
	att = saturate(att);
	return att;
}


float spotSpecBumpLight2( in float3 position, in float3 normal, in float3 eye,
						 in float3x3 tsMatrix, in SpotLight light,
						 out float3 tsLightDir, out float3 tsHalfVec, out float3 spotDir )
{
	float3 dir = light.position - position;
	float3 lDir = normalize( dir );
	float	distance = dot( dir, lDir );

	float3 h = lDir + eye;
	
	tsHalfVec = mul( tsMatrix, h );
	tsLightDir = mul( tsMatrix, dir ); // transform light direction to texture space
	
	//spotDir = dir;
	//spotDir = lDir;
	spotDir = (lDir*0.5) + float3(0.5,0.5,0.5);
	
	float att = (-distance + light.attenuation.x) * light.attenuation.y;
	att = saturate(att);
	return att;
}


void spotLightBump( in float3 position, in float3 normal, in float3 eye, in SpotLight light, out float3 diffuse, out float3 spec)
{
	float3 lDir = normalize( light.position - position );
	float	distance = dot( light.position - position, lDir );
	float3 h = normalize(eye + lDir);
	
	float4 att = {(-distance + light.attenuation.x) * light.attenuation.y, 
				dot( light.direction, normal ),  
				(dot( light.direction, lDir ) -light.attenuation.z) / (1 - light.attenuation.z),
				dot(normal, h)};
	att = saturate(att);
	
	diffuse = (att.x * att.y * att.z * light.colour.xyz);	
	
	spec = att.x * att.z * pow(att.w,32) * light.colour;	
}


#define BW_VERTEX_LIGHTING( o, ambientColour, selfIllumination, pos, normal, combineResults )\
{\
	if (!combineResults)\
	{\
		o.sunlight = ambientColour;\
		o.diffuse = selfIllumination;\
\
		if (nDirectionals > 0)\
		{\
			o.sunlight.xyz += directionalLight( normal, directionalLights[0] );\
		}\
\
		for (int i = 1; i < nDirectionals; i++)\
		{\
			o.diffuse.xyz += directionalLight( normal, directionalLights[i] );\
		}\
\
		for (int i = 0; i < nPoints; i++)\
		{\
			o.diffuse.xyz += pointLight( pos, normal, pointLights[i] );\
		}\
\
		for (int i = 0; i < nSpots; i++)\
		{\
			o.diffuse.xyz += spotLight( pos, normal, spotLights[i] );\
		}\
		\
		o.diffuse.xyz = mod2LightAdjust( o.diffuse, diffuseLightExtraModulation );\
		o.sunlight.xyz = mod2LightAdjust( o.sunlight, diffuseLightExtraModulation );\
	}\
	else\
	{\
		o.sunlight = ambientColour + selfIllumination;\
\
		for (int i = 0; i < nDirectionals; i++)\
		{\
			o.sunlight.xyz += directionalLight( normal, directionalLights[i] );\
		}\
\
		for (int i = 0; i < nPoints; i++)\
		{\
			o.sunlight.xyz += pointLight( pos, normal, pointLights[i] );\
		}\
\
		for (int i = 0; i < nSpots; i++)\
		{\
			o.sunlight.xyz += spotLight( pos, normal, spotLights[i] );\
		}\
		\
		o.sunlight.xyz = mod2LightAdjust( o.sunlight, diffuseLightExtraModulation );\
	}\
}

 
 /**
 *	This method calculates the diffuse (vertex) lighting for normal map shaders.
 *	Priority is given for bumped (pixel) lights which is why we are using the end of
 *	the lighting arrays as opposed to the start of the lighting arrays.
 *
 *	initial colour would be either ambient (outdoors) or static vertex light (indoors)  
 */
float4 normalMapDiffuse(
	in float3 pos,
	in float3 normal,
	in float4 initialColour,
	in float selfIllumination,
	in int nDir,
	in int nPoint,
	in int nDirBump )
{
	float4 diffuse = initialColour + selfIllumination;
	
	if (nDir > 0)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump] );
	if (nDir > 1)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump + 1] );

	int nPointBump = 2 - nDirBump;
	for ( int n=0; n<nPoint; n++ )
	{
		diffuse.xyz += pointLight( pos, normal, pointLights[n+nPointBump] );
	}
	
	return diffuse;
}

 /**
 *	This method calculates the diffuse (vertex) lighting for character lighting spot shaders.
 *	Priority is given for bumped (pixel) lights which is why we are using the end of
 *	the lighting arrays as opposed to the start of the lighting arrays.
 *
 *	initial colour would be either ambient (outdoors) or static vertex light (indoors)  
 */
float4 normalMapSpotDiffuse(
	in float3 pos,
	in float3 normal,
	in float4 initialColour,
	in int nSpot,
	in int nDir,
	in int nPoint,
	in int nDirBump )
{
	float4 diffuse = initialColour;
	
	if (nSpot > 0)
		diffuse.xyz += spotLight( pos, normal, spotLights[1] );
	
	if (nDir > 0)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump] );
	if (nDir > 1)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump + 1] );

	if (nPoint > 0)
		diffuse.xyz += pointLight( pos, normal, pointLights[1] );
	if (nPoint > 1)
		diffuse.xyz += pointLight( pos, normal, pointLights[2] );
	if (nPoint > 2)
		diffuse.xyz += pointLight( pos, normal, pointLights[3] );
		
	return diffuse;
}


/**
 *	This method calculates the diffuse bump pixel lighting..
 *	result.x = bump light 1 attenuation
 *	result.y = bump light 2 attenuation 
 */
float2 normalMapDiffuseBump(
	in float3 pos,
	in float3x3 tsMatrix,	
	in int nDirBump,
	in int nPointBump,
	out float3 dLight1,
	out float3 dLight2
	)
{
	float2 attenuation;	
	float3 tempDLight1 = (0,0,0);
	float3 tempDLight2 = (0,0,0);	
	
	if (nDirBump > 1)
		attenuation.x = directionalBumpLight( tsMatrix, directionalLights[1], tempDLight1 );
	else if (nPointBump > 0)
		attenuation.x = pointBumpLight( pos, tsMatrix, pointLights[0], tempDLight1 );
	else
		attenuation.x = 0;

	if (nDirBump > 0)
		attenuation.y = directionalBumpLight( tsMatrix, directionalLights[0], tempDLight2 );
	else if (nPointBump > 1)
		attenuation.y = pointBumpLight( pos, tsMatrix, pointLights[1], tempDLight2 );
	else
		attenuation.y = 0;
		
	dLight1 = tempDLight1;
	dLight2 = tempDLight2;	
		
	return attenuation;
}


/**
 *	This method calculates the diffuse bump pixel lighting for clspot shaders
 *	result = bump light 1 attenuation 
 */
float normalMapSpotBumpDiffuse(
	in float3 pos,
	in float3x3 tsMatrix,
	in int nDirBump,
	in int nPointBump,	
	out float3 dLight
	)
{
	float attenuation;
	float3 tempDLight = (0,0,0);
	
	if (nDirBump > 0)
		attenuation = directionalBumpLight( tsMatrix, directionalLights[0], tempDLight );
	else if (nPointBump > 0)
		attenuation = pointBumpLight( pos, tsMatrix, pointLights[0], tempDLight );
	else
		attenuation = 0;
		
	dLight = tempDLight;
	return attenuation;
}


/**
 *	This method calculates the specular bumped (pixel) lighting for character lighting shaders. 
 *	result.x = specular bump light 1 attenuation
 *	result.y = specular bump light 2 attenuation
 */
float2 normalMapSpecularBump(
	in float3 pos,
	in float3 eye,
	in float3x3 tsMatrix,	
	in int nSpecularPoints,
	in int nSpecularDirectionals,	
	out float3 sLight1,
	out float3 sLight2
	)
{
	float2 attenuation;	
	float3 tempSLight1 = (0,0,0);
	float3 tempSLight2 = (0,0,0);	

	BW_CALC_SPEC_LIGHT_COUNTS

	if (nDirSpecBump > 1)
		attenuation.x = directionalSpecBumpLight( eye, tsMatrix, specularDirectionalLights[1], tempSLight1 );
	else if (nPointSpecBump > 0)
		attenuation.x = pointSpecBumpLight( pos, eye, tsMatrix, specularPointLights[0], tempSLight1 );
	else
		attenuation.x = 0;

	if (nDirSpecBump > 0)
		attenuation.y = directionalSpecBumpLight( eye, tsMatrix, specularDirectionalLights[0], tempSLight2 );
	else if (nPointSpecBump > 1)
		attenuation.y = pointSpecBumpLight( pos, eye, tsMatrix, specularPointLights[1], tempSLight2 );
	else
		attenuation.y = 0;
		
	sLight1 = tempSLight1;
	sLight2 = tempSLight2;
		
	return attenuation;
}


float normalMapSpotSpecularBump(
	in float3 pos,
	in float3 eye,
	in float3x3 tsMatrix,
	in int nSpecularPoints,
	in int nSpecularDirectionals,
	out float3 sLight
	)
{
	float attenuation;
	float3 tempSLight = (0,0,0);

	BW_CALC_SPEC_LIGHT_COUNTS

	if (nDirSpecBump > 0)
		attenuation = directionalSpecBumpLight( eye, tsMatrix, specularDirectionalLights[0], tempSLight );
	else if (nPointSpecBump > 0)
		attenuation = pointSpecBumpLight( pos, eye, tsMatrix, specularPointLights[0], tempSLight );
	else
		attenuation = 0;
		
	sLight = tempSLight;
	return attenuation;
}



float4 normalMapDiffuse_vs1(
	in float3 pos,
	in float3 normal,
	in float4 initialColour,
	in float selfIllumination,
	in int nSpotLights,
	in int nDir,
	in int nPoint,
	in int nDirBump )
{
	float4 diffuse = initialColour + selfIllumination;
	
	int ns = nSpotLights < 2 ? nSpotLights : 2;
	for (int l = 0; l < ns; l++)
	{
		diffuse.xyz += spotLight( pos, normal, spotLights[l] );
	}	

	if (nDir > 0)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump] );
	if (nDir > 1)
		diffuse.xyz += directionalLight( normal, directionalLights[nDirBump + 1] );

	int nPointBump = 2 - nDirBump;
	for ( int n=0; n<nPoint; n++ )
	{
		diffuse.xyz += pointLight( pos, normal, pointLights[n+nPointBump] );
	}
		
	return diffuse;
}


float4 normalMapDiffuseBump_vs1(
	in float3 pos,
	in float3x3 tsMatrix,		
	in int nPointBump,
	in int nDirBump,
	out float3 dLight1,
	out float3 dLight2 )
{
	float4 attenuation = {0,0,0,0};
	
	float3 tempDLight1 = (0,0,0);
	float3 tempDLight2 = (0,0,0);
	
	if (nDirBump > 1)
		attenuation.x = directionalBumpLight( tsMatrix, directionalLights[1], tempDLight1 );
	else if (nPointBump > 0)
		attenuation.xyz = pointBumpLight( pos, tsMatrix, pointLights[0], tempDLight1 );
	else
		attenuation.xyz = 0;

	if (nDirBump > 0)
		attenuation.w = directionalBumpLight( tsMatrix, directionalLights[0], tempDLight2 );
	else if (nPointBump > 1)
		attenuation.w = pointBumpLight( pos, tsMatrix, pointLights[1], tempDLight2 );
	else
		attenuation.w = 0;
		
	dLight1 = tempDLight1;
	dLight2 = tempDLight2;
		
	return attenuation;
}


float3 normalMapSpecularBump_vs1(
	in float3 pos,
	in float3 eye,
	in float3x3 tsMatrix,	
	in int nSpecularPoints,
	in int nSpecularDirectionals,	
	out float3 sLight	
	)
{
	float3 attenuation;	
	float3 tempSLight = (0,0,0);	

	int nPointSpecBump = min( nSpecularPoints, 1 );
	int nDirSpecBump = min(1 - nPointSpecBump, min(1, nSpecularDirectionals) );

	if (nPointSpecBump > 0)
		attenuation = pointSpecBumpLight( pos, eye, tsMatrix, specularPointLights[0], tempSLight.xyz );
	else if (nDirSpecBump > 0)
		attenuation = directionalSpecBumpLight( eye, tsMatrix, specularDirectionalLights[0], tempSLight.xyz );
	else 
		attenuation = 0;
		
	sLight = tempSLight;	
		
	return attenuation;
}



#endif  //LIGHTING_HELPERS_FXH