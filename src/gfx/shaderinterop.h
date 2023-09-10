#pragma once

#ifdef __cplusplus
	#include "core/math.h"
#endif

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define PAD uint MACRO_CONCAT(padding, __COUNTER__)

#define SHADOWMAP_SIZE     2048
#define SHADOWMAP_CASCADES 4

struct SceneInfo
{
	float4x4	View;
	float4x4	InvView;
	float4x4	PrevView;
	float4x4	Projection;
	float4x4	InvProjection;
	float4x4	ViewProjection;
	float4x4	SunViewProj;
	float4		SunDirection;
	float3		CameraPos;

	uint		FrameIndex;
	uint		SkyIndex;
	uint		MaterialsBufferIndex;
	uint		InstancesBufferIndex;
	uint		SceneViewToRender;

	uint		bSunCastsShadows : 1;
	PAD;
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

	float4 AlbedoFactor;
	float  MetallicFactor;
	float  RoughnessFactor;
	float3 EmissiveFactor;
};

struct MeshVertex
{
	float3 Position;
	float3 Normal;
	float2 UV;
};

struct Instance
{
	uint		Material;
	uint		BufferIndex;
	uint		PositionsOffset;
	uint		NormalsOffset;
	uint		TexCoordsOffset;
	uint		IndicesOffset;

	float4x4	LocalTransform;
};
