#include "bindings.hlsli"

uint instanceID;
uint cascadeIndex;

float4 VSMain(uint vertexID : SV_VertexID) : SV_Position
{
	Instance instance = GetInstance(instanceID);
    float3 pos = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.PositionsOffset);

    float4x4 mvp = mul(GShadowData.LightViewProj[cascadeIndex], instance.LocalTransform);
    return mul(mvp, float4(pos, 1.0f));
}

float4 PSMain(float4 v : SV_Position) : SV_Target
{
    return float4(v.zzz, 1.0f);
}
