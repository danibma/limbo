#include "common.hlsli"

//
// Vertex Shader
//
QuadResult VSMain(uint vertexID : SV_VertexID)
{
	uint vertex = vertexID * 4;

	QuadResult result;
	result.Position.x = quadVertices[vertex];
	result.Position.y = quadVertices[vertex + 1];
	result.Position.z = 0.0f;
	result.Position.w = 1.0f;

	result.UV.x = quadVertices[vertex + 2];
	result.UV.y = quadVertices[vertex + 3];

	return result;
}

//
// Pixel Shader
//
Texture2D g_Albedo;
Texture2D g_Normal;
Texture2D g_Roughness;
Texture2D g_Metallic;
Texture2D g_Emissive;

float4 PSMain(QuadResult quad) : SV_Target0
{
	float4 albedo = g_Albedo.Sample(LinearWrap, quad.UV);

	return albedo;
}

