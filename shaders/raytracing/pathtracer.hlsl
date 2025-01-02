#include "raytracingcommon.hlsli"
#include "../random.hlsli"
#include "../brdf.hlsli"

RaytracingAccelerationStructure tSceneAS : register(t0, space0);
RWTexture2D<float4> uRenderTarget : register(u0);
RWTexture2D<float4> uAccumulationBuffer : register(u1);

ConstantBuffer<PathTracerConstants> c : register(b0);

struct Light
{
	float3 LightPos;
	float3 LightColor;
};
ConstantBuffer<Light> cLight : register(b1);

enum class EBRDFType
{
	Diffuse,
	Specular
};

struct ShadowPayload
{
	bool bHit;
};

struct LightData
{
	float3 Position;
	float Distance;
	float Intensity;
};

#define SUN_ONLY 1

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
	ray.TMin = 0.00001f;
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

// Approximates luminance from an RGB value
float Luminance(float3 color)
{
	return dot(color, float3(0.2126f, 0.7152f, 0.0722f));
}

float GetBRDFProbability(ShadingData shading, float3 V)
{
	float F0 = Luminance(lerp(MIN_DIELECTRICS_F0, shading.Albedo, shading.Metallic));
	float diffuseReflectance = Luminance(shading.Albedo * (1 - shading.Metallic));
	float fresnel = saturate(Luminance(FresnelSchlick(dot(shading.ShadingNormal, V), F0)));
	
	float specular = fresnel;
	float diffuse = diffuseReflectance * (1 - fresnel); // If diffuse term is weighted by Fresnel, apply it here as well

	return clamp(specular / (specular + diffuse), 0.1f, 0.9f);
}

void EvaluateIndirectCombinedBRDF(EBRDFType brdfType, float2 u, ShadingData shadingData, float3 V, out float3 rayDirection, out float3 brdfWeight)
{
	float NdotV = max(dot(shadingData.ShadingNormal, V), 0.00001f);
	
	float3 F0 = lerp(MIN_DIELECTRICS_F0.xxx, shadingData.Albedo, shadingData.Metallic);
	float3 fresnel = FresnelSchlick(NdotV, F0);
	
	if (brdfType == EBRDFType::Specular)
	{
		float3 Wh = shadingData.ShadingNormal;

		// If the surface is rough, sample a new direction using the GGX normal distribution function
		if (shadingData.Roughness > 0.0f)
		{
			Wh = ImportanceSampleGGX(u, shadingData.Roughness, shadingData.ShadingNormal);
		}

		// -V = previous ray direction
		rayDirection = normalize(reflect(-V, Wh));
		
		float NdotL = max(dot(shadingData.ShadingNormal, rayDirection), 0.00001f);
		brdfWeight = fresnel * SchlickGGX(NdotL, NdotV, shadingData.Roughness);
	}
	else
	{
		float pdf;
		rayDirection = CosineWeightSampleHemisphere(u, pdf);

		// DiffuseReflectance from lambertian diffuse
		brdfWeight = (1.0 - shadingData.Metallic) * shadingData.Albedo;
		brdfWeight *= (1.0 - fresnel);
	}
}

LightData SampleLightData(inout uint4 seed, float3 hitPosition, out float lightSampleWeight)
{
	const uint lightCount = 2; // One directional and one point
	uint randomLightIndex = min(lightCount - 1, uint(Random01_PCG4(seed) * lightCount));

	lightSampleWeight = float(lightCount);

	LightData data;
	if (randomLightIndex == 0 || SUN_ONLY) // Directional
	{
		data.Position = normalize(GSceneInfo.SunDirection.xyz);
		data.Intensity = GSceneInfo.SunIntensity;
		data.Distance = FLT_MAX;
	}
	else // Point
	{
		data.Position = cLight.LightPos - hitPosition;
		data.Intensity = 10.0f; // TODO
		data.Distance = length(data.Position);
	}

	return data;
}

