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

float GT(float x, float P, float a, float m, float l, float c, float b) {
    // Uchimura 2017, "HDR theory and practice"
    // Math: https://www.desmos.com/calculator/gslcdxvipg
    // Source: https://www.slideshare.net/nikuque/hdr-theory-and-practicce-jp
    float l0 = ((P - m) * l) / a;
    float L0 = m - m / a;
    float L1 = m + (1.0 - m) / a;
    float S0 = m + l0;
    float S1 = m + a * l0;
    float C2 = (a * P) / (P - S1);
    float CP = -C2 / P;

    float w0 = 1.0 - smoothstep(0.0, m, x);
    float w2 = step(m + l0, x);
    float w1 = 1.0 - w0 - w2;

    float T = m * pow(x / m, c) + b;
    float S = P - (P - S1) * exp(CP * (x - S0));
    float L = m + a * (x - m);

    return T * w0 + L * w1 + S * w2;
}

// https://www.shadertoy.com/view/WdjSW3
float GT(float x)
{
    const float P = 1.0;  // max display brightness
    const float a = 1.0;  // contrast
    const float m = 0.22; // linear section start
    const float l = 0.4;  // linear section length
    const float c = 1.33; // black
    const float b = 0.0;  // pedestal
    return GT(x, P, a, m, l, c, b);
}

float3 agxDefaultContrastApprox(float3 x)
{
    float3 x2 = x * x;
    float3 x4 = x2 * x2;
  
    return + 15.5     * x4 * x2
           - 40.14    * x4 * x
           + 31.96    * x4
           - 6.868    * x2 * x
           + 0.4298   * x2
           + 0.1191   * x
           - 0.00232;
}

// https://www.shadertoy.com/view/mdcSDH
float3 agx(float3 color)
{
    const float3x3 agxTransform = float3x3(
        0.842479062253094, 0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772, 0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);

    const float minEv = -12.47393;
    const float maxEv = 4.026069;
    color = mul(color, agxTransform);
    color = clamp(log2(color), minEv, maxEv);
    color = (color - minEv) / (maxEv - minEv);
    return agxDefaultContrastApprox(color);
}


float3 agxLook(float3 val)
{
    const float3 lw = float3(0.2126, 0.7152, 0.0722);
    float luma = dot(val, lw);

    float3 offset = float3(0, 0, 0);

    // Settings from https://x.com/VrKomarov/status/1815111602397462799
    float3 slope = 1.150f;
    float3 power = 1.210f;
    float sat = 1.0;

    // ASC CDL
    val = pow(val * slope + offset, power);
    return luma + sat * (val - luma);
}

float3 GammaCorrect(float3 color)
{
    if (any(cEnableGammaCorrection))
        return pow(color, (float3) (1.0 / 2.2));

    return color;
}

float4 MainPS(in QuadResult quad) : SV_Target
{
    float3 finalColor = Sample2D(cSceneTextureIdx, SLinearWrap, quad.UV).rgb;

    switch ((Tonemap)cTonemapMode)
    {
    case Tonemap::AcesFilm:
        finalColor = GammaCorrect(AcesFilm(finalColor));
        break;
    case Tonemap::Reinhard:
        finalColor = GammaCorrect(ReinhardTonemap(finalColor));
        break;
    case Tonemap::Uncharted2:
        finalColor = GammaCorrect(Uncharted2(finalColor));
        break;
    case Tonemap::Unreal:
        finalColor = Unreal(finalColor);
        break;
    case Tonemap::GT:
        finalColor = GammaCorrect(float3(GT(finalColor.x), GT(finalColor.y), GT(finalColor.z)));
        break;
    case Tonemap::Agx:
        finalColor = agxLook(agx(finalColor));
        break;
    default:
        break;
    }

	return float4(finalColor, 1.0f);
}