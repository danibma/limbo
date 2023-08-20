#include "raytracingcommon.hlsli"

[shader("closesthit")]
void MaterialClosestHit(inout MaterialPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.PrimitiveID		= PrimitiveIndex();
    payload.InstanceID		= InstanceIndex();
	payload.Distance		= RayTCurrent();
	payload.Barycentrics	= attr.barycentrics;
}

[shader("miss")]
void MaterialMiss(inout MaterialPayload payload)
{
	payload = (MaterialPayload)0;
}

[shader("anyhit")]
void MaterialAnyHit(inout MaterialPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics)) IgnoreHit();
}