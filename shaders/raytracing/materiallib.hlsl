#include "raytracingcommon.hlsli"

[shader("closesthit")]
void MaterialClosestHit(inout MaterialPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	Instance instance = GetInstance(InstanceIndex());
	MeshVertex vertex = GetVertex(instance, attr.barycentrics, PrimitiveIndex());

	Material material = GetMaterial(instance.Material);

    // In the future is probably a good idea to implement some mip mapping calculation, like the Ray Cone method from
    // Texture Level of Detail Strategies for Real-Time Ray Tracing - RayTracing Gems book
	const float mipLevel = 0;

	float4 albedo = material.AlbedoFactor;
	albedo *= SampleLevel2D(material.AlbedoIndex, LinearWrap, vertex.UV, mipLevel);

	payload.ColorAndDistance = float4(albedo.xyz, RayTCurrent());
}

[shader("miss")]
void MaterialMiss(inout MaterialPayload payload)
{
	payload.ColorAndDistance = (float4) 0;
}

[shader("anyhit")]
void MaterialAnyHit(inout MaterialPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics)) IgnoreHit();
}