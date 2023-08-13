#include "common.hlsli"

float4x4 viewProj;
float4x4 view;
float4x4 model;

// https://github.com/graphitemaster/normals_revisited
float3 TransformDirection(in float4x4 transform, in float3 direction)
{
    float4x4 result;
#define minor(m, r0, r1, r2, c0, c1, c2)                                \
        (m[c0][r0] * (m[c1][r1] * m[c2][r2] - m[c1][r2] * m[c2][r1]) -  \
         m[c1][r0] * (m[c0][r1] * m[c2][r2] - m[c0][r2] * m[c2][r1]) +  \
         m[c2][r0] * (m[c0][r1] * m[c1][r2] - m[c0][r2] * m[c1][r1]))
    result[0][0] =  minor(transform, 1, 2, 3, 1, 2, 3);
    result[1][0] = -minor(transform, 1, 2, 3, 0, 2, 3);
    result[2][0] =  minor(transform, 1, 2, 3, 0, 1, 3);
    result[3][0] = -minor(transform, 1, 2, 3, 0, 1, 2);
    result[0][1] = -minor(transform, 0, 2, 3, 1, 2, 3);
    result[1][1] =  minor(transform, 0, 2, 3, 0, 2, 3);
    result[2][1] = -minor(transform, 0, 2, 3, 0, 1, 3);
    result[3][1] =  minor(transform, 0, 2, 3, 0, 1, 2);
    result[0][2] =  minor(transform, 0, 1, 3, 1, 2, 3);
    result[1][2] = -minor(transform, 0, 1, 3, 0, 2, 3);
    result[2][2] =  minor(transform, 0, 1, 3, 0, 1, 3);
    result[3][2] = -minor(transform, 0, 1, 3, 0, 1, 2);
    result[0][3] = -minor(transform, 0, 1, 2, 1, 2, 3);
    result[1][3] =  minor(transform, 0, 1, 2, 0, 2, 3);
    result[2][3] = -minor(transform, 0, 1, 2, 0, 1, 3);
    result[3][3] =  minor(transform, 0, 1, 2, 0, 1, 2);
    return mul(result, float4(direction, 0.0f)).xyz;
#undef minor    // cleanup
}

struct VSOut
{
    float4 Position : SV_Position;
    float4 PixelPos : PixelPosition;
    float4 WorldPos : WorldPosition;
    float3 Normal   : Normal;
    float2 UV       : TEXCOORD;
};

VSOut VSMain(float3 pos : InPosition, float3 normal : InNormal, float2 uv : InUV)
{
    VSOut result;

    float4x4 mvp = mul(viewProj, model);
    result.Position = mul(mvp, float4(pos, 1.0f));
    result.PixelPos = mul(view, mul(model, float4(pos, 1.0f)));
    result.WorldPos = mul(model, float4(pos, 1.0f));
    result.Normal = TransformDirection(model, normal);
    result.UV = uv;

	return result;
}

ConstantBuffer<Material> g_Material : register(b1);

float3 camPos;

struct DeferredShadingOutput
{
    float4 PixelPosition            : SV_Target0;
    float4 WorldPosition            : SV_Target1;
    float4 Albedo                   : SV_Target2;
    float4 Normal                   : SV_Target3;
    float4 RoughnessMetallicAO      : SV_Target4;
    float4 Emissive                 : SV_Target5;
};

DeferredShadingOutput PSMain(VSOut input)
{
    DeferredShadingOutput result;

    Texture2D<float4> albedoTexture = ResourceDescriptorHeap[g_Material.AlbedoIndex];
    float4 albedo = albedoTexture.Sample(LinearWrap, input.UV);

    float4 finalAlbedo = g_Material.AlbedoFactor;
    finalAlbedo *= albedo;

    if (finalAlbedo.a <= ALPHA_THRESHOLD)
        discard;

	float roughness = g_Material.RoughnessFactor;
    float metallic = g_Material.MetallicFactor;
    if (g_Material.RoughnessMetalIndex != -1)
    {
        Texture2D<float4> roughnessMetalTexture = ResourceDescriptorHeap[g_Material.RoughnessMetalIndex];
	    float4 roughnessMetalMap  = roughnessMetalTexture.Sample(LinearWrap, input.UV);
        roughness *= roughnessMetalMap.g;
        metallic *= roughnessMetalMap.b;
    }

    float4 emissive = 0.0f;
    if (g_Material.EmissiveIndex != -1)
    {
        Texture2D<float4> emissiveTexture = ResourceDescriptorHeap[g_Material.EmissiveIndex];
		emissive = emissiveTexture.Sample(LinearWrap, input.UV);
    }

    float ao = 1.0f;
    if (g_Material.AmbientOcclusionIndex != -1)
    {
        Texture2D<float4> AOTexture = ResourceDescriptorHeap[g_Material.AmbientOcclusionIndex];
        float4 AOMap = AOTexture.Sample(LinearWrap, input.UV);
		ao = AOMap.r;
    }

    float3 normal = normalize(input.Normal);
    if (g_Material.NormalIndex != -1)
    {
        Texture2D<float4> normalTexture = ResourceDescriptorHeap[g_Material.NormalIndex];
        float4 normalMap = normalTexture.Sample(LinearWrap, input.UV);

        float3 viewDirection = normalize(camPos - input.WorldPos.xyz);

        // Use ddx/ddy to get the exact data for this pixel, since the GPU computes 2x2 pixels at a time
        float3 dp1  = ddx(-viewDirection);
        float3 dp2  = ddy(-viewDirection);
        float2 duv1 = ddx(input.UV);
        float2 duv2 = ddy(input.UV);

        float3 dp1perp   = normalize(cross(normal, dp1));
        float3 dp2perp   = normalize(cross(dp2, normal));

        // https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping
        float3 tangent   = dp2perp * duv1.x + dp1perp * duv2.x;
        float3 bitangent = dp2perp * duv1.y + dp1perp * duv2.y;

        float invmax     = rsqrt(max(dot(tangent, tangent), dot(bitangent, bitangent)));

        // Since this is a orthogonal matrix (each axis is a perpendicular unit vector) its transpose equals its inverse.
        // With this we can transform all the world-space vectors into tangent-space, and vice-versa
        float3x3 tbn     = transpose(float3x3(tangent * invmax, bitangent * invmax, normal));
        float3 rgbNormal = 2.0f * normalMap.rgb - 1.0f; // from [0, 1] to [-1, 1]

        normal = mul(tbn, rgbNormal);
    }

    result.PixelPosition        = input.PixelPos;
    result.WorldPosition        = input.WorldPos;
    result.Albedo               = finalAlbedo;
    result.Normal               = float4(normal, 1.0f);
    result.RoughnessMetallicAO  = float4(roughness, metallic, ao, 1.0f);
    result.Emissive             = emissive;

    return result;
}
