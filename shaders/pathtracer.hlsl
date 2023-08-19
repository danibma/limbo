#include "common.hlsli"
#include "raytracingcommon.hlsli"

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

cbuffer $Globals : register(b0)
{
    float4 camPos;
    float4x4 invViewProj;
};

struct HitInfo
{
    float4 colorAndDistance;
};

RayDesc GeneratePinholeCameraRay(float2 pixel)
{
    float4x4 view  = GSceneInfo.InvView;
    float3   right = float3(view[0][0], view[1][0], view[2][0]);
    float3   up    = float3(view[0][1], view[1][1], view[2][1]);
    float3   front = float3(view[0][2], view[1][2], view[2][2]);
    float3   pos   = float3(view[0][3], view[1][3], view[2][3]);

	// Set up the ray.
	RayDesc ray;
    ray.Origin = pos;
	ray.TMin = 0.f;
	ray.TMax = FLT_MAX;

	// Extract the aspect ratio and fov from the projection matrix.
    float aspect = GSceneInfo.Projection[1][1] / GSceneInfo.Projection[0][0];
    float tanHalfFovY = 1.f / GSceneInfo.Projection[1][1];

	// Compute the ray direction.
	ray.Direction = normalize(
		(pixel.x * right * tanHalfFovY * aspect) -
		(pixel.y * up * tanHalfFovY) +
		-front); // right-hand coords
    return ray;
}

[shader("raygeneration")]
void RayGen()
{
    float2 pixel = float2(DispatchRaysIndex().xy);
    float2 resolution = float2(DispatchRaysDimensions().xy);

    // Initialize the ray payload
    HitInfo payload;
    payload.colorAndDistance = float4(0, 0, 0, 0);


    pixel = (((pixel + 0.5f) / resolution) * 2.f - 1.f);
    RayDesc ray = GeneratePinholeCameraRay(pixel);
    TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);

    // Write the raytraced color to the output texture.
    RenderTarget[DispatchRaysIndex().xy] = payload.colorAndDistance;
}

[shader("closesthit")]
void PrimaryClosestHit(inout HitInfo payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    Instance instance = GetInstance(InstanceIndex());
    uint3 primitive = BufferLoad<uint3>(instance.BufferIndex, PrimitiveIndex(), instance.IndicesOffset);

    float3 normals[3];
    for (int i = 0; i < 3; ++i)
    {
        uint vertexID = primitive[i];
        normals[i] = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.NormalsOffset);
    }

    float3 vertexNormal = InterpolateVertex(normals[0], normals[1], normals[2], barycentrics);
    //vertexNormal = TransformDirection(instance.LocalTransform, vertexNormal);

    payload.colorAndDistance = float4(vertexNormal, 1.0f);
}

[shader("miss")]
void PrimaryMiss(inout HitInfo payload)
{
    payload.colorAndDistance = float4(0, 0, 0, 1);
}