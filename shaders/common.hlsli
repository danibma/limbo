﻿#pragma once

#include "bindings.hlsli"

/*
 *              Naming Convention
 *  Variables:
 *      UAV               : 'u' prefix, eg. uRenderTarget
 *      SRV               : 't' prefix, eg. tSceneAS
 *      Root Constants/CBV: 'c' prefix, eg. cTextureIdx
 *
 *  Function:
 *      Entry Points: stage sufix, eg. GenerateMipCS, MainVS
 */

static const float PI       = 3.141592;
static const float TwoPI    = 2 * PI;
static const float HalfPI   = 0.5f * PI;
static const float INV_PI   = 0.31830988618379067154;
static const float INV_2PI  = 0.15915494309189533577;
static const float INV_4PI  = 0.07957747154594766788;

static const float FLT_MAX  = 3.402823466e+38f;

static const float ALPHA_THRESHOLD = 0.5f;

// Mesh Shaders
#define MS_NUM_BLOCKS 128

// https://github.com/graphitemaster/normals_revisited
float3 TransformNormal(in float4x4 transform, in float3 direction)
{
    float4x4 result;
#define minor(m, r0, r1, r2, c0, c1, c2)                                \
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

/**
 * Unpack a normal stored in a normal map. The X and Y components are rescaled from [0,1] to [-1,1] and Z is reconstructed.
 * Taken from UE4 - Common.ush
 */
float4 UnpackNormalMap(float4 textureSample)
{
    float2 normalXY = textureSample.rg;
	
    normalXY = normalXY * float2(2.0f, 2.0f) - float2(1.0f, 1.0f);
    float normalZ = sqrt(saturate(1.0f - dot(normalXY, normalXY)));
    return float4(normalXY.xy, normalZ, 1.0f);
}

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

float3 NormalFromDepth(in uint2 threadID, in uint depthTextureIndex, float2 dimensions, in float4x4 inverseProjection)
{
    // Improved normal reconstruction from depth by János Turánszki - https://wickedengine.net/2019/09/22/improved-normal-reconstruction-from-depth/
    // This is the basic version, which means it can contain flat normals, it's most noticeable in edges

    Texture2D<float> depthTexture = GetTexture<float>(depthTextureIndex);

    float2 uv = (threadID + 0.5f) / dimensions;

    float2 uv0 = uv; // center
    float2 uv1 = uv + float2(1, 0) / dimensions; // right 
    float2 uv2 = uv + float2(0, 1) / dimensions; // top

    float depth0 = depthTexture.SampleLevel(SLinearWrap, uv0, 0).r;
    float depth1 = depthTexture.SampleLevel(SLinearWrap, uv1, 0).r;
    float depth2 = depthTexture.SampleLevel(SLinearWrap, uv2, 0).r;

    float3 P0 = ReconstructPosition(uv0, depth0, inverseProjection);
    float3 P1 = ReconstructPosition(uv1, depth1, inverseProjection);
    float3 P2 = ReconstructPosition(uv2, depth2, inverseProjection);

    float3 normal = normalize(cross(P2 - P0, P1 - P0));

    return normal;
}