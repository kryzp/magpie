#include "render_graph.h"

#include "core/common.h"

#include "vulkan/core.h"
#include "vulkan/command_buffer.h"
#include "vulkan/swapchain.h"

using namespace mgp;

RenderGraph::RenderGraph(const VulkanCore *core)
	: m_core(core)
	, m_passes()
{
}

RenderGraph::~RenderGraph()
{
}

void RenderGraph::record(CommandBuffer &cmd, Swapchain *swapchain)
{
	if (!validate())
	{
		MGP_ERROR("invalid render graph.");
		return;
	}

	for (cauto &p : m_passes)
	{
		// very scuffed right now just to get the ball rolling i guess
		// JIT transitions
		// yes, not the best
		// yes, i'll make it better... at some point... (tbd)

		for (cauto &cout : p.m_colourOutputs)
		{
			cmd.transitionLayout(
				cout.view->getInfo(),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}

		for (cauto &cin : p.m_attachments)
		{
			cmd.transitionLayout(
				cin.view->getInfo(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}

		if (p.m_depthStencilOutput.view)
		{
			cmd.transitionLayout(
				p.m_depthStencilOutput.view->getInfo(),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			);
		}

		if (p.m_depthStencilInput.view)
		{
			cmd.transitionLayout(
				p.m_depthStencilInput.view->getInfo(),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			);
		}

		RenderInfo info(m_core);

		for (int i = 0; i < p.m_colourOutputs.size(); i++)
		{
			cauto &attachment = p.m_colourOutputs[i];

			info.setSize(attachment.view->getInfo().getWidth(), attachment.view->getInfo().getHeight());
			info.setMSAA(attachment.view->getInfo().getSamples());

			info.addColourAttachment(
				attachment.loadOp,
				*attachment.view,
				attachment.resolve
			);

			info.setClearColour(i, attachment.clear);
		}

		if (p.m_depthStencilOutput.view)
		{
			info.setClearDepth(p.m_depthStencilOutput.clear);

			info.addDepthAttachment(
				p.m_depthStencilOutput.loadOp,
				*p.m_depthStencilOutput.view,
				nullptr
			);
		}

		cmd.beginRendering(info);
		p.m_buildFunc(cmd, info);
		cmd.endRendering();
	}

	m_passes.clear();
}

void RenderGraph::addPass(const RenderPassDefinition &passDef)
{
	m_passes.push_back(passDef);
}

bool RenderGraph::validate() const
{
	return true;
}

void RenderGraph::reorderPasses()
{
}
