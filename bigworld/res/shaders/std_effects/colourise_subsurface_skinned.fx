#include "stdinclude.fxh"

BW_ARTIST_EDITABLE_ADDRESS_MODE(BW_WRAP)
#include "bw_four_channel_customise.fxh"
BW_FOUR_CHANNEL_COLOURISE( clothesColour1, "Clothes Colour 1", "Custom colour for part 1. Selected by the red channel of the mask map.",
							clothesColour2, "Clothes Colour 2", "Custom colour for part 2. Selected by the green channel of the mask map.",
							clothesColour3, "Clothes Colour 3", "Custom colour for part 3. Selected by the blue channel of the mask map. Unavailable on pixel shader 1.1 cards.",
							clothesColour4, "Clothes Colour 4", "Custom colour for part 4. Selected by the alpha channel of the mask map." )
#define COLOURISE_DIFFUSE_MAP

#include "subsurface_skinned.fx"
