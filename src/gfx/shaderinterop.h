﻿#pragma once

#ifdef __cplusplus
	#include "core/math.h"
#endif

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define PAD uint MACRO_CONCAT(padding, __COUNTER__)

struct SceneInfo
{
	uint MaterialsBufferIndex;
	uint InstancesBufferIndex;
	PAD;
	PAD;
};
#ifdef __cplusplus
static_assert(sizeof(SceneInfo) % 16 == 0);
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

struct MeshVertex
{
	float3 Position;
	float3 Normal;
	float2 UV;
};

struct Instance
{
	uint Material;
};
