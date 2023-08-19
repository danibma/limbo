#pragma once

template<typename T>
T InterpolateVertex(T a0, T a1, T a2, float3 b)
{
    return b.x * a0 + b.y * a1 + b.z * a2;
}

MeshVertex GetVertex(Instance instance, float2 attribBarycentrics, uint primitiveIndex)
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

    MeshVertex vertex;
    vertex.Position = InterpolateVertex(normals[0], normals[1], normals[2], barycentrics);
    vertex.Normal   = InterpolateVertex(normals[0], normals[1], normals[2], barycentrics);
    vertex.UV       = InterpolateVertex(uv[0], uv[1], uv[2], barycentrics);

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
    MeshVertex vertex = GetVertex(instance, attribBarycentrics, PrimitiveIndex());

    Material material = GetMaterial(instance.Material);

    float4 finalAlbedo = material.AlbedoFactor;
    finalAlbedo *= SampleLevel2D(material.AlbedoIndex, LinearWrap, vertex.UV, 0);

    return finalAlbedo.a > ALPHA_THRESHOLD;
}