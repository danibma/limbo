float4x4 viewProj;
float4x4 model;

struct VSOut
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD;
};

VSOut VSMain(float3 pos : Position, float2 uv : UV)
{
    VSOut result;

    float4x4 mvp = mul(viewProj, model);
    result.position = mul(mvp, float4(pos, 1.0f));
    result.uv = uv;

	return result;
}

Texture2D<float4> g_diffuseTexture;
SamplerState LinearWrap;

float4 PSMain(VSOut input) : SV_Target
{
    float4 finalColor = g_diffuseTexture.Sample(LinearWrap, input.uv);

    return finalColor;
}
