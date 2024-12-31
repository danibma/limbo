#include "bindings.hlsli"

uint cCascadeIndex;
uint cInstanceID;

float4 MainVS(uint vertexID : SV_VertexID) : SV_Position
{
	Instance instance = GetInstance(cInstanceID);
	MeshVertex vertex = BufferLoad<MeshVertex>(instance.BufferIndex, vertexID, instance.VerticesOffset);

    float4x4 mvp = mul(GShadowData.LightViewProj[cCascadeIndex], instance.LocalTransform);
    return mul(mvp, float4(vertex.Position, 1.0f));
}
