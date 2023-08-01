#include "common.hlsli"

Texture2D g_EnvironmentEquirectangular : register(t0);
RWTexture2DArray<float4> g_OutEnvironmentCubemap : register(u0);

[numthreads(32, 32, 1)]
void EquirectToCubemap(uint3 ThreadID : SV_DispatchThreadID)
{
	float outputWidth, outputHeight, outputDepth;
	g_OutEnvironmentCubemap.GetDimensions(outputWidth, outputHeight, outputDepth);
	float3 v = GetSamplingVector(ThreadID, outputWidth, outputHeight);
	
	// Convert Cartesian direction vector to spherical coordinates.
	float phi = atan2(v.z, v.x);
	float theta = acos(v.y);

	// Sample equirectangular texture.
	float4 color = g_EnvironmentEquirectangular.SampleLevel(LinearWrap, float2(phi / TwoPI, theta / PI), 0);

	// Write out color to output cubemap.
	g_OutEnvironmentCubemap[ThreadID] = color;
}

//
// Vertex Shader
//
float4x4 view;
float4x4 proj;

struct SkyboxVertexOutput
{
    float4 pixel_position : SV_Position;
    float4 local_position : Position;
};

SkyboxVertexOutput VSMain(float3 pos : Position, float3 normal : Normal, float2 uv : UV)
{
    float4x4 rot_view = view;
    rot_view._m03_m13_m23 = 0.0; // remove translation
    
    SkyboxVertexOutput output;
    output.local_position = float4(pos, 1.0f);
    output.pixel_position = mul(mul(proj, rot_view), float4(pos, 1.0f));
    output.pixel_position.z = 1.0f;
	
    return output;
}

//
// Pixel Shader
//
TextureCube g_EnvironmentCube;

float4 PSMain(SkyboxVertexOutput vertex) : SV_Target
{
    float3 color = g_EnvironmentCube.SampleLevel(LinearWrap, vertex.local_position.xyz, 0).xyz;

    return float4(color, 1.0);
}
