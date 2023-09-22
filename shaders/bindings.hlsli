#pragma once

#include "../src/gfx/shaderinterop.h"

SamplerState SLinearWrap     : register(s0, space1);
SamplerState SLinearClamp    : register(s1, space1);
SamplerState SPointWrap      : register(s2, space1);

ConstantBuffer<SceneInfo> GSceneInfo : register(b100);
ConstantBuffer<ShadowData> GShadowData : register(b101);

template<typename T = float4>
Texture2D<T> GetTexture(int index)
{
    return ResourceDescriptorHeap[NonUniformResourceIndex(index)];
}

float4 Sample2D(int index, SamplerState s, float2 uv)
{
    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    return texture.Sample(s, uv);
}

float4 SampleLevel2D(int index, SamplerState s, float2 uv, float level)
{
    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
	return texture.SampleLevel(s, uv, level);
}

float4 SampleLevelCube(int index, SamplerState s, float3 uv, float level)
{
    TextureCube texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    return texture.SampleLevel(s, uv, level);
}

float4 GatherAlpha2D(int index, SamplerState s, float2 uv, float level)
{
    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
	return texture.GatherAlpha(s, uv, level);
}

float2 GetDimensions2D(int index)
{
    float width, height;
    Texture2D texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    texture.GetDimensions(width, height);
    return float2(width, height);
}

float3 GetDimensionsCube(int index)
{
    float width, height, levels;
    TextureCube texture = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    texture.GetDimensions(0, width, height, levels);
    return float3(width, height, levels);
}

template<typename T>
T BufferLoad(uint bufferIndex, uint elementIndex, uint byteOffset = 0)
{
    ByteAddressBuffer buffer = ResourceDescriptorHeap[NonUniformResourceIndex(bufferIndex)];
    return buffer.Load<T>(elementIndex * sizeof(T) + byteOffset);
}

Material GetMaterial(int index)
{
    StructuredBuffer<Material> materials = ResourceDescriptorHeap[NonUniformResourceIndex(GSceneInfo.MaterialsBufferIndex)];
    return materials[NonUniformResourceIndex(index)];
}

Instance GetInstance(int index)
{
    StructuredBuffer<Instance> instances = ResourceDescriptorHeap[NonUniformResourceIndex(GSceneInfo.InstancesBufferIndex)];
    return instances[NonUniformResourceIndex(index)];
}