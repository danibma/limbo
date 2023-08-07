#include "common.hlsli"
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

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0f);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0f);
    float NdotL = max(dot(N, L), 0.0f);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(clamp(1.0 - cosTheta, 0.0f, 1.0f), 5.0f);
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max((float3) (1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Camera info
float4 camPos;

// Light info
float3 lightPos;
float3 lightColor;

Texture2D g_WorldPosition;
Texture2D g_Albedo;
Texture2D g_Normal;
Texture2D g_Roughness;
Texture2D g_Metallic;
Texture2D g_Emissive;

TextureCube g_IrradianceMap;
TextureCube g_PrefilterMap;
Texture2D   g_LUT;

float4 PSMain(QuadResult quad) : SV_Target
{
	// gbuffer values
    float3 worldPos     = g_WorldPosition.Sample(LinearClamp, quad.UV).rgb;
    float3 albedo       = g_Albedo.Sample(LinearClamp, quad.UV).rgb;
    float3 normal       = g_Normal.Sample(LinearClamp, quad.UV).rgb;
    float3 emissive     = g_Emissive.Sample(LinearClamp, quad.UV).rgb;
    float metallic      = g_Metallic.Sample(LinearClamp, quad.UV).r;
    float roughness     = g_Roughness.Sample(LinearClamp, quad.UV).r;
	
    if (albedo.x == 0.0f && albedo.y == 0.0f && albedo.z == 0.0f)
        discard;

	float3 N = normalize(normal);
    float3 V = normalize(camPos.rgb - worldPos); // view direction
    float3 R = reflect(-V, N);
    
    float3 F0 = (float3) 0.04f;
    F0 = lerp(F0, albedo, metallic);
    
    // reflectance equation
    float3 Lo = (float3) 0.0f;
    
    // do this per light, atm we only have one
	{
        float3 L = normalize(lightPos - worldPos); // light direction
        float3 H = normalize(V + L); // half vector
        float distance = length(lightPos - worldPos);
        float attenuation = 1.0f / (distance * distance);
        float3 radiance = lightColor * attenuation;
    
		// cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0f), F0);
    
        float3 kS = F;
        float3 kD = (float3) 1.0f - kS;
        kD *= 1.0f - metallic;

		// Lambertian diffuse
        float3 diffuse = kD * albedo / PI;
    
        float3 numerator = NDF * G * F;
        float denominator = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0) + 0.0001;
        float3 specular = numerator / denominator;
    
		// add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0f);
        Lo += (diffuse + specular) * radiance * NdotL;
    }
	
    float3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
  
    float3 irradiance = g_IrradianceMap.SampleLevel(LinearClamp, N, 0).rgb;
    float3 diffuse = irradiance * albedo;
  
    const float MAX_REFLECTION_LOD = 4.0;
    float3 prefilteredColor = g_PrefilterMap.SampleLevel(LinearClamp, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 envBRDF = g_LUT.SampleLevel(LinearClamp, float2(max(dot(N, V), 0.0), roughness), 0).rg;
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
  
    float3 ambient = (kD * diffuse + specular) + emissive;
    
    float3 color = ambient + Lo;
    float4 finalColor = float4(color, 1.0f);
    
    return finalColor;
}
