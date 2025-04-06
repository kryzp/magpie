#pragma once

/*
	Debated whether or not this should go into /rendering/ or /vulkan/
	Ultimately put it into VulkanCore since it handles low-level management stuff

	Implementation based on https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/, but somewhat simplified for now
	For instance, it's automatically assumed that all passes are already in order as they are pushed. That is, if we iterate through the m_passes
	list, we can be certain whatever pass we reach will not be dependent on passes in front of it.
*/

#include <vector>
#include <string>
#include <functional>

#include "third_party/volk.h"

#include "image_view.h"

namespace mgp
{
	class VulkanCore;
	class CommandBuffer;
	class RenderInfo;
	class Swapchain;
	class ImageView;
	class Image;

	class RenderGraph
	{
	public:
		enum class SizeClass
		{
			SWAPCHAIN_RELATIVE,
			ABSOLUTE
		};

		struct AttachmentInfo
		{
			ImageView *view = nullptr;
			ImageView *resolve = nullptr;

			VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
			VkClearValue clear = {};
		};

		struct BufferInfo
		{
			VkDeviceSize size = 0;
			VkBufferUsageFlags usage = 0;
			bool persistent = true;
		};

		class RenderPassDefinition
		{
			friend class RenderGraph;

		public:
			RenderPassDefinition() = default;
			~RenderPassDefinition() = default;

			RenderPassDefinition &addAttachments(const std::vector<AttachmentInfo> &attachments)
			{
				m_attachments.insert(m_attachments.end(), attachments.begin(), attachments.end());
				return *this;
			}

			RenderPassDefinition &addColourOutputs(const std::vector<AttachmentInfo> &outputs)
			{
				m_colourOutputs.insert(m_colourOutputs.end(), outputs.begin(), outputs.end());
				return *this;
			}

			RenderPassDefinition &setDepthStencilInput(const AttachmentInfo &attachment)
			{
				m_depthStencilInput = attachment;
				return *this;
			}

			RenderPassDefinition &setDepthStencilOutput(const AttachmentInfo &attachment)
			{
				m_depthStencilOutput = attachment;
				return *this;
			}

			RenderPassDefinition &setBuildFn(const std::function<void(CommandBuffer &, const RenderInfo &)> &fn)
			{
				m_buildFunc = fn;
				return *this;
			}

		private:
			std::vector<AttachmentInfo> m_attachments;
			std::vector<AttachmentInfo> m_colourOutputs;

			AttachmentInfo m_depthStencilInput;
			AttachmentInfo m_depthStencilOutput;

			std::function<void(CommandBuffer &, const RenderInfo &)> m_buildFunc = nullptr;
		};

		class ComputeTaskDefinition
		{
		};

		RenderGraph(const VulkanCore *core);
		~RenderGraph();

		void record(CommandBuffer &cmd, Swapchain *swapchain);

		void addPass(const RenderPassDefinition &passDef);
//		void addTask(const ComputeTaskDefinition &taskDef);

	private:
		bool validate() const;
		void reorderPasses();

		const VulkanCore *m_core;

		std::vector<RenderPassDefinition> m_passes;
	};
}
