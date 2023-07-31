#include "common.hlsli"

float4x4 viewProj;
float4x4 model;

struct VSOut
{
    float4 Position : SV_Position;
    float4 Normal   : Normal;
    float2 UV       : TEXCOORD;
};

VSOut VSMain(float3 pos : Position, float3 normal : Normal, float2 uv : UV)
{
    VSOut result;

    float4x4 mvp = mul(viewProj, model);
    result.Position = mul(mvp, float4(pos, 1.0f));
    result.Normal = float4(normal, 1.0f);
    result.UV = uv;

	return result;
}

Texture2D g_albedoTexture;
Texture2D g_roughnessMetalTexture;
Texture2D g_emissiveTexture;

struct DeferredShadingOutput
{
    float4 Albedo       : SV_Target0;
    float4 Normal       : SV_Target1;
    float4 Roughness    : SV_Target2;
    float4 Metallic     : SV_Target3;
    float4 Emissive     : SV_Target4;
};

DeferredShadingOutput PSMain(VSOut input)
{
    DeferredShadingOutput result;

    float4 albedo               = g_albedoTexture.Sample(LinearWrap, input.UV);
    float4 roughnessMetalMap    = g_roughnessMetalTexture.Sample(LinearWrap, input.UV);
    float4 emissiveMap          = g_emissiveTexture.Sample(LinearWrap, input.UV);

    float roughness     = roughnessMetalMap.g;
    float metallic      = roughnessMetalMap.b;

    result.Albedo       = albedo;
    result.Normal       = input.Normal;
    result.Roughness    = roughness;
    result.Metallic     = metallic;
    result.Emissive     = emissiveMap;

    return result;
}
