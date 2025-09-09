
internal RenderingAttachment RenderingAttachmentInitColour(VkAttachmentLoadOp load_op,
							   ImageView *view,
							   ImageView *resolve,
							   v4 clear_colour)
{
	RenderingAttachment attachment = {0};

	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .color = { clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a } };

	if (resolve) {
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	} else {
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	attachment.image = view->image;

	attachment.width = view->image->width >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;

	return attachment;
}

internal RenderingAttachment RenderingAttachmentInitDepth(VkAttachmentLoadOp load_op,
							  ImageView *view,
							  ImageView *resolve,
							  f32 clear_depth, u32 clear_stencil)
{
	RenderingAttachment attachment = {0};

	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->view;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .depthStencil = { clear_depth, clear_stencil } };

	if (resolve) {
		attachment.info.resolveImageView = resolve->view;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	} else {
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	attachment.image = view->image;

	attachment.width = view->image->width >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;

	return attachment;
}

internal VkImageMemoryBarrier2 RenderGraphTransitionImage(Image *image, ImageAccessType target_access)
{
	ImageAccessInfo src_access_info = SyncGetSrcImageAccessInfo(image->access_type);
	ImageAccessInfo dst_access_info = SyncGetDstImageAccessInfo(target_access);
	
	image->access_type = target_access;
	
	return ImageGetMemoryBarrier(image, src_access_info, dst_access_info);
}

internal VkBufferMemoryBarrier2 RenderGraphTransitionBuffer(GPUBuffer *buffer, GPUBufferAccessType target_access)
{
	GPUBufferAccessInfo src_access_info = SyncGetSrcBufferAccessInfo(buffer->access_type);
	GPUBufferAccessInfo dst_access_info = SyncGetDstBufferAccessInfo(target_access);
	
	buffer->access_type = target_access;
	
	return GPUBufferGetMemoryBarrier(buffer, src_access_info, dst_access_info);
}

internal void RenderGraphExecute(RenderGraph *graph, RenderState *rs, MemoryArena *arena)
{
	ScratchArena scratch = GetScratch(arena, 1);
	
	CommandBuffer *cmd = &rs->cmd;

	for (i32 i = 0; i < graph->pass_count; i++) {
		RenderPass *pass = &graph->passes[i];

		switch (pass->type) {
		case RenderPassType_Graphics: {
			u32 image_barrier_count = 0;
			VkImageMemoryBarrier2 *image_barriers = MemoryArenaPushC(scratch.arena, sizeof(VkImageMemoryBarrier2),
										 pass->graphics.attachment_count + pass->graphics.view_count);
			
			RenderInfo render_info = {0};
			render_info.view_mask = pass->graphics.view_mask;

			for (i32 j = 0; j < pass->graphics.attachment_count; j++) {
				RenderingAttachment *attachment = pass->graphics.attachments + j;

				render_info.width = attachment->width;
				render_info.height = attachment->height;
				
				render_info.samples = attachment->image->samples;

				ImageAccessType target_access = ImageAccessType_Undefined;
				
				if (ImageIsDepth(attachment->image)) {
					render_info.depth_attachment = attachment->info;
					target_access = ImageAccessType_DepthWrite;
				} else {
					render_info.colour_attachments[render_info.colour_attachment_count++] = attachment->info;
					target_access = ImageAccessType_ColourWrite;
				}

				image_barriers[image_barrier_count++] = RenderGraphTransitionImage(attachment->image, target_access);
			}

			for (i32 j = 0; j < pass->graphics.view_count; j++) {
				ImageView *view = pass->graphics.views[j];
				image_barriers[image_barrier_count++] = RenderGraphTransitionImage(view->image, ImageAccessType_GraphicsRead);
			}
			
			u32 buffer_barrier_count = 0;
			VkBufferMemoryBarrier2 *buffer_barriers = MemoryArenaPushC(scratch.arena, sizeof(VkBufferMemoryBarrier2),
										   pass->graphics.buffer_count);

			for (i32 j = 0; j < pass->graphics.buffer_count; j++) {
				GPUBuffer *buffer = pass->graphics.buffers[j];
				buffer_barriers[buffer_barrier_count++] = RenderGraphTransitionBuffer(buffer, GPUBufferAccessType_GraphicsReadWrite);
			}

			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   buffer_barrier_count, buffer_barriers,
					   image_barrier_count, image_barriers);

			CmdBeginRendering(cmd, &render_info);
			pass->graphics.Record(rs, &render_info, pass->context);
			CmdEndRendering(cmd);
			
			break;
		}

		case RenderPassType_Compute: {
			u32 buffer_barrier_count = 0;
			VkBufferMemoryBarrier2 *buffer_barriers = MemoryArenaPushC(scratch.arena, sizeof(VkBufferMemoryBarrier2),
										   pass->compute.buffer_count);

			for (i32 j = 0; j < pass->compute.buffer_count; j++) {
				GPUBuffer *buffer = pass->compute.buffers[j];
				buffer_barriers[buffer_barrier_count++] = RenderGraphTransitionBuffer(buffer, GPUBufferAccessType_ComputeReadWrite);
			}
			
			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   buffer_barrier_count, buffer_barriers,
					   0, NULL);

			pass->compute.Record(rs, pass->context);
			
			break;
		}

		case RenderPassType_Mipmap: {
			Image *image = pass->mipmap.image;
			
			VkImageMemoryBarrier2 transition_barrier = RenderGraphTransitionImage(image, ImageAccessType_TransferDst);

			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   0, NULL,
					   1, &transition_barrier);
			
			CmdGenerateMipmaps(cmd, image);

			image->access_type = ImageAccessType_GraphicsRead;

			break;
		}

		case RenderPassType_Present: {
			Image *swapchain = pass->present.swapchain;

			VkImageMemoryBarrier2 present_barrier = RenderGraphTransitionImage(swapchain, ImageAccessType_Present);

			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   0, NULL,
					   1, &present_barrier);

			swapchain->access_type = ImageAccessType_Present;
			
			break;
		}
		}
	}

	graph->pass_count = 0;

	ReleaseScratch(&scratch);
}

internal void RenderGraphPush(RenderGraph *graph, RenderPass *pass)
{
	Assert(graph->pass_count < ArraySize(graph->passes) &&
	       "Cannot push more render passes.");

	graph->passes[graph->pass_count] = *pass;
	graph->pass_count++;
}
