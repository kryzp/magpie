#include "skybox_renderer.h"

#include "assets/texture_serializer.h"
#include "assets/shader_serializer.h"
#include "math/vec3.h"

#include "deferred_renderer.h"

using namespace gfx;

void SkyboxRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;

	hdr_texture = assets.get_asset<ast::TextureAsset>(assets.from_file_path("environment_map.hdr"))->texture;

	shader                = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("skybox"))->shader;
	hdr_to_cubemap_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("hdr_to_environment_cubemap"))->shader;

	Vec3 vertices[] = {
		{ -1.f,  1.f,  1.f },
		{ -1.f,  1.f, -1.f },
		{  1.f,  1.f, -1.f },
		{  1.f,  1.f,  1.f },
		{ -1.f, -1.f,  1.f },
		{ -1.f, -1.f, -1.f },
		{  1.f, -1.f, -1.f },
		{  1.f, -1.f,  1.f }
	};

	u32 indices[] = {
		0, 2, 1,
		2, 0, 3,

		7, 5, 6,
		5, 7, 4,

		4, 1, 5,
		1, 4, 0,

		3, 6, 2,
		6, 3, 7,

		1, 6, 5,
		6, 1, 2,

		4, 3, 0,
		3, 4, 7
	};

	mesh.init(device, sizeof(Vec3),
		array_size(vertices), vertices,
		array_size(indices), indices
	);

	cubemap = device->alloc_texture_cubemap(1024, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
}

void SkyboxRenderer::destroy()
{
	mesh.destroy();
	
	device->destroy_texture(cubemap);
}

void SkyboxRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const GpuBuffer *frame_data
)
{
	DeferredRendererInfo deferred_info = bb.get<DeferredRendererInfo>();

	struct SkyboxStageData {
		RenderResourceHandle colour;
		RenderResourceHandle depth;
		RenderResourceHandle cubemap;
	};

	graph.push_stage<SkyboxStageData>(
		"Skybox Rendering",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, SkyboxStageData &data) -> void {
			data.colour = builder.write_colour(deferred_info.gbuffer.lighting);
			data.depth = builder.write_depth(deferred_info.gbuffer.depth);
			data.cubemap = builder.read_texture(graph.import_texture(cubemap));
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const SkyboxStageData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;
			
			const Texture *colour = resources.get_texture(data.colour);
			const TextureView *cubemap_view = resources.get_texture_view(data.cubemap);

			GraphicsPipelineDef pipeline_def(shader);
			pipeline_def.has_depth_attachment = true;
			pipeline_def.depth_stencil_state.depth_test_enabled = true;
			pipeline_def.depth_stencil_state.depth_write_enabled = false;
			pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
			pipeline_def.colour_attachment_formats.push_back(colour->get_format());

			PipelineState st = ctx.device.fetch_pipeline(pipeline_def);

			struct {
				u64 frame_data_buffer;
				u64 vertex_buffer;
				u32 cubemap_id;
				u32 sampler_id;
			} args;

			args.frame_data_buffer = frame_data->get_device_address();
			args.vertex_buffer = this->mesh.vertex_buffer->get_device_address();
			args.cubemap_id = cubemap_view->get_bindless_sampled();
			args.sampler_id = Sampler::linear->get_bindless_handle();

			cmd.bind_bindless(st.bind_point, st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(st.bind_point, st.pipeline);

			cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

			this->mesh.bind_indices(cmd);
			this->mesh.draw_indexed(cmd);
		}
	);
}

void SkyboxRenderer::render_hdr_to_skybox(
	RenderGraph &graph,
	const GpuBuffer *cubemap_capture_transforms
)
{
	struct HdrToSkyboxData {
		int foo;
	};

	graph.push_stage<HdrToSkyboxData>(
		"HDR to Skybox",
		RenderStage::TYPE_GRAPHICS,
		[&](RenderGraphBuilder &builder, HdrToSkyboxData &data) -> void {
			builder.set_multi_view_mask(0b111111);
			builder.write_colour(graph.import_texture(cubemap));
		},
		[=](const RenderContext &ctx, const RenderStageResources &resources, const HdrToSkyboxData &data) -> void {
			CommandBuffer &cmd = ctx.cmd;

			struct {
				u64 transform_matrix_buffer;
				u64 vertex_buffer;
				u32 hdr_image_id;
				u32 linear_sampler_id;
			} args;

			args.transform_matrix_buffer = cubemap_capture_transforms->get_device_address();
			args.vertex_buffer = this->mesh.vertex_buffer->get_device_address();
			args.hdr_image_id = ctx.device.fetch_texture_view_std(this->hdr_texture)->get_bindless_sampled();
			args.linear_sampler_id = Sampler::linear->get_bindless_handle();

			GraphicsPipelineDef pipeline_def(hdr_to_cubemap_shader);
			pipeline_def.depth_stencil_state.depth_test_enabled = false;
			pipeline_def.depth_stencil_state.depth_write_enabled = false;
			pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
			pipeline_def.view_mask = 0b111111;

			PipelineState st = ctx.device.fetch_pipeline(pipeline_def);

			cmd.bind_bindless(st.bind_point, st.layout, ctx.device.get_bindless());
			cmd.bind_pipeline(st.bind_point, st.pipeline);

			cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

			this->mesh.bind_indices(cmd);
			this->mesh.draw_indexed(cmd);
		}
	);
}

const Mesh &SkyboxRenderer::get_mesh() const
{
	return mesh;
}

const Texture *SkyboxRenderer::get_environment_map() const
{
	return cubemap;
}
