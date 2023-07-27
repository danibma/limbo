#pragma once

SamplerState LinearWrap;

static const float PI = 3.141592;
static const float TwoPI = 2 * PI;

struct QuadResult
{
	float4 position : SV_Position;
	float2 uv		: TEXCOORD0;
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