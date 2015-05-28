// Header

cbuffer CONSTANT : register(b2)
{
};

Texture2D tex0 : register(t0);
sampler tex0sampler : register(s0);

struct PixelInput
{
	float4 Position : SV_POSITION;
	float4 diff_color : COLOR0;
	float4 spec_color : COLOR1;
	float4 dst_reg3 : COLOR2;
	float4 dst_reg4 : COLOR3;
	float fogc : FOG;
	float4 dummy : COLOR4;
	float4 tc0 : TEXCOORD0;
	float4 tc1 : TEXCOORD1;
	float4 tc2 : TEXCOORD2;
	float4 tc3 : TEXCOORD3;
	float4 tc4 : TEXCOORD4;
	float4 tc5 : TEXCOORD5;
	float4 tc6 : TEXCOORD6;
	float4 tc7 : TEXCOORD7;
	float4 tc8 : TEXCOORD8;
};

struct PixelOutput
{
	float4 ocol0 : SV_TARGET0;
};

PixelOutput main(PixelInput In)
{
	float4 tc0 = In.tc0;
	float4 diff_color = In.diff_color;
	float4 r0 = float4(0., 0., 0., 0.);
	float4 h0 = float4(0., 0., 0., 0.);
	r0.w = tex0.Sample(tex0sampler, tc0.xy).w;
	h0 = diff_color;
	r0.w = (h0 * r0).w;
	r0.xyz = h0.xyz;

	PixelOutput Out;
	Out.ocol0 = r0;
	return Out;
}
