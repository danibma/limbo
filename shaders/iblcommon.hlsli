#pragma once

#include "common.hlsli"
#include "random.hlsli"
#include "brdf.hlsli"

static const float Epsilon = 0.00001;

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 Hammersley(uint i)
{
    return Hammersley(i, NumSamples);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float SchlickGGX_IBL(float NdotL, float V, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
    return SchlickG1(NdotL, k) * SchlickG1(V, k);
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html & Epic's s2013 presentation
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
	float phi = TwoPI * Xi.x;

    float3 H;
    H.x = sinTheta * cos(phi);
    H.y = sinTheta * sin(phi);
    H.z = cosTheta;
    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);

	// Tangent to world space
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

// Convert point from tangent/shading space to world space.
// Removed from the ImportanceSampleGGX, from Epic's s2013 presentation
float3 TangentToWorld(const float3 V, const float3 N)
{
    float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 TangentX = normalize(cross(UpVector, N));
    float3 TangentY = cross(N, TangentX);

    return TangentX * V.x + TangentY * V.y + N * V.z;
}

// Calculate normalized sampling direction vector based on current fragment coordinates.
// This is essentially "inverse-sampling": we reconstruct what the sampling vector would be if we wanted it to "hit"
// this particular fragment in a cubemap.
float3 GetSamplingVector(uint3 ThreadID, float OutputWidth, float OutputHeight)
{
	float2 st = ThreadID.xy / float2(OutputWidth, OutputHeight); // from screen coordinates to texture coordinates([0, 1])
	float2 uv = 2.0 * float2(st.x, 1.0 - st.y) - 1.0; // from [0, 1] to [-1, 1]

	// Select vector based on cubemap face index.
	float3 ret;
	switch (ThreadID.z)
	{
		case 0:
			ret = float3(1.0, uv.y, -uv.x);
			break;
		case 1:
			ret = float3(-1.0, uv.y, uv.x);
			break;
		case 2:
			ret = float3(uv.x, 1.0, -uv.y);
			break;
		case 3:
			ret = float3(uv.x, -1.0, uv.y);
			break;
		case 4:
			ret = float3(uv.x, uv.y, 1.0);
			break;
		case 5:
			ret = float3(-uv.x, uv.y, -1.0);
			break;
	}
	return normalize(ret);
}