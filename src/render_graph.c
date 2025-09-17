
internal RenderingAttachment RenderingAttachmentInitColour(VkAttachmentLoadOp load_op,
							   ImageView *view,
							   ImageView *resolve,
							   v4 clear_colour)
{
	RenderingAttachment attachment = {0};

	attachment.info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
	attachment.info.imageView = view->handle;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .color = { clear_colour.r, clear_colour.g, clear_colour.b, clear_colour.a } };

	if (resolve) {
		attachment.info.resolveImageView = resolve->handle;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
	} else {
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	attachment.view = view;

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
	attachment.info.imageView = view->handle;
	attachment.info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
	attachment.info.loadOp = load_op;
	attachment.info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.info.clearValue = (VkClearValue){ .depthStencil = { clear_depth, clear_stencil } };

	if (resolve) {
		attachment.info.resolveImageView = resolve->handle;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_GENERAL;
		attachment.info.resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
	} else {
		attachment.info.resolveImageView = VK_NULL_HANDLE;
		attachment.info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.info.resolveMode = VK_RESOLVE_MODE_NONE;
	}

	attachment.view = view;
	
	attachment.width = view->image->width >> view->base_mip_level;
	attachment.height = view->image->height >> view->base_mip_level;

	return attachment;
}

// TODO: URGENT!!!
//       LAYOUT TRANSITIONS ONLY TRANSITION THE FIRST (0) ASPECT OF THE IMAGE.
//       THEREFORE IN THE CASE OF DEPTH_STENCIL, WE ONLY TRANSITION DEPTH.
//       SO STENCIL WILL NOT WORK.
//       FIX.

internal void RenderGraphTransitionImageView(ImageView *view, ImageAccessType dst_access,
					     VkImageMemoryBarrier2 *barriers, u32 *barrier_count)
{
	ImageAccessInfo dst_access_info = SyncGetDstImageAccessInfo(dst_access);

	for (u32 i = 0; i < view->layer_count; i++) {
		for (u32 j = 0; j < view->mip_level_count; j++) {
			ImageAccessType src_access = ImageGetAccessType(view->image,
									view->base_mip_level + j,
									view->layer + i,
									0);

			if (src_access == dst_access)
				continue;
		
			ImageAccessInfo src_access_info = SyncGetSrcImageAccessInfo(src_access);
		
			VkImageMemoryBarrier2 b = ImageGetMemoryBarrier(view->image,
									src_access_info,
									dst_access_info,
									view->base_mip_level + j, 1,
									view->layer + i, 1);
			
			ImageSetAccessType(view->image,
					   j, i, 0,
					   dst_access);
			
			barriers[*barrier_count] = b;
			*barrier_count = *barrier_count + 1;
		}
	}
}

internal void RenderGraphTransitionImage(Image *image, ImageAccessType dst_access,
					 VkImageMemoryBarrier2 *barriers, u32 *barrier_count)
{
	ImageAccessInfo dst_access_info = SyncGetDstImageAccessInfo(dst_access);

	for (u32 i = 0; i < image->mipmap_count; i++) {
		for (u32 j = 0; j < ImageLayerCount(image); j++) {
			ImageAccessType src_access = ImageGetAccessType(image, i, j, 0);

			if (src_access == dst_access)
				continue;
		
			ImageAccessInfo src_access_info = SyncGetSrcImageAccessInfo(src_access);

			VkImageMemoryBarrier2 b = ImageGetMemoryBarrier(image,
									src_access_info,
									dst_access_info,
									i, 1,
									j, 1);

			ImageSetAccessType(image,
					   i, j, 0,
					   dst_access);
				
			barriers[*barrier_count] = b;
			*barrier_count = *barrier_count + 1;
		}
	}
}

