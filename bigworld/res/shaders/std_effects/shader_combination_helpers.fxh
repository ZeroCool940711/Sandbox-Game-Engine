#ifndef SHADER_COMBINATION_HELPERS_FXH
#define SHADER_COMBINATION_HELPERS_FXH

#define LIGHTONLY_SHADER_COUNT			45

#define LIGHTONLY_VSHADER_COMBINATIONS( version, vs_main, combineLights )\
	compile version vs_main(0,0,0,combineLights),\
	compile version vs_main(1,0,0,combineLights),\
	compile version vs_main(2,0,0,combineLights),\
	compile version vs_main(0,1,0,combineLights),\
	compile version vs_main(1,1,0,combineLights),\
	compile version vs_main(2,1,0,combineLights),\
	compile version vs_main(0,2,0,combineLights),\
	compile version vs_main(1,2,0,combineLights),\
	compile version vs_main(2,2,0,combineLights),\
	compile version vs_main(0,3,0,combineLights),\
	compile version vs_main(1,3,0,combineLights),\
	compile version vs_main(2,3,0,combineLights),\
	compile version vs_main(0,4,0,combineLights),\
	compile version vs_main(1,4,0,combineLights),\
	compile version vs_main(2,4,0,combineLights),\
	compile version vs_main(0,0,1,combineLights),\
	compile version vs_main(1,0,1,combineLights),\
	compile version vs_main(2,0,1,combineLights),\
	compile version vs_main(0,1,1,combineLights),\
	compile version vs_main(1,1,1,combineLights),\
	compile version vs_main(2,1,1,combineLights),\
	compile version vs_main(0,2,1,combineLights),\
	compile version vs_main(1,2,1,combineLights),\
	compile version vs_main(2,2,1,combineLights),\
	compile version vs_main(0,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights),\
	compile version vs_main(2,3,1,combineLights),\
	compile version vs_main(0,4,1,combineLights),\
	compile version vs_main(1,4,1,combineLights),\
	compile version vs_main(2,4,1,combineLights),\
	compile version vs_main(0,0,2,combineLights),\
	compile version vs_main(1,0,2,combineLights),\
	compile version vs_main(2,0,2,combineLights),\
	compile version vs_main(0,1,2,combineLights),\
	compile version vs_main(1,1,2,combineLights),\
	compile version vs_main(2,1,2,combineLights),\
	compile version vs_main(0,2,2,combineLights),\
	compile version vs_main(1,2,2,combineLights),\
	compile version vs_main(2,2,2,combineLights),\
	compile version vs_main(0,3,2,combineLights),\
	compile version vs_main(1,3,2,combineLights),\
	compile version vs_main(2,3,2,combineLights),\
	compile version vs_main(0,4,2,combineLights),\
	compile version vs_main(1,4,2,combineLights),\
	compile version vs_main(2,4,2,combineLights)
	
//The following combinations shave some lights off the most
//complicated scenarios (when there are 2 spots).  This helps
//compiled vertex shaders on shader 1.1 cards
//(vshader instruction count limited)
#define LIGHTONLY_VSHADER_COMBINATIONS_LIMITED( version, vs_main, combineLights )\
	compile version vs_main(0,0,0,combineLights),\
	compile version vs_main(1,0,0,combineLights),\
	compile version vs_main(2,0,0,combineLights),\
	compile version vs_main(0,1,0,combineLights),\
	compile version vs_main(1,1,0,combineLights),\
	compile version vs_main(2,1,0,combineLights),\
	compile version vs_main(0,2,0,combineLights),\
	compile version vs_main(1,2,0,combineLights),\
	compile version vs_main(2,2,0,combineLights),\
	compile version vs_main(0,3,0,combineLights),\
	compile version vs_main(1,3,0,combineLights),\
	compile version vs_main(2,3,0,combineLights),\
	compile version vs_main(0,4,0,combineLights),\
	compile version vs_main(1,4,0,combineLights),\
	compile version vs_main(2,4,0,combineLights),\
	compile version vs_main(0,0,1,combineLights),\
	compile version vs_main(1,0,1,combineLights),\
	compile version vs_main(2,0,1,combineLights),\
	compile version vs_main(0,1,1,combineLights),\
	compile version vs_main(1,1,1,combineLights),\
	compile version vs_main(2,1,1,combineLights),\
	compile version vs_main(0,2,1,combineLights),\
	compile version vs_main(1,2,1,combineLights),\
	compile version vs_main(2,2,1,combineLights),\
	compile version vs_main(0,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights),\
	compile version vs_main(2,3,1,combineLights),\
	compile version vs_main(0,4,1,combineLights),\
	compile version vs_main(1,4,1,combineLights),\
	compile version vs_main(2,4,1,combineLights),\
	compile version vs_main(0,0,2,combineLights),\
	compile version vs_main(1,0,2,combineLights),\
	compile version vs_main(2,0,2,combineLights),\
	compile version vs_main(0,1,2,combineLights),\
	compile version vs_main(1,1,2,combineLights),\
	compile version vs_main(2,1,2,combineLights),\
	compile version vs_main(0,2,2,combineLights),\
	compile version vs_main(1,2,2,combineLights),\
	compile version vs_main(2,2,2,combineLights),\
	compile version vs_main(0,3,2,combineLights),\
	compile version vs_main(1,3,2,combineLights),\
	compile version vs_main(2,3,2,combineLights),\
	compile version vs_main(0,3,2,combineLights),\
	compile version vs_main(1,3,2,combineLights),\
	compile version vs_main(1,3,2,combineLights)
	
	
