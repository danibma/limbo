#include "random.hlsli"

// SSAO technique based on https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html

Texture2D g_Positions;
Texture2D g_Normals;
Texture2D<float> g_SceneDepth;

float radius;
float power;

#define KERNEL_SIZE 16

RWTexture2D<float4> g_UnblurredSSAOTexture;

[numthreads(16, 16, 1)]
void ComputeSSAO(uint2 threadID : SV_DispatchThreadID)
{
    float width, height;
    g_UnblurredSSAOTexture.GetDimensions(width, height);
    float2 targetDimensions = float2(width, height);
    float2 uv = (threadID + 0.5f) / targetDimensions;

    float3 pixelPos = g_Positions.SampleLevel(SLinearWrap, uv, 0).rgb;
    float3 normal = NormalFromDepth(threadID, g_SceneDepth, GSceneInfo.InvProjection);

    uint seed = RandomSeed(threadID, targetDimensions, GSceneInfo.FrameIndex);
    float3 randomVec = float3(Random01(seed), Random01(seed), Random01(seed)) * 2.0f - 1.0f;

    // Gramm-Schmidt process to create an orthogonal basis
    float3   tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    float3   bitangent = cross(normal, tangent);
    float3x3 TBN       = float3x3(tangent, bitangent, normal);

    float bias = 0.0025f;
    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
#if 0
        float3 kernelSample = float3(Random01(seed), Random01(seed), Random01(seed));
#else
        float pdf;
        float3 kernelSample = HemisphereSampleCosineWeight(Hammersley(i, KERNEL_SIZE), pdf);
#endif

		// get sample position
        float3 samplePos = mul(kernelSample, TBN); // from tangent to view-space
        samplePos = pixelPos + samplePos * radius;
    
        float4 offset = float4(samplePos, 1.0);
        offset        = mul(GSceneInfo.Projection, offset); // from view to clip-space
        offset.xyz   /= offset.w; // perspective divide
        offset.xy     = offset.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f); // transform to range 0.0 - 1.0 and inverse the y

        if (all(offset.xy >= 0) && all(offset.xy <= 1))
        {
            float sampleDepth = g_Positions.SampleLevel(SLinearWrap, offset.xy, 0).z;

            float rangeCheck = smoothstep(0.0, 1.0, radius / abs(pixelPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias) * rangeCheck;
        }
    }
    occlusion = 1.0 - (occlusion / KERNEL_SIZE);
    g_UnblurredSSAOTexture[threadID] = pow(occlusion, power);
}

Texture2D<float4>   g_SSAOTexture;
RWTexture2D<float4> g_BlurredSSAOTexture;

[numthreads(16, 16, 1)]
void BlurSSAO(uint2 threadID : SV_DispatchThreadID)
{
    int blurSize = 2;

    float width, height, depth;
    g_SSAOTexture.GetDimensions(0, width, height, depth);

    float2 UVs = float2(threadID.x / width, threadID.y / height);

    float2 texelSize = 1.0 / float2(width, height);
    float result = 0.0;
    for (int x = -blurSize; x < blurSize; ++x)
    {
        for (int y = -blurSize; y < blurSize; ++y)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += g_SSAOTexture.SampleLevel(SLinearWrap, UVs + offset, 0).r;
        }
    }
    g_BlurredSSAOTexture[threadID] = result / (4.0 * 4.0);
}
