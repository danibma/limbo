#include "stdafx.h"
#include "renderer.h"
#include "gfx/techniques/composite/composite.h"
#include "gfx/techniques/pathtracing/pathtracing.h"

namespace limbo::Gfx
{
	class PathTracer : public RendererRegister<PathTracer>
	{
	public:
		static constexpr std::string_view Name = "Path Tracer";
		static constexpr RendererRequiredFeatures RequiredFeatures = RF_RayTracing;

		PathTracer() {}

		virtual RenderTechniquesList SetupRenderTechniques() override
		{
			RenderTechniquesList result;
			SETUP_RENDER_TECHNIQUE(PathTracing, result);
			SETUP_RENDER_TECHNIQUE(Composite, result);
			return result;
		}
	};
}
