﻿#include "raytracingcommon.hlsli"
#include "../random.hlsli"

struct AORayPayload
{
	float aoVal; // Stores 0 on a ray hit, 1 on ray miss
};

RaytracingAccelerationStructure SceneAS : register(t0, space0);

Texture2D g_Positions : register(t1, space0);
Texture2D g_Normals   : register(t2, space0);

RWTexture2D<float4> g_Output : register(u0, space0);

cbuffer $Globals : register(b0, space0)
{
	float radius;
}

float ShootAmbientOcclusionRay(float3 orig, float3 dir, float minT, float maxT)
{
    AORayPayload rayPayload = (AORayPayload)0.0f;

    RayDesc rayAO = { orig, minT, dir, maxT };

	// We're going to tell our ray to never run the closest-hit shader and to
	//     stop as soon as we find *any* intersection
    uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

	// Trace our ray. 
    TraceRay(
        SceneAS,    // AccelerationStructure
        rayFlags,   // RayFlags
        0xFF,       // InstanceInclusionMask
        0,          // RayContributionToHitGroupIndex
        1,          // MultiplierForGeometryContributionToHitGroupIndex
        0,          // MissShaderIndex
        rayAO,      // Ray
        rayPayload  // Payload
    );

    return rayPayload.aoVal;
}

[shader("raygeneration")]
void RTAORayGen()
{
    uint2 pixel		 = DispatchRaysIndex().xy;
    uint2 resolution = DispatchRaysDimensions().xy;

    uint seed = RandomSeed(pixel, resolution, GSceneInfo.FrameIndex);

    float4 position = g_Positions[pixel];
    float4 normal   = g_Normals[pixel];

    float ao = 1.0f;

    if (position.w != 0.0f)
    {
        // Random ray, sampled on cosine-weighted hemisphere around normal 
        float3 dir = GetCosHemisphereSample(seed, normal.xyz);

        ao = ShootAmbientOcclusionRay(position.xyz, dir, 0.1f, radius);
    }

    g_Output[pixel] = float4(ao, ao, ao, 1.0f);
}

[shader("miss")]
void RTAOMiss(inout AORayPayload payload)
{
	payload.aoVal = 1.0f;
}

[shader("anyhit")]
void RTAOAnyHit(inout AORayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics)) IgnoreHit();
}
