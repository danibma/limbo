#include "common.hlsli"

static const float quadVertices[] =
{
	// positions  // texCoords
	-1.0f, 1.0f, 0.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,

	-1.0f, 1.0f, 0.0f, 0.0f,
	 1.0f, -1.0f, 1.0f, 1.0f,
	 1.0f, 1.0f, 1.0f, 0.0f
};

QuadResult VSMain(uint vertexID : SV_VertexID)
{
    uint vertex = vertexID * 4;

    QuadResult result;
    result.Position.x = quadVertices[vertex];
    result.Position.y = quadVertices[vertex + 1];
    result.Position.z = 0.0f;
    result.Position.w = 1.0f;
    result.UV.x = quadVertices[vertex + 2];
    result.UV.y = quadVertices[vertex + 3];

    return result;
}
	
