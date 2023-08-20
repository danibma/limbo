#include "raytracingcommon.hlsli"

struct AORayPayload
{
	float aoVal; // Stores 0 on a ray hit, 1 on ray miss
};

[shader("raygeneration")]
void RTAORayGen()
{

}

[shader("closesthit")]
void RTAOClosestHit(inout AORayPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	
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
