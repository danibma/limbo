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

// note: For Raytracing at the moment, probably going to get removed in the future
struct Viewport
{
	float left;
	float top;
	float right;
	float bottom;
};

struct RayGenConstantBuffer
{
	Viewport viewport;
	Viewport stencil;
};