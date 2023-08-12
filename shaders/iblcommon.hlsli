﻿#pragma once

#include "common.hlsli"
#include "random.hlsli"

static const float Epsilon = 0.00001;

static const uint NumSamples = 1024;
static const float InvNumSamples = 1.0 / float(NumSamples);

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 Hammersley(uint i)
{
    return Hammersley(i, NumSamples);
}

// Uniformly sample point on a hemisphere.
// Cosine-weighted sampling would be a better fit for Lambertian BRDF but since this
// compute shader runs only once as a pre-processing step performance is not *that* important.
// See: "Physically Based Rendering" 2nd ed., section 13.6.1.
float3 SampleHemisphere(float u1, float u2)
{
	const float u1p = sqrt(max(0.0, 1.0 - u1 * u1));
	return float3(cos(TwoPI * u2) * u1p, sin(TwoPI * u2) * u1p, u1);
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

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NDF_GGX(float NdotH, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
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