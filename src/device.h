#pragma once

namespace limbo
{
	class Device
	{
	public:
		static Device* ptr;

		virtual ~Device() = default;
	};
}
