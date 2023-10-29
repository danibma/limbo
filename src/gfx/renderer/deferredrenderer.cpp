#include "stdafx.h"
#include "renderer.h"
#include "gfx/techniques/composite/composite.h"
#include "gfx/techniques/deferredlighting/deferredlighting.h"
#include "gfx/techniques/gbuffer/gbuffer.h"
#include "gfx/techniques/rtao/rtao.h"
#include "gfx/techniques/shadowmapping/shadowmapping.h"
#include "gfx/techniques/skybox/skybox.h"
#include "gfx/techniques/ssao/ssao.h"

namespace limbo::Gfx
{
	class DeferredRenderer : public RendererRegister<DeferredRenderer>
	{
	public:
		static constexpr std::string_view Name = "Deferred";
		static constexpr RendererRequiredFeatures RequiredFeatures = RF_None;

		DeferredRenderer() {}

		virtual RenderTechniquesList SetupRenderTechniques() override
		{
			RenderTechniquesList result;
			SETUP_RENDER_TECHNIQUE(ShadowMapping, result);
			SETUP_RENDER_TECHNIQUE(GBuffer, result);
			SETUP_RENDER_TECHNIQUE(SSAO, result);
			SETUP_RENDER_TECHNIQUE(RTAO, result);
			SETUP_RENDER_TECHNIQUE(Skybox, result);
			SETUP_RENDER_TECHNIQUE(DeferredLighting, result);
			SETUP_RENDER_TECHNIQUE(Composite, result);
			return result;
		}
	};
}
