#pragma once

#include "../common.hlsli"

template<typename T>
T InterpolateVertex(T a0, T a1, T a2, float3 b)
{
    return b.x * a0 + b.y * a1 + b.z * a2;
}

struct VertexAttributes
{
    float3 Position;
    float3 Normal;
    float3 GeometryNormal;
    float2 UV;
};

VertexAttributes GetVertexAttributes(Instance instance, float2 attribBarycentrics, uint primitiveIndex)
{
    float3 barycentrics = float3(1 - attribBarycentrics.x - attribBarycentrics.y, attribBarycentrics.x, attribBarycentrics.y);
    uint3  primitive    = BufferLoad<uint3>(instance.BufferIndex, primitiveIndex, instance.IndicesOffset);

    float3 positions[3];
    float3 normals[3];
    float2 uv[3];
    for (int i = 0; i < 3; ++i)
    {
        uint vertexID = primitive[i];
        positions[i]  = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.PositionsOffset);
        normals[i]    = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.NormalsOffset);
        uv[i]         = BufferLoad<float2>(instance.BufferIndex, vertexID, instance.TexCoordsOffset);
    }

    VertexAttributes vertex;
    vertex.Position = InterpolateVertex(positions[0], positions[1], positions[2], barycentrics);
    vertex.UV       = InterpolateVertex(uv[0], uv[1], uv[2], barycentrics);
    vertex.Normal   = InterpolateVertex(normals[0], normals[1], normals[2], barycentrics);
    vertex.Normal   = normalize(mul(vertex.Normal, (float3x3)instance.LocalTransform));
    
	// Calculate geometry normal from triangle vertices positions
    float3 edge20 = positions[2] - positions[0];
    float3 edge21 = positions[2] - positions[1];
    float3 edge10 = positions[1] - positions[0];
    vertex.GeometryNormal = normalize(mul(cross(edge20, edge10), (float3x3)instance.LocalTransform));

    return vertex;
}

float3 GetSky(float3 rayDir)
{
    float3 uv = normalize(rayDir);
    TextureCube<float4> skyTexture = ResourceDescriptorHeap[GSceneInfo.SkyIndex];
    return skyTexture.SampleLevel(LinearWrap, uv, 0).rgb;
}

// Returns true if it passed the alpha test
bool AnyHitAlphaTest(float2 attribBarycentrics)
{
    Instance instance = GetInstance(InstanceIndex());
    VertexAttributes vertex = GetVertexAttributes(instance, attribBarycentrics, PrimitiveIndex());

    Material material = GetMaterial(instance.Material);

    float4 finalAlbedo = material.AlbedoFactor;
    finalAlbedo *= SampleLevel2D(material.AlbedoIndex, LinearWrap, vertex.UV, 0);

    return finalAlbedo.a > ALPHA_THRESHOLD;
}

struct ShadingData
{
    float4 Albedo;
    float4 Normal;
    float4 RoughnessMetallicAO;
    float4 Emissive;
};

ShadingData GetShadingData(Material material, VertexAttributes vertex)
{
    ShadingData data;

    // In the future is probably a good idea to implement some mip mapping calculation, like the Ray Cone method from
    // Texture Level of Detail Strategies for Real-Time Ray Tracing - RayTracing Gems book
    const float mipLevel = 0;

    float4 finalAlbedo = material.AlbedoFactor;
    finalAlbedo *= SampleLevel2D(material.AlbedoIndex, LinearWrap, vertex.UV, mipLevel);

    float roughness = material.RoughnessFactor;
    float metallic = material.MetallicFactor;
    if (material.RoughnessMetalIndex != -1)
    {
        float4 roughnessMetalMap = SampleLevel2D(material.RoughnessMetalIndex, LinearWrap, vertex.UV, mipLevel);
        roughness *= roughnessMetalMap.g;
        metallic *= roughnessMetalMap.b;
    }

    float4 emissive = 0.0f;
    if (material.EmissiveIndex != -1)
    {
        emissive = SampleLevel2D(material.EmissiveIndex, LinearWrap, vertex.UV, mipLevel);
    }

    float ao = 1.0f;
    if (material.AmbientOcclusionIndex != -1)
    {
        float4 AOMap = SampleLevel2D(material.AmbientOcclusionIndex, LinearWrap, vertex.UV, mipLevel);
        ao = AOMap.r;
    }

    float3 normal = normalize(vertex.Normal);
    if (material.NormalIndex != -1 && false) // no support for normal mapping in ray tracing for now
    {
        float4 normalMap = SampleLevel2D(material.NormalIndex, LinearWrap, vertex.UV, mipLevel);
    }

    data.Albedo                 = finalAlbedo;
    data.Normal                 = float4(normal, 1.0f);
    data.RoughnessMetallicAO    = float4(roughness, metallic, ao, 1.0f);
    data.Emissive               = emissive;

    return data;
}

float4 GetSceneDebugView(in ShadingData shadingData)
{
    if (GSceneInfo.SceneViewToRender == 1)
        return shadingData.Albedo;
    else if (GSceneInfo.SceneViewToRender == 2)
        return shadingData.Normal;
    else if (GSceneInfo.SceneViewToRender == 3)
        return 0.0f;
    else if (GSceneInfo.SceneViewToRender == 4)
        return float4((float3)shadingData.RoughnessMetallicAO.y, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 5)
        return float4((float3)shadingData.RoughnessMetallicAO.x, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 6)
        return shadingData.Emissive;
    else if (GSceneInfo.SceneViewToRender == 7)
        return float4((float3)shadingData.RoughnessMetallicAO.z, 1.0f);

    return 0.0f;
}

struct MaterialPayload
{
    float   Distance;
    uint    PrimitiveID;
    uint    InstanceID;
    float2  Barycentrics;

    bool IsHit() { return Distance > 0; }
};

MaterialPayload TraceMaterialRay(in RaytracingAccelerationStructure tlas, in RayDesc ray, in RAY_FLAG flag = RAY_FLAG_NONE, in uint instanceMask = 0xFF)
{
    MaterialPayload payload = (MaterialPayload)0;

    TraceRay(
	    tlas,           //AccelerationStructure
	    RAY_FLAG_NONE,  //RayFlags
	    0xFF,           //InstanceInclusionMask
	    0,              //RayContributionToHitGroupIndex
	    1,              //MultiplierForGeometryContributionToHitGroupIndex
	    0,              //MissShaderIndex
	    ray,            //Ray
	    payload         //Payload
    );

    return payload;
}
