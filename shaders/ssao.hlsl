#include "random.hlsli"

// SSAO technique based on https://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html

#define KERNEL_SIZE 16
#define BLOCK_SIZE 16

float cRadius;
float cPower;
uint cPositionsMapIdx;
uint cSceneDepthIdx;

RWTexture2D<float4> uUnblurredSSAOTexture;

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void ComputeSSAOCS(uint2 threadID : SV_DispatchThreadID)
{
    uint width, height;
    uUnblurredSSAOTexture.GetDimensions(width, height);
    float2 targetDimensions = float2(width, height);
    float2 uv = (threadID + 0.5f) / targetDimensions;

    float3 pixelPos = SampleLevel2D(cPositionsMapIdx, SLinearClamp, uv, 0).rgb;
    float3 normal = NormalFromDepth(threadID, cSceneDepthIdx, targetDimensions, GSceneInfo.InvProjection);

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
        float pdf;
        float3 kernelSample = CosineWeightSampleHemisphere(Hammersley(i, KERNEL_SIZE), pdf);

		// get sample position
        float3 samplePos = mul(kernelSample, TBN); // from tangent to view-space
        samplePos = pixelPos + samplePos * cRadius;
    
        float4 offset = float4(samplePos, 1.0);
        offset        = mul(GSceneInfo.Projection, offset); // from view to clip-space
        offset.xyz   /= offset.w; // perspective divide
        offset.xy     = offset.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f); // transform to range 0.0 - 1.0 and inverse the y

        if (all(offset.xy >= 0) && all(offset.xy <= 1))
        {
            float sampleDepth = SampleLevel2D(cPositionsMapIdx, SLinearClamp, offset.xy, 0).z;

            float rangeCheck = smoothstep(0.0, 1.0, cRadius / abs(pixelPos.z - sampleDepth));
            occlusion += (sampleDepth >= samplePos.z + bias) * rangeCheck;
        }
    }
    occlusion = 1.0 - (occlusion / KERNEL_SIZE);
    uUnblurredSSAOTexture[threadID] = pow(occlusion, cPower);
}
