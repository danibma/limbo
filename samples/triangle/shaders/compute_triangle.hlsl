RWTexture2D<float4> output;

// Transform a given vertex in NDC [-1,1] to raster-space [0, {w|h}]

const static float3 triangle_vertices[3] =
{
    -1.0,  1.0, 1.0,
     1.0,  1.0, 1.0,
     0.0, -1.0, 1.0
};

float sign(float3 current_pixel, float3 v0, float3 v1)
{
    return (current_pixel.x - v1.x) * (v0.y - v1.y) - (v0.x - v1.x) * (current_pixel.y - v1.y);
}

[numthreads(8, 8, 1)]
void DrawTriangle(uint3 ThreadID : SV_DispatchThreadID)
{
    float outputWidth, outputHeight;
    output.GetDimensions(outputWidth, outputHeight);
    
    float3 v0 = float3(outputWidth * (triangle_vertices[0].x + 1.0f) / 2, outputHeight * (triangle_vertices[0].y + 1.f) / 2, 1.0f);
    float3 v1 = float3(outputWidth * (triangle_vertices[1].x + 1.0f) / 2, outputHeight * (triangle_vertices[1].y + 1.f) / 2, 1.0f);
    float3 v2 = float3(outputWidth * (triangle_vertices[2].x + 1.0f) / 2, outputHeight * (triangle_vertices[2].y + 1.f) / 2, 1.0f);
    
    float d1, d2, d3;
    bool has_neg, has_pos;

    d1 = sign(ThreadID, v0, v1);
    d2 = sign(ThreadID, v1, v2);
    d3 = sign(ThreadID, v2, v0);
    
    has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

    if (!(has_neg && has_pos))
        output[ThreadID.xy] = float4(1.0f, 0.0f, 0.0f, 1.0f);
    else
        output[ThreadID.xy] = float4(0.0f, 0.0f, 0.0f, 1.0f);
}