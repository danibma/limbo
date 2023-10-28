#pragma once
#include "gfx/gfx.h"

#include <map>

namespace limbo::RHI
{
	class CommandContext;
}

namespace limbo::Gfx
{
	class RenderContext;
	class RenderTechnique
	{
		LB_NON_COPYABLE(RenderTechnique);

	public:
		RenderTechnique(const std::string_view& name);
		virtual ~RenderTechnique() = default;

		/**
		 * Initialize any internal data or state.
		 * This is called by the scene renderer during setup and should be used to
		 * create any required CPU|GPU resources.
		 */
		virtual bool Init() = 0;

		/**
		 * Conditionally choose to render or not this render technique
		 * @return true if this render technique is supposed to be rendered, and false if not
		 */
		virtual bool ConditionalRender(RenderContext& context);

		/**
		 * Perform Render operations
		 */
		virtual void Render(RHI::CommandContext& cmd, RenderContext& context) = 0;

		/**
		 * Perform UI Render operations
		 */
		virtual void RenderUI(RenderContext& context);

	protected:
		std::string_view m_Name;
	};
}
