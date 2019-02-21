// The heat shimmer effect for PC
// This effect takes a full-screen mesh with vertices in clip space.
// Two sets of uvs are output, one being the standard uvs, the other
// begin heat-shimmer perturbed uvs ( done in this shader )

texture BackBuffer;

float4 SCREEN_FACTOR_OFFSET;
float4 ANIMATION;
float TIME_OFFSET = 0.2;
float4 SIN_VEC = {1.f, -0.16161616f, 0.0083333f, -0.00019841f};
float4 COS_VEC = {-0.5f, 0.041666666f, -0.0013888889f, 0.000024801587f};
float4 NOISE_FREQ_S;
float4 NOISE_FREQ_T;
float4 WAVE_SPEED = {0.2f, 0.15f, 0.4f, 0.4f};
float4 FIXUPS = {1.02f, 0.003f, 0.f, 0.f};
float4 UVFIX;
float FULLSCREEN_ALPHA = 0.f;
float TWO_PI = 6.283185307179586476925286766559f;

struct VertexXYZNUV
{
   float4 pos:		POSITION;
   float3 normal:	NORMAL;
   float2 tc:		TEXCOORD0;
};

struct Output
{
	float4 pos: POSITION;
	float2 tc:  TEXCOORD0;
	float2 tc1: TEXCOORD1;	
	float4 diffuse: COLOR;
};


Output vs_main( VertexXYZNUV input )
{
	Output o = (Output)0;
	float4 r0;
	float4 r1;
	float4 r2;
	float4 r3;
	float4 r5;
	float4 r7;
	float4 r9;	
	float4 r11;
	
	// Transform vertex to render target (screen space)
	o.pos.xy = input.pos.xy + SCREEN_FACTOR_OFFSET.zw;	
	o.pos.zw = 1.0;	

	// Set UV 0 to be standard tex coordinates ( unperturbed )
	o.tc = input.tc;	
	r11.xy = input.tc.xy;	

	// Output colour as passed in alpha
	// This is for full-screen effects ( like when you
	// are inside a shockwave )
	o.diffuse = FULLSCREEN_ALPHA;	

	// Animate the uv.
	r0 = (NOISE_FREQ_S * r11.x);	
	r0 = r0 + (NOISE_FREQ_T * r11.y);	

	r1 = ANIMATION.zzzz;	
	r0 = r0 + r1 * WAVE_SPEED;	
	r0.xy = frac(r0);	
	r1.xy = frac(r0.zwzw);
	r0.zw = r1.xyxy;
	r0 = r0 * FIXUPS.x;
	r0 = r0 - 0.5;
	r1 = r0 * TWO_PI;

	r2 = r1 * r1;
	r3 = r2 * r1;
	r5 = r3 * r2;
	r7 = r5 * r2;
	r9 = r7 * r2;

	r0 = r1 + r3 * SIN_VEC.x;
	r0 = r0 + r5 * SIN_VEC.y;
	r0 = r0 + r7 * SIN_VEC.z;
	r0 = r0 + r9 * SIN_VEC.w;

	// And output uv.  scale results by the spread of the effect.
	r11.xy = r11.xy + (ANIMATION.xy * r0.xy);
	o.tc1 = r11.xy + UVFIX.xy;

	return o;
}


//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique standard
<
	string label = "SHADER_MODEL_0";
>
{
	pass Pass_0
	{
		SRCBLEND = ONE;
		DESTBLEND = ZERO;		
		ZENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		ALPHATESTENABLE = FALSE;
		CULLMODE = NONE;
		LIGHTING = FALSE;
		FOGENABLE = FALSE;
		SPECULARENABLE = FALSE;
		COLORWRITEENABLE = RED | GREEN | BLUE;
		
		Texture[0] = <BackBuffer>;
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TEXTURE;
		ALPHAOP[0] = ADD;
		ALPHAARG1[0] = DIFFUSE;
		ALPHAARG2[0] = TEXTURE;
		TEXCOORDINDEX[0] = 0;
		MAXMIPLEVEL[0] = 0;
		MIPMAPLODBIAS[0] = 0;
		
		Texture[1] = <BackBuffer>;
		COLOROP[1] = BLENDCURRENTALPHA;
		COLORARG1[1] = TEXTURE;
		COLORARG2[1] = CURRENT;
		ALPHAOP[1] = DISABLE;
		TEXCOORDINDEX[1] = 1;
		MAXMIPLEVEL[1] = 0;
		MIPMAPLODBIAS[1] = 0;
		
		COLOROP[2] = DISABLE;
		ALPHAOP[2] = DISABLE;
		
		MAGFILTER[0] = POINT;
		MIPFILTER[0] = POINT;
		MINFILTER[0] = POINT;		
		ADDRESSU[0] = CLAMP;
		ADDRESSV[0] = CLAMP;
		ADDRESSW[0] = CLAMP;
		MAGFILTER[1] = LINEAR;
		MIPFILTER[1] = LINEAR;
		MINFILTER[1] = LINEAR;
		ADDRESSU[1] = MIRROR;
		ADDRESSV[1] = MIRROR;
		ADDRESSW[1] = MIRROR;
				
		VertexShader = compile vs_1_1 vs_main();
		PixelShader = NULL;
	}
}

technique debug
{
	pass Pass_0
	{
		SRCBLEND = ONE;
		DESTBLEND = ZERO;		
		ZENABLE = FALSE;
		ALPHABLENDENABLE = FALSE;
		ALPHATESTENABLE = FALSE;
		CULLMODE = NONE;
		LIGHTING = FALSE;
		FOGENABLE = FALSE;
		SPECULARENABLE = FALSE;
		TEXTUREFACTOR = 0xFFFFFFFF;
		
		TEXTURE[0] = (BackBuffer);
		COLOROP[0] = SELECTARG1;
		COLORARG1[0] = TFACTOR | COMPLEMENT;
		ALPHAOP[0] = ADD;
		ALPHAARG1[0] = DIFFUSE;
		ALPHAARG2[0] = TEXTURE;
		
		TEXTURE[1] = (BackBuffer);
		COLOROP[1] = BLENDCURRENTALPHA;
		COLORARG1[1] = TFACTOR;
		COLORARG2[1] = CURRENT;
		ALPHAOP[1] = DISABLE;
		COLOROP[2] = DISABLE;
		
		MAGFILTER[0] = POINT;
		MIPFILTER[0] = POINT;
		MINFILTER[0] = POINT;		
		ADDRESSU[0] = CLAMP;
		ADDRESSV[0] = CLAMP;
		ADDRESSW[0] = CLAMP;
		MAGFILTER[1] = POINT;
		MIPFILTER[1] = POINT;
		MINFILTER[1] = POINT;
		ADDRESSU[1] = CLAMP;
		ADDRESSV[1] = CLAMP;
		ADDRESSW[1] = CLAMP;
				
		VertexShader = compile vs_1_1 vs_main();
		PixelShader = NULL;
	}
}