#include "iblcommon.hlsli"
#include "quad.hlsli"

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

// Returns number of mipmap levels for specular IBL environment map.
uint QueryTextureLevels(TextureCube tex)
{
    uint width, height, levels;
    tex.GetDimensions(0, width, height, levels);
    return levels;
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
    return F0 + (max(1 - roughness, F0) - F0) * pow(1 - saturate(NdotV), 5.0);
}

float ComputeSpecOcclusion(float NdotV , float AO , float roughness)
{
	return saturate(pow( NdotV + AO , exp2( -16.0f * roughness - 1.0f )) - 1.0f + AO );
}

// Camera info
float4 camPos;

// Light info
float3 lightPos;
float3 lightColor;

Texture2D g_WorldPosition;
Texture2D g_Albedo;
Texture2D g_Normal;
Texture2D g_RoughnessMetallicAO;
Texture2D g_Emissive;
Texture2D g_SSAO;

TextureCube g_IrradianceMap;
TextureCube g_PrefilterMap;
Texture2D   g_LUT;

uint sceneToRender;
uint bEnableSSAO;

float4 PSMain(QuadResult quad) : SV_Target
{
	// gbuffer values
    float3 worldPos             = g_WorldPosition.Sample(LinearClamp, quad.UV).rgb;
    float3 albedo               = g_Albedo.Sample(LinearClamp, quad.UV).rgb;
    float3 normal               = g_Normal.Sample(LinearClamp, quad.UV).rgb;
    float3 emissive             = g_Emissive.Sample(LinearClamp, quad.UV).rgb;
    float alpha                 = g_Albedo.Sample(LinearClamp, quad.UV).a;
    float3 roughnessMetallicAO  = g_RoughnessMetallicAO.Sample(LinearClamp, quad.UV).rgb;
    float roughness             = roughnessMetallicAO.x;
    float metallic              = roughnessMetallicAO.y;
    float ao                    = roughnessMetallicAO.z;
    if (bEnableSSAO == 1)
        ao *= g_SSAO.Sample(LinearClamp, quad.UV).r;

    if (alpha == 0.0f)
        discard;

    if (sceneToRender == 1)
        return float4(albedo, 1.0f);
    else if (sceneToRender == 2)
        return float4(normal, 1.0f);
    else if (sceneToRender == 3)
        return float4(worldPos, 1.0f);
    else if (sceneToRender == 4)
        return float4((float3)metallic, 1.0f);
    else if (sceneToRender == 5)
        return float4((float3)roughness, 1.0f);
    else if (sceneToRender == 6)
        return float4(emissive, 1.0f);
    else if (sceneToRender == 7)
        return float4((float3)ao, 1.0f);

    // Outgoing light direction (vector from world-space fragment position to the "eye").
    float3 V = normalize(camPos.xyz - worldPos);

	float3 N = normalize(normal);

    // Angle between surface normal and outgoing light direction.
    float NdotV = abs(saturate(dot(N, V))) + 1e-5f;
		
	// Specular reflection vector.
    float3 R = 2.0 * NdotV * N - V;

    float3 fDieletric = (float3)0.02f;
    float3 F0 = lerp(fDieletric, albedo, metallic);
    
    // reflectance equation
    float3 directLighting = 0.0f;
    
    // do this per light, atm we only have one
	{
        float3 L = normalize(lightPos - worldPos); // light direction
        float3 H = normalize(V + L); // half vector

        float distance = length(lightPos - worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightColor * attenuation;

        float NdotL = max(dot(N, L),  0.0f);
        float NdotH = max(dot(N, H), 0.0);
        float HdotV = max(dot(H, V), 0.0f);
    
		// terms for the cook-torrance specular microfacet BRDF
        float  D = NDF_GGX(NdotH, roughness);
        float  G = SchlickGGX(NdotL, NdotV, roughness);
        float3 F = FresnelSchlick(HdotV, F0);

        // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
		// Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
		// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.
        float3 kD = lerp(float3(1, 1, 1) - F, float3(0, 0, 0), metallic);

		// Lambert diffuse BRDF.
		// We don't scale by 1/PI for lighting & material units to be more convenient.
		// See: https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
        float3 diffuse = kD * albedo;
    
        // Cook-Torrance specular microfacet BRDF.
        float3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * NdotL * NdotV);
    
		// add to outgoing radiance Lo
        directLighting += (diffuse + specularBRDF) * radiance * NdotL;
    }

    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    // Get diffuse contribution factor (as with direct lighting).
    float3 kD = lerp(1.0 - F, 0.0, metallic);
  
    float3 irradiance = g_IrradianceMap.SampleLevel(LinearClamp, N, 0).rgb;
    float3 diffuse = kD * irradiance * albedo * ao;
  
    float specularLevels = QueryTextureLevels(g_PrefilterMap);
    float3 prefilteredColor = g_PrefilterMap.SampleLevel(LinearClamp, R, roughness * specularLevels).rgb;

    // Split-sum approximation factors for Cook-Torrance specular BRDF.
    float2 envBRDF = g_LUT.Sample(LinearClamp, float2(NdotV, roughness)).rg;

    // Total specular IBL contribution.
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y) * ComputeSpecOcclusion(NdotV, ao, roughness);
  
    float3 ambient = diffuse + specular + emissive;
    
    float3 color = ambient + directLighting;
    float4 finalColor = float4(color, 1.0f);
    
    return finalColor;
}
