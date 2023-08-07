#include "common.hlsli"

float4x4 viewProj;
float4x4 model;

// https://github.com/graphitemaster/normals_revisited
float3 TransformDirection(in float4x4 transform, in float3 direction)
{
    float4x4 result;
#define minor(m, r0, r1, r2, c0, c1, c2)                            \
        (m[c0][r0] * (m[c1][r1] * m[c2][r2] - m[c1][r2] * m[c2][r1]) -  \
         m[c1][r0] * (m[c0][r1] * m[c2][r2] - m[c0][r2] * m[c2][r1]) +  \
         m[c2][r0] * (m[c0][r1] * m[c1][r2] - m[c0][r2] * m[c1][r1]))
    result[0][0] = minor(transform, 1, 2, 3, 1, 2, 3);
    result[1][0] = -minor(transform, 1, 2, 3, 0, 2, 3);
    result[2][0] = minor(transform, 1, 2, 3, 0, 1, 3);
    result[3][0] = -minor(transform, 1, 2, 3, 0, 1, 2);
    result[0][1] = -minor(transform, 0, 2, 3, 1, 2, 3);
    result[1][1] = minor(transform, 0, 2, 3, 0, 2, 3);
    result[2][1] = -minor(transform, 0, 2, 3, 0, 1, 3);
    result[3][1] = minor(transform, 0, 2, 3, 0, 1, 2);
    result[0][2] = minor(transform, 0, 1, 3, 1, 2, 3);
    result[1][2] = -minor(transform, 0, 1, 3, 0, 2, 3);
    result[2][2] = minor(transform, 0, 1, 3, 0, 1, 3);
    result[3][2] = -minor(transform, 0, 1, 3, 0, 1, 2);
    result[0][3] = -minor(transform, 0, 1, 2, 1, 2, 3);
    result[1][3] = minor(transform, 0, 1, 2, 0, 2, 3);
    result[2][3] = -minor(transform, 0, 1, 2, 0, 1, 3);
    result[3][3] = minor(transform, 0, 1, 2, 0, 1, 2);
    return mul(result, float4(direction, 0.0f)).xyz;
#undef minor    // cleanup
}

struct VSOut
{
    float4 Position : SV_Position;
    float4 WorldPos : WorldPosition;
    float4 Normal   : Normal;
    float2 UV       : TEXCOORD;
};

VSOut VSMain(float3 pos : InPosition, float3 normal : InNormal, float2 uv : InUV)
{
    VSOut result;

    float4x4 mvp = mul(viewProj, model);
    result.Position = mul(mvp, float4(pos, 1.0f));
    result.WorldPos = mul(model, float4(pos, 1.0f));
    result.Normal = float4(TransformDirection(model, normal), 1.0f);
    result.UV = uv;

	return result;
}

Texture2D g_albedoTexture;
Texture2D g_roughnessMetalTexture;
Texture2D g_emissiveTexture;
ConstantBuffer<MaterialFactors> g_MaterialFactors : register(b1);

struct DeferredShadingOutput
{
    float4 WorldPosition : SV_Target0;
    float4 Albedo        : SV_Target1;
    float4 Normal        : SV_Target2;
    float4 Roughness     : SV_Target3;
    float4 Metallic      : SV_Target4;
    float4 Emissive      : SV_Target5;
};

DeferredShadingOutput PSMain(VSOut input)
{
    DeferredShadingOutput result;

    float4 albedo               = g_albedoTexture.Sample(LinearWrap, input.UV);
    float4 roughnessMetalMap    = g_roughnessMetalTexture.Sample(LinearWrap, input.UV);
    float4 emissiveMap          = g_emissiveTexture.Sample(LinearWrap, input.UV);

    // not a good solution, it works almost fine with the intel sponza, but the ivys and the trees do not work
    if (albedo.a < 0.99f)
        discard;

    float roughness     = roughnessMetalMap.g * g_MaterialFactors.RoughnessFactor;
    float metallic      = roughnessMetalMap.b * g_MaterialFactors.MetallicFactor;

    float4 finalAlbedo  = g_MaterialFactors.AlbedoFactor;
    
    if (albedo.a != 0)
        finalAlbedo *= albedo;
    
    result.WorldPosition = input.WorldPos;
    result.Albedo        = finalAlbedo;
    result.Normal        = input.Normal;
    result.Roughness     = float4((float3)roughness, 1.0f);
    result.Metallic      = float4((float3)metallic, 1.0f);
    result.Emissive      = emissiveMap;

    return result;
}
