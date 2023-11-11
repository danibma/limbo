#include "common.hlsli"

struct VSOut
{
    float4 Position : SV_Position;
    float4 PixelPos : PixelPosition;
    float4 WorldPos : WorldPosition;
    float3 Normal   : Normal;
    float2 UV       : TEXCOORD;
};

uint cInstanceID;

VSOut MainVS(uint vertexID : SV_VertexID)
{
    VSOut result;

    Instance instance = GetInstance(cInstanceID);
    float3 pos    = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.PositionsOffset);
    float3 normal = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.NormalsOffset);
    float2 uv     = BufferLoad<float2>(instance.BufferIndex, vertexID, instance.TexCoordsOffset);

    float4x4 mvp    = mul(GSceneInfo.ViewProjection, instance.LocalTransform);
    result.Position = mul(mvp, float4(pos, 1.0f));
    result.PixelPos = mul(GSceneInfo.View, mul(instance.LocalTransform, float4(pos, 1.0f)));
    result.WorldPos = mul(instance.LocalTransform, float4(pos, 1.0f));
    result.Normal   = TransformNormal(instance.LocalTransform, normal);
    result.UV       = uv;

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

DeferredShadingOutput MainPS(VSOut input)
{
    DeferredShadingOutput result;

    Instance instance = GetInstance(cInstanceID);
    Material material = GetMaterial(instance.Material);

    float4 finalAlbedo = material.AlbedoFactor;
    if (material.AlbedoIndex != -1)
    {
	    float4 albedo = Sample2D(material.AlbedoIndex, SLinearWrap, input.UV);
	    finalAlbedo *= albedo;
    }

    if (finalAlbedo.a <= ALPHA_THRESHOLD)
        discard;

	float roughness = material.RoughnessFactor;
    float metallic = material.MetallicFactor;
    if (material.RoughnessMetalIndex != -1)
    {
        float4 roughnessMetalMap = Sample2D(material.RoughnessMetalIndex, SLinearWrap, input.UV);
        roughness *= roughnessMetalMap.g;
        metallic *= roughnessMetalMap.b;
    }

    float3 emissive = material.EmissiveFactor;
    if (material.EmissiveIndex != -1)
    {
        emissive = Sample2D(material.EmissiveIndex, SLinearWrap, input.UV).rgb;
    }

    float ao = 1.0f;
    if (material.AmbientOcclusionIndex != -1)
    {
        float4 AOMap = Sample2D(material.AmbientOcclusionIndex, SLinearWrap, input.UV);
		ao = AOMap.r;
    }

    float3 normal = normalize(input.Normal);
    if (material.NormalIndex != -1)
    {
        float4 normalMap = Sample2D(material.NormalIndex, SLinearWrap, input.UV);

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
    result.Emissive             = float4(emissive, 1.0f);

    return result;
}
