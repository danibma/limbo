#include "common.hlsli"

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

float4x4 model;
uint instanceID;

VSOut VSMain(float3 pos : InPosition, float3 normal : InNormal, float2 uv : InUV)
{
    VSOut result;

    float4x4 mvp = mul(GSceneInfo.ViewProjection, model);
    result.Position = mul(mvp, float4(pos, 1.0f));
    result.PixelPos = mul(GSceneInfo.View, mul(model, float4(pos, 1.0f)));
    result.WorldPos = mul(model, float4(pos, 1.0f));
    result.Normal = TransformDirection(model, normal);
    result.UV = uv;

	return result;
}

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

    Instance instance = GetInstance(instanceID);
    Material material = GetMaterial(instance.Material);

    float4 albedo = Sample2D(material.AlbedoIndex, LinearWrap, input.UV);

    float4 finalAlbedo = material.AlbedoFactor;
    finalAlbedo *= albedo;

    if (finalAlbedo.a <= ALPHA_THRESHOLD)
        discard;

	float roughness = material.RoughnessFactor;
    float metallic = material.MetallicFactor;
    if (material.RoughnessMetalIndex != -1)
    {
        float4 roughnessMetalMap = Sample2D(material.RoughnessMetalIndex, LinearWrap, input.UV);
        roughness *= roughnessMetalMap.g;
        metallic *= roughnessMetalMap.b;
    }

    float4 emissive = 0.0f;
    if (material.EmissiveIndex != -1)
    {
        emissive = Sample2D(material.EmissiveIndex, LinearWrap, input.UV);
    }

    float ao = 1.0f;
    if (material.AmbientOcclusionIndex != -1)
    {
        float4 AOMap = Sample2D(material.AmbientOcclusionIndex, LinearWrap, input.UV);
		ao = AOMap.r;
    }

    float3 normal = normalize(input.Normal);
    if (material.NormalIndex != -1)
    {
        float4 normalMap = Sample2D(material.NormalIndex, LinearWrap, input.UV);

        float3 viewDirection = normalize(GSceneInfo.CameraPos - input.WorldPos.xyz);

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
