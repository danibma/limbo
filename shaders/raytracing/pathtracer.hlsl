#include "raytracingcommon.hlsli"
#include "../random.hlsli"

RaytracingAccelerationStructure tSceneAS : register(t0, space0);
RWTexture2D<float4> uRenderTarget : register(u0);
RWTexture2D<float4> uAccumulationBuffer : register(u1);

cbuffer Constants : register(b0)
{
	uint cAccumulatedFrames;
};

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
    const uint depth = 10;
	const uint samples = 5;

    pixel = (((pixel + 0.5f) / resolution) * 2.f - 1.f);

	float3 color = 0.0f;
	for (uint s = 0; s < samples; ++s)
	{
		RayDesc ray = GeneratePinholeCameraRay(pixel);

		float3 result = 0.0f;
		float3 attenuation = 1.0f;
		for (int i = 0; i < depth; ++i)
		{
			MaterialRayTracingPayload payload = TraceMaterialRay(tSceneAS, ray, RAY_FLAG_FORCE_OPAQUE);

			Instance instance = GetInstance(payload.InstanceID);
			VertexAttributes vertex = GetVertexAttributes(instance, payload.Barycentrics, payload.PrimitiveID);
			Material material = GetMaterial(instance.Material);
			ShadingData shadingData = GetShadingData(material, vertex);

			if (GSceneInfo.SceneViewToRender > 0)
			{
				uRenderTarget[DispatchRaysIndex().xy] = GetSceneDebugView(shadingData);
				return;
			}

			float3 geometryNormal = vertex.GeometryNormal;

			if (!payload.IsFrontFace())
				geometryNormal = -geometryNormal;

			if (!payload.IsHit())
			{
				result = GetSky(ray.Direction);
				break;
			}

			ray.Origin = ray.Origin + payload.Distance * ray.Direction;
			ray.Direction = GetCosHemisphereSample(seed, geometryNormal);
			attenuation *= shadingData.Albedo;
		}

		color += result * attenuation;
	}

	color /= float(samples);

	float3 accumulatedColor = color;
	
	if (cAccumulatedFrames > 1)
	{
		float3 previousColor = uAccumulationBuffer[DispatchRaysIndex().xy].rgb;
		accumulatedColor += previousColor;
	}

    uAccumulationBuffer[DispatchRaysIndex().xy] = float4(accumulatedColor, 1.0f);
	uRenderTarget[DispatchRaysIndex().xy] = float4(accumulatedColor / cAccumulatedFrames, 1.0f);
}
