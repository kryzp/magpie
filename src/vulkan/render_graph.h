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

			RenderPassDefinition &setColourAttachments(const std::vector<AttachmentInfo> &attachments)
			{
				m_colourAttachments = attachments;
				return *this;
			}

			RenderPassDefinition &setDepthStencilAttachment(const AttachmentInfo &attachment)
			{
				m_depthStencilAttachment = attachment;
				return *this;
			}

			RenderPassDefinition &setInputViews(const std::vector<ImageView *> &views)
			{
				m_inputViews = views;
				return *this;
			}

			RenderPassDefinition &setBuildFn(const std::function<void(CommandBuffer &, const RenderInfo &)> &fn)
			{
				m_buildFunc = fn;
				return *this;
			}

		private:
			std::vector<AttachmentInfo> m_colourAttachments;
			AttachmentInfo m_depthStencilAttachment;

			std::vector<ImageView *> m_inputViews;

			std::function<void(CommandBuffer &, const RenderInfo &)> m_buildFunc = nullptr;
		};

		class ComputeTaskDefinition
		{
			friend class RenderGraph;

		public:
			ComputeTaskDefinition() = default;
			~ComputeTaskDefinition() = default;

			ComputeTaskDefinition &setInputStorageViews(const std::vector<ImageView *> &views)
			{
				m_inputStorageViews = views;
				return *this;
			}

			ComputeTaskDefinition &setBuildFn(const std::function<void(CommandBuffer &)> &fn)
			{
				m_buildFunc = fn;
				return *this;
			}

		private:
			std::vector<ImageView *> m_inputStorageViews;

			std::function<void(CommandBuffer &)> m_buildFunc = nullptr;
		};

		RenderGraph(const VulkanCore *core);
		~RenderGraph();

		void record(CommandBuffer &cmd, Swapchain *swapchain);

		void addPass(const RenderPassDefinition &passDef);
		void addTask(const ComputeTaskDefinition &taskDef);
		
	private:
		bool validate() const;

		struct PassHandle
		{
			enum Type
			{
				PASS_TYPE_RENDER,
				PASS_TYPE_COMPUTE
			};

			Type type;
			int index;
		};

		void handleRenderPass(CommandBuffer &cmd, Swapchain *swapchain, const RenderPassDefinition &pass);
		void handleComputeTask(CommandBuffer &cmd, Swapchain *swapchain, const ComputeTaskDefinition &task);

		const VulkanCore *m_core;

		std::vector<PassHandle> m_passes;

		std::vector<RenderPassDefinition> m_renderPasses;
		std::vector<ComputeTaskDefinition> m_computeTasks;
	};
}
