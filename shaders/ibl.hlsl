#include "iblcommon.hlsli"

// Code inspired by
// Epic's Real Shading in Unreal Engine 4 - https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// PBR by Michal Siejak - https://github.com/Nadrin/PBR

TextureCube g_EnvironmentCubemap;

//
// Equirectangular to cubemap
//
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

    // Normalize spherical coordinates
    phi /= TwoPI;
    theta /= PI;

	// Sample equirectangular texture.
    float4 color = g_EnvironmentEquirectangular.SampleLevel(LinearWrap, float2(phi, theta), 0);

	// Write out color to output cubemap.
    g_OutEnvironmentCubemap[ThreadID] = color;
}

//
// Irradiance map
//
RWTexture2DArray<float4> g_IrradianceMap;

[numthreads(32, 32, 1)]
void DrawIrradianceMap(uint3 threadID : SV_DispatchThreadID)
{
    uint outputWidth, outputHeight, outputDepth;
    g_IrradianceMap.GetDimensions(outputWidth, outputHeight, outputDepth);
    
    float3 N = GetSamplingVector(threadID, float(outputWidth), float(outputHeight));

	// Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
    float3 irradiance = 0.0;
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = Hammersley(i);
        float3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N);
        float cosTheta = max(0.0, dot(Li, N));

		// PIs here cancel out because of division by pdf.
        irradiance += 2.0 * g_EnvironmentCubemap.SampleLevel(LinearWrap, Li, 0).rgb * cosTheta;
    }
    irradiance /= float(NumSamples);

    g_IrradianceMap[threadID] = float4(irradiance, 1.0);
}

//
// Specular
//
RWTexture2DArray<float4> g_PreFilteredMap;

// Roughness value to pre-filter for.
float roughness;

[numthreads(32, 32, 1)]
void PreFilterEnvMap(uint3 ThreadID : SV_DispatchThreadID)
{
	// Make sure we won't write past output when computing higher mipmap levels.
    uint outputWidth, outputHeight, outputDepth;
    g_PreFilteredMap.GetDimensions(outputWidth, outputHeight, outputDepth);
    if (ThreadID.x >= outputWidth || ThreadID.y >= outputHeight)
    {
        return;
    }
	
	// Get input cubemap dimensions at zero mipmap level.
    float inputWidth, inputHeight, inputLevels;
    g_EnvironmentCubemap.GetDimensions(0, inputWidth, inputHeight, inputLevels);

	// Approximation: Assume zero viewing angle (isotropic reflections).
    float3 N = GetSamplingVector(ThreadID, float(outputWidth), float(outputHeight));
    float3 V = N;
	
    float3 color = 0;
    float weight = 0;

	// Convolve environment map using GGX NDF importance sampling.
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley(i);
        float3 H = ImportanceSampleGGX(Xi, roughness, N);

		// Compute incident direction (L) by reflecting viewing direction (V) around half-vector (H).
        float3 L = 2.0 * dot(V, H) * H - V;

        float NdotL = saturate(dot(N, L));
        if (NdotL > 0.0)
        {
            color  += g_EnvironmentCubemap.SampleLevel(LinearWrap, L, 0).rgb * NdotL;
            weight += NdotL;
        }
    }
    color /= weight;

    g_PreFilteredMap[ThreadID] = float4(color, 1.0);
}

//
// LUT BRDF
//
RWTexture2D<float2> LUT : register(u0);

// Single term for separable Schlick-GGX below.
float SchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float SchlickGGX_IBL(float NdotL, float V, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
    return SchlickG1(NdotL, k) * SchlickG1(V, k);
}

// Pre-integrates Cook-Torrance specular BRDF for varying roughness and viewing directions.
// Results are saved into 2D LUT texture in the form of A and B split-sum approximation terms,
// which act as a scale and bias to F0 (Fresnel reflectance at normal incidence) during rendering.
[numthreads(32, 32, 1)]
void ComputeBRDFLUT(uint2 ThreadID : SV_DispatchThreadID)
{
	// Get output LUT dimensions.
    float outputWidth, outputHeight;
    LUT.GetDimensions(outputWidth, outputHeight);

	// Get integration parameters.
    float NdotV = ThreadID.x / outputWidth;
    float roughness = ThreadID.y / outputHeight;

	// Make sure viewing angle is non-zero to avoid divisions by zero (and subsequently NaNs).
    NdotV = max(NdotV, Epsilon);

    float3 V;
    V.x = sqrt(1.0f - NdotV * NdotV); // sin
    V.y = 0;
    V.z = NdotV; // cos

	float A = 0;
    float B = 0;

    float3 N = float3(0.0, 0.0, 1.0);

    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 Xi = Hammersley(i);
        float3 H = ImportanceSampleGGX(Xi, roughness, N);

		// Compute incident direction
        float3 L = 2.0 * dot(V, H) * H - V;

        float NdotL = saturate(L.z);
        float NdotH = saturate(H.z);
        float VdotH = saturate(dot(V, H));

        if (NdotL > 0.0)
        {
            float G = SchlickGGX_IBL(NdotL, NdotV, roughness);
            float Gv = G * VdotH / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5);

            A += (1 - Fc) * Gv;
            B += Fc * Gv;
        }
    }

    LUT[ThreadID] = float2(A, B) * InvNumSamples;
}
