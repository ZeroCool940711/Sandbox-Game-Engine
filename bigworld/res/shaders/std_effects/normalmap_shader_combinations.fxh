#ifndef NORMALMAP_SHADER_COMBINATIONS_FXH
#define NORMALMAP_SHADER_COMBINATIONS_FXH

#define NORMALMAP_SHADER_COUNT			90
#define NORMALMAP_DIFFUSE_ONLY_COUNT	15

//This macro defines 90 shaders based on nDir, nPoints, nSpecDir, nSpecPoint
#define NORMALMAP_VSHADER_COMBINATIONS( version, vs )\
	compile version vs(0,0,0,0),\
	compile version vs(1,0,0,0),\
	compile version vs(2,0,0,0),\
	compile version vs(0,1,0,0),\
	compile version vs(1,1,0,0),\
	compile version vs(2,1,0,0),\
	compile version vs(0,2,0,0),\
	compile version vs(1,2,0,0),\
	compile version vs(2,2,0,0),\
	compile version vs(0,3,0,0),\
	compile version vs(1,3,0,0),\
	compile version vs(2,3,0,0),\
	compile version vs(0,4,0,0),\
	compile version vs(1,4,0,0),\
	compile version vs(2,4,0,0),\
	compile version vs(0,0,1,0),\
	compile version vs(1,0,1,0),\
	compile version vs(2,0,1,0),\
	compile version vs(0,1,1,0),\
	compile version vs(1,1,1,0),\
	compile version vs(2,1,1,0),\
	compile version vs(0,2,1,0),\
	compile version vs(1,2,1,0),\
	compile version vs(2,2,1,0),\
	compile version vs(0,3,1,0),\
	compile version vs(1,3,1,0),\
	compile version vs(2,3,1,0),\
	compile version vs(0,4,1,0),\
	compile version vs(1,4,1,0),\
	compile version vs(2,4,1,0),\
	compile version vs(0,0,2,0),\
	compile version vs(1,0,2,0),\
	compile version vs(2,0,2,0),\
	compile version vs(0,1,2,0),\
	compile version vs(1,1,2,0),\
	compile version vs(2,1,2,0),\
	compile version vs(0,2,2,0),\
	compile version vs(1,2,2,0),\
	compile version vs(2,2,2,0),\
	compile version vs(0,3,2,0),\
	compile version vs(1,3,2,0),\
	compile version vs(2,3,2,0),\
	compile version vs(0,4,2,0),\
	compile version vs(1,4,2,0),\
	compile version vs(2,4,2,0),\
	compile version vs(0,0,0,1),\
	compile version vs(1,0,0,1),\
	compile version vs(2,0,0,1),\
	compile version vs(0,1,0,1),\
	compile version vs(1,1,0,1),\
	compile version vs(2,1,0,1),\
	compile version vs(0,2,0,1),\
	compile version vs(1,2,0,1),\
	compile version vs(2,2,0,1),\
	compile version vs(0,3,0,1),\
	compile version vs(1,3,0,1),\
	compile version vs(2,3,0,1),\
	compile version vs(0,4,0,1),\
	compile version vs(1,4,0,1),\
	compile version vs(2,4,0,1),\
	compile version vs(0,0,1,1),\
	compile version vs(1,0,1,1),\
	compile version vs(2,0,1,1),\
	compile version vs(0,1,1,1),\
	compile version vs(1,1,1,1),\
	compile version vs(2,1,1,1),\
	compile version vs(0,2,1,1),\
	compile version vs(1,2,1,1),\
	compile version vs(2,2,1,1),\
	compile version vs(0,3,1,1),\
	compile version vs(1,3,1,1),\
	compile version vs(2,3,1,1),\
	compile version vs(0,4,1,1),\
	compile version vs(1,4,1,1),\
	compile version vs(2,4,1,1),\
	compile version vs(0,0,0,2),\
	compile version vs(1,0,0,2),\
	compile version vs(2,0,0,2),\
	compile version vs(0,1,0,2),\
	compile version vs(1,1,0,2),\
	compile version vs(2,1,0,2),\
	compile version vs(0,2,0,2),\
	compile version vs(1,2,0,2),\
	compile version vs(2,2,0,2),\
	compile version vs(0,3,0,2),\
	compile version vs(1,3,0,2),\
	compile version vs(2,3,0,2),\
	compile version vs(0,4,0,2),\
	compile version vs(1,4,0,2),\
	compile version vs(2,4,0,2)


int normalMap_vsIndex( int nDir, int nPoint, int nSpecDir, int nSpecPoint )
{
	int nSpecIndex = nSpecDir;
	if (nSpecPoint > 0)
	{
		if (nSpecPoint == 1)
		{
			
			if (nSpecDir == 0)
			{
				nSpecIndex = 3;
			}
			else
			{
				nSpecIndex = 4;
			}
		}
		else
		{
			if(nSpecDir>0)
				nSpecIndex = 4;
			else
				nSpecIndex = 5;
		}
	}
	
	return min(nDir, 2) + (min(nPoint, 4) * 3) + (min(nSpecIndex, 6) * 15);
};


//This macro defines 15 shaders based on nDir, nPoints (diffuse only lighting)
#define NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS( version, vs )\
	compile version vs(0,0),\
	compile version vs(1,0),\
	compile version vs(2,0),\
	compile version vs(0,1),\
	compile version vs(1,1),\
	compile version vs(2,1),\
	compile version vs(0,2),\
	compile version vs(1,2),\
	compile version vs(2,2),\
	compile version vs(0,3),\
	compile version vs(1,3),\
	compile version vs(2,3),\
	compile version vs(0,4),\
	compile version vs(1,4),\
	compile version vs(2,4)
	
//This macro defines 15 shaders based on nDir, nPoints (diffuse only lighting).
//It limits some lighting combinations due to vs_1_1 instruction count limits.
#define NORMALMAP_DIFFUSEONLY_VSHADER_COMBINATIONS_LIMITED( version, vs )\
	compile version vs(0,0),\
	compile version vs(1,0),\
	compile version vs(2,0),\
	compile version vs(0,1),\
	compile version vs(1,1),\
	compile version vs(2,1),\
	compile version vs(0,2),\
	compile version vs(1,2),\
	compile version vs(2,2),\
	compile version vs(0,3),\
	compile version vs(1,2),\
	compile version vs(2,2),\
	compile version vs(0,3),\
	compile version vs(1,2),\
	compile version vs(2,2)


int diffuseOnlyVSIndex( int nDir, int nPoint, int staticLighting )
{
	return min(4,nPoint) * 3 + min(nDir,2) + int(staticLighting) * 15;
}


#endif	//NORMALMAP_SHADER_COMBINATIONS_FXH