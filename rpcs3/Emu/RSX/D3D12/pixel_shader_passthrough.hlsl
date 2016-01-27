Texture2D InputTexture : register(t0);
sampler bilinearSampler : register(s0);

struct PixelInput
{
	float4 Pos : SV_POSITION;
	float2 TexCoords : TEXCOORDS0;
};

float4 main(PixelInput In) : SV_TARGET
{
	return InputTexture.Sample(bilinearSampler, In.TexCoords);
}