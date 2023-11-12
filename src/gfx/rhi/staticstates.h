#pragma once

#include "definitions.h"

namespace limbo::RHI
{
	template<bool BlendEnable = false,
	         bool LogicOpEnable = false,
	         D3D12_BLEND SrcBlend = D3D12_BLEND_SRC_ALPHA,
	         D3D12_BLEND DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
	         D3D12_BLEND_OP BlendOp = D3D12_BLEND_OP_ADD,
	         D3D12_BLEND SrcBlendAlpha = D3D12_BLEND_ONE,
	         D3D12_BLEND DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
	         D3D12_BLEND_OP BlendOpAlpha = D3D12_BLEND_OP_ADD,
	         D3D12_LOGIC_OP LogicOp = D3D12_LOGIC_OP_NOOP
	>
	class TStaticBlendState
	{
	public:
		static D3D12_RENDER_TARGET_BLEND_DESC GetRHI()
		{
			return {
				.BlendEnable = BlendEnable,
				.LogicOpEnable = LogicOpEnable,
				.SrcBlend = SrcBlend,
				.DestBlend = DestBlend ,
				.BlendOp = BlendOp ,
				.SrcBlendAlpha = SrcBlendAlpha ,
				.DestBlendAlpha = DestBlendAlpha ,
				.BlendOpAlpha = BlendOpAlpha ,
				.LogicOp = LogicOp ,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
			};
		}
	};

	template<bool DepthEnable = true,
	         bool StencilEnable = false,
	         D3D12_COMPARISON_FUNC DepthFunc = D3D12_COMPARISON_FUNC_LESS,
	         D3D12_STENCIL_OP FrontFace_StencilFailOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_STENCIL_OP FrontFace_StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_STENCIL_OP FrontFace_StencilPassOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_COMPARISON_FUNC FrontFace_StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS,
	         D3D12_STENCIL_OP BackFace_StencilFailOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_STENCIL_OP BackFace_StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_STENCIL_OP BackFace_StencilPassOp = D3D12_STENCIL_OP_KEEP,
	         D3D12_COMPARISON_FUNC BackFace_StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
	>
	class TStaticDepthStencilState
	{
	public:
		static D3D12_DEPTH_STENCIL_DESC1 GetRHI()
		{
			return {
				.DepthEnable = DepthEnable,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = DepthFunc,
				.StencilEnable = StencilEnable,
				.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
				.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
				.FrontFace = {
					.StencilFailOp = FrontFace_StencilFailOp,
					.StencilDepthFailOp = FrontFace_StencilDepthFailOp,
					.StencilPassOp = FrontFace_StencilPassOp,
					.StencilFunc = FrontFace_StencilFunc
				},
				.BackFace = {
					.StencilFailOp = BackFace_StencilFailOp,
					.StencilDepthFailOp = BackFace_StencilDepthFailOp,
					.StencilPassOp = BackFace_StencilPassOp,
					.StencilFunc = BackFace_StencilFunc
				}
			};
		}
	};

	template<D3D12_FILL_MODE FillMode = D3D12_FILL_MODE_SOLID,
	         D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_BACK,
	         bool FrontCounterClockwise = true,
	         int32 DepthBias = 0,
	         float DepthBiasClamp = 0.0f,
	         float SlopeScaledDepthBias = 0.0f,
	         bool DepthClipEnable = true,
	         bool MultisampleEnable = false,
	         bool AntialiasedLineEnable = false,
	         uint32 ForcedSampleCount = 0,
	         D3D12_CONSERVATIVE_RASTERIZATION_MODE ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
	>
	class TStaticRasterizerState
	{
	public:
		static D3D12_RASTERIZER_DESC GetRHI()
		{
			return {
				.FillMode = FillMode,
				.CullMode = CullMode,
				.FrontCounterClockwise = FrontCounterClockwise,
				.DepthBias = DepthBias,
				.DepthBiasClamp = DepthBiasClamp,
				.SlopeScaledDepthBias = SlopeScaledDepthBias,
				.DepthClipEnable = DepthClipEnable,
				.MultisampleEnable = MultisampleEnable,
				.AntialiasedLineEnable = AntialiasedLineEnable,
				.ForcedSampleCount = ForcedSampleCount,
				.ConservativeRaster = ConservativeRaster
			};
		}
	};
}