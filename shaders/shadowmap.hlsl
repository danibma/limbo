#include "bindings.hlsli"

float4x4 OrthographicProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
   float4x4 result =
	{
	    2.0f / (right - left), 0, 0, -(right + left) / (right - left),
	    0, 2.0f / (top - bottom), 0, -(top + bottom) / (top - bottom),
	    0, 0, -1.0f / (zFar - zNear), -zNear / (zFar - zNear),
	    0, 0, 0, 1,
	};
    return result;
}

uint instanceID;

float4 VSMain(uint vertexID : SV_VertexID) : SV_Position
{
	Instance instance = GetInstance(instanceID);
    float3 pos = BufferLoad<float3>(instance.BufferIndex, vertexID, instance.PositionsOffset);

    float4x4 mvp = mul(GSceneInfo.SunViewProj, instance.LocalTransform);
    return mul(mvp, float4(pos, 1.0f));
}

float4 PSMain(float4 v : SV_Position) : SV_Target
{
    return float4(v.zzz, 1.0f);
}