internal void RenderGraphTransitionBuffer(GPUBuffer *buffer, GPUBufferAccessType dst_access,
					  VkBufferMemoryBarrier2 *barriers, u32 *barrier_count)
{
	GPUBufferAccessInfo src_access_info = SyncGetSrcBufferAccessInfo(buffer->access_type);
	GPUBufferAccessInfo dst_access_info = SyncGetDstBufferAccessInfo(dst_access);
	
	buffer->access_type = dst_access;

	barriers[*barrier_count] = GPUBufferGetMemoryBarrier(buffer, src_access_info, dst_access_info);
	*barrier_count = *barrier_count + 1;
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
			VkImageMemoryBarrier2 *image_barriers = MemoryArenaPushC(scratch.arena,
										 64,
										 sizeof(VkImageMemoryBarrier2));
			
			RenderInfo render_info = {0};
			render_info.view_mask = pass->graphics.view_mask;

			for (i32 j = 0; j < pass->graphics.attachment_count; j++) {
				RenderingAttachment *attachment = pass->graphics.attachments + j;

				render_info.width = attachment->width;
				render_info.height = attachment->height;
				
				render_info.samples = attachment->view->image->samples;

				ImageAccessType target_access = ImageAccessType_Undefined;
				
				if (ImageIsDepth(attachment->view->image)) {
					render_info.depth_attachment = attachment->info;
					target_access = ImageAccessType_DepthWrite;
				} else {
					render_info.colour_attachments[render_info.colour_attachment_count++] = attachment->info;
					target_access = ImageAccessType_ColourWrite;
				}

				RenderGraphTransitionImageView(attachment->view, target_access,
							       image_barriers, &image_barrier_count);
			}

			for (i32 j = 0; j < pass->graphics.view_count; j++) {
				ImageView *view = pass->graphics.views[j];
				RenderGraphTransitionImageView(view, ImageAccessType_GraphicsRead,
							       image_barriers, &image_barrier_count);
			}
			
			u32 buffer_barrier_count = 0;
			VkBufferMemoryBarrier2 *buffer_barriers = MemoryArenaPushC(scratch.arena,
										   pass->graphics.buffer_count,
										   sizeof(VkBufferMemoryBarrier2));

			for (i32 j = 0; j < pass->graphics.buffer_count; j++) {
				GPUBuffer *buffer = pass->graphics.buffers[j];
				RenderGraphTransitionBuffer(buffer, GPUBufferAccessType_GraphicsReadWrite,
							    buffer_barriers, &buffer_barrier_count);
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
			u32 image_barrier_count = 0;
			VkImageMemoryBarrier2 *image_barriers = MemoryArenaPushC(scratch.arena,
										 64,
										 sizeof(VkImageMemoryBarrier2));

			for (i32 j = 0; j < pass->compute.read_only_view_count; j++) {
				ImageView *view = pass->compute.read_only_views[j];
				RenderGraphTransitionImageView(view, ImageAccessType_ComputeRead,
							       image_barriers, &image_barrier_count);
			}

			for (i32 j = 0; j < pass->compute.rw_view_count; j++) {
				ImageView *view = pass->compute.rw_views[j];
				RenderGraphTransitionImageView(view, ImageAccessType_ComputeReadWrite,
							       image_barriers, &image_barrier_count);
			}
			
			u32 buffer_barrier_count = 0;
			VkBufferMemoryBarrier2 *buffer_barriers = MemoryArenaPushC(scratch.arena,
										   64,
										   sizeof(VkBufferMemoryBarrier2));

			for (i32 j = 0; j < pass->compute.buffer_count; j++) {
				GPUBuffer *buffer = pass->compute.buffers[j];
				RenderGraphTransitionBuffer(buffer, GPUBufferAccessType_ComputeReadWrite,
							    buffer_barriers, &buffer_barrier_count);
			}
			
			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   buffer_barrier_count, buffer_barriers,
					   image_barrier_count, image_barriers);
			
			pass->compute.Record(rs, pass->context);
			
			break;
		}

		case RenderPassType_Transfer: {
			u32 image_barrier_count = 0;
			VkImageMemoryBarrier2 *image_barriers = MemoryArenaPushC(scratch.arena,
										 64,
										 sizeof(VkImageMemoryBarrier2));

			for (i32 j = 0; j < pass->transfer.src_count; j++) {
				ImageView *view = pass->transfer.src[j];
				RenderGraphTransitionImageView(view, ImageAccessType_TransferSrc,
							       image_barriers, &image_barrier_count);
			}

			for (i32 j = 0; j < pass->transfer.dst_count; j++) {
				ImageView *view = pass->transfer.dst[j];
				RenderGraphTransitionImageView(view, ImageAccessType_TransferDst,
							       image_barriers, &image_barrier_count);
			}
			
			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   0, NULL,
					   image_barrier_count, image_barriers);
			
			pass->transfer.Record(rs, pass->context);
			
			break;
		}
			
		case RenderPassType_Mipmap: {
			Image *image = pass->mipmap.image;

			u32 count = 0;
			VkImageMemoryBarrier2 *barriers = MemoryArenaPushC(scratch.arena,
									   64,
									   sizeof(VkImageMemoryBarrier2));

			RenderGraphTransitionImage(image, ImageAccessType_TransferDst,
						   barriers, &count);

			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   0, NULL,
					   count, barriers);
			
			CmdGenerateMipmaps(cmd, image);

			break;
		}

		case RenderPassType_Present: {
			Image *swapchain = pass->present.swapchain;

			u32 count = 0;
			VkImageMemoryBarrier2 present_barrier = {0};

			RenderGraphTransitionImage(swapchain, ImageAccessType_Present,
						   &present_barrier, &count);

			CmdPipelineBarrier(cmd, 0,
					   0, NULL,
					   0, NULL,
					   1, &present_barrier);

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
