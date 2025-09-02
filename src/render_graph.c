
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

internal void RenderGraphExecuteRenderPasses(RenderGraph *graph,
					     RenderState *rs)
{
	CommandBuffer *cmd = &rs->cmd;

	for (i32 i = 0; i < graph->pass_count; i++) {
		RenderPass pass = graph->passes[i];

		switch (pass.type) {
		case RenderPassType_Graphics: {
			RenderInfo render_info = {0};
			render_info.view_mask = pass.graphics.view_mask;

			for (i32 j = 0; j < pass.graphics.attachment_count; j++) {
				RenderingAttachment *attachment = pass.graphics.attachments + j;

				render_info.width = attachment->width;
				render_info.height = attachment->height;

				render_info.samples = attachment->image->samples;

				if (ImageIsDepth(attachment->image)) {
					render_info.depth_attachment = attachment->info;

					CmdTransitionImageLayout(cmd, attachment->image,
								 VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
				} else {
					render_info.colour_attachments[render_info.colour_attachment_count++] = attachment->info;

					CmdTransitionImageLayout(cmd, attachment->image,
								 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				}
			}

			for (i32 j = 0; j < pass.graphics.view_count; j++) {
				ImageView *view = pass.graphics.views[j];

				if (ImageIsDepth(view->image)) {
					CmdTransitionImageLayout(cmd, view->image,
								 VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
				} else {
					CmdTransitionImageLayout(cmd, view->image,
								 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
				}
			}

			CmdBeginRendering(cmd, &render_info);
			pass.graphics.Record(rs, &render_info, pass.context);
			CmdEndRendering(cmd);
			
			break;
		}

		case RenderPassType_Mipmap: {
			CmdPrepareForMipmapping(cmd, pass.mipmap.image);
			CmdGenerateMipmaps(cmd, pass.mipmap.image);
			break;
		}

		case RenderPassType_Compute: {
			// TODO
			
			break;
		}
		}
	}

	graph->pass_count = 0;
}

internal void RenderGraphPush(RenderGraph *graph, RenderPass *pass)
{
	Assert(graph->pass_count < ArraySize(graph->passes) &&
	       "Cannot push more render passes.");

	graph->passes[graph->pass_count] = *pass;
	graph->pass_count++;
}
