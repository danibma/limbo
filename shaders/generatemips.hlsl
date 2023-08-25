#include "common.hlsli"

Texture2D<float4>	PreviousMip;
RWTexture2D<float4> OutputMip;

float2 TexelSize; // 1.0 / destination dimension
uint bIsRGB;

[numthreads(8, 8, 1)]
void GenerateMip(uint2 threadID : SV_DispatchThreadID)
{
	//DTid is the thread ID * the values from numthreads above and in this case correspond to the pixels location in number of pixels.
	//As a result texcoords (in 0-1 range) will point at the center between the 4 pixels used for the mipmap.
    float2 texcoords = TexelSize * (threadID + 0.5);

	//The samplers linear interpolation will mix the four pixel values to the new pixels color
    float4 color = PreviousMip.SampleLevel(SLinearClamp, texcoords, 0);

    float4 alpha = PreviousMip.GatherAlpha(SLinearClamp, texcoords, 0);
    color.a = max(alpha.r, max(alpha.g, max(alpha.b, alpha.a)));

    if (bIsRGB == 1)
        color = float4(pow(color.rgb, (float3) (1.0 / 2.2)), color.a);

	//Write the final color into the destination texture.
    OutputMip[threadID] = color;
}