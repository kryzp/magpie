#include "skybox_renderer.h"

#include "assets/shader_serializer.h"
#include "math/vec3.h"

#include "deferred_renderer.h"

using namespace gfx;

void SkyboxRenderer::init(Device *device, ast::AssetManager &assets)
{
	this->device = device;

	shader                = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://skybox.msh"))->shader;
	hdr_to_cubemap_shader = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("assets://hdr_to_environment_cubemap.msh"))->shader;

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

	gfx::IndexType indices[] = {
		1, 2, 0,
		3, 0, 2,

		6, 5, 7,
		4, 7, 5,

		5, 1, 4,
		0, 4, 1,

		2, 6, 3,
		7, 3, 6,

		5, 6, 1,
		2, 1, 6,

		0, 3, 4,
		7, 4, 3
	};

	mesh.create_buffers(
		device, sizeof(Vec3),
		array_size(vertices),
		array_size(indices)
	);

	GpuBuffer *staging_buffer = device->alloc_stage(
		mesh.get_vertex_buffer_size() + mesh.get_index_buffer_size()
	);
	
	mesh.write_to_staging_buffer(staging_buffer, 0, vertices, indices);

	device->submit_graphics_immediate([&](CommandBuffer &cmd) {
		mesh.batch_upload(cmd, staging_buffer, 0);
	});

	device->destroy_buffer(staging_buffer);

	cubemap = device->alloc_texture_cubemap(512, VK_FORMAT_R32G32B32A32_SFLOAT, 8);
}

void SkyboxRenderer::destroy()
{
	mesh.destroy_buffers();
	
	device->destroy_texture(cubemap);
}

void SkyboxRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const GpuBuffer *frame_data
)
{
	DeferredRendererInfo deferred_info = bb.get<DeferredRendererInfo>();

	RenderStage &skybox_stage = graph.push_stage("Skybox Rendering", RenderStage::TYPE_GRAPHICS);

	RenderResourceHandle colour_handle = skybox_stage.write_colour(deferred_info.gbuffer.lighting);
	RenderResourceHandle depth_handle = skybox_stage.write_depth(deferred_info.gbuffer.depth);
	RenderResourceHandle cubemap_handle = skybox_stage.read_texture(graph.import_texture(cubemap, { VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT }));

	skybox_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
			
		const Texture *colour = resources.get_texture(colour_handle);
		const TextureView *cubemap_view = resources.get_texture_view(cubemap_handle, SubresourceRange::all_colour());

		GraphicsPipelineDef pipeline_def(shader);
		pipeline_def.has_depth_attachment = true;
		pipeline_def.depth_stencil_state.depth_test_enabled = true;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.colour_attachment_formats.push_back(colour->get_format());

		PipelineState st = ctx.cache.fetch_pipeline(pipeline_def);

		struct {
			u64 frame_data_buffer;
			u64 vertex_buffer;
			u32 cubemap_id;
			u32 sampler_id;
		} args;

		args.frame_data_buffer = frame_data->get_device_address();
		args.vertex_buffer = this->mesh.vertex_buffer->get_device_address();
		args.cubemap_id = cubemap_view->get_bindless_handle();
		args.sampler_id = Sampler::linear->get_bindless_handle();

		cmd.bind_bindless(st.bind_point, st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(st.bind_point, st.pipeline);

		cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		this->mesh.bind_indices(cmd);
		this->mesh.draw_indexed(cmd);
	});
}

void SkyboxRenderer::render_hdr_to_skybox(
	RenderGraph &graph,
	const Texture *hdr_texture,
	const GpuBuffer *cubemap_capture_transforms
)
{
	RenderResourceHandle cubemap_handle = graph.import_texture(cubemap, { VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE }, VK_IMAGE_LAYOUT_UNDEFINED);

	RenderStage &hdr_to_skybox_stage = graph.push_stage("HDR to Skybox", RenderStage::TYPE_GRAPHICS);
	hdr_to_skybox_stage.set_multi_view_mask(0b111111);
	hdr_to_skybox_stage.write_colour(cubemap_handle);

	hdr_to_skybox_stage.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;

		struct {
			u64 transform_matrix_buffer;
			u64 vertex_buffer;
			u32 hdr_image_id;
			u32 linear_sampler_id;
		} args;

		args.transform_matrix_buffer = cubemap_capture_transforms->get_device_address();
		args.vertex_buffer = this->mesh.vertex_buffer->get_device_address();
		args.hdr_image_id = ctx.cache.fetch_texture_view_std(hdr_texture)->get_bindless_handle();
		args.linear_sampler_id = Sampler::linear->get_bindless_handle();

		GraphicsPipelineDef pipeline_def(hdr_to_cubemap_shader);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
		pipeline_def.view_mask = 0b111111;

		PipelineState st = ctx.cache.fetch_pipeline(pipeline_def);

		cmd.bind_bindless(st.bind_point, st.layout, ctx.device.get_bindless());
		cmd.bind_pipeline(st.bind_point, st.pipeline);

		cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		this->mesh.bind_indices(cmd);
		this->mesh.draw_indexed(cmd);
	});

	RenderStage &hdr_to_skybox_mipmapping = graph.push_stage("HDR to Skybox (Mipmapping)", RenderStage::TYPE_TRANSFER);
	RenderResourceHandle cubemap_blit_dst_handle = hdr_to_skybox_mipmapping.blit_texture_dst(cubemap_handle);

	hdr_to_skybox_mipmapping.set_record([=](const RenderContext &ctx, const RenderStageResources &resources) -> void {
		CommandBuffer &cmd = ctx.cmd;
		cmd.generate_mipmaps(resources.get_texture(cubemap_blit_dst_handle));
	});
}

const Mesh &SkyboxRenderer::get_mesh() const
{
	return mesh;
}

const Texture *SkyboxRenderer::get_environment_map() const
{
	return cubemap;
}
