#pragma once

#include "../src/gfx/shaderinterop.h"

SamplerState LinearWrap;
SamplerState LinearClamp;
SamplerState PointClamp;

static const float PI       = 3.141592;
static const float TwoPI    = 2 * PI;
static const float HalfPI   = 0.5f * PI;
static const float INV_PI   = 0.31830988618379067154;
static const float INV_2PI  = 0.15915494309189533577;
static const float INV_4PI  = 0.07957747154594766788;

static const float ALPHA_THRESHOLD = 0.5f;

uint Flatten2D(uint2 index, uint2 dimensions)
{
    return index.x + index.y * dimensions.x;
}

float3 ReconstructPosition(in float2 uv, in float z, in float4x4 inverseProjection)
{
    float x = uv.x * 2.0f - 1.0f;
    float y = (1.0 - uv.y) * 2.0f - 1.0f;
    float4 position_s = float4(x, y, z, 1.0f);
    float4 position_v = mul(inverseProjection, position_s);
    return position_v.xyz / position_v.w;
}

float3 NormalFromDepth(in uint2 threadID, in Texture2D<float> depthTexture, in float4x4 inverseProjection)
{
    // Improved normal reconstruction from depth by János Turánszki - https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
    // This is the basic version, which means it can contain flat normals, it's most noticeable in edges

    float width, height, depth;
    depthTexture.GetDimensions(0, width, height, depth);
    float2 depthDimensions = float2(width, height);

    float2 uv = (threadID + 0.5f) / depthDimensions;

    float2 uv0 = uv; // center
    float2 uv1 = uv + float2(1, 0) / depthDimensions; // right 
    float2 uv2 = uv + float2(0, 1) / depthDimensions; // top

    float depth0 = depthTexture.SampleLevel(PointClamp, uv0, 0).r;
    float depth1 = depthTexture.SampleLevel(PointClamp, uv1, 0).r;
    float depth2 = depthTexture.SampleLevel(PointClamp, uv2, 0).r;

    float3 P0 = ReconstructPosition(uv0, depth0, inverseProjection);
    float3 P1 = ReconstructPosition(uv1, depth1, inverseProjection);
    float3 P2 = ReconstructPosition(uv2, depth2, inverseProjection);

    float3 normal = normalize(cross(P2 - P0, P1 - P0));

    return normal;
}