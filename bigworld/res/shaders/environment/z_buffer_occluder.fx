#include "stdinclude.fxh"

// Auto Variables
float4x4 worldViewProj : WorldViewProjection;

// Manual Variables
bool clampToFarPlane = false;

// Vertex Formats

struct VertexXYZ
{
   float4 pos:		POSITION;   
};

struct OUTPUT
{
	float4 pos: POSITION;
	float4 col: COLOR;
};

bool fogEnabled : FogEnabled = true;
bool lightEnable = false;

OUTPUT vs_main( VertexXYZ input )
{
	OUTPUT o = (OUTPUT)0;
	
	o.pos = mul(input.pos, worldViewProj);
	if (clampToFarPlane)
	{		
		o.pos.z = o.pos.w;
	}
	o.col = (1,1,1,1);
	
	return o;
}


float4 ps_main( OUTPUT i ) : COLOR0
{
	float4 colour = i.col;	
	return colour;
}


#if (COMPILE_SHADER_MODEL_2 || COMPILE_SHADER_MODEL_3)
technique pixelShader2_0
{
   pass Pass_0
   {
		ALPHATESTENABLE = FALSE;      
		ZENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		CULLMODE = NONE;      

		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps_main();
   }
   
   pass Pass_1
   {
		ALPHATESTENABLE = FALSE;      
		ZENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZWRITEENABLE = FALSE;
		ZFUNC = ALWAYS;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		CULLMODE = NONE;      

		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps_main();
   }
}
#endif


#if (COMPILE_SHADER_MODEL_1)
technique pixelShader1_1
{
   pass Pass_0
   {
		ALPHATESTENABLE = FALSE;      
		ZENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		CULLMODE = NONE;      

		VertexShader = compile vs_1_1 vs_main();
		PixelShader = compile ps_1_1 ps_main();
   }
}
#endif


#if (COMPILE_SHADER_MODEL_0)
technique fixed_function
{
   pass Pass_0
   {
		ALPHATESTENABLE = FALSE;      
		ZENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ZERO;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		FOGENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		POINTSPRITEENABLE = FALSE;
		STENCILENABLE = FALSE;
		CULLMODE = NONE;
		
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = DIFFUSE;
		ALPHAOP[0] = SELECTARG1;
		ALPHAARG1[0] = DIFFUSE;
		
		BW_TEXTURESTAGE_TERMINATE(1)

		VertexShader = compile vs_2_0 vs_main();
		PixelShader = NULL;            
   }
}
#endif