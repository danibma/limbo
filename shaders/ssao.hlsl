#include "quad.hlsli"
#include "common.hlsli"

QuadResult VSMain(uint vertexID : SV_VertexID)
{
	VS_DRAW_QUAD(vertexID);
}

Texture2D g_Positions;
Texture2D g_Normals;
Texture2D g_SSAONoise;

// tile noise texture over screen, based on screen dimensions divided by noise size
float2 noiseScale;
float radius;

float4x4 projection;

#define KERNEL_SIZE 64
cbuffer _SampleKernel
{
    float4 sampleKernel[KERNEL_SIZE];
};

float4 PSMain(QuadResult quad) : SV_Target
{
    float3 pixelPos  = g_Positions.Sample(LinearWrap, quad.UV).rgb;
    float3 normal    = g_Normals.Sample(LinearWrap, quad.UV).rgb;
    float3 randomVec = g_SSAONoise.Sample(LinearWrap, quad.UV * noiseScale).rgb;

    // Gramm-Schmidt process to create an orthogonal basis
    float3   tangent   = normalize(randomVec - normal * dot(randomVec, normal));
    float3   bitangent = cross(normal, tangent);
    float3x3 TBN       = float3x3(tangent, bitangent, normal);

    float bias = 0.0025f;
    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
		// get sample position
        float3 samplePos = mul(TBN, sampleKernel[i].xyz); // from tangent to view-space
        samplePos = pixelPos + samplePos * radius;
    
        float4 offset = float4(samplePos, 1.0);
        offset        = mul(projection, offset); // from view to clip-space
        offset.xyz   /= offset.w; // perspective divide
        offset.xyz    = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0
        offset.y      = -offset.y;

        float sampleDepth = g_Positions.Sample(LinearWrap, offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(pixelPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }
    occlusion = 1.0 - (occlusion / KERNEL_SIZE);

    return float4(occlusion, 1.0f, 1.0f, 1.0f);
}

Texture2D<float4>   g_SSAOTexture;
RWTexture2D<float4> g_BlurredSSAOTexture;

[numthreads(8, 8, 1)]
void BlurSSAO(uint2 threadID : SV_DispatchThreadID)
{
    float width, height, depth;
    g_SSAOTexture.GetDimensions(0, width, height, depth);

    float2 UVs = float2(threadID.x / width, threadID.y / height);

    float2 texelSize = 1.0 / float2(width, height);
    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    {
        for (int y = -2; y < 2; ++y)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += g_SSAOTexture.SampleLevel(LinearWrap, UVs + offset, 0).r;
        }
    }
    g_BlurredSSAOTexture[threadID] = result / (4.0 * 4.0);
}
