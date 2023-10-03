#include "raytracingcommon.hlsli"

[shader("closesthit")]
void MaterialClosestHit(inout MaterialRayTracingPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.PrimitiveID		= PrimitiveIndex();
    payload.InstanceID		= InstanceIndex();
	payload.Distance		= RayTCurrent();
	payload.Barycentrics	= attr.barycentrics;
}

[shader("miss")]
void MaterialMiss(inout MaterialRayTracingPayload payload)
{
	payload = (MaterialRayTracingPayload)0;
}

[shader("anyhit")]
void MaterialAnyHit(inout MaterialRayTracingPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics)) IgnoreHit();
}