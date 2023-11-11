#include "raytracingcommon.hlsli"

RWTexture2D<float4> uDenoisedRTAOImage;

uint cAccumCount;
uint cPreviousRTAOImageIdx;
uint cCurrentRTAOImageIdx;

[numthreads(8, 8, 1)]
void RTAOAccumulateCS(uint2 threadID : SV_DispatchThreadID)
{
    float4 curColor = GetTexture(cCurrentRTAOImageIdx)[threadID];
    float4 prevColor = GetTexture(cPreviousRTAOImageIdx)[threadID];

    // Do a weighted sum, weighing last frame's color based on total count
    uDenoisedRTAOImage[threadID] = (cAccumCount * prevColor + curColor) / (cAccumCount + 1);
}