#include "post_processing.h"

#include "assets/shader_serializer.h"

using namespace gfx;

void PostProcessingRenderer::init(Device *device, ast::AssetManager &assets, RenderGraph &graph)
{
	ast::ShaderAsset *shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("hdr_tonemapping", ast::ASSET_TYPE_SHADER));
	shader = shader_asset->shader;

	AttachmentInfo output_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	AttachmentInfo colour_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	colour_info.is_storage = true;

	output_attachment = graph.create_texture_resource(output_info);
	colour_attachment = graph.create_texture_resource(colour_info);

	exposure = 1.2f;
}

void PostProcessingRenderer::destroy()
{
}

void PostProcessingRenderer::add_render_stages(RenderGraph &graph, RenderGraphBlackboard &bb, const SceneView &view, const RenderResourceHandle &skybox)
{
	RenderStage &stage = graph.add_stage(RenderStage::TYPE_COMPUTE);
	stage.add_colour_output(colour_attachment);
	stage.add_texture(skybox, sync::TEXTURE_ACCESS_graphics_r);

	stage.set_record([&, skybox](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;

		RenderResource &skybox_attachment_resource = graph.get_resource(skybox);
		RenderResource &colour_attachment_resource = graph.get_resource(colour_attachment);

		Texture &in_texture = graph.get_physical_texture(skybox_attachment_resource);
		Texture &out_texture = graph.get_physical_texture(colour_attachment_resource);

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

		args.width = in_texture.get_width();
		args.height = in_texture.get_height();
		args.exposure = this->exposure;
		args.input_image_id = ctx.device.fetch_texture_view_std(in_texture).get_bindless().sampled;
		args.output_image_id = ctx.device.fetch_texture_view_std(out_texture).get_bindless().storage;
		
		cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(args), &args);

		cmd.dispatch(
			out_texture.get_width(),
			out_texture.get_height(),
			1
		);
	});

	RenderStage &blit_to_swapchain = graph.add_stage(RenderStage::TYPE_TRANSFER);
	blit_to_swapchain.add_colour_output(output_attachment);
	blit_to_swapchain.add_texture(colour_attachment, sync::TEXTURE_ACCESS_blit_src);

	blit_to_swapchain.set_record([&](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;
		
		RenderResource &colour_attachment_resource = graph.get_resource(colour_attachment);
		RenderResource &output_attachment_resource = graph.get_resource(output_attachment);

		Texture &src_texture = graph.get_physical_texture(colour_attachment_resource);
		Texture &dst_texture = graph.get_physical_texture(output_attachment_resource);

		VkImageBlit2 region = {};
		region.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
		
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = src_texture.get_layer_count();
		region.srcOffsets[0] = (VkOffset3D){ 0, 0, 0 };
		region.srcOffsets[1] = (VkOffset3D){ (int)src_texture.get_width(), (int)src_texture.get_height(), 1 };

		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = dst_texture.get_layer_count();
		region.dstOffsets[0] = (VkOffset3D){ 0, 0, 0 };
		region.dstOffsets[1] = (VkOffset3D){ (int)dst_texture.get_width(), (int)dst_texture.get_height(), 1 };

		cmd.blit(
			src_texture, dst_texture,
			{ region },
			VK_FILTER_LINEAR
		);
	});
}

void PostProcessingRenderer::set_exposure(float exp)
{
	exposure = exp;
}
