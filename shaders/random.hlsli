#pragma once

#include "common.hlsli"

// Hash Functions for GPU Rendering - Nathan Reed
// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/

uint RandomSeed(uint seed)
{
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

uint RandomSeed(uint2 pixel, uint2 resolution, uint frameIndex)
{
	uint rngState = Flatten2D(pixel, resolution) ^ RandomSeed(frameIndex);
	return RandomSeed(rngState);
}

uint XORShift(inout uint seed)
{
	// Xorshift algorithm from George Marsaglia's paper.
	seed ^= (seed << 13);
	seed ^= (seed >> 17);
	seed ^= (seed << 5);
	return seed;
}

float Random01(inout uint seed)
{
	return asfloat(0x3f800000 | XORShift(seed) >> 9) - 1.0;
}

// Samples a direction within a hemisphere oriented along +Z axis with a *cosine-weighted* distribution
// Source: "Sampling Transformations Zoo" - Shirley et al. Ray Tracing Gems
float3 HemisphereSampleCosineWeight(float2 u, out float pdf)
{
	float a = sqrt(u.x);
	float b = 2.0f * PI * u.y;

	float3 result = float3(
		a * cos(b),
		a * sin(b),
		sqrt(1.0f - u.x));

	pdf = result.z * INV_PI;

	return result;
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 GetPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 GetCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
    float2 randVal = float2(Random01(randSeed), Random01(randSeed));

	// Cosine weighted hemisphere sample from RNG
    float3 bitangent = GetPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
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

// Van der Corput radical inverse - http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float RadicalInverse_VdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

// Sample i-th point from Hammersley point set of NumSamples points total.
float2 Hammersley(uint i, uint samples)
{
	return float2(float(i) / float(samples), RadicalInverse_VdC(i));
}
