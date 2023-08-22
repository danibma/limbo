#include "common.hlsli"

//
// Vertex Shader
//
struct SkyboxVertexOutput
{
    float4 pixel_position : SV_Position;
    float4 local_position : Position;
};

SkyboxVertexOutput VSMain(float3 pos : Position)
{
    float4x4 rot_view = GSceneInfo.View;
    rot_view._m03_m13_m23 = 0.0; // remove translation
    
    SkyboxVertexOutput output;
    output.local_position = float4(pos, 1.0f);
    output.pixel_position = mul(mul(GSceneInfo.Projection, rot_view), float4(pos, 1.0f));
    output.pixel_position.z = 1.0f;
	
    return output;
}

//
// Pixel Shader
//
TextureCube g_EnvironmentCube;

float4 PSMain(SkyboxVertexOutput vertex) : SV_Target
{
    float3 color = g_EnvironmentCube.SampleLevel(SLinearWrap, vertex.local_position.xyz, 0).xyz;

    return float4(color, 1.0);
}
