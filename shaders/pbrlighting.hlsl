#include "quad.hlsli"
#include "iblcommon.hlsli"
#include "brdf.hlsli"

/*
    A lot of references were used to (learn how to) implement this shader.
    Most of this references are linked in brdf.hlsli and ibl.hlsl.

    Alongside the references linked in those files, https://learnopengl.com/ PBR section was a huge help as well.
*/

QuadResult VSMain(uint vertexID : SV_VertexID) { VS_DRAW_QUAD(vertexID); }

// Light info
float3 lightPos;
float3 lightColor;

// GBuffer textures
Texture2D g_WorldPosition;
Texture2D g_Albedo;
Texture2D g_Normal;
Texture2D g_RoughnessMetallicAO;
Texture2D g_Emissive;
Texture2D g_AmbientOcclusion;

// IBL Textures
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
    if (GSceneInfo.SceneViewToRender == 2)
        return float4(normal, 1.0f);
    if (GSceneInfo.SceneViewToRender == 3)
        return float4(worldPos, 1.0f);
    if (GSceneInfo.SceneViewToRender == 4)
        return float4((float3)metallic, 1.0f);
    if (GSceneInfo.SceneViewToRender == 5)
        return float4((float3)roughness, 1.0f);
    if (GSceneInfo.SceneViewToRender == 6)
        return float4(emissive, 1.0f);
    if (GSceneInfo.SceneViewToRender == 7)
        return float4((float3)ao, 1.0f);

    MaterialProperties material;
    material.BaseColor = albedo;
    material.Normal    = normal;
    material.Roughness = roughness;
    material.Metallic  = metallic;
    material.AO        = ao;

    float3 V = normalize(GSceneInfo.CameraPos - worldPos);
	float3 N = normalize(normal);
    float3 L = normalize(lightPos - worldPos); // light direction

    // reflectance equation
    float3 directLighting = 0.0f;
    
    // Per Light
	{
        float distance = length(lightPos - worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightColor * attenuation;

        directLighting += DefaultBRDF(V, N, L, material) * radiance;
    }

    float3 indirectLighting = CalculateIBL(N, V, material, g_IrradianceMap, g_PrefilterMap, g_LUT);
    
    float3 color = directLighting + indirectLighting + emissive;
    float4 finalColor = float4(color, 1.0f);
    
    return finalColor;
}
