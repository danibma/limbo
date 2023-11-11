#include "common.hlsli"

static const float QuadVertices[] =
{
	// positions  // texCoords
	-1.0f, 1.0f, 0.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,

	-1.0f, 1.0f, 0.0f, 0.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 0.0f
};

QuadResult MainVS(uint vertexID : SV_VertexID)
{
    uint vertex = vertexID * 4;

    QuadResult result;
    result.Position.x = QuadVertices[vertex];
    result.Position.y = QuadVertices[vertex + 1];
    result.Position.z = 0.0f;
    result.Position.w = 1.0f;
    result.UV.x = QuadVertices[vertex + 2];
    result.UV.y = QuadVertices[vertex + 3];

    return result;
}
	
