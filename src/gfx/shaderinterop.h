#pragma once

#ifdef __cplusplus
	#include "core/math.h"

	#define __CONST constexpr
#else
	#define __CONST static const
#endif

#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )
#define __PAD uint MACRO_CONCAT(padding, __COUNTER__)

#define SHADOWMAP_CASCADES 4

__CONST uint SHADOWMAP_SIZES[SHADOWMAP_CASCADES] =
{
	2048,
	2048,
	2048,
	2048,
};

struct SceneInfo
{
	float4x4	View;
	float4x4	InvView;
	float4x4	PrevView;
	float4x4	Projection;
	float4x4	InvProjection;
	float4x4	ViewProjection;
	float4		SunDirection;
	float3		CameraPos;

	uint		FrameIndex;
	uint		SkyIndex;
	uint		MaterialsBufferIndex;
	uint		InstancesBufferIndex;
	uint		SceneViewToRender;

	uint		bSunCastsShadows;
	uint		bShowShadowCascades;
	__PAD;
	__PAD;
};
#ifdef __cplusplus
static_assert(sizeof(SceneInfo) % 16 == 0);
#endif

struct ShadowData
{
	float4x4 LightViewProj[SHADOWMAP_CASCADES];
	float4	 SplitDepth;
	uint4	 ShadowMap;
};
#ifdef __cplusplus
static_assert(sizeof(ShadowData) % 16 == 0);
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

//
// RayTracing Payloads
//
struct MaterialRayTracingPayload
{
	float   Distance;
	uint    PrimitiveID;
	uint    InstanceID;
	float2  Barycentrics;
	uint    FrontFace;

	bool IsHit() { return Distance > 0; }
	bool IsFrontFace() { return FrontFace > 0; }
};

struct AORayPayload
{
	float AOVal; // Stores 0 on a ray hit, 1 on ray miss
};
