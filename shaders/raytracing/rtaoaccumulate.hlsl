#include "raytracingcommon.hlsli"

RWTexture2D<float4> g_DenoisedRTAOImage;

uint accumCount;
uint previousRTAOImage;
uint currentRTAOImage;

[numthreads(8, 8, 1)]
void RTAOAccumulate(uint2 threadID : SV_DispatchThreadID)
{
    float4 curColor = GetTexture(currentRTAOImage)[threadID];
    float4 prevColor = GetTexture(previousRTAOImage)[threadID];

    // Do a weighted sum, weighing last frame's color based on total count
    g_DenoisedRTAOImage[threadID] = (accumCount * prevColor + curColor) / (accumCount + 1);
}