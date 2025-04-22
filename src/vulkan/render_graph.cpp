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

//	std::vector<PassHandle> passStack = flattenGraphRecursive(m_passes[m_passes.size() - 1]);

//	std::reverse(passStack.begin(), passStack.end());

//	std::vector<PassHandle> leanPassStack;

	/*
	for (auto &p1 : passStack)
	{
		bool seen = false;

		for (auto &p2 : leanPassStack)
		{
			if (p1 == p2)
			{
				seen = true;
				break;
			}
		}

		if (!seen)
		{
			leanPassStack.push_back(p1);
		}
	}
	*/

	for (cauto &p : m_passes /*leanPassStack*/)
	{
		// very scuffed right now just to get the ball rolling i guess
		// JIT transitions
		// yes, not the best
		// yes, i'll make it better... at some point... (tbd)

		switch (p.type)
		{
			case PassHandle::PASS_TYPE_RENDER:
			{
				handleRenderPass(cmd, swapchain, p);
				break;
			}

			case PassHandle::PASS_TYPE_COMPUTE:
			{
				handleComputeTask(cmd, swapchain, p);
				break;
			}
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

/*
std::vector<RenderGraph::PassHandle> RenderGraph::flattenGraphRecursive(const PassHandle &handle)
{
	std::vector<PassHandle> flattened = { handle };
	std::vector<PassHandle> dependencies;

	switch (handle.type)
	{
		case PassHandle::PASS_TYPE_RENDER:
		{
			RenderPassDefinition *renderPass = &m_renderPasses[handle.index];

			for (auto &input : renderPass->m_inputAttachments)
				dependencies.push_back(input.parent);

			break;
		}

		case PassHandle::PASS_TYPE_COMPUTE:
		{
			ComputeTaskDefinition *computeTask = &m_computeTasks[handle.index];

			for (auto &input : computeTask->m_storageAttachments)
				dependencies.push_back(input.parent);

			break;
		}
	}

	for (auto &dependency : dependencies)
	{
		std::vector<PassHandle> subgraph = flattenGraphRecursive(dependency);
		flattened.insert(flattened.end(), subgraph.begin(), subgraph.end());
	}

	return flattened;
}
*/

void RenderGraph::handleRenderPass(CommandBuffer &cmd, Swapchain *swapchain, const PassHandle &handle)
{
	const RenderPassDefinition &pass = m_renderPasses[handle.index];

	RenderInfo info(m_core);

	int nColourAttachments = 0;

	for (auto &attachment : pass.m_outputAttachments)
	{
		Image *image = attachment.view->getImage();

		// currently we assume all attachments have the same size and MSAA sample count
		// however, in the future, we could try to automatically resize all attachments
		// depending on either a given size or to be consistent with the size of the
		// first attachment in the list or something i dont really know right now
		info.setSize(image->getWidth(), image->getHeight());
		info.setMSAA(image->getSamples());

		if (image->isDepth())
		{
			info.addDepthAttachment(
				attachment.loadOp,
				*attachment.view,
				attachment.resolve
			);

			info.setClearDepth(attachment.clear);

			cmd.transitionLayout(
				*attachment.view->getImage(),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			);
		}
		else
		{
			info.addColourAttachment(
				attachment.loadOp,
				*attachment.view,
				attachment.resolve
			);

			info.setClearColour(nColourAttachments++, attachment.clear);

			cmd.transitionLayout(
				*attachment.view->getImage(),
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			);
		}
	}

	for (cauto &view : pass.m_views)
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

	for (cauto &attachment : pass.m_inputAttachments)
	{
		if (attachment.view->getImage()->isDepth())
		{
			cmd.transitionLayout(
				*attachment.view->getImage(),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			);
		}
		else
		{
			cmd.transitionLayout(
				*attachment.view->getImage(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);
		}
	}

	cmd.beginRendering(info);
	pass.m_buildFunc(cmd, info);
	cmd.endRendering();
}

void RenderGraph::handleComputeTask(CommandBuffer &cmd, Swapchain *swapchain, const PassHandle &handle)
{
	const ComputeTaskDefinition &task = m_computeTasks[handle.index];

	for (cauto &attachment : task.m_storageAttachments)
	{
		cmd.transitionLayout(
			*attachment.view->getImage(),
			VK_IMAGE_LAYOUT_GENERAL
		);
	}

	for (cauto &view : task.m_storageViews)
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
