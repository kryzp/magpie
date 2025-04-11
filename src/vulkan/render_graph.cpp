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

		switch (p.type)
		{
			case PassHandle::PASS_TYPE_RENDER:
				handleRenderPass(cmd, swapchain, m_renderPasses[p.index]);
				break;

			case PassHandle::PASS_TYPE_COMPUTE:
				handleComputeTask(cmd, swapchain, m_computeTasks[p.index]);
				break;
		}
	}

	m_passes.clear();

	m_renderPasses.clear();
	m_computeTasks.clear();
}

bool RenderGraph::validate() const
{
	return true;
}

void RenderGraph::handleRenderPass(CommandBuffer &cmd, Swapchain *swapchain, const RenderPassDefinition &pass)
{
	for (cauto &cout : pass.m_colourAttachments)
	{
		cmd.transitionLayout(
			*cout.view->getImage(),
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);
	}

	if (pass.m_depthStencilAttachment.view)
	{
		cmd.transitionLayout(
			*pass.m_depthStencilAttachment.view->getImage(),
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		);
	}

	for (cauto &view : pass.m_inputViews)
	{
		if (view->getImage()->isDepth())
		{
			cmd.transitionLayout(
				*view->getImage(),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			);
		}
		else
		{
			cmd.transitionLayout(
				*view->getImage(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}

	RenderInfo info(m_core);

	for (int i = 0; i < pass.m_colourAttachments.size(); i++)
	{
		cauto &attachment = pass.m_colourAttachments[i];

		info.setSize(attachment.view->getImage()->getWidth(), attachment.view->getImage()->getHeight());
		info.setMSAA(attachment.view->getImage()->getSamples());

		info.addColourAttachment(
			attachment.loadOp,
			*attachment.view,
			attachment.resolve
		);

		info.setClearColour(i, attachment.clear);
	}

	if (pass.m_depthStencilAttachment.view)
	{
		info.setSize(pass.m_depthStencilAttachment.view->getImage()->getWidth(), pass.m_depthStencilAttachment.view->getImage()->getHeight());

		info.addDepthAttachment(
			pass.m_depthStencilAttachment.loadOp,
			*pass.m_depthStencilAttachment.view,
			pass.m_depthStencilAttachment.resolve
		);

		info.setClearDepth(pass.m_depthStencilAttachment.clear);
	}

	cmd.beginRendering(info);
	pass.m_buildFunc(cmd, info);
	cmd.endRendering();
}

void RenderGraph::handleComputeTask(CommandBuffer &cmd, Swapchain *swapchain, const ComputeTaskDefinition &task)
{
	for (cauto &view : task.m_inputStorageViews)
	{
		cmd.transitionLayout(
			*view->getImage(),
			VK_IMAGE_LAYOUT_GENERAL
		);
	}

	task.m_buildFunc(cmd);
}

void RenderGraph::addPass(const RenderPassDefinition &passDef)
{
	PassHandle handle;
	handle.type = PassHandle::PASS_TYPE_RENDER;
	handle.index = m_renderPasses.size();

	m_renderPasses.push_back(passDef);
	m_passes.push_back(handle);
}

void RenderGraph::addTask(const ComputeTaskDefinition &taskDef)
{
	PassHandle handle;
	handle.type = PassHandle::PASS_TYPE_COMPUTE;
	handle.index = m_computeTasks.size();

	m_computeTasks.push_back(taskDef);
	m_passes.push_back(handle);
}
