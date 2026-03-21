#include "ibl_renderer.h"

#include "assets/shader_serializer.h"

using namespace gfx;

void IBLRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;
	this->assets = &assets;

	brdf_shader_asset       = assets.from_file_path("assets://shaders/passes/brdf_lut.slang");
	irradiance_shader_asset = assets.from_file_path("assets://shaders/passes/irradiance_convolution.slang");
	prefilter_shader_asset  = assets.from_file_path("assets://shaders/passes/prefilter_convolution.slang");

	brdf = device->alloc_texture_2d(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);
	irradiance = device->alloc_texture_cubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
	prefilter = device->alloc_texture_cubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, 5);
}

void IBLRenderer::destroy()
{
	device->destroy_texture(brdf);
	device->destroy_texture(irradiance);
	device->destroy_texture(prefilter);
}

void IBLRenderer::render_brdf(
	RenderGraph &graph
)
{
	RenderStage &brdf_stage = graph.push_stage("BRDF LUT Generation", RenderStage::TYPE_GRAPHICS);
	brdf_stage.write_colour(graph.import_texture(brdf));
	brdf_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		GraphicsPipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(brdf_shader_asset)->shader);
		pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32_SFLOAT };

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		cmd.draw(3);
	});
}

void IBLRenderer::render_environment_map(
	RenderGraph &graph,
	const Texture *environment_map,
	const Mesh &skybox,
	const GpuBuffer *capture_transforms
)
{
	RenderResourceHandle env_handle = graph.import_texture(environment_map);
	RenderResourceHandle irradiance_handle = graph.import_texture(irradiance);
	RenderResourceHandle prefilter_handle = graph.import_texture(prefilter);

	RenderStage &irradiance_stage = graph.push_stage("Irradiance Map Convolution", RenderStage::TYPE_GRAPHICS);
	irradiance_stage.set_multi_view_mask(0b111111);
	irradiance_stage.write_colour(irradiance_handle);

	RenderResourceHandle irradiance_environment_map_handle = irradiance_stage.read_texture(env_handle);

	irradiance_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		const TextureView *environment_map_texture = resources.get_texture_view(irradiance_environment_map_handle, SubresourceRange::all_colour());

		GraphicsPipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(irradiance_shader_asset)->shader);
		pipeline_def.multi_view_mask = 0b111111;
		pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };

		PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

		struct {
			u64 transform_matrices;
			u64 vertices;
			u32 environment_map;
			u32 linear_sampler;
		} args;

		args.transform_matrices = capture_transforms->get_device_address();
		args.vertices = skybox.vertex_buffer->get_device_address();
		args.environment_map = environment_map_texture->get_bindless_handle();
		args.linear_sampler = Sampler::linear->get_bindless_handle();

		cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		skybox.bind_indices(cmd);
		skybox.draw_indexed(cmd);
	});
	

	const u32 mip_levels = prefilter->get_mipmap_count();

	for (int i = 0; i < mip_levels; i++) {
		RenderStage &prefilter_stage = graph.push_stage("Prefilter Map Convolution", RenderStage::TYPE_GRAPHICS);
		prefilter_stage.set_multi_view_mask(0b111111);

		SubresourceRange range = {};
		range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
		range.base_mip = i;
		range.mips = 1;
		range.base_layer = 0;
		range.layers = 6;

		prefilter_stage.write_colour(prefilter_handle, range);

		RenderResourceHandle prefilter_environment_map_handle = prefilter_stage.read_texture(env_handle);

		prefilter_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
			CommandBuffer &cmd = ctx.cmd;

			const TextureView *environment_map_view = resources.get_texture_view(prefilter_environment_map_handle, SubresourceRange::all_colour());

			GraphicsPipelineDef pipeline_def(assets->get_asset<ast::ShaderAsset>(prefilter_shader_asset)->shader);
			pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
			pipeline_def.multi_view_mask = 0b111111;

			PipelineState pipeline_st = ctx.cache.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

			struct {
				u64 transform_matrices;
				u64 vertices;
				u32 environment_map;
				u32 linear_sampler;
				float roughness;
			} args;

			args.transform_matrices = capture_transforms->get_device_address();
			args.vertices = skybox.vertex_buffer->get_device_address();
			args.environment_map = environment_map_view->get_bindless_handle();
			args.linear_sampler = Sampler::linear->get_bindless_handle();
			args.roughness = (float)i / (float)(mip_levels - 1);

			cmd.push_constants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

			skybox.bind_indices(cmd);
			skybox.draw_indexed(cmd);
		});
	}
}
