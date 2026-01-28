#include "post_processing.h"

#include "assets/shader_serializer.h"

#include "deferred_renderer.h"

using namespace gfx;

void PostProcessingRenderer::init(ast::AssetManager &assets)
{
	exposure = 1.2f;

	shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("hdr_tonemapping"))->shader;
}

void PostProcessingRenderer::destroy()
{
}

void PostProcessingRenderer::add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, RenderResourceHandle output_attachment)
{
	DeferredRendererInfo deferred_info = bb.get<DeferredRendererInfo>();

	RenderResourceHandle colour_attachment;

	struct PostProcessingInfo {
		RenderResourceHandle lighting;
		RenderResourceHandle colour;
	};

	graph.push_stage<PostProcessingInfo>(
		"Post Processing Tonemapping",
		RenderStage::TYPE_COMPUTE,
		[&](RenderGraphBuilder &builder, PostProcessingInfo &data) -> void {
			AttachmentInfo colour_info(VK_FORMAT_R32G32B32A32_SFLOAT);
			colour_info.is_storage = true;
			colour_attachment = builder.create_texture(colour_info);

			data.lighting = builder.read_texture(deferred_info.lighting);
			data.colour = builder.write_colour(colour_attachment);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const PostProcessingInfo &data) -> void {
			CommandBuffer &cmd = ctx.cmd;

			const Texture *in_texture = resources.get_texture(data.lighting);
			const Texture *out_texture = resources.get_texture(data.colour);

			ComputePipelineDef pipeline_def(shader);
			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

			struct {
				u32 width;
				u32 height;
				float exposure;
				u32 input_image_id;
				u32 output_image_id;
			} args;

			args.width = in_texture->get_width();
			args.height = in_texture->get_height();
			args.exposure = this->exposure;
			args.input_image_id = ctx.device.fetch_texture_view_std(in_texture).get_bindless().sampled;
			args.output_image_id = ctx.device.fetch_texture_view_std(out_texture).get_bindless().storage;
		
			cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args);

			cmd.dispatch(
				out_texture->get_width(),
				out_texture->get_height(),
				1
			);
		}
	);

	struct BlitToSwapchainData {
		RenderResourceHandle input;
		RenderResourceHandle output;
	};

	graph.push_stage<BlitToSwapchainData>(
		"Post Processing Blit",
		RenderStage::TYPE_TRANSFER,
		[&](RenderGraphBuilder &builder, BlitToSwapchainData &data) -> void {
			data.input = builder.blit_texture_src(colour_attachment);
			data.output = builder.blit_texture_dst(output_attachment);
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const BlitToSwapchainData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;
		
			const Texture *src_texture = resources.get_texture(data.input);
			const Texture *dst_texture = resources.get_texture(data.output);

			VkImageBlit2 region = {};
			region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		
			region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.srcSubresource.mipLevel = 0;
			region.srcSubresource.baseArrayLayer = 0;
			region.srcSubresource.layerCount = src_texture->get_layer_count();
			region.srcOffsets[0] = { 0, 0, 0 };
			region.srcOffsets[1] = { (int)src_texture->get_width(), (int)src_texture->get_height(), 1 };

			region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.dstSubresource.mipLevel = 0;
			region.dstSubresource.baseArrayLayer = 0;
			region.dstSubresource.layerCount = dst_texture->get_layer_count();
			region.dstOffsets[0] = { 0, 0, 0 };
			region.dstOffsets[1] = { (int)dst_texture->get_width(), (int)dst_texture->get_height(), 1 };

			cmd.blit(
				src_texture, dst_texture,
				{ region },
				VK_FILTER_LINEAR
			);
		}
	);
}

float PostProcessingRenderer::get_exposure() const
{
	return exposure;
}

void PostProcessingRenderer::set_exposure(float exp)
{
	exposure = exp;
}
