#pragma once

#include "../src/gfx/shaderinterop.h"

SamplerState LinearWrap     : register(s0, space1);
SamplerState LinearClamp    : register(s1, space1);
SamplerState PointClamp     : register(s2, space1);

ConstantBuffer<SceneInfo> GSceneInfo : register(b100);

float4 Sample2D(int index, SamplerState s, float2 uv)
{
	Texture2D texture = ResourceDescriptorHeap[index];
    return texture.Sample(s, uv);
}

float4 SampleLevel2D(int index, SamplerState s, float2 uv, float level)
{
	Texture2D texture = ResourceDescriptorHeap[index];
	return texture.SampleLevel(s, uv, level);
}

float4 GatherAlpha2D(int index, SamplerState s, float2 uv, float level)
{
	Texture2D texture = ResourceDescriptorHeap[index];
	return texture.GatherAlpha(s, uv, level);
}

float2 GetDimensions2D(int index)
{
    float width, height;
    Texture2D texture = ResourceDescriptorHeap[index];
    texture.GetDimensions(width, height);
    return float2(width, height);
}

Material GetMaterial(int index)
{
    StructuredBuffer<Material> materials = ResourceDescriptorHeap[GSceneInfo.MaterialsBufferIndex];
    return materials[NonUniformResourceIndex(index)];
}

Instance GetInstance(int index)
{
    StructuredBuffer<Instance> instances = ResourceDescriptorHeap[GSceneInfo.InstancesBufferIndex];
    return instances[NonUniformResourceIndex(index)];
}