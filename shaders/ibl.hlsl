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

    float3 S, T;
    ComputeBasisVectors(N, S, T);

	// Monte Carlo integration of hemispherical irradiance.
	// As a small optimization this also includes Lambertian BRDF assuming perfectly white surface (albedo of 1.0)
	// so we don't need to normalize in PBR fragment shader (so technically it encodes exitant radiance rather than irradiance).
    float3 irradiance = 0.0;
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = Hammersley(i);
        float3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
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

	// Solid angle associated with a single cubemap texel at zero mipmap level.
	// This will come in handy for importance sampling below.
    float wt = 4.0 * PI / (6 * inputWidth * inputHeight);
	
	// Approximation: Assume zero viewing angle (isotropic reflections).
    float3 N = GetSamplingVector(ThreadID, float(outputWidth), float(outputHeight));
    float3 Lo = N;
	
    float3 S, T;
    ComputeBasisVectors(N, S, T);

    float3 color = 0;
    float weight = 0;

	// Convolve environment map using GGX NDF importance sampling.
	// Weight by cosine term since Epic claims it generally improves quality.
    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = Hammersley(i);
        float3 Lh = TangentToWorld(SampleGGX(u.x, u.y, roughness), N, S, T);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
        float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

        float cosLi = saturate(dot(N, Li));
        if (cosLi > 0.0)
        {
			// Use Mipmap Filtered Importance Sampling to improve convergence.
			// See: https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html, section 20.4

            float cosLh = saturate(dot(N, Lh));

			// GGX normal distribution function (D term) probability density function.
			// Scaling by 1/4 is due to change of density in terms of Lh to Li (and since N=V, rest of the scaling factor cancels out).
            float pdf = NDF_GGX(cosLh, roughness) * 0.25;

			// Solid angle associated with this sample.
            float ws = 1.0 / (NumSamples * pdf);

			// Mip level to sample from.
            float mipLevel = max(0.5 * log2(ws / wt) + 1.0, 0.0);

            color  += g_EnvironmentCubemap.SampleLevel(LinearWrap, Li, mipLevel).rgb * cosLi;
            weight += cosLi;
        }
    }
    color /= weight;

    g_PreFilteredMap[ThreadID] = float4(color, 1.0);
}

// Pre-integrates Cook-Torrance specular BRDF for varying roughness and viewing directions.
// Results are saved into 2D LUT texture in the form of DFG1 and DFG2 split-sum approximation terms,
// which act as a scale and bias to F0 (Fresnel reflectance at normal incidence) during rendering.
RWTexture2D<float2> LUT : register(u0);

// Single term for separable Schlick-GGX below.
float SchlickG1(float cosTheta, float k)
{
    return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float SchlickGGX_IBL(float cosLi, float cosLo, float roughness)
{
    float r = roughness;
    float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
    return SchlickG1(cosLi, k) * SchlickG1(cosLo, k);
}

[numthreads(32, 32, 1)]
void ComputeBRDFLUT(uint2 ThreadID : SV_DispatchThreadID)
{
	// Get output LUT dimensions.
    float outputWidth, outputHeight;
    LUT.GetDimensions(outputWidth, outputHeight);

	// Get integration parameters.
    float cosLo = ThreadID.x / outputWidth;
    float roughness = ThreadID.y / outputHeight;

	// Make sure viewing angle is non-zero to avoid divisions by zero (and subsequently NaNs).
    cosLo = max(cosLo, Epsilon);

	// Derive tangent-space viewing vector from angle to normal (pointing towards +Z in this reference frame).
    float3 Lo = float3(sqrt(1.0 - cosLo * cosLo), 0.0, cosLo);

	// We will now pre-integrate Cook-Torrance BRDF for a solid white environment and save results into a 2D LUT.
	// DFG1 & DFG2 are terms of split-sum approximation of the reflectance integral.
	// For derivation see: "Moving Frostbite to Physically Based Rendering 3.0", SIGGRAPH 2014, section 4.9.2.
    float DFG1 = 0;
    float DFG2 = 0;

    for (uint i = 0; i < NumSamples; ++i)
    {
        float2 u = Hammersley(i);

		// Sample directly in tangent/shading space since we don't care about reference frame as long as it's consistent.
        float3 Lh = SampleGGX(u.x, u.y, roughness);

		// Compute incident direction (Li) by reflecting viewing direction (Lo) around half-vector (Lh).
        float3 Li = 2.0 * dot(Lo, Lh) * Lh - Lo;

        float cosLi = Li.z;
        float cosLh = Lh.z;
        float cosLoLh = max(dot(Lo, Lh), 0.0);

        if (cosLi > 0.0)
        {
            float G = SchlickGGX_IBL(cosLi, cosLo, roughness);
            float Gv = G * cosLoLh / (cosLh * cosLo);
            float Fc = pow(1.0 - cosLoLh, 5);

            DFG1 += (1 - Fc) * Gv;
            DFG2 += Fc * Gv;
        }
    }

    LUT[ThreadID] = float2(DFG1, DFG2) * InvNumSamples;
}
