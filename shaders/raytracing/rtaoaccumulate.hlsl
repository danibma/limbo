#include "raytracingcommon.hlsli"

Texture2D g_PreviousRTAOImage;
Texture2D g_CurrentRTAOImage;

RWTexture2D<float4> g_DenoisedRTAOImage;

uint accumCount;

[numthreads(8, 8, 1)]
void RTAOAccumulate(uint2 threadID : SV_DispatchThreadID)
{
    float4 curColor = g_CurrentRTAOImage[threadID];
    float4 prevColor = g_PreviousRTAOImage[threadID];

    // Do a weighted sum, weighing last frame's color based on total count
    g_DenoisedRTAOImage[threadID] = (accumCount * prevColor + curColor) / (accumCount + 1);
}