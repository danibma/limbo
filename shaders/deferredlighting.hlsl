#include "iblcommon.hlsli"
#include "brdf.hlsli"

/*
    A lot of references were used to (learn how to) implement this shader.
    Most of this references are linked in brdf.hlsli and ibl.hlsl.

    Alongside the references linked in those files, https://learnopengl.com/ PBR section was a huge help as well.
*/

// Light info
cbuffer Random : register(b0)
{
    uint bEnableAO;
    float3 lightPos;
    float3 lightColor;
}

// GBuffer textures
cbuffer Textures : register(b1)
{
    uint PixelPosition;
    uint WorldPosition;
    uint Albedo;
    uint Normal;
    uint RoughnessMetallicAO;
    uint Emissive;
    uint AmbientOcclusion;

	// IBL Textures
    uint IrradianceMap;
    uint PrefilterMap;
    uint LUT;
}

float ShadowPCF3x3(float3 sc, uint cascadeIndex)
{
    if (sc.z > 1.0f)
        return 1.0;

    float depth = sc.z;
    const float dx = 1.0f / SHADOWMAP_SIZES[cascadeIndex];
    float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    float percentLit = 0.0f;
	[unroll(9)] 
    for (int i = 0; i < 9; ++i)
    {
        percentLit += Sample2DCmpLevelZero(GShadowData.ShadowMap[cascadeIndex], SShadowWrapSampler, sc.xy + offsets[i], depth).x;
    }
    percentLit /= 9.0f;
    return percentLit + 0.05f;
}

float4 PSMain(QuadResult quad) : SV_Target
{
	// gbuffer values
    float3 pixelPos             = Sample2D(PixelPosition, SLinearClamp, quad.UV).rgb;
    float3 worldPos             = Sample2D(WorldPosition, SLinearClamp, quad.UV).rgb;
    float3 albedo               = Sample2D(Albedo, SLinearClamp, quad.UV).rgb;
    float3 normal               = Sample2D(Normal, SLinearClamp, quad.UV).rgb;
    float3 emissive             = Sample2D(Emissive, SLinearClamp, quad.UV).rgb;
    float alpha                 = Sample2D(Albedo, SLinearClamp, quad.UV).a;
    float3 roughnessMetallicAO  = Sample2D(RoughnessMetallicAO, SLinearClamp, quad.UV).rgb;
    float roughness             = roughnessMetallicAO.x;
    float metallic              = roughnessMetallicAO.y;
    float ao                    = roughnessMetallicAO.z;
    if (bEnableAO > 0)
        ao *= Sample2D(AmbientOcclusion, SLinearClamp, quad.UV).r;

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

    float3 indirectLighting = CalculateIBL(N, V, material, IrradianceMap, PrefilterMap, LUT);
    float3 lightRadiance = (directLighting + indirectLighting);

    float shadow = 1.0f;

    [branch]
    if (any(GSceneInfo.bSunCastsShadows))
    {
        // Get cascade index for the current fragment's view position
        uint cascadeIndex = 0;
        for (uint i = 0; i < SHADOWMAP_CASCADES - 1; ++i)
        {
            if (pixelPos.z < GShadowData.SplitDepth[i])
            {
                cascadeIndex = i + 1;
            }
        }

    	float4 shadowPosition = mul(GShadowData.LightViewProj[cascadeIndex], float4(worldPos, 1.0f));
        shadowPosition = shadowPosition / shadowPosition.w;
        shadowPosition.xy = 0.5 * shadowPosition.xy + 0.5;
        shadowPosition.y = 1.0 - shadowPosition.y;

        shadow = ShadowPCF3x3(shadowPosition.xyz, cascadeIndex);

        if (any(GSceneInfo.bShowShadowCascades))
        {
            switch (cascadeIndex)
            {
                case 0:
                    return float4(lightRadiance, 1.0f) * float4(0.5f, 0.5f, 1.0f, 1.0f);
                case 1:
                    return float4(lightRadiance, 1.0f) * float4(1.0f, 0.0f, 0.0f, 1.0f);
                case 2:
                    return float4(lightRadiance, 1.0f) * float4(0.0f, 1.0f, 0.0f, 1.0f);
                case 3:
                    return float4(lightRadiance, 1.0f) * float4(0.0f, 0.0f, 1.0f, 1.0f);
            }
        }
    }

    float3 color = (lightRadiance * shadow) + emissive;
    float4 finalColor = float4(color, 1.0f);
    
    return finalColor;
}
