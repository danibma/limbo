#include "common.hlsli"

RaytracingAccelerationStructure Scene : register(t0);
RWTexture2D<float4> renderOutput : register(u0);

// Params
cbuffer $Globals : register(b0)
{
    uint dispatchWidth;
    uint dispatchHeight;
    float holeSize;
};

struct RayPayload
{
    float dummy; // Minimum of 4 bytes required for payloads.
};

[shader("raygeneration")]
void RayGenerationShader()
{
	// Orthographic projection, just as if we were already in NDC.
    float2 vpos = DispatchRaysIndex().xy;
    float3 rayOrigin = float3(-1, 1, -5);

    rayOrigin.xy += float2(2, -2) * (vpos / float2(dispatchWidth, dispatchHeight));
    
    float3 rayDir = float3(0, 0, 1);

    RayDesc myRay = { rayOrigin, 0.0f, rayDir, 100.0f };
    RayPayload payload = { 0.0f };

    uint missShaderIndex = 1;
    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, missShaderIndex, myRay, payload);
}

[shader("anyhit")]
void AnyHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(attr.barycentrics.xy, 1 - attr.barycentrics.x - attr.barycentrics.y);
    float3 triangleCentre = 1.0f / 3.0f;

    float distanceToCentre = length(triangleCentre - barycentrics);

    if (distanceToCentre < holeSize)
        IgnoreHit();
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(attr.barycentrics.xy, 1 - attr.barycentrics.x - attr.barycentrics.y);
    renderOutput[DispatchRaysIndex().xy] = float4(barycentrics, 1);
}

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    renderOutput[DispatchRaysIndex().xy] = float4(0, 1, 0, 1);
}