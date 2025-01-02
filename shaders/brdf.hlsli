#pragma once

// Resources used:
// Epic's Real Shading in Unreal Engine 4 - https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// BRDF Crash course by Jakub Boksansky - https://boksajak.github.io/files/CrashCourseBRDF.pdf
// Moving Frostbite to Physically Based Rendering - https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf

// Specifies minimal reflectance for dielectrics (when metalness is zero)
// Nothing has lower reflectance than 2%, but we use 4% UE4, Frostbite, etc.
#define MIN_DIELECTRICS_F0 0.04f

struct MaterialProperties
{
    float3 BaseColor;
    float3 Normal;

    float Roughness;
    float Metallic;
    float AO;
};

struct BRDFContext
{
    float3 N; // - Surface normal
    float3 V; // - View vector (towards viewer)
    float3 L; // - Light vector
    float3 H; // - Microfacet normal, aka half-vector == normalize(V + L)

    float NdotL;
    float NdotH;
    float NdotV;
    float HdotV;

    float3 DiffuseReflectance;
    float3 F0; // Reflectance at normal incidence (0 degrees)

	// Commonly used terms for BRDF evaluation
    float3 F; // Fresnel term

    float Roughness;
};

/*
	Cook-Torrance Microfacet Specular BRDF
	
	      D * F * G
	---------------------
	  4 * NdotL * NdotV
	
	D(H)       - Normal Distribution Function - Describes the concentration of microfacets oriented to reflect specularily
	F(V, H)    - Fresnel effect - Evaluates the amount of light that is reflected depending of the viewing angle
	G(L, V, H) - Geometric Attenuation - Accounts for attenuation of reflected light due to the geometry of the microsurface which occurs when some microfacets block each other

*/

// GGX/Towbridge-Reitz normal distribution function.
// Uses Disney's reparametrization of alpha = roughness^2.
float NDF_GGX(float NdotH, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (NdotH * NdotH) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Normal Schlick but using Epic's modification, using a Spherical Gaussian approximation to replace the power.
// Epic claims that this is slightly more efficient to calculate
float3 FresnelSchlick(float HdotV, float3 F0)
{
	return F0 + (1.0 - F0) * pow(2, (-5.55473f * HdotV - 6.98316f));
}

// Fresnel Schlick taking count of roughness, by Sébastien Lagarde - https://seblagarde.wordpress.com/2011/08/17/hello-world/
float3 FresnelSchlickRoughness(float NdotV, float3 F0, float roughness)
{
#if 0
	return F0 + (max(1 - roughness, F0) - F0) * pow(1 - saturate(NdotV), 5.0);
#else
	float val = 1.0 - NdotV;
	return F0 + (max(1.0 - roughness, F0) - F0) * (val * val * val * val * val); //Faster than pow
#endif
}


// Single term for separable Schlick-GGX below.
float SchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float SchlickGGX(float NdotL, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return SchlickG1(NdotL, k) * SchlickG1(NdotV, k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method (IBL version).
float SchlickGGX_IBL(float NdotL, float NdotV, float roughness)
{
	float r = roughness;
	float k = (r * r) / 2.0; // Epic suggests using this roughness remapping for IBL lighting.
	return SchlickG1(NdotV, k) * SchlickG1(NdotL, k);
}

// Importance sample GGX normal distribution function for a fixed roughness value.
// For derivation see: http://blog.tobias-franke.eu/2014/03/30/notes_on_importance_sampling.html & Epic's s2013 presentation
float3 ImportanceSampleGGX(float2 Xi, float roughness, float3 N)
{
	float alpha = roughness * roughness;

	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (alpha * alpha - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta); // Trig. identity
	float phi = TwoPI * Xi.x;

	float3 H;
	H.x = sinTheta * cos(phi);
	H.y = sinTheta * sin(phi);
	H.z = cosTheta;
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);

	// Tangent to world space
	return normalize(TangentX * H.x + TangentY * H.y + N * H.z);
}

float3 Microfacet(in BRDFContext brdfContext)
{
    float D  = NDF_GGX(brdfContext.NdotH, brdfContext.Roughness);
    float G  = SchlickGGX(brdfContext.NdotL, brdfContext.NdotV, brdfContext.Roughness);
    float3 F = brdfContext.F; // The Fresnel term is calculated in InitBRDF because we use it to attenuate the diffuse BRDF as well

    return ((F * D * G) / (4.0 * brdfContext.NdotL * brdfContext.NdotV)) * brdfContext.NdotL;
}

/*
	Lambertian Diffuse Model
	
	- V = View vector
	- L = Light vector
	
	    Cdiff
	------------
		 PI
	
	Cdiff - diffuse reflectance - base color
*/

float3 Diffuse_Lambert(in BRDFContext brdfContext)
{
    return brdfContext.DiffuseReflectance * (INV_PI * brdfContext.NdotL);
}

//--------------------------------------------
//					BRDF
//--------------------------------------------

BRDFContext InitBRDF(float3 V, float3 N, float3 L, in MaterialProperties material)
{
    float3 H = normalize(V + L);

    BRDFContext brdf;
    brdf.V = V;
    brdf.N = N;
    brdf.L = L;
    brdf.H = H;

    brdf.NdotL = min(max(1e-5f, dot(N, L)), 1.0f);
    brdf.NdotV = min(max(1e-5f, dot(N, V)), 1.0f);

    brdf.NdotH = saturate(dot(N, H));
    brdf.HdotV = saturate(dot(H, V));

    brdf.DiffuseReflectance = material.BaseColor * (1 - material.Metallic);

    brdf.F0 = lerp(MIN_DIELECTRICS_F0, material.BaseColor, material.Metallic);
    brdf.F  = FresnelSchlick(brdf.HdotV, brdf.F0);

    // Preserve specular highlight
    brdf.Roughness = max(0.05, material.Roughness);

    return brdf;
}

float3 DefaultBRDF(float3 V, float3 N, float3 L, MaterialProperties material)
{
    BRDFContext brdfContext = InitBRDF(V, N, L, material);

    float3 specularBRDF = Microfacet(brdfContext);
    float3 diffuseBRDF  = Diffuse_Lambert(brdfContext);

    return (1 - brdfContext.F) * diffuseBRDF + specularBRDF;
}