// Clever offset_ray function from Ray Tracing Gems chapter 6
// Offsets the ray origin from current position p, along normal n (which must be geometric normal)
// so that no self-intersection can occur.
float3 OffsetRay(const float3 p, const float3 n)
{
	static const float origin = 1.0f / 32.0f;
	static const float float_scale = 1.0f / 65536.0f;
	static const float int_scale = 256.0f;

	int3 of_i = int3(int_scale * n.x, int_scale * n.y, int_scale * n.z);

	float3 p_i = float3(
		asfloat(asint(p.x) + ((p.x < 0) ? -of_i.x : of_i.x)),
		asfloat(asint(p.y) + ((p.y < 0) ? -of_i.y : of_i.y)),
		asfloat(asint(p.z) + ((p.z < 0) ? -of_i.z : of_i.z)));

	return float3(abs(p.x) < origin ? p.x + float_scale * n.x : p_i.x,
		abs(p.y) < origin ? p.y + float_scale * n.y : p_i.y,
		abs(p.z) < origin ? p.z + float_scale * n.z : p_i.z);
}

bool CastShadowRay(float3 hitPosition, float3 N, float3 L, float lightDistance)
{
	RayDesc ray;
	ray.Origin = OffsetRay(hitPosition, N);
	ray.Direction = L;
	ray.TMin = 0.0f;
	ray.TMax = lightDistance;

	ShadowPayload payload;
	payload.bHit = true; // It will be set to false on a miss

	TraceRay(
		tSceneAS,				// AccelerationStructure
		RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_FORCE_NON_OPAQUE, // RayFlags
		0xFF,					// InstanceInclusionMask
		1,                      // RayContributionToHitGroupIndex
		0,                      // MultiplierForGeometryContributionToHitGroupIndex
		1,                      // MissShaderIndex
		ray,                    // Ray
		payload                 // Payload
	);

	return !payload.bHit;
}

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
	payload.bHit = false;
}

[shader("anyhit")]
void ShadowAnyHit(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics))
		IgnoreHit();
	else
		AcceptHitAndEndSearch();
}

