#include "render/pass/simple_passes.h"
#include "render/graph/graph.h"
#include "graphics/device.h"
#include "graphics/util.h"

void rnd::HdrToEnvPassFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const HdrToEnvPassData *user_data = static_cast<const HdrToEnvPassData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->hdr_to_cubemap_shader, *ctx.render_info);

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	struct {
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 hdr_image;
		u32 linear_sampler;
	} args;

	args.transform_matrix_buffer = device->BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer = device->BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.hdr_image = device->TextureViewBindless(user_data->hdr_view);
	args.linear_sampler = device->SamplerBindless(frame_params->linear_sampler);

	cmd.BindPipelineState(pipeline_st);
	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	frame_params->skybox_mesh->RecordBindIndexBuffer(cmd);
	frame_params->skybox_mesh->Draw(cmd);
}

void rnd::BRDFLutPassFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const BRDFLutPassData *user_data = static_cast<const BRDFLutPassData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::GraphicsPipelineDef pipeline_def(frame_params->brdf_lut_generation_shader, *ctx.render_info);

	gfx::PipelineState pipeline_st = gfx::Device::Get()->FetchGraphicsPipeline(pipeline_def);

	cmd.BindPipelineState(pipeline_st);
	cmd.Draw(3);
}

void rnd::IBLPassIrradianceFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const IBLPassIrradianceData *user_data = static_cast<const IBLPassIrradianceData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->irradiance_cubemap_gen_shader, *ctx.render_info);

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	cmd.BindPipelineState(pipeline_st);

	struct {
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
	} args;

	args.transform_matrix_buffer = device->BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer = device->BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.environment_map = device->TextureViewBindless(user_data->env_view);
	args.linear_sampler = device->SamplerBindless(frame_params->linear_sampler);

	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	frame_params->skybox_mesh->RecordBindIndexBuffer(cmd);
	frame_params->skybox_mesh->Draw(cmd);
}

void rnd::IBLPassPrefilterFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const IBLPassPrefilterData *user_data = static_cast<const IBLPassPrefilterData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->prefilter_cubemap_gen_shader);
	pipeline_def.multi_view_mask = 0b111111;
	pipeline_def.colour_attachment_count = 1;
	pipeline_def.colour_attachment_formats[0] = VK_FORMAT_R32G32B32A32_SFLOAT;

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	cmd.BindPipelineState(pipeline_st);

	struct {
		u64 transform_matrix_buffer;
		u64 vertex_buffer;
		u32 environment_map;
		u32 linear_sampler;
		float roughness;
	} args;

	args.transform_matrix_buffer = device->BufferAddress(frame_params->cubemap_capture_transform_buffer);
	args.vertex_buffer = device->BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.environment_map = device->TextureViewBindless(user_data->env_view);
	args.linear_sampler = device->SamplerBindless(frame_params->linear_sampler);
	args.roughness = user_data->roughness;

	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	frame_params->skybox_mesh->RecordBindIndexBuffer(cmd);
	frame_params->skybox_mesh->Draw(cmd);
}

void rnd::GenerateMipsPassFn(const PassContext &ctx)
{
	const GenerateMipsPassData *user_data = static_cast<const GenerateMipsPassData *>(ctx.user_data);
	ctx.cmd->GenerateMipmaps(user_data->texture);
}

void rnd::SkyboxPassFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const SkyboxPassData *user_data = static_cast<const SkyboxPassData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::GraphicsPipelineDef pipeline_def(frame_params->skybox_shader, *ctx.render_info);
	pipeline_def.depth_stencil_state.depth_test_enabled = true;
	pipeline_def.depth_stencil_state.depth_write_enabled = false;
	pipeline_def.depth_stencil_state.depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;

	gfx::PipelineState pipeline_st = device->FetchGraphicsPipeline(pipeline_def);

	struct {
		u64 frame_data_buffer;
		u64 vertex_buffer;
		u32 cubemap_texture;
		u32 linear_sampler;
	} args;

	args.frame_data_buffer = frame_params->frame_data.gpu;
	args.vertex_buffer = device->BufferAddress(frame_params->skybox_mesh->vertex_buffer);
	args.cubemap_texture = device->TextureViewBindless(user_data->cubemap);
	args.linear_sampler = device->SamplerBindless(frame_params->linear_sampler);

	cmd.BindPipelineState(pipeline_st);
	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_ALL_GRAPHICS, args, 0);

	frame_params->skybox_mesh->RecordBindIndexBuffer(cmd);
	frame_params->skybox_mesh->Draw(cmd);
}

void rnd::PostProcessingPassFn(const PassContext &ctx)
{
	const gfx::CmdBuffer &cmd = *ctx.cmd;
	const PostProcessingPassData *user_data = static_cast<const PostProcessingPassData *>(ctx.user_data);
	const FrameParams *frame_params = user_data->frame_params;

	gfx::Device *device = gfx::Device::Get();

	gfx::ComputePipelineDef pipeline_def(frame_params->tonemapping_shader);

	gfx::PipelineState pipeline_st = device->FetchComputePipeline(pipeline_def);

	struct {
		u32 width;
		u32 height;
		float exposure;
		u32 input_texture;
		u32 output_texture;
		u32 encode_srgb;
	} args;

	gfx::ResourceKey input_key = ctx.graph->ResolveTexture(user_data->input);
	gfx::ResourceKey output_key = ctx.graph->ResolveTexture(user_data->output);

	const gfx::Texture *input_texture = device->TextureFromKey(input_key);
	const gfx::Texture *output_texture = device->TextureFromKey(output_key);

	gfx::ResourceKey input_view = device->TextureViewAuto(input_key);
	gfx::ResourceKey output_view = device->TextureViewAuto(output_key);

	args.width = input_texture->width;
	args.height = input_texture->height;
	args.exposure = user_data->exposure;
	args.input_texture = device->TextureViewBindless(input_view);
	args.output_texture = device->TextureViewBindless(output_view);
	args.encode_srgb = user_data->encode_srgb ? 1 : 0;

	cmd.BindPipelineState(pipeline_st);
	cmd.PushConstants(pipeline_st.layout, VK_SHADER_STAGE_COMPUTE_BIT, args, 0);

	cmd.Dispatch(
		gfx::ComputeGroupCount(output_texture->width, 8),
		gfx::ComputeGroupCount(output_texture->height, 8),
		1
	);
}
