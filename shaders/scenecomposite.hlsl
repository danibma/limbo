#include "common.hlsli"

uint		g_bEnableTonemap; // 0 = None, 1 = AcesFilm, 2 = Reinhard
uint		g_sceneTexture;

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
    float3 finalColor = Sample2D(g_sceneTexture, SLinearWrap, quad.UV).rgb;

    if (g_bEnableTonemap == 1)
    {
        finalColor = AcesFilm(finalColor);
        finalColor = pow(finalColor, (float3) (1.0 / 2.2)); // gamma correction    
    }
    
	return float4(finalColor, 1.0f);
}