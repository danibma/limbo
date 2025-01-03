#pragma once

#include "../common.hlsli"
#include "../brdf.hlsli"

template<typename T>
T InterpolateVertex(T a0, T a1, T a2, float3 b)
{
    return b.x * a0 + b.y * a1 + b.z * a2;
}

struct VertexAttributes
{
    float3 Position;
    float3 ShadingNormal;
    float3 GeometryNormal;
    float4 Tangent;
    float2 UV;
};

VertexAttributes GetVertexAttributes(Instance instance, float2 attribBarycentrics, uint primitiveIndex)
{
    float3 barycentrics = float3(1 - attribBarycentrics.x - attribBarycentrics.y, attribBarycentrics.x, attribBarycentrics.y);
    uint3  primitive    = BufferLoad<uint3>(instance.BufferIndex, primitiveIndex, instance.IndicesOffset);

    MeshVertex vertex[3];
    for (int i = 0; i < 3; ++i)
    {
        uint vertexID = primitive[i];
        vertex[i] = BufferLoad<MeshVertex>(instance.BufferIndex, vertexID, instance.VerticesOffset);
    }

    VertexAttributes vertexAttrib;
    vertexAttrib.Position = mul(instance.LocalTransform, float4(InterpolateVertex(vertex[0].Position, vertex[1].Position, vertex[2].Position, barycentrics), 1.0f)).rgb;
    vertexAttrib.UV       = InterpolateVertex(vertex[0].UV, vertex[1].UV, vertex[2].UV, barycentrics);

    vertexAttrib.ShadingNormal = InterpolateVertex(vertex[0].Normal, vertex[1].Normal, vertex[2].Normal, barycentrics);
    vertexAttrib.ShadingNormal = TransformNormal(instance.LocalTransform, vertexAttrib.ShadingNormal);

    vertexAttrib.Tangent = InterpolateVertex(vertex[0].Tangent, vertex[1].Tangent, vertex[2].Tangent, barycentrics);
    vertexAttrib.Tangent.xyz = TransformNormal(instance.LocalTransform, vertexAttrib.Tangent.xyz);

    // Calculate geometry normal from triangle vertices positions
    float3 edge20 = vertex[2].Position - vertex[0].Position;
    float3 edge21 = vertex[2].Position - vertex[1].Position;
    float3 edge10 = vertex[1].Position - vertex[0].Position;
    vertexAttrib.GeometryNormal = normalize(cross(edge20, edge10));

    return vertexAttrib;
}

float3 GetSky(float3 rayDir)
{
    float3 uv = normalize(rayDir);
    TextureCube<float4> skyTexture = ResourceDescriptorHeap[GSceneInfo.SkyIndex];
    return skyTexture.SampleLevel(SLinearWrap, uv, 0).rgb;
}

// Returns true if it passed the alpha test
bool AnyHitAlphaTest(float2 attribBarycentrics)
{
    Instance instance = GetInstance(InstanceIndex());
    VertexAttributes vertex = GetVertexAttributes(instance, attribBarycentrics, PrimitiveIndex());

    Material material = GetMaterial(instance.Material);

    float4 finalAlbedo = material.BaseColorFactor;
    if (material.BaseColorIndex != -1)
    {
        float4 albedo = SampleLevel2D(material.BaseColorIndex, SLinearWrap, vertex.UV, 0);
        finalAlbedo *= albedo;
    }

    return finalAlbedo.a > ALPHA_THRESHOLD;
}

ShadingData GetShadingData(Material material, VertexAttributes vertex)
{
    ShadingData data;
    data.bIsSpecularGloss = any(material.bIsSpecularGlossModel);

    // In the future is probably a good idea to implement some mip mapping calculation, like the Ray Cone method from
    // Texture Level of Detail Strategies for Real-Time Ray Tracing - RayTracing Gems book
    const float mipLevel = 0;

    float4 finalBaseColor = material.BaseColorFactor;
    if (material.BaseColorIndex != -1)
    {
        float4 baseColor = SampleLevel2D(material.BaseColorIndex, SLinearWrap, vertex.UV, mipLevel);
        finalBaseColor *= baseColor;
    }

    float roughness = 0.0f;
    float metallic = 0.0f;
    float3 specular = 0.0f;
    if (data.bIsSpecularGloss)
    {
        roughness = material.RoughnessFactor; // The roughness factor is already converted(1 - glossiness) when loading the scene
        specular = material.SpecularFactor;
        if (material.RoughnessMetalIndex != -1) // This is the SpecularGlossiness texture
        {
            float4 specularGlossinessMap = SampleLevel2D(material.RoughnessMetalIndex, SLinearWrap, vertex.UV, mipLevel);
            specular *= specularGlossinessMap.rgb;
            roughness = 1 - specularGlossinessMap.a;
        }
    }
    else
    {
        roughness = material.RoughnessFactor;
        metallic = material.MetallicFactor;
        if (material.RoughnessMetalIndex != -1)
        {
            float4 roughnessMetalMap = SampleLevel2D(material.RoughnessMetalIndex, SLinearWrap, vertex.UV, mipLevel);
            roughness *= roughnessMetalMap.g;
            metallic *= roughnessMetalMap.b;
        }
    }
    
    float3 emissive = material.EmissiveFactor;
    if (material.EmissiveIndex != -1)
    {
        emissive = SampleLevel2D(material.EmissiveIndex, SLinearWrap, vertex.UV, mipLevel).rgb;
    }

    float3 normal = normalize(vertex.ShadingNormal);
    if (material.NormalIndex != -1)
    {
        float3 normalMap = UnpackNormalMap(SampleLevel2D(material.NormalIndex, SLinearWrap, vertex.UV, mipLevel)).rgb;

        float4 tangent = normalize(vertex.Tangent);
        float3 bitangent = cross(normal, tangent.xyz) * tangent.w;
        float3x3 TBN = transpose(float3x3(tangent.xyz, bitangent, normal));
        normal = normalize(mul(TBN, normalMap));
    }

    data.BaseColor        = finalBaseColor.rgb;
    data.ShadingNormal    = normal;
    data.GeometryNormal   = vertex.GeometryNormal;
    data.Roughness        = roughness;
    data.Specular         = specular;
    data.Metallic         = metallic;
    data.Emissive         = emissive;
    data.AO               = 1.0f;

    return data;
}

float4 GetSceneDebugView(in ShadingData shadingData)
{
    if (GSceneInfo.SceneViewToRender == 1)
        return float4(shadingData.BaseColor, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 2)
        return float4(shadingData.ShadingNormal, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 3)
        return float4(shadingData.CalculateDiffuseReflectance(), 1.0f);
    else if (GSceneInfo.SceneViewToRender == 4)
        return float4((float3)shadingData.Metallic, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 5)
        return float4((float3)shadingData.Roughness, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 6)
        return float4(shadingData.Emissive, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 7)
        return 0.0f;

    return 0.0f;
}