#pragma once
#include "gfx/techniques/rendertechnique.h"

namespace limbo::Gfx
{
	class Scene;
	class Skybox : public RenderTechnique
	{
	private:
		Scene* m_SkyboxCube;

	public:
		Skybox();
		virtual ~Skybox() override;

		bool Init() override;
		void Render(RHI::CommandContext& cmd, RenderContext& context) override;
	};
}
