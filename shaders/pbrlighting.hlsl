#include "quad.hlsli"
#include "iblcommon.hlsli"
#include "brdf.hlsli"

//
// Vertex Shader
//
QuadResult VSMain(uint vertexID : SV_VertexID)
{
    VS_DRAW_QUAD(vertexID);
}

//
// Pixel Shader
//

// Returns number of mipmap levels for specular IBL environment map.
uint QueryTextureLevels(TextureCube tex)
{
    uint width, height, levels;
    tex.GetDimensions(0, width, height, levels);
    return levels;
}

// Fresnel Schlick taking count of roughness, by Sébastien Lagarde - https://seblagarde.wordpress.com/2011/08/17/hello-world/
float3 FresnelSchlickRoughness(float NdotV, float3 F0, float roughness)
{
    return F0 + (max(1 - roughness, F0) - F0) * pow(1 - saturate(NdotV), 5.0);
}

// 4.10.2 Specular occlusion from frostbite - https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float ComputeSpecOcclusion(float NdotV , float AO , float roughness)
{
	return saturate(pow( NdotV + AO , exp2( -16.0f * roughness - 1.0f )) - 1.0f + AO );
}

// Light info
float3 lightPos;
float3 lightColor;

Texture2D g_WorldPosition;
Texture2D g_Albedo;
Texture2D g_Normal;
Texture2D g_RoughnessMetallicAO;
Texture2D g_Emissive;
Texture2D g_AmbientOcclusion;

TextureCube g_IrradianceMap;
TextureCube g_PrefilterMap;
Texture2D   g_LUT;

uint bEnableAO;

float4 PSMain(QuadResult quad) : SV_Target
{
	// gbuffer values
    float3 worldPos             = g_WorldPosition.Sample(SLinearClamp, quad.UV).rgb;
    float3 albedo               = g_Albedo.Sample(SLinearClamp, quad.UV).rgb;
    float3 normal               = g_Normal.Sample(SLinearClamp, quad.UV).rgb;
    float3 emissive             = g_Emissive.Sample(SLinearClamp, quad.UV).rgb;
    float alpha                 = g_Albedo.Sample(SLinearClamp, quad.UV).a;
    float3 roughnessMetallicAO  = g_RoughnessMetallicAO.Sample(SLinearClamp, quad.UV).rgb;
    float roughness             = roughnessMetallicAO.x;
    float metallic              = roughnessMetallicAO.y;
    float ao                    = roughnessMetallicAO.z;
    if (bEnableAO > 0)
        ao *= g_AmbientOcclusion.Sample(SLinearClamp, quad.UV).r;

    if (alpha == 0.0f)
        discard;

    if (GSceneInfo.SceneViewToRender == 1)
        return float4(albedo, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 2)
        return float4(normal, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 3)
        return float4(worldPos, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 4)
        return float4((float3)metallic, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 5)
        return float4((float3)roughness, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 6)
        return float4(emissive, 1.0f);
    else if (GSceneInfo.SceneViewToRender == 7)
        return float4((float3)ao, 1.0f);

    MaterialProperties material;
    material.BaseColor = albedo;
    material.Normal    = normal;
    material.Roughness = roughness;
    material.Metallic  = metallic;

    float3 V = normalize(GSceneInfo.CameraPos - worldPos);
	float3 N = normalize(normal);
    float3 L = normalize(lightPos - worldPos); // light direction

    float  NdotV;
    float3 F0;

    // reflectance equation
    float3 directLighting = 0.0f;
    
    // Per Light
	{
        float distance = length(lightPos - worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightColor * attenuation;

        BRDFContext brdf;
        directLighting += DefaultBRDF(brdf, V, N, L, material) * radiance;

        NdotV = brdf.NdotV;
        F0    = brdf.F0;
    }

	// Specular reflection vector.
    float3 R = 2.0 * NdotV * N - V;

    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);

    // Get diffuse contribution factor (as with direct lighting).
    float3 kD = lerp(1.0 - F, 0.0, metallic);
  
    float3 irradiance = g_IrradianceMap.SampleLevel(SLinearClamp, N, 0).rgb;
    float3 diffuse = kD * irradiance * albedo * ao;
  
    float specularLevels = QueryTextureLevels(g_PrefilterMap);
    float3 prefilteredColor = g_PrefilterMap.SampleLevel(SLinearClamp, R, roughness * specularLevels).rgb;

    // Split-sum approximation factors for Cook-Torrance specular BRDF.
    float2 envBRDF = g_LUT.Sample(SLinearClamp, float2(NdotV, roughness)).rg;

    // Total specular IBL contribution.
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y) * ComputeSpecOcclusion(NdotV, ao, roughness);
  
    float3 ambient = diffuse + specular + emissive;
    
    float3 color = ambient + directLighting;
    float4 finalColor = float4(color, 1.0f);
    
    return finalColor;
}
