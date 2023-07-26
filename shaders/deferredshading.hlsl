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

SamplerState LinearWrap;
Texture2D<float4> g_albedoTexture;
Texture2D<float4> g_roughnessMetalTexture;
Texture2D<float4> g_normalTexture;
Texture2D<float4> g_emissiveTexture;

float4 PSMain(VSOut input) : SV_Target
{
    float4 albedo               = g_albedoTexture.Sample(LinearWrap, input.uv);
    float4 roughnessMetalMap    = g_roughnessMetalTexture.Sample(LinearWrap, input.uv);
    float4 normalMap            = g_normalTexture.Sample(LinearWrap, input.uv);
    float4 emissiveMap          = g_emissiveTexture.Sample(LinearWrap, input.uv);

    float roughness = roughnessMetalMap.g;
    float metallic = roughnessMetalMap.b;

    return albedo;
}
