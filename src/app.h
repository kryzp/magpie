#ifndef APP_H
#define APP_H

#ifdef _WIN32
# define APP_API __declspec(dllexport)
#else
# define APP_API __attribute__((visibility("default")))
#endif

#include "core/core.h"
#include "core/core_memory_arena.h"
#include "core/core_types.h"

#include "platform/platform.h"

#include "rendering/graphics.h"
#include "rendering/rendering.h"
#include "rendering/camera.h"
#include "rendering/render_graph.h"
#include "rendering/model.h"
#include "rendering/shaders.h"
#include "rendering/gbuffer.h"

#include "assets/assets.h"

#include "timer.h"

struct camera_driver {
	bool active;
	float yaw;
	float pitch;
	float target_yaw;
	float target_pitch;
};

struct app {
	struct memory_arena permanent_arena;
	struct memory_arena frame_arena;
	struct memory_arena scene_arena;

	struct core core;

	struct gfx_device graphics_device;
	struct gfx_swapchain swapchain;
	struct gfx_render_graph render_graph;
	struct gfx_shaders shaders;

	struct gfx_buffer frame_data_buffer;
	struct gfx_gbuffer gbuffer;
	struct gfx_texture lighting_attachment;
	struct gfx_texture brdf_lut_texture;
	struct gfx_buffer cubemap_capture_transforms;
	struct gfx_sampler linear_sampler;
	struct gfx_mesh light_sphere_mesh;
	struct gfx_mesh skybox_mesh;
	struct gfx_texture skybox_cubemap;
	struct gfx_environment_probe environment_probe;
	struct gfx_render_scene render_scene;

	u32 damaged_helmet_objects[16][16];
	u32 light;
	
	struct asset_store assets;

	struct timer global_timer;
	struct timer delta_timer;
	float delta_accumulator;

	struct gfx_camera main_camera;
	struct camera_driver main_camera_driver;
};

APP_API void app_init(struct platform *platform_);
APP_API bool app_tick(struct platform *platform_);
APP_API void app_destroy(struct platform *platform_);
APP_API void app_before_hot_reload(struct platform *platform_);
APP_API void app_after_hot_reload(struct platform *platform_);

extern struct app *app;
extern struct platform *platform;

#endif // APP_H

#if 0
	EnvironmentMapFromHDR(&core->render_graph, &core->skybox_cubemap, AssetsImageFromHandle(&core->assets, environment_asset_handle));
	BRDFLookUpGenerate(&core->render_graph, &core->brdf_lut_image);
	IBLRendererGenerateEnvironmentProbe(&core->render_graph, &ibl_renderer_input);

// TODO: This is kind of a placeholder.
//       At least, the code is pretty bad.
//       --> Shouldn't be calling GPUBufferWrite() for every
//           object individually, at the very least.
internal void CoreFrameDataUploadObjects(CoreFrameData *frame, RenderState *rs, Scene *scene)
{
	u32 mesh_count = 0;
	u32 light_count = 0;
	
	for (SceneObject *object = scene->objects; object; object = object->next) {

		// If the object has a mesh.
		if (object->mesh_id != SCENE_INVALID_HANDLE) {
			GPU_ObjectData object_data = {0};
			object_data.model_matrix = object->transform;
			object_data.normal_matrix = M4Inverse(M4Transpose(object->transform));

			GPUBufferWrite(&frame->object_buffer,
				       &object_data,
				       sizeof(GPU_ObjectData),
				       sizeof(GPU_ObjectData) * mesh_count);

			mesh_count++;
		}

		// If the object has a light.
		if (object->light_id != SCENE_INVALID_HANDLE) {
			Light *light = &rs->lights[object->light_id];
			
			float epsilon_intensity = .1f;
			float light_max = V3MaxValue(light->colour);
			float heuristic_radius = SquareRoot((light->intensity * light_max) / (light->falloff * epsilon_intensity));

			GPU_Light gpu_light = {0};
			gpu_light.position     = object->transform.c[3]; // Last column of transformation matrix is translation.
			gpu_light.colour.xyz   = light->colour;
			gpu_light.colour.w     = light->intensity;
			gpu_light.attenuation  = v4(light->falloff, 0.f, 0.f, 0.f);
			gpu_light.transform    = M4Transform(gpu_light.position.xyz,
							     QuatInitIdentity(),
							     v3u(heuristic_radius),
							     v3u(0.f));

			GPUBufferWrite(&frame->light_buffer,
				       &gpu_light,
				       sizeof(GPU_Light),
				       sizeof(GPU_Light) * light_count);

			light_count++;
		}
	}
}

