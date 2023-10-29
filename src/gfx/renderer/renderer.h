#pragma once
#include "gfx/rhi/device.h"
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	using RenderTechniquesList = std::vector<std::unique_ptr<RenderTechnique>>;

	class Renderer
	{
		LB_NON_COPYABLE(Renderer);

	public:
		Renderer() = default;

		static std::unique_ptr<Renderer> Create(const std::string_view& name)
		{
			auto& list = GetList();
			auto it = list.find(name);
			if (ensure(it != list.cend()))
				return it->second();
			
			return nullptr;
		}

		static std::vector<std::string_view> GetNames()
		{
			std::vector<std::string_view> result;
			for (auto& i : GetList())
			{
				/** Remove the path tracer from the UI, when ray tracing is not supported */
				if (i.first == "Path Tracer" && !RHI::GetGPUInfo().bSupportsRaytracing)
					continue;

				result.push_back(i.first);
			}
			return result;
		}

		/**
		 * Sets up the required render techniques.
		 * @returns a list of all the required render techniques.
		 */
		virtual RenderTechniquesList SetupRenderTechniques() = 0;

	private:
		using FunctionType = std::unique_ptr<Renderer>(*)();
		static auto& GetList()
		{
			static std::unordered_map<std::string_view, FunctionType> list;
			return list;
		}

		template<typename T>
		friend struct RendererRegister;
	};

	/**
	 * Class used to encapsulate the self-registration
	 * @tparam T generic type parameter of the child type to register
	 */
	template<typename T>
	struct RendererRegister : public Renderer
	{
		static bool RegisterType()
		{
			auto name = T::Name;
			Renderer::GetList()[name] = []() -> std::unique_ptr<Renderer>
			{
				return std::make_unique<T>();
			};
			return true;
		}

		static bool Registered; /**< Internal boolean used to force @RegisterType to be called */

	private:
		friend T;

		RendererRegister()
			: Renderer ()
		{
			(void)Registered;
		}
	};

	template<typename T>
	bool RendererRegister<T>::Registered = RendererRegister<T>::RegisterType();
}
