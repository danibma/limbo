float4x4 viewProj;
float4x4 model;

float4 VSMain(float3 pos : Position) : SV_Position
{
    float4x4 mvp = mul(viewProj, model);
    float4 position = mul(mvp, float4(pos, 1.0f));

	return position;
}

float3 color;

float4 PSMain(float4 position : SV_Position) : SV_Target
{
    return float4(color, 1.0f);
}