internal void CoreRender()
{
	Scene *scene = &core->scene;
	RenderState *rs = &core->render_state;
	CoreFrameData *frame = CoreCurrentFrame();
	
	rs->cmd = GraphicsBeginPresent();

	CoreFrameDataUploadPerFrameBuffer(frame, &core->main_camera);
	CoreFrameDataUploadObjects(frame, rs, scene);

	RenderStateMergeMeshes(rs);

	// TODO: Only bother recomputing mesh_pass (and its derivatives)
	//       when the scene actually changes.
	MeshPass mesh_pass = {0};
	MeshPassPopulate(&mesh_pass, &core->frame_arena, rs, scene);

	if (core->instance_buffer_dirty) {
		GPU_Instance *instances_array = GPUBufferData(&core->instance_buffer);
		RenderStateFillInstancesArray(rs, &mesh_pass, instances_array);
		core->instance_buffer_dirty = false;
	}
	
	if (core->indirect_buffer_dirty) {
		GPU_Indirect *indirect_array = GPUBufferData(&core->clear_indirect_buffer);
		RenderStateFillIndirectArray(rs, &mesh_pass, indirect_array);
		core->indirect_buffer_dirty = false;
	}
	
	// ---

	struct clear_draw_indirect_buffer_pass_context clear_draw_pass_context = {0};
	clear_draw_pass_context.mesh_pass = &mesh_pass;
	
	RenderPass clear_draw_indirect_buffer_pass = {0};
	clear_draw_indirect_buffer_pass.type = RenderPassType_Transfer;
	clear_draw_indirect_buffer_pass.transfer.Record = CoreClearDrawIndirectBuffer;
	clear_draw_indirect_buffer_pass.transfer.src_buffer_copy_count = 1;
	clear_draw_indirect_buffer_pass.transfer.src_buffer_copies[0] = &core->clear_indirect_buffer;
	clear_draw_indirect_buffer_pass.transfer.dst_buffer_copy_count = 1;
	clear_draw_indirect_buffer_pass.transfer.dst_buffer_copies[0] = &core->draw_indirect_buffer;

	MemoryCopy(clear_draw_indirect_buffer_pass.context, &clear_draw_pass_context, sizeof(clear_draw_pass_context));

	RenderGraphPush(&core->render_graph, &clear_draw_indirect_buffer_pass);

	struct frustum_culling_input frustum_culling_input = {0};
	frustum_culling_input.camera          = &core->main_camera;
	frustum_culling_input.mesh_pass       = &mesh_pass;
	frustum_culling_input.instance_buffer = &core->instance_buffer;
	frustum_culling_input.indirect_buffer = &core->draw_indirect_buffer;
	frustum_culling_input.output_buffer   = &core->compacted_instance_buffer;
	
	ComputeFrustumCulling(&core->render_graph, &frustum_culling_input);
	
	struct deferred_renderer_input deferred_renderer_input = {0};
	deferred_renderer_input.scene             = &core->scene;
	deferred_renderer_input.camera            = &core->main_camera;
	deferred_renderer_input.probe             = &core->environment_probe;
	deferred_renderer_input.mesh_pass         = &mesh_pass;
	deferred_renderer_input.frame_data_buffer = &frame->frame_data_buffer;
	deferred_renderer_input.object_buffer     = &frame->object_buffer;
	deferred_renderer_input.instance_buffer   = &core->compacted_instance_buffer;
        deferred_renderer_input.indirect_buffer   = &core->draw_indirect_buffer;
	deferred_renderer_input.light_buffer      = &frame->light_buffer;
	deferred_renderer_input.gbuffer           = &core->gbuffer;
	deferred_renderer_input.lighting          = FetchStandardImageView(&core->lighting_attachment);
	
	DeferredRenderFrame(&core->render_graph, &deferred_renderer_input);
	
	struct skybox_renderer_input skybox_renderer_input = {0};
	skybox_renderer_input.skybox            = FetchStandardImageView(&core->skybox_cubemap);
	skybox_renderer_input.frame_data_buffer = &frame->frame_data_buffer;
	skybox_renderer_input.target            = FetchStandardImageView(&core->lighting_attachment);
	skybox_renderer_input.depth             = FetchStandardImageView(&core->gbuffer.depth);
	
	SkyboxRender(&core->render_graph, &skybox_renderer_input);

	struct post_processing_input post_processing_input = {0};
	post_processing_input.exposure = 1.15f;
	post_processing_input.input = FetchStandardImageView(&core->lighting_attachment);
	post_processing_input.output = FetchStandardImageView(&core->lighting_attachment);

	PostProcessingPass(&core->render_graph, &post_processing_input);
	
	// ---

	RenderPass lighting_to_swapchain_pass = {0};
	lighting_to_swapchain_pass.type = RenderPassType_Transfer;
	lighting_to_swapchain_pass.transfer.Record = CoreLightingAttachmentBlitToSwapchain;
	lighting_to_swapchain_pass.transfer.src_view_blit_count = 1;
	lighting_to_swapchain_pass.transfer.src_view_blits[0] = FetchStandardImageView(&core->lighting_attachment);
	lighting_to_swapchain_pass.transfer.dst_view_blit_count = 1;
	lighting_to_swapchain_pass.transfer.dst_view_blits[0] = SwapchainCurrentImageView(&graphics_device->swapchain);
	
	RenderGraphPush(&core->render_graph, &lighting_to_swapchain_pass);
	
	RenderPass present_pass = {0};
	present_pass.type = RenderPassType_Present;
	present_pass.present.swapchain = SwapchainCurrentImage(&graphics_device->swapchain);

	RenderGraphPush(&core->render_graph, &present_pass);
	
	// ---
	
	RenderGraphExecute(&core->render_graph, &core->render_state, &core->frame_arena);
	GraphicsEndPresent(&core->render_state.cmd);
}

#endif // 0