[shader("raygeneration")]
void PTRayGen()
{
    float2 pixel = float2(DispatchRaysIndex().xy);
    float2 resolution = float2(DispatchRaysDimensions().xy);

    uint4 seed = InitSeed_PCG4(pixel, resolution, GSceneInfo.FrameIndex);
    const uint maxBounces = 8;
	const uint minBounces = 3;
	const uint samples = 1;

	// Jittering primary ray directions for antialiasing. Add a random offset to the pixel's screen coordinates.
	if (c.bAntiAliasingEnabled)
	{
		float2 offset = float2(Random01_PCG4(seed), Random01_PCG4(seed));
		pixel += lerp(-0.5f, 0.5f, offset);	
	}
	
    pixel = (((pixel + 0.5f) / resolution) * 2.f - 1.f);

	float3 color = 0.0f;
	for (uint s = 0; s < samples; ++s)
	{
		RayDesc ray = GeneratePinholeCameraRay(pixel);

		float3 radiance = 0.0f;
		float3 throughput = 1.0f;
		for (int bounce = 0; bounce < maxBounces; ++bounce)
		{
			PathTracingPayload payload = (PathTracingPayload)0;

			TraceRay(
				tSceneAS,				// AccelerationStructure
				RAY_FLAG_NONE,			// RayFlags
				0xFF,					// InstanceInclusionMask
				0,                      // RayContributionToHitGroupIndex
				0,                      // MultiplierForGeometryContributionToHitGroupIndex
				0,                      // MissShaderIndex
				ray,                    // Ray
				payload                 // Payload
			);

			Instance instance = GetInstance(payload.InstanceID);
			VertexAttributes vertex = GetVertexAttributes(instance, payload.Barycentrics, payload.PrimitiveID);
			Material material = GetMaterial(instance.Material);
			ShadingData shadingData = GetShadingData(material, vertex);

			if (GSceneInfo.SceneViewToRender > 0)
			{
				uRenderTarget[DispatchRaysIndex().xy] = GetSceneDebugView(shadingData);
				return;
			}

			if (!payload.IsHit())
			{
				radiance += throughput * GetSky(ray.Direction);
				break;
			}

			radiance += throughput * shadingData.Emissive;

			float3 V = -ray.Direction;
			if (dot(shadingData.ShadingNormal, V) < 0.0f) shadingData.ShadingNormal = -shadingData.ShadingNormal;
			if (dot(shadingData.ShadingNormal, shadingData.GeometryNormal) < 0.0f) shadingData.GeometryNormal = -shadingData.GeometryNormal;
			ray.Origin = OffsetRay(vertex.Position, shadingData.GeometryNormal);

			// Directional light calculation
			{
				float lightSampleWeight;
				LightData light = SampleLightData(seed, vertex.Position, lightSampleWeight);
				float3 L = normalize(light.Position);
				if (CastShadowRay(vertex.Position, shadingData.GeometryNormal, L, light.Distance))
				{
					MaterialProperties props;
					props.BaseColor = shadingData.Albedo;
					props.Normal    = shadingData.ShadingNormal;
					props.Roughness = shadingData.Roughness;
					props.Metallic  = shadingData.Metallic;
					props.AO        = 1.0f;

					radiance += throughput * DefaultBRDF(V, shadingData.ShadingNormal, L, props) * (light.Intensity * lightSampleWeight);
				}
			}

			// Russian roulette, decides if paths should terminate earlier
			if (bounce > minBounces)
			{
				float rr_p = min(0.95f, Luminance(throughput));
				if (rr_p < Random01_PCG4(seed))
					break;
				else
					throughput /= rr_p;
			}

			// Indirect lighting
			{
				EBRDFType brdfType;
				if (shadingData.Metallic == 1 && shadingData.Roughness == 0)
				{
					// Fast path for mirrors
					brdfType = EBRDFType::Specular;
				}
				else
				{
					float brdfProbability = GetBRDFProbability(shadingData, V);
					if (Random01_PCG4(seed) < brdfProbability)
					{
						brdfType = EBRDFType::Specular;
						throughput /= brdfProbability;
					}
					else
					{
						brdfType = EBRDFType::Diffuse;
						throughput /= (1 - brdfProbability);
					}
				}
			
				float3 brdfWeight;
				EvaluateIndirectCombinedBRDF(brdfType, float2(Random01_PCG4(seed), Random01_PCG4(seed)), shadingData, V, ray.Direction, brdfWeight);
				throughput *= brdfWeight;
			}
		}

		color += radiance;
	}

	color /= float(samples);

	float3 accumulatedColor = color;
	
	if (c.NumAccumulatedFrames > 1 && c.bAccumulateEnabled)
	{
		float3 previousColor = uAccumulationBuffer[DispatchRaysIndex().xy].rgb;
		accumulatedColor += previousColor;
	}

    uAccumulationBuffer[DispatchRaysIndex().xy] = float4(accumulatedColor, 1.0f);
	uRenderTarget[DispatchRaysIndex().xy] = float4(accumulatedColor / c.NumAccumulatedFrames, 1.0f);
}

[shader("closesthit")]
void PTClosestHit(inout PathTracingPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	payload.PrimitiveID		= PrimitiveIndex();
	payload.InstanceID		= InstanceIndex();
	payload.Distance		= RayTCurrent();
	payload.Barycentrics	= attr.barycentrics;
}

[shader("miss")]
void PTMiss(inout PathTracingPayload payload)
{
	payload = (PathTracingPayload)0;
}

[shader("anyhit")]
void PTAnyHit(inout PathTracingPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
	if (!AnyHitAlphaTest(attr.barycentrics)) IgnoreHit();
}