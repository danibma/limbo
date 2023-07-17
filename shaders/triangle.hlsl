float4 VSMain(float3 position : Position) : SV_Position
{
	return float4(position, 1.0f);
}

float4 PSMain(float4 position : SV_Position) : SV_Target
{
	return (float4)1.0f;
}
