#include "skybox_renderer.h"

#include "assets/texture_serializer.h"
#include "assets/shader_serializer.h"

using namespace gfx;

struct FrameData {
	Mat4 view;
	Mat4 projection;
	Mat4 view_projection;
	Mat4 view_projection_no_translation;
	Mat4 inv_view;
	Mat4 inv_projection;
	Vec3 camera_position;
	Vec2 window_resolution;
	float time;
};

void SkyboxRenderer::init(Device *device, ast::AssetManager &assets, RenderGraph &graph)
{
	this->device = device;

	frame_data_buffer = device->alloc_gpu_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(FrameData)
	);

	Mat4 capture_view_matrices[] = {
		Mat4::lookat(Vec3::zero(), Vec3( 1.f,  0.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Right
		Mat4::lookat(Vec3::zero(), Vec3(-1.f,  0.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Left
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  0.f,  1.f), Vec3(0.f, -1.f, 0.f)), // Up
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  0.f, -1.f), Vec3(0.f,  1.f, 0.f)), // Down
		Mat4::lookat(Vec3::zero(), Vec3( 0.f,  1.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Forward
		Mat4::lookat(Vec3::zero(), Vec3( 0.f, -1.f,  0.f), Vec3(0.f,  0.f, 1.f)), // Backwards
	};

	Mat4 capture_projection_matrix = Mat4::perspective(90.f, 1.f, 0.1f, 10.f);

	for (int i = 0; i < 6; i++) {
		capture_view_matrices[i] = capture_projection_matrix * capture_view_matrices[i];
	}

	cubemap_capture_transforms = device->alloc_gpu_buffer(
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(Mat4) * 6
	);

	cubemap_capture_transforms.write(capture_view_matrices, sizeof(capture_view_matrices), 0);

	linear_sampler = device->create_sampler(VK_FILTER_LINEAR);

	ast::TextureAsset *hdr_texture_asset = assets.get_asset<ast::TextureAsset>(assets.from_file_path("environment_map.hdr", ast::ASSET_TYPE_TEXTURE));
	hdr_texture = hdr_texture_asset->texture;

	ast::ShaderAsset *shader_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("skybox", ast::ASSET_TYPE_SHADER));
	shader = shader_asset->shader;

	ast::ShaderAsset *shader_hdr_cubemap_asset = assets.get_asset<ast::ShaderAsset>(assets.from_file_path("hdr_to_environment_cubemap", ast::ASSET_TYPE_SHADER));
	hdr_to_cubemap_shader = shader_hdr_cubemap_asset->shader;

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

	u16 indices[] = {
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

	AttachmentInfo environment_cubemap_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	environment_cubemap_info.size_class = AttachmentInfo::SIZE_CLASS_ABSOLUTE;
	environment_cubemap_info.size_x = 1024.f;
	environment_cubemap_info.size_y = 1024.f;
	environment_cubemap_info.layers = 6;
	environment_cubemap_info.is_cubemap = true;

	cubemap_attachment = graph.create_texture_resource(environment_cubemap_info);

	gfx::AttachmentInfo output_attachment_info(VK_FORMAT_R32G32B32A32_SFLOAT);
	output_attachment_info.size_class = gfx::AttachmentInfo::SIZE_CLASS_SWAPCHAIN_RELATIVE;
	output_attachment_info.size_x = 1.f;
	output_attachment_info.size_y = 1.f;

	output_attachment = graph.create_texture_resource(output_attachment_info);
}

void SkyboxRenderer::destroy()
{
	mesh.destroy();

	device->destroy_sampler(linear_sampler);
	device->destroy_gpu_buffer(cubemap_capture_transforms);
	device->destroy_gpu_buffer(frame_data_buffer);
}

void SkyboxRenderer::add_render_stages(
	RenderGraph &graph, RenderGraphBlackboard &bb,
	const SceneView &view
)
{
	if (!created_cubemap) {
		add_create_cubemap_stage(graph);
		created_cubemap = true;
	}

	RenderClear clear(0.f, 1.f, 0.f, 1.f);

	RenderStage &stage = graph.add_stage(RenderStage::TYPE_GRAPHICS);
	stage.set_scene_view(view);
	stage.add_colour_output(output_attachment, &clear);
	stage.add_texture(cubemap_attachment, sync::TEXTURE_ACCESS_graphics_r);

	stage.set_record([&](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;

		const Camera &camera = *ctx.view.camera;

		FrameData data = {};
		data.view = camera.get_view();
		data.projection = camera.get_projection();
		data.view_projection = camera.get_projection() * camera.get_view();
		data.view_projection_no_translation = camera.get_projection() * camera.get_view().remove_translation();
		data.inv_view = data.view.inverse();
		data.inv_projection = data.projection.inverse();
		data.camera_position = camera.get_position();
		data.window_resolution = Vec2(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
		data.time = ctx.elapsed_time;
		this->frame_data_buffer.write(&data, sizeof(FrameData), 0);

		GraphicsPipelineDef pipeline_def(shader);
//		pipeline_def.has_depth_attachment = true;
//		pipeline_def.depth_stencil_state.depth_test_enabled = true;
//		pipeline_def.depth_stencil_state.depth_write_enabled = false;
//		pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
		pipeline_def.colour_attachment_formats.push_back(graph.get_physical_texture(graph.get_resource(output_attachment)).get_format());

		PipelineState st = ctx.device.fetch_pipeline(pipeline_def);

		struct {
			u64 frame_data_buffer;
			u64 vertex_buffer;
			u32 cubemap_id;
			u32 sampler_id;
		} args;

		TextureView cubemap_view = graph.get_physical_texture_view(graph.get_resource(cubemap_attachment));

		args.frame_data_buffer = this->frame_data_buffer.get_device_address();
		args.vertex_buffer = this->mesh.vertex_buffer.get_device_address();
		args.cubemap_id = cubemap_view.get_bindless().sampled;
		args.sampler_id = this->linear_sampler.get_bindless().sampler;

		cmd.bind_bindless(st.bind_point, st.layout, graph.get_device().get_bindless());
		cmd.bind_pipeline(st.bind_point, st.pipeline);

		cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		this->mesh.bind_indices(cmd);
		this->mesh.draw_indexed(cmd);
	});
}

void SkyboxRenderer::add_create_cubemap_stage(RenderGraph &graph)
{
	RenderStage &stage = graph.add_stage(RenderStage::TYPE_GRAPHICS);

	stage.set_multi_view_mask(0b111111);

	stage.add_colour_output(cubemap_attachment);

	stage.set_record([&](const RenderContext &ctx) -> void {
		CommandBuffer &cmd = ctx.cmd;

		struct {
			u64 transform_matrix_buffer;
			u64 vertex_buffer;
			u32 hdr_image_id;
			u32 linear_sampler_id;
		} args;

		args.transform_matrix_buffer = this->cubemap_capture_transforms.get_device_address();
		args.vertex_buffer = this->mesh.vertex_buffer.get_device_address();
		args.hdr_image_id = this->device->fetch_texture_view_std(this->hdr_texture).get_bindless().sampled;
		args.linear_sampler_id = this->linear_sampler.get_bindless().sampler;

		GraphicsPipelineDef pipeline_def(hdr_to_cubemap_shader);
		pipeline_def.depth_stencil_state.depth_test_enabled = false;
		pipeline_def.depth_stencil_state.depth_write_enabled = false;
		pipeline_def.colour_attachment_formats.push_back(VK_FORMAT_R32G32B32A32_SFLOAT);
		pipeline_def.view_mask = 0b111111;

		PipelineState st = device->fetch_pipeline(pipeline_def);

		cmd.bind_bindless(st.bind_point, st.layout, device->get_bindless());
		cmd.bind_pipeline(st.bind_point, st.pipeline);

		cmd.push_constants(st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(args), &args);

		this->mesh.bind_indices(cmd);
		this->mesh.draw_indexed(cmd);
	});
}
