﻿#pragma once

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

// Returns number of mipmap levels for specular IBL environment map.
uint QueryTextureLevels(uint tex)
{
    float3 dimensions = GetDimensionsCube(tex);
    return uint(dimensions.z);
}

// 4.10.2 - Specular occlusion from frostbite - https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float ComputeSpecOcclusion(float NdotV, float AO, float roughness)
{
    return saturate(pow(NdotV + AO, exp2(-16.0f * roughness - 1.0f)) - 1.0f + AO);
}

//
// IBL Calculation
//

float3 Diffuse_IBL(in ShadingData shading, in float3 F, in float3 N, uint IrradianceMap)
{
	// Get diffuse contribution factor (as with direct lighting).
    float3 kD = (1.0f - F) * (1.0f - shading.Metallic * (1.0f - shading.Roughness));
  
    float3 irradiance = SampleLevelCube(IrradianceMap, SLinearClamp, N, 0).rgb;
    return kD * irradiance * shading.BaseColor * shading.AO;
}

float3 Specular_IBL(in ShadingData shading, float3 F, float3 N, float3 V, float NdotV, uint PrefilterEnvMap, uint BRDFLUT)
{
	// Specular reflection vector.
    float3 R = 2.0 * NdotV * N - V;

    float specularLevels = QueryTextureLevels(PrefilterEnvMap);
    float3 prefilteredColor = SampleLevelCube(PrefilterEnvMap, SLinearClamp, R, shading.Roughness * specularLevels).rgb;

    // Split-sum approximation factors for Cook-Torrance specular BRDF.
    float2 envBRDF = Sample2D(BRDFLUT, SLinearClamp, float2(NdotV, shading.Roughness)).rg;

    // Total specular IBL contribution.
    return prefilteredColor * (F * envBRDF.x + envBRDF.y) * ComputeSpecOcclusion(NdotV, shading.AO, shading.Roughness);
}

float3 CalculateIBL(float3 N, float3 V, in ShadingData shading, uint IrradianceMap, uint PrefilterEnvMap, uint BRDFLUT)
{
    // Preserve specular highlight
    shading.Roughness = max(0.05, shading.Roughness);

    float NdotV = saturate(dot(N, V));
    float3 F0 = lerp(MIN_DIELECTRICS_F0, shading.BaseColor, shading.Metallic);

    float3 F = FresnelSchlickRoughness(NdotV, F0, shading.Roughness);

    float3 diffuse  = Diffuse_IBL(shading, F, N, IrradianceMap);
    float3 specular = Specular_IBL(shading, F, N, V, NdotV, PrefilterEnvMap, BRDFLUT);

    return diffuse + specular;
}