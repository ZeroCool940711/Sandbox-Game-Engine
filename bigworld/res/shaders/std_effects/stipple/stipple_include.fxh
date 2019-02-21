
#define STIPPLE_SAMPLER_DECLARE				\
sampler stippleSampler = sampler_state		\
{											\
	Texture = (stippleMap);					\
	ADDRESSU = WRAP;						\
	ADDRESSV = WRAP;						\
	ADDRESSW = WRAP;						\
	MAGFILTER = POINT;						\
	MINFILTER = POINT;						\
	MIPFILTER = POINT;						\
	MAXMIPLEVEL = 0;						\
	MIPMAPLODBIAS = 0;						\
};

#define STIPPLE_UV_COORDS( i, o )			\
o = i.xy / i.w;								\
o *= float2(screen[2],screen[3]);			\
o *= float2(0.25,0.25);						\
o -= float2(-0.001,-0.001);				