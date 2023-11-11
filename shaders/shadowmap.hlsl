#include "bindings.hlsli"

uint cCascadeIndex;
uint cInstanceID;

float4 MainVS(uint vertexID : SV_VertexID) : SV_Position
{
	Instance instance = GetInstance(cInstanceID);
    float3 pos = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.PositionsOffset);

    float4x4 mvp = mul(GShadowData.LightViewProj[cCascadeIndex], instance.LocalTransform);
    return mul(mvp, float4(pos, 1.0f));
}
