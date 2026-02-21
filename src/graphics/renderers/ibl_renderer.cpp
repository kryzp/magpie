#include "ibl_renderer.h"

#include "assets/shader_serializer.h"

using namespace gfx;

void IBLRenderer::init(ast::AssetManager &assets)
{
	brdf_shader       = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("brdf_lut"))->shader;
	irradiance_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("irradiance_convolution"))->shader;
	prefilter_shader  = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("prefilter_convolution"))->shader;
}

void IBLRenderer::destroy()
{
}

void IBLRenderer::render_brdf(
	RenderGraph &graph,
	Texture *brdf
)
{
	struct RenderBrdfData {
		RenderResourceHandle output;
	};

	graph.push_stage<RenderBrdfData>(
		"BRDF LUT Generation",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, RenderBrdfData &data) -> void {
			data.output = builder.write_colour(graph.import_texture(brdf, { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }, VK_IMAGE_LAYOUT_UNDEFINED));
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const RenderBrdfData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;

			GraphicsPipelineDef pipeline_def(brdf_shader);
			pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32_SFLOAT };

			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(pipeline_st.bind_point, pipeline_st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(pipeline_st.bind_point, pipeline_st.pipeline);

			cmd.draw_vertices_n(3);
		}
	);
}

void IBLRenderer::render_environment_map(
	RenderGraph &graph,
	const Texture *irradiance,
	const Texture *prefilter,
	const Texture *environment_map,
	const Mesh &skybox,
	const GpuBuffer *capture_transforms
)
{
	struct IrradianceStageData {
		RenderResourceHandle environment_map;
	};

	graph.push_stage<IrradianceStageData>(
		"Irradiance Map Convolution",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, IrradianceStageData &data) -> void {
			builder.set_multi_view_mask(0b111111);

			builder.write_colour(graph.import_texture(irradiance, { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }, VK_IMAGE_LAYOUT_UNDEFINED));
			
			data.environment_map = builder.read_texture(graph.import_texture(environment_map, { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }));
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const IrradianceStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;

			const TextureView *environment_map_texture = resources.get_texture_view(data.environment_map, SubresourceRange::all_colour());

			GraphicsPipelineDef pipeline_def(irradiance_shader);
			pipeline_def.view_mask = 0b111111;
			pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };

			PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

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
		}
	);
	
	const u32 mip_levels = prefilter->get_mipmap_count();

	for (int i = 0; i < mip_levels; i++) {
		struct PrefilterStageData {
			RenderResourceHandle environment_map;
		};

		graph.push_stage<PrefilterStageData>(
			"Prefilter Map Convolution",
			RenderStage::TYPE_GRAPHICS,
			[&](RenderGraphBuilder &builder, PrefilterStageData &data) -> void {
				builder.set_multi_view_mask(0b111111);

				SubresourceRange range = {};
				range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
				range.base_mip = i;
				range.mips = 1;
				range.base_layer = 0;
				range.layers = 6;

				builder.write_colour(graph.import_texture(prefilter, { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }, VK_IMAGE_LAYOUT_UNDEFINED), range);

				data.environment_map = builder.read_texture(graph.import_texture(environment_map, { VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE }));
			},
			[=](const RenderContext &ctx, const RenderStageResources &resources, const PrefilterStageData &data) -> void {
				CommandBuffer &cmd = ctx.cmd;

				const TextureView *environment_map_view = resources.get_texture_view(data.environment_map, SubresourceRange::all_colour());

				GraphicsPipelineDef pipeline_def(prefilter_shader);
				pipeline_def.colour_attachment_formats = { VK_FORMAT_R32G32B32A32_SFLOAT };
				pipeline_def.view_mask = 0b111111;

				PipelineState pipeline_st = ctx.device.fetch_pipeline(pipeline_def);

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
			}
		);
	}
}
