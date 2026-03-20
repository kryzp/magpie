#include "post_processing.h"

#include "assets/shader_serializer.h"

#include "deferred_renderer.h"

using namespace gfx;

void PostProcessingRenderer::init(ast::AssetManager &assets)
{
	this->assets = &assets;

	exposure = 1.f;

	tonemapping_asset = assets.from_file_path("assets://hdr_tonemapping.msh");
}

void PostProcessingRenderer::destroy()
{
}

void PostProcessingRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	RenderResourceHandle input_attachment,
	RenderResourceHandle output_attachment
)
{
	AttachmentInfo colour_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	colour_info.is_storage = true;
	RenderResourceHandle colour_attachment = graph.create_texture(colour_info);

	RenderStage &tonemapping_stage = graph.push_stage("Post Processing (Tonemapping)", RenderStage::TYPE_COMPUTE);
	RenderResourceHandle lighting_handle = tonemapping_stage.read_texture_compute(input_attachment);
	RenderResourceHandle colour_handle = tonemapping_stage.write_texture_compute(colour_attachment);

	tonemapping_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		const Texture *in_texture = resources.get_texture(lighting_handle);
		const Texture *out_texture = resources.get_texture(colour_handle);

		ComputePipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(tonemapping_asset)->shader);
		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

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
		args.input_image_id = ctx.cache.fetch_texture_view_std(in_texture)->get_bindless_handle();
		args.output_image_id = ctx.cache.fetch_texture_view_std(out_texture)->get_bindless_handle();
		
		cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args);

		cmd.dispatch(
			compute_group_count(out_texture->get_width(), 1),
			compute_group_count(out_texture->get_height(), 1),
			1
		);
	});

	RenderStage &blit_stage = graph.push_stage("Post Processing (Blit)", RenderStage::TYPE_TRANSFER);
	RenderResourceHandle blit_input_handle = blit_stage.blit_texture_src(colour_attachment);
	RenderResourceHandle blit_output_handle = blit_stage.blit_texture_dst(output_attachment);

	blit_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
		
		const Texture *src_texture = resources.get_texture(blit_input_handle);
		const Texture *dst_texture = resources.get_texture(blit_output_handle);

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
	});
}

float PostProcessingRenderer::get_exposure() const
{
	return exposure;
}

void PostProcessingRenderer::set_exposure(float exp)
{
	exposure = exp;
}
