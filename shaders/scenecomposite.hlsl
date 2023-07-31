#include "common.hlsli"

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

Texture2D	g_sceneTexture;
uint		g_TonemapMode; // 0 = None, 1 = AcesFilm, 2 = Reinhard

float3 AcesFilm(const float3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

float3 ReinhardTonemap(float3 finalColor)
{
	return finalColor / (finalColor + (float3) 1.0f);
}

float4 PSMain(in QuadResult quad) : SV_Target
{
	float3 finalColor = g_sceneTexture.Sample(LinearWrap, quad.UV).rgb;

    if (g_TonemapMode == 1)
    {
        finalColor = AcesFilm(finalColor);
        finalColor = pow(finalColor, (float3) (1.0 / 2.2)); // gamma correction    
    }
    else if (g_TonemapMode == 2)
    {
        finalColor = ReinhardTonemap(finalColor);
        finalColor = pow(finalColor, (float3) (1.0 / 2.2)); // gamma correction    
    }
    
	return float4(finalColor, 1.0f);
}