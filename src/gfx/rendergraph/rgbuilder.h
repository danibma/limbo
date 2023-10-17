#pragma once

// This render graph system is inspired by Epic's RDG and EA SEED Halcyon R&D Engine
// Epic's RDG - A Crash Course: https://epicgames.ent.box.com/s/ul1h44ozs0t2850ug0hrohlzm53kxwrz
// Halcyon - Rapid Innovation using Modern Graphics: https://media.contentapi.ea.com/content/dam/ea/seed/presentations/wihlidal2019-rebootdevelopblue-halcyon-rapid-innovation.pdf

#include "rgregistry.h"
#include "gfx/rhi/commandcontext.h"

namespace limbo::Gfx
{
	enum class RGPassFlags : uint8
	{
		Graphics,
		Compute,
		Copy
	};

	class RGPass
	{
	private:
		class IRGPassCallback
		{
		public:
			IRGPassCallback() = default;
			virtual void Execute(RHI::CommandContext* context) = 0;
		};

		template<typename Lambda>
		class RGPassCallback : public IRGPassCallback
		{
		public:
			RGPassCallback(Lambda&& lambda)
				: m_Lambda(std::forward<Lambda&&>(lambda)) {}

			virtual void Execute(RHI::CommandContext* context) override
			{
				m_Lambda(context);
			}

		private:
			Lambda m_Lambda;
		};

	public:
		RGPass(const char* name, RGPassFlags flags)
			: m_Name(name), m_Flags(flags)
		{
		}

		RGPass& DepthStencil(RGHandle texture);
		RGPass& RenderTarget(RGHandle texture);

		template<typename Execution>
		void Bind(Execution&& execution)
		{
			m_Callback = std::make_unique<RGPassCallback<Execution>>(std::forward<Execution&&>(execution));
		}

	private:
		std::string m_Name;
		RGPassFlags m_Flags;
		RGHandle m_DepthStencil;
		TStaticArray<RGHandle, RHI::MAX_RENDER_TARGETS> m_RenderTargets;
		uint8 m_NumRenderTargets;
		std::unique_ptr<IRGPassCallback> m_Callback;

		friend class RGBuilder;
	};

	class RGBuilder
	{
	public:
		RGBuilder(RHI::CommandContext* context)
			: m_Context(context) {}

		RGPass& AddPass(const char* eventName, RGPassFlags flags)
		{
			return m_Passes.emplace_back(eventName, flags);
			// TODO: At the moment this is instant, we don't want this to be executed instantly
			// TODO: Use the event name to call the profile events
		}

		RGHandle Create(const RGTexture::TSpec& spec)
		{
			return m_Registry.Create(spec);
		}

		RGHandle Create(const RGBuffer::TSpec& spec)
		{
			return m_Registry.Create(spec);
		}

		void Execute();

	private:
		RGRegistry m_Registry;
		std::vector<RGPass> m_Passes;
		RHI::CommandContext* m_Context;
	};
}
