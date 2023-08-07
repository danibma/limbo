#pragma once

struct QuadResult
{
	float4 Position : SV_Position;
	float2 UV		: TEXCOORD0;
};

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