//The following combinations shave some lights off the most
//complicated scenarios (when there are 2 spots).  This helps
//compiled vertex shaders on shader 1.1 cards
//(vshader instruction count limited)
#define LIGHTONLY_VSHADER_COMBINATIONS_LIMITED_2( version, vs_main, combineLights )\
	compile version vs_main(0,0,0,combineLights),\
	compile version vs_main(1,0,0,combineLights),\
	compile version vs_main(2,0,0,combineLights),\
	compile version vs_main(0,1,0,combineLights),\
	compile version vs_main(1,1,0,combineLights),\
	compile version vs_main(2,1,0,combineLights),\
	compile version vs_main(0,2,0,combineLights),\
	compile version vs_main(1,2,0,combineLights),\
	compile version vs_main(2,2,0,combineLights),\
	compile version vs_main(0,3,0,combineLights),\
	compile version vs_main(1,3,0,combineLights),\
	compile version vs_main(2,3,0,combineLights),\
	compile version vs_main(0,4,0,combineLights),\
	compile version vs_main(1,4,0,combineLights),\
	compile version vs_main(2,4,0,combineLights),\
	compile version vs_main(0,0,1,combineLights),\
	compile version vs_main(1,0,1,combineLights),\
	compile version vs_main(2,0,1,combineLights),\
	compile version vs_main(0,1,1,combineLights),\
	compile version vs_main(1,1,1,combineLights),\
	compile version vs_main(2,1,1,combineLights),\
	compile version vs_main(0,2,1,combineLights),\
	compile version vs_main(1,2,1,combineLights),\
	compile version vs_main(2,2,1,combineLights),\
	compile version vs_main(0,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights),\
	compile version vs_main(2,3,1,combineLights),\
	compile version vs_main(0,4,1,combineLights),\
	compile version vs_main(1,4,1,combineLights),\
	compile version vs_main(2,4,1,combineLights),\
	compile version vs_main(0,0,1,combineLights),\
	compile version vs_main(1,0,1,combineLights),\
	compile version vs_main(2,0,1,combineLights),\
	compile version vs_main(0,1,1,combineLights),\
	compile version vs_main(1,1,1,combineLights),\
	compile version vs_main(2,1,1,combineLights),\
	compile version vs_main(0,2,1,combineLights),\
	compile version vs_main(1,2,1,combineLights),\
	compile version vs_main(2,2,1,combineLights),\
	compile version vs_main(0,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights),\
	compile version vs_main(2,3,1,combineLights),\
	compile version vs_main(0,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights),\
	compile version vs_main(1,3,1,combineLights)


#define LIGHTONLY_VSHADERS( arrayName, version, vs_main, vs_mainStaticLighting, combineLights )\
VertexShader arrayName[ LIGHTONLY_SHADER_COUNT + LIGHTONLY_SHADER_COUNT ] = {\
	LIGHTONLY_VSHADER_COMBINATIONS( version, vs_main, combineLights ),\
	LIGHTONLY_VSHADER_COMBINATIONS( version, vs_mainStaticLighting, combineLights )\
};

#define LIGHTONLY_VSHADERS_LIMITED( arrayName, version, vs_main, vs_mainStaticLighting, combineLights )\
VertexShader arrayName[ LIGHTONLY_SHADER_COUNT + LIGHTONLY_SHADER_COUNT ] = {\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED( version, vs_main, combineLights ),\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED( version, vs_mainStaticLighting, combineLights )\
};

#define LIGHTONLY_VSHADERS_LIMITED_2( arrayName, version, vs_main, vs_mainStaticLighting, combineLights )\
VertexShader arrayName[ LIGHTONLY_SHADER_COUNT + LIGHTONLY_SHADER_COUNT ] = {\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED_2( version, vs_main, combineLights ),\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED_2( version, vs_mainStaticLighting, combineLights )\
};
	

//You should use this method for choosing the vertex shader if you use LIGHTONLY_VSHADER_COMBINATIONS
int lightonlyVShaderIndex( int nDirectionalLights, int nPointLights, int nSpotLights, int staticLighting )
{
	return nDirectionalLights + (nPointLights * 3) + (nSpotLights * 15) + int(staticLighting) * LIGHTONLY_SHADER_COUNT;
};


#define LIGHTONLY_SKINNED_VSHADERS( arrayName, version, vs_main, combineLights )\
VertexShader arrayName[LIGHTONLY_SHADER_COUNT] = {\
	LIGHTONLY_VSHADER_COMBINATIONS( version, vs_main, combineLights )\
};

#define LIGHTONLY_SKINNED_VSHADERS_LIMITED( arrayName, version, vs_main, combineLights )\
VertexShader arrayName[LIGHTONLY_SHADER_COUNT] = {\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED( version, vs_main, combineLights )\
};

#define LIGHTONLY_SKINNED_VSHADERS_LIMITED_2( arrayName, version, vs_main, combineLights )\
VertexShader arrayName[LIGHTONLY_SHADER_COUNT] = {\
	LIGHTONLY_VSHADER_COMBINATIONS_LIMITED_2( version, vs_main, combineLights )\
};

//You should use this method for choosing the vertex shader if you use LIGHTONLY_SKINNED_VSHADER_COMBINATIONS
int lightonlySkinnedVShaderIndex( int nDirectionalLights, int nPointLights, int nSpotLights )
{
	return lightonlyVShaderIndex( nDirectionalLights, nPointLights, nSpotLights, 0 );
};


#endif	//SHADER_COMBINATION_HELPERS_FXH