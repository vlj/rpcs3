
struct VertexInput
{
	float2 Pos : POSITION;
	float2 TexCoords : TEXCOORDS0;
};

struct PixelInput
{
	float4 Pos : SV_POSITION;
	float2 TexCoords : TEXCOORDS0;
};

PixelInput main(VertexInput In)
{
	PixelInput Out;
	Out.Pos = float4(In.Pos, 0., 1.);
	Out.TexCoords = In.TexCoords;
	return Out;
}