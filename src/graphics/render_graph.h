#pragma once

#include <vector>
#include <functional>

#include <Volk/volk.h>

#include "render_info.h"

namespace mgp
{
	class GraphicsCore;
	class Swapchain;

	struct RenderGraphAttachment
	{
		ImageView *view;
		ImageView *resolve;

		VkResolveModeFlagBits resolveMode;

		VkAttachmentLoadOp loadOp;
		VkAttachmentStoreOp storeOp;

		Colour colourClear;

		float depthClear;
		uint32_t stencilClear;

		static RenderGraphAttachment getColour(VkAttachmentLoadOp loadOp, ImageView *view, ImageView *resolve, const Colour &colourClear)
		{
			RenderGraphAttachment attachment = {};
			attachment.view = view;
			attachment.loadOp = loadOp;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.colourClear = colourClear;
			attachment.resolve = resolve;
			attachment.resolveMode = resolve ? VK_RESOLVE_MODE_AVERAGE_BIT : VK_RESOLVE_MODE_NONE;

			return attachment;
		}

		static RenderGraphAttachment getDepth(VkAttachmentLoadOp loadOp, ImageView *view, ImageView *resolve, float depthClear, uint32_t stencilClear)
		{
			RenderGraphAttachment attachment = {};
			attachment.view = view;
			attachment.loadOp = loadOp;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.depthClear = depthClear;
			attachment.stencilClear = stencilClear;
			attachment.resolve = resolve;
			attachment.resolveMode = resolve ? VK_RESOLVE_MODE_SAMPLE_ZERO_BIT : VK_RESOLVE_MODE_NONE;

			return attachment;
		}
	};

	class RenderPassDef
	{
	public:
		RenderPassDef() = default;
		~RenderPassDef() = default;

		RenderPassDef &setAttachments(const std::vector<RenderGraphAttachment> &attachments)
		{
			m_attachments = attachments;
			return *this;
		}

		RenderPassDef &setInputViews(const std::vector<ImageView *> &views)
		{
			m_views = views;
			return *this;
		}

		RenderPassDef &setRecordFn(const std::function<void(CommandBuffer *, const RenderInfo &)> &fn)
		{
			m_recordFn = fn;
			return *this;
		}

		const std::vector<RenderGraphAttachment> &getAttachments() const
		{
			return m_attachments;
		}

		const std::vector<ImageView *> &getInputViews() const
		{
			return m_views;
		}

		const std::function<void(CommandBuffer *, const RenderInfo &)> &getRecordFn() const
		{
			return m_recordFn;
		}

	private:
		std::vector<RenderGraphAttachment> m_attachments;
		std::vector<ImageView *> m_views;
		std::function<void(CommandBuffer *, const RenderInfo &)> m_recordFn = nullptr;
	};

	class ComputeTaskDef
	{
	public:
		ComputeTaskDef() = default;
		~ComputeTaskDef() = default;
			
		ComputeTaskDef &setStorageViews(const std::vector<ImageView *> &views)
		{
			m_storageViews = views;
			return *this;
		}

		ComputeTaskDef &setRecordFn(const std::function<void(CommandBuffer *)> &fn)
		{
			m_recordFn = fn;
			return *this;
		}

		const std::vector<ImageView *> &getStorageViews() const
		{
			return m_storageViews;
		}

		const std::function<void(CommandBuffer *)> &getRecordFn() const
		{
			return m_recordFn;
		}

	private:
		std::vector<ImageView *> m_storageViews;
		std::function<void(CommandBuffer *)> m_recordFn = nullptr;
	};

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
		RenderGraph(GraphicsCore *gfx);
		~RenderGraph() = default;

		void recordTo(CommandBuffer *cmd, Swapchain *swapchain);

		void addPass(const RenderPassDef &passDef);
		void addTask(const ComputeTaskDef &taskDef);

	private:
		GraphicsCore *m_gfx;

		bool validate() const;

		void handleRenderPass(CommandBuffer *cmd, Swapchain *swapchain, const PassHandle &handle);
		void handleComputeTask(CommandBuffer *cmd, Swapchain *swapchain, const PassHandle &handle);
		
		std::vector<PassHandle> m_passes;

		std::vector<RenderPassDef> m_renderPasses;
		std::vector<ComputeTaskDef> m_computeTasks;
	};
}
