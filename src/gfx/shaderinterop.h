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

/* According to https://developer.nvidia.com/blog/advanced-api-performance-mesh-shaders/ */
__CONST int MESHLET_MAX_TRIANGLES = 124;
__CONST int MESHLET_MAX_VERTICES = 64;

#define SHADOWMAP_CASCADES 4

__CONST uint SHADOWMAP_SIZES[SHADOWMAP_CASCADES] =
{
	2048,
	2048,
	2048,
	2048,
};

enum class Tonemap
{
	None = 0,
	AcesFilm,
	Reinhard,
	Uncharted2,
	Unreal,
	GT,
	Agx,

	MAX
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
	float		SunIntensity;

	uint		FrameIndex;
	uint		SkyIndex;
	uint		MaterialsBufferIndex;
	uint		InstancesBufferIndex;
	uint		SceneViewToRender;

	uint		bSunCastsShadows;
	uint		bShowShadowCascades;
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
	float3 Tangent;
	float2 UV;
};

struct Instance
{
	uint		Material;
	uint		BufferIndex;
	uint		VerticesOffset;
	uint		IndicesOffset;
	uint		MeshletsOffset;
	uint		MeshletsTrianglesOffset;
	uint		MeshletsVerticesOffset;

	float4x4	LocalTransform;
};

struct Meshlet
{
	/* Offsets within MeshletVertices and MeshletTriangles buffers inside the correspoding scene's GeometryBuffer */
	uint VertexOffset;
	uint TriangleOffset;

	/* Number of vertices and triangles used in the meshlet */
	uint VertexCount;
	uint TriangleCount;

	struct Triangle
	{
		uint V0 : 10;
		uint V1 : 10;
		uint V2 : 10;
		uint : 2;
	};
};

struct PathTracerConstants
{
	uint NumAccumulatedFrames;
	uint bAccumulateEnabled;
	uint bAntiAliasingEnabled;
};

//
// RayTracing Payloads
//
struct PathTracingPayload
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
