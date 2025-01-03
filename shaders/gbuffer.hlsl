#include "common.hlsli"

struct VSOut
{
    float4 Position : SV_Position;
    float4 PixelPos : PixelPosition;
    float4 WorldPos : WorldPosition;
    float3 Normal   : Normal;
	float3x3 TBN	: TBN; 
    float2 UV       : TEXCOORD;

    uint MeshletIndex : MeshletIndex;
};

uint cInstanceID;

VSOut MainVS(uint vertexID : SV_VertexID)
{
    VSOut result = (VSOut)0;

    Instance instance = GetInstance(cInstanceID);
    MeshVertex vertex = BufferLoad<MeshVertex>(instance.BufferIndex, vertexID, instance.VerticesOffset);

	float3 normal = TransformNormal(instance.LocalTransform, vertex.Normal);
	float3 tangent = normalize(TransformNormal(instance.LocalTransform, vertex.Tangent.xyz));

	float3 bitangent = cross(normal, tangent) * vertex.Tangent.w;
	
    float4x4 mvp    = mul(GSceneInfo.ViewProjection, instance.LocalTransform);
    result.Position = mul(mvp, float4(vertex.Position, 1.0f));
    result.PixelPos = mul(GSceneInfo.View, mul(instance.LocalTransform, float4(vertex.Position, 1.0f)));
    result.WorldPos = mul(instance.LocalTransform, float4(vertex.Position, 1.0f));
    result.Normal   = normal;
	result.TBN		= transpose(float3x3(tangent, bitangent, normal));
    result.UV       = vertex.UV;

	return result;
}

[outputtopology("triangle")]
[numthreads(MS_NUM_BLOCKS, 1, 1)]
void MainMS(uint threadID : SV_DispatchThreadID, uint gtID : SV_GroupThreadID, uint groupID : SV_GroupID, 
		    out indices uint3 tris[MESHLET_MAX_TRIANGLES], out vertices VSOut verts[MESHLET_MAX_VERTICES])
{
    Instance instance = GetInstance(cInstanceID);
    Meshlet meshlet = BufferLoad<Meshlet>(instance.BufferIndex, groupID, instance.MeshletsOffset);

    SetMeshOutputCounts(meshlet.VertexCount, meshlet.TriangleCount);

    if (gtID < meshlet.TriangleCount)
    {
        uint triangleID = gtID + meshlet.TriangleOffset;
    	Meshlet::Triangle t = BufferLoad<Meshlet::Triangle>(instance.BufferIndex, triangleID, instance.MeshletsTrianglesOffset);
        tris[gtID] = uint3(t.V0, t.V1, t.V2);
    }

    if (gtID < meshlet.VertexCount)
    {
	    VSOut result;

        uint vertexID = BufferLoad<uint>(instance.BufferIndex, gtID + meshlet.VertexOffset, instance.MeshletsVerticesOffset);

    	MeshVertex vertex = BufferLoad<MeshVertex>(instance.BufferIndex, vertexID, instance.VerticesOffset);

    	float3 normal = normalize(TransformNormal(instance.LocalTransform, vertex.Normal));
    	float3 tangent = normalize(TransformNormal(instance.LocalTransform, vertex.Tangent.xyz));

    	float3 bitangent = cross(normal, tangent) * vertex.Tangent.w;
	
    	float4x4 mvp    = mul(GSceneInfo.ViewProjection, instance.LocalTransform);
    	result.Position = mul(mvp, float4(vertex.Position, 1.0f));
    	result.PixelPos = mul(GSceneInfo.View, mul(instance.LocalTransform, float4(vertex.Position, 1.0f)));
    	result.WorldPos = mul(instance.LocalTransform, float4(vertex.Position, 1.0f));
    	result.Normal   = normal;
    	result.TBN		= transpose(float3x3(tangent, bitangent, normal));
    	result.UV       = vertex.UV;
    	
        result.MeshletIndex = groupID;
		verts[gtID] = result;    
    }
}

struct DeferredShadingOutput
{
    float4 PixelPosition            : SV_Target0;
    float4 WorldPosition            : SV_Target1;
    float4 Albedo                   : SV_Target2;
    float4 Normal                   : SV_Target3;
    float4 RoughnessMetallicAO      : SV_Target4;
    float4 Emissive                 : SV_Target5;
};

DeferredShadingOutput MainPS(VSOut input)
{
    DeferredShadingOutput result;

    Instance instance = GetInstance(cInstanceID);
    Material material = GetMaterial(instance.Material);

    float4 finalAlbedo = material.BaseColorFactor;
    if (material.BaseColorIndex != -1)
    {
	    float4 albedo = Sample2D(material.BaseColorIndex, SLinearWrap, input.UV);
	    finalAlbedo *= albedo;
    }

    if (finalAlbedo.a <= ALPHA_THRESHOLD)
        discard;

	float roughness = material.RoughnessFactor;
    float metallic = material.MetallicFactor;
    if (material.RoughnessMetalIndex != -1)
    {
        float4 roughnessMetalMap = Sample2D(material.RoughnessMetalIndex, SLinearWrap, input.UV);
        roughness *= roughnessMetalMap.g;
        metallic *= roughnessMetalMap.b;
    }

    float3 emissive = material.EmissiveFactor;
    if (material.EmissiveIndex != -1)
    {
        emissive = Sample2D(material.EmissiveIndex, SLinearWrap, input.UV).rgb;
    }

    float ao = 1.0f;
    if (material.AmbientOcclusionIndex != -1)
    {
        float4 AOMap = Sample2D(material.AmbientOcclusionIndex, SLinearWrap, input.UV);
		ao = AOMap.r;
    }

    float3 normal = normalize(input.Normal);
    if (material.NormalIndex != -1)
    {
        float4 normalMap = UnpackNormalMap(Sample2D(material.NormalIndex, SLinearWrap, input.UV));
        normal = normalize(mul(input.TBN, normalMap.rgb));
    }

    //uint meshletIndex = input.MeshletIndex;
	//finalAlbedo = float4(float(meshletIndex & 1), float(meshletIndex & 3) / 4, float(meshletIndex & 7) / 8, 1);

    result.PixelPosition        = input.PixelPos;
    result.WorldPosition        = input.WorldPos;
    result.Albedo               = finalAlbedo;
    result.Normal               = float4(normal, 1.0f);
    result.RoughnessMetallicAO  = float4(roughness, metallic, ao, 1.0f);
    result.Emissive             = float4(emissive, 1.0f);

    return result;
}
