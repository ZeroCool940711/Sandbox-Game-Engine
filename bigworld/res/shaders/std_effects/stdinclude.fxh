#ifndef STDINCLUDE_FXH
#define STDINCLUDE_FXH

// standard include file for .fx files
// contains most common code used in .fx pipeline

#include "optinclude.fxh"

#include "d3d_state_mirror.fxh"

#include "lighting_helpers.fxh"
#include "fog_helpers.fxh"
#include "vertex_declarations.fxh"
#include "material_helpers.fxh"
#include "sky_light_map_helpers.fxh"
#include "fresnel_helpers.fxh"
#include "depth_helpers.fxh"

//For now, everyone effect uses the camera position.  In reality,
//this is used for sky light maps (thus all outdoor shaders) and
//reflection maps (any specular or chrome effect).
float3 wsCameraPos : CameraPos;

int     mipFilter     : MipFilter = 2;
int     minMagFilter  : MinMagFilter = 2;
int     maxAnisotropy : MaxAnisotropy = 1;

#define BW_SAMPLER(map, addressType)\
sampler_state\
{\
	Texture = (map);\
	ADDRESSU = addressType;\
	ADDRESSV = addressType;\
	ADDRESSW = addressType;\
	MAGFILTER = LINEAR;\
	MINFILTER = (minMagFilter);\
	MIPFILTER = (mipFilter);\
	MAXANISOTROPY = (maxAnisotropy);\
	MAXMIPLEVEL = 0;\
	MIPMAPLODBIAS = 0;\
};


// A null technique to remove shader cap warnings when a shader is tightly
// controlled by the application (not part of the std_effects set)
#define BW_NULL_TECHNIQUE \
technique null\
{\
   pass Pass_0\
   {\
      VertexShader = NULL;\
      PixelShader = NULL;\
   }\
}

#endif