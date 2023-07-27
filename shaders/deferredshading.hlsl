#include "common.hlsli"

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

Texture2D g_albedoTexture;
Texture2D g_roughnessMetalTexture;
Texture2D g_emissiveTexture;

struct DeferredShadingOutput
{
    float4 albedo       : SV_Target0;
    float4 roughness    : SV_Target1;
    float4 metallic     : SV_Target2;
    float4 emissive     : SV_Target3;
};

DeferredShadingOutput PSMain(VSOut input)
{
    DeferredShadingOutput result;

    float4 albedo               = g_albedoTexture.Sample(LinearWrap, input.uv);
    float4 roughnessMetalMap    = g_roughnessMetalTexture.Sample(LinearWrap, input.uv);
    float4 emissiveMap          = g_emissiveTexture.Sample(LinearWrap, input.uv);

    float roughness     = roughnessMetalMap.g;
    float metallic      = roughnessMetalMap.b;

    result.albedo       = albedo;
    result.roughness    = roughness;
    result.metallic     = metallic;
    result.emissive     = emissiveMap;

    return result;
}
