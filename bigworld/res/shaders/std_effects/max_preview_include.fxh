// This value is there so that the bool check boxes work properly.
string ParamID = "0x0001";

//-----------------------------------------
// Includes for ALL max preview effects.
//-----------------------------------------

// 3d studio max lighting values
float4 lightDir : Direction 
<
string UIName = "Light Direction";
string Object = "TargetLight";
int RefID = 0;
> = {-0.577, -0.577, 0.577,1.0};

float4 lightColour : LightColor 
<
int LightRef = 0;
> = float4( 1.0f, 1.0f, 1.0f, 1.0f );    // diffuse

BW_ARTIST_EDITABLE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_SELF_ILLUMINATION
BW_ARTIST_EDITABLE_MOD2X
BW_ARTIST_EDITABLE_DOUBLE_SIDED
BW_ARTIST_EDITABLE_ALPHA_TEST
#ifndef COLOURISE_DIFFUSE_MAP
BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)
#endif
BW_NON_EDITABLE_LIGHT_ENABLE

#include "unskinned_effect_include.fxh"

float4x4 worldInverse : WorldI;
float4x4 viewInverse  : ViewI;
float4x4 worldViewProj : WorldViewProjection;
float4x4 worldView : WorldView;
float4x4 worldViewInverse : WorldViewI;
