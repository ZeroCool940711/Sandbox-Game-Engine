texture diffuseMap 
< 
	bool artistEditable = true; 
	string UIName = "Diffuse Map";
	string UIDesc = "The diffuse map for the material";
>;

technique EffectOverride
{
	pass Pass_0
	{
		TextureFactor = -1;
		StencilEnable = FALSE;
		PointSpriteEnable = FALSE;
		ZWriteEnable = FALSE;
		AlphaRef = 0;
		CullMode = CCW;
		SrcBlend = SRCALPHA;
		AlphaBlendEnable = TRUE;
		ColorWriteEnable = RED | GREEN | BLUE;
		DestBlend = ONE;
		AlphaTestEnable = FALSE;
		ZEnable = False;
		FogEnable = FALSE;
		ZFunc = NEVER;

		MipFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		AlphaOp[0] = SELECTARG1;
		MagFilter[0] = LINEAR;
		ColorArg1[0] = CURRENT;
		ColorArg2[0] = TEXTURE;
		ColorOp[0] = MODULATE2X;
		AddressW[0] = WRAP;
		AddressV[0] = WRAP;
		Texture[0] = (diffuseMap);
		AddressU[0] = WRAP;
		AlphaArg1[0] = CURRENT;
		AlphaArg2[0] = TEXTURE;
		TexCoordIndex[0] = 0;

		AlphaOp[1] = DISABLE;
		ColorOp[1] = DISABLE;

		PixelShader = NULL;
	}
}

