#pragma once

#ifdef __cplusplus
	#include "core/math.h"
#endif

struct Material
{
	int    AlbedoIndex;
	int    NormalIndex;
	int    RoughnessMetalIndex;
	int    EmissiveIndex;
	int    AmbientOcclusionIndex;

	uint3  SPACING;

	float4 AlbedoFactor;
	float  MetallicFactor;
	float  RoughnessFactor;
};
