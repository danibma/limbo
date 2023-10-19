#pragma once

#include "core.h"

#include <glm/vec2.hpp> 
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/quaternion_float.hpp>

typedef glm::vec2 float2;
typedef glm::vec3 float3;
typedef glm::vec4 float4;

typedef uint32	   uint;
typedef glm::uvec2 uint2;
typedef glm::uvec3 uint3;
typedef glm::uvec4 uint4;

typedef glm::quat quat;

typedef glm::mat3x3 float3x3;
typedef glm::mat3x4 float3x4;
typedef glm::mat4x4 float4x4;

namespace limbo::Math
{
	constexpr float PI = 3.14159265358979323846f;
	constexpr float INV_PI = 0.31830988618379067154f;
	constexpr float INV_2PI = 0.15915494309189533577f;
	constexpr float INV_4PI = 0.07957747154594766788f;
	constexpr float PI_DIV_2 = 1.57079632679489661923f;
	constexpr float PI_DIV_4 = 0.78539816339744830961f;
	constexpr float SQRT_2 = 1.41421356237309504880f;

	constexpr float RadiansToDegrees = 180.0f / PI;
	constexpr float DegreesToRadians = PI / 180.0f;

	inline constexpr float Radians(float degrees) { return degrees * DegreesToRadians; }
	inline constexpr float Degrees(float radians) { return radians * RadiansToDegrees; }

	// https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
	inline float4x4 InfReversedProj_RH(float fovY_radians, float aspectRatio, float zNear)
	{
		float f = 1.0f / tan(fovY_radians / 2.0f);
		return float4x4(
			f / aspectRatio, 0.0f, 0.0f, 0.0f,
			0.0f, f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, -1.0f,
			0.0f, 0.0f, zNear, 0.0f);
	}
}

