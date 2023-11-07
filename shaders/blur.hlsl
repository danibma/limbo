#include "bindings.hlsli"

#define BLOCK_SIZE 1024

groupshared float4 SharedPoints[4 + BLOCK_SIZE + 4];

static const float GaussFilter[9] =
{
    0.0002, 0.0060, 0.0606, 0.2417, 0.3829, 0.2417, 0.0606, 0.0060, 0.0002
};

uint InputTextureIdx;
RWTexture2D<float4> OutputTexture;

[numthreads(BLOCK_SIZE, 1, 1)]
void Blur_Horizontal(uint3 groupThreadID : SV_GroupThreadID, uint3 threadID : SV_DispatchThreadID, uint groupIdx : SV_GroupIndex)
{
    Texture2D inputTexture = GetTexture(InputTextureIdx);
    float4 data = inputTexture.Load(threadID);

    SharedPoints[groupThreadID.x + 4] = data;
    if (groupIdx == 0)
    {
        SharedPoints[0] = inputTexture.Load(threadID - int3(4, 0, 0));
        SharedPoints[1] = inputTexture.Load(threadID - int3(3, 0, 0));
        SharedPoints[2] = inputTexture.Load(threadID - int3(2, 0, 0));
        SharedPoints[3] = inputTexture.Load(threadID - int3(1, 0, 0));
    }

    if (groupIdx == BLOCK_SIZE - 1)
    {
        SharedPoints[4 + BLOCK_SIZE + 0] = inputTexture.Load(threadID + int3(1, 0, 0));
        SharedPoints[4 + BLOCK_SIZE + 1] = inputTexture.Load(threadID + int3(2, 0, 0));
        SharedPoints[4 + BLOCK_SIZE + 2] = inputTexture.Load(threadID + int3(3, 0, 0));
        SharedPoints[4 + BLOCK_SIZE + 3] = inputTexture.Load(threadID + int3(4, 0, 0));
    }
    GroupMemoryBarrierWithGroupSync();

    float4 blurredColor = float4(0.0, 0.0, 0.0, 0.0);
    for (int x = 0; x < 9; x++)
        blurredColor += SharedPoints[groupThreadID.x + x] * GaussFilter[x];

    OutputTexture[threadID.xy] = blurredColor;
}

[numthreads(1, BLOCK_SIZE, 1)]
void Blur_Vertical(uint3 groupThreadID : SV_GroupThreadID, uint3 threadID : SV_DispatchThreadID, uint groupIdx : SV_GroupIndex)
{
    Texture2D inputTexture = GetTexture(InputTextureIdx);
    float4 data = inputTexture.Load(threadID);

    SharedPoints[groupThreadID.y + 4] = data;
    if (groupIdx == 0)
    {
        SharedPoints[0] = inputTexture.Load(threadID - int3(0, 4, 0));
        SharedPoints[1] = inputTexture.Load(threadID - int3(0, 3, 0));
        SharedPoints[2] = inputTexture.Load(threadID - int3(0, 2, 0));
        SharedPoints[3] = inputTexture.Load(threadID - int3(0, 1, 0));
    }

    if (groupIdx == BLOCK_SIZE - 1)
    {
        SharedPoints[4 + BLOCK_SIZE + 0] = inputTexture.Load(threadID + int3(0, 1, 0));
        SharedPoints[4 + BLOCK_SIZE + 1] = inputTexture.Load(threadID + int3(0, 2, 0));
        SharedPoints[4 + BLOCK_SIZE + 2] = inputTexture.Load(threadID + int3(0, 3, 0));
        SharedPoints[4 + BLOCK_SIZE + 3] = inputTexture.Load(threadID + int3(0, 4, 0));
    }
    GroupMemoryBarrierWithGroupSync();

    float4 blurredColor = float4(0.0, 0.0, 0.0, 0.0);
    for (int y = 0; y < 9; y++)
        blurredColor += SharedPoints[groupThreadID.y + y] * GaussFilter[y];

    OutputTexture[threadID.xy] = blurredColor;
}