#include "common.hlsli"

uint cTonemapMode; // Tonemap enum in composite.cpp
uint cEnableGammaCorrection;
uint cSceneTextureIdx;

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

// Hable 2010, "Filmic Tonemapping Operators"
// http://filmicworlds.com/blog/filmic-tonemapping-operators/
float3 Uncharted2(float3 x)
{
    x *= 16.0;
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Unreal 3, Documentation: "Color Grading"
// https://www.shadertoy.com/view/llXyWr
float3 Unreal(float3 x)
{
    // Adapted to be close to ACES, with similar range
    // Gamma 2.2 correction is baked in, don't use with sRGB conversion!
    return x / (x + 0.155) * 1.019;
}

float4 MainPS(in QuadResult quad) : SV_Target
{
    float3 finalColor = Sample2D(cSceneTextureIdx, SLinearWrap, quad.UV).rgb;

    if (cTonemapMode == 1)
        finalColor = AcesFilm(finalColor);
    else if (cTonemapMode == 2)
        finalColor = ReinhardTonemap(finalColor);
    else if (cTonemapMode == 3)
        finalColor = Uncharted2(finalColor);
    else if (cTonemapMode == 4)
        finalColor = Unreal(finalColor);

    // Gamma Correction
    if (any(cEnableGammaCorrection) && cTonemapMode != 4)
        finalColor = pow(finalColor, (float3) (1.0 / 2.2));
    
	return float4(finalColor, 1.0f);
}