#pragma once

/*
	Debated whether or not this should go into /rendering/ or /vulkan/
	Ultimately put it into VulkanCore since it handles low-level management stuff

	Implementation based on https://themaister.net/blog/2017/08/15/render-graphs-and-vulkan-a-deep-dive/, but somewhat simplified for now
	For instance, it's automatically assumed that all passes are already in order as they are pushed. That is, if we iterate through the m_passes
	list, we can be certain whatever pass we reach will not be dependent on passes in front of it.
*/

/*
Current Todo:
- proper actual dependency system
- have an intermediary "GraphResource" components that can have writers, from which we can then flatten the graph and proceed as normal
- getImageViewResource(<view>), getBufferResource(<buffer>), etc...
- 
*/

#include <vector>
#include <string>
#include <functional>

#include "third_party/volk.h"

#include "math/calc.h"

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
		struct PassHandle
		{
			enum Type
			{
				PASS_TYPE_RENDER,
				PASS_TYPE_COMPUTE
			};

			Type type;
			int index;

			PassHandle()
				: type(PASS_TYPE_RENDER)
				, index(-1)
			{
			}

			bool operator == (const PassHandle &other) { return this->type == other.type && this->index == other.index; }
			bool operator != (const PassHandle &other) { return this->type != other.type || this->index != other.index; }
		};

	public:
		struct AttachmentInfo
		{
			ImageView *view = nullptr;
			ImageView *resolve = nullptr;

			VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_MAX_ENUM;
			VkClearValue clear = {};
		};

		// todo
		struct BufferInfo
		{
			VkDeviceSize size = 0;
			VkBufferUsageFlags usage = 0;
			bool persistent = true;
		};

		struct Resource
		{
			enum Type
			{
				TYPE_IMAGE,
				TYPE_BUFFER
			};

			ImageView *view;
			GPUBuffer *buffer;

			std::vector<PassHandle> writes; // passes that write to me
		};

		class RenderPassDefinition
		{
			friend class RenderGraph;

		public:
			RenderPassDefinition() = default;
			~RenderPassDefinition() = default;

			RenderPassDefinition &setOutputAttachments(const std::vector<AttachmentInfo> &attachments)
			{
				m_outputAttachments = attachments;
				return *this;
			}

			RenderPassDefinition &setInputAttachments(const std::vector<AttachmentInfo> &attachments)
			{
				m_inputAttachments = attachments;
				return *this;
			}

			RenderPassDefinition &setViews(const std::vector<ImageView *> &views)
			{
				m_views = views;
				return *this;
			}

			RenderPassDefinition &setBuildFn(const std::function<void(CommandBuffer &, const RenderInfo &)> &fn)
			{
				m_buildFunc = fn;
				return *this;
			}

		private:
			std::vector<AttachmentInfo> m_outputAttachments;
			std::vector<AttachmentInfo> m_inputAttachments;

			std::vector<ImageView *> m_views;

			std::function<void(CommandBuffer &, const RenderInfo &)> m_buildFunc = nullptr;
		};

		class ComputeTaskDefinition
		{
			friend class RenderGraph;

		public:
			ComputeTaskDefinition() = default;
			~ComputeTaskDefinition() = default;
			
			ComputeTaskDefinition &setStorageAttachments(const std::vector<AttachmentInfo> &attachments)
			{
				m_storageAttachments = attachments;
				return *this;
			}

			ComputeTaskDefinition &setStorageViews(const std::vector<ImageView *> &views)
			{
				m_storageViews = views;
				return *this;
			}

			ComputeTaskDefinition &setBuildFn(const std::function<void(CommandBuffer &)> &fn)
			{
				m_buildFunc = fn;
				return *this;
			}

		private:
			std::vector<AttachmentInfo> m_storageAttachments;
			std::vector<ImageView *> m_storageViews;

			std::function<void(CommandBuffer &)> m_buildFunc = nullptr;
		};

		RenderGraph(const VulkanCore *core);
		~RenderGraph();

		void record(CommandBuffer &cmd, Swapchain *swapchain);

		void addPass(const RenderPassDefinition &passDef);
		void addTask(const ComputeTaskDefinition &taskDef);
		
	private:
		bool validate() const;

//		std::vector<PassHandle> flattenGraphRecursive(const PassHandle &handle);

		void handleRenderPass(CommandBuffer &cmd, Swapchain *swapchain, const PassHandle &handle);
		void handleComputeTask(CommandBuffer &cmd, Swapchain *swapchain, const PassHandle &handle);

		const VulkanCore *m_core;

		std::vector<PassHandle> m_passes;

		std::vector<RenderPassDefinition> m_renderPasses;
		std::vector<ComputeTaskDefinition> m_computeTasks;
	};
}
