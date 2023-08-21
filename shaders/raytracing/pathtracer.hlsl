#include "raytracingcommon.hlsli"
#include "../random.hlsli"

RaytracingAccelerationStructure Scene : register(t0, space0);
RWTexture2D<float4> RenderTarget : register(u0);

// From Ray Tracing Gems II - Chapter 14
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

    uint seed = RandomSeed(pixel, resolution, GSceneInfo.FrameIndex);

    pixel = (((pixel + 0.5f) / resolution) * 2.f - 1.f);
    RayDesc ray = GeneratePinholeCameraRay(pixel);

    float3 radiance = 0.0f;
    float3 weight   = 1.0f;

    const int bounces = 5;
    for (int i = 0; i < bounces; ++i)
    {
        MaterialPayload payload = TraceMaterialRay(Scene, ray, RAY_FLAG_FORCE_OPAQUE);

        if (payload.IsHit() <= 0)
        {
            RenderTarget[DispatchRaysIndex().xy] = float4(GetSky(ray.Direction), 1.0f);
            return;
        }

        Instance instance = GetInstance(payload.InstanceID);
        VertexAttributes vertex = GetVertexAttributes(instance, payload.Barycentrics, payload.PrimitiveID);
        Material material = GetMaterial(instance.Material);
        ShadingData shadingData = GetShadingData(material, vertex);


        if (GSceneInfo.SceneViewToRender > 0)
        {
            RenderTarget[DispatchRaysIndex().xy] = GetSceneDebugView(shadingData);
            return;
        }

        float3 geometryNormal = vertex.GeometryNormal;

        if (!payload.IsFrontFace())
            geometryNormal = -geometryNormal;

        radiance += weight * shadingData.Emissive;
        ray.Origin += ray.Direction * payload.Distance;
        ray.Direction = GetCosHemisphereSample(seed, geometryNormal);
        weight *= shadingData.Albedo * 2.0f * dot(geometryNormal, ray.Direction);
    }

    radiance /= float(bounces);
	RenderTarget[DispatchRaysIndex().xy] = float4(radiance, 1.0f);
}
