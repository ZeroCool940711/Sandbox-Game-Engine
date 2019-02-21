float4x4 worldViewProj : WorldViewProjection;

struct VertexXYZ
{
	float4 pos : POSITION;
};

struct Output
{
	float4 pos : POSITION;
	float depth : TEXCOORD;
};

Output vs_main( VertexXYZ input )
{
	Output o = (Output)0;

	o.pos = mul(input.pos, worldViewProj);
	o.depth = 1.f - o.pos.z;

	return o;
}

float4 ps_main( Output i ) : COLOR
{
	return float4(1,1,1,1);
	//return float4( i.depth, i.depth, i.depth, i.depth );
}

//--------------------------------------------------------------//
// Technique Section for standard
//--------------------------------------------------------------//
technique standard
<
	string channel = "shimmer";
	bool skinned = true;
	string label = "SHADER_MODEL_2";
>
{
	pass Pass_0
	{
		ALPHATESTENABLE = FALSE;		
		ALPHABLENDENABLE = TRUE;
		SRCBLEND = ONE;
		DESTBLEND = ONE;
		ZENABLE = TRUE;
		ZWRITEENABLE = FALSE;
		ZFUNC = LESSEQUAL;
		CULLMODE = NONE;
		VertexShader = compile vs_2_0 vs_main();
		PixelShader = compile ps_2_0 ps_main();
	}
}
