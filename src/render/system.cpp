#include "render/system.h"
#include "render/blackboard.h"
#include "render/graph/graph.h"
#include "render/pass/culling_pass.h"
#include "render/pass/forward_pass.h"
#include "render/pass/simple_passes.h"
#include "graphics/device.h"
#include "asset/asset_system.h"
#include "os/job.h"

static rnd::System *selected_system = nullptr;

rnd::System::System(LogChannel log_channel_)
	: log_channel(log_channel_)
{
	gfx::Device *device = gfx::Device::Get();
	ast::AssetSystem *assets = ast::AssetSystem::Get();

	gfx::BufferAllocInfo ring_buffer_alloc_info = {};
	ring_buffer_alloc_info.size = RENDER_FRAME_UPLOAD_BUDGET * GRAPHICS_FRAMES_IN_FLIGHT;
	ring_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ring_buffer_alloc_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	upload_ring = gfx::RingBuffer(ring_buffer_alloc_info);

	for (int i = 0; i < GRAPHICS_FRAMES_IN_FLIGHT; i++)
		upload_slices[i] = upload_ring.SliceView(i * RENDER_FRAME_UPLOAD_BUDGET, RENDER_FRAME_UPLOAD_BUDGET);

	Mat4 capture_view_matrices[] = {
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f)),  // +x :: right
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f)), // -x :: left
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, 1.f), Vec3(0.f, -1.f, 0.f)), // +z :: up
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 0.f, -1.f), Vec3(0.f, 1.f, 0.f)), // -z :: down
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f), Vec3(0.f, 0.f, 1.f)),  // +y :: forward
		Mat4::LookAt(Vec3(0.f, 0.f, 0.f), Vec3(0.f, -1.f, 0.f), Vec3(0.f, 0.f, 1.f)), // -y :: backward
	};

	Mat4 capture_projection_matrix = Mat4::Perspective(90.f, 1.f, 0.1f, 10.f);

	for (int i = 0; i < 6; i++)
		capture_view_matrices[i] = capture_projection_matrix * capture_view_matrices[i];

	gfx::BufferAllocInfo cubemap_capture_buffer_alloc_info = {};
	cubemap_capture_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	cubemap_capture_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	cubemap_capture_buffer_alloc_info.size = sizeof(capture_view_matrices);

	cubemap_capture_transform_buffer = device->BufferAlloc(cubemap_capture_buffer_alloc_info);

	device->BufferWrite(
		cubemap_capture_transform_buffer,
		capture_view_matrices,
		sizeof(capture_view_matrices), 0
	);

	// Init shaders.
	{
		job::Counter counter(0);

		shaders.debug_line_handle             = assets->RequireAsset("engine://shaders/passes/debug/debug_line.slang", &counter);
		shaders.depth_prepass_handle          = assets->RequireAsset("engine://shaders/passes/forward/depth_prepass.slang", &counter);
		shaders.forward_handle                = assets->RequireAsset("engine://shaders/passes/forward/forward.slang", &counter);
		shaders.shadow_handle                 = assets->RequireAsset("engine://shaders/passes/shadow/shadow_mapping.slang", &counter);
		shaders.shadow_masked_handle          = assets->RequireAsset("engine://shaders/passes/shadow/shadow_mapping_masked.slang", &counter);
		shaders.shadow_cull_handle            = assets->RequireAsset("engine://shaders/passes/shadow/shadow_culling.slang", &counter);
		shaders.cull_frustum_handle           = assets->RequireAsset("engine://shaders/passes/culling/frustum_culling.slang", &counter);
		shaders.skybox_handle                 = assets->RequireAsset("engine://shaders/passes/post/skybox.slang", &counter);
		shaders.tonemapping_handle            = assets->RequireAsset("engine://shaders/passes/post/hdr_tonemapping.slang", &counter);
		shaders.brdf_lut_generation_handle    = assets->RequireAsset("engine://shaders/passes/ibl/brdf_lut.slang", &counter);
		shaders.hdr_to_cubemap_handle         = assets->RequireAsset("engine://shaders/passes/ibl/hdr_to_environment_cubemap.slang", &counter);
		shaders.irradiance_cubemap_gen_handle = assets->RequireAsset("engine://shaders/passes/ibl/irradiance_convolution.slang", &counter);
		shaders.prefilter_cubemap_gen_handle  = assets->RequireAsset("engine://shaders/passes/ibl/prefilter_convolution.slang", &counter);

		assets->WaitForLoad(&counter);
	}

	// Init samplers.
	{
		samplers.linear = device->SamplerCreateF(VK_FILTER_LINEAR);
		samplers.nearest = device->SamplerCreateF(VK_FILTER_NEAREST);

		gfx::SamplerCreateInfo material_info;
		material_info.filter = VK_FILTER_LINEAR;
		material_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		material_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		material_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		material_info.anisotropy = true;
		samplers.material = device->SamplerCreate(material_info);

		bool linear_compare_supported = device->FormatSupports(RENDER_SHADOW_MAP_FORMAT, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

		gfx::SamplerCreateInfo shadow_info;
		shadow_info.filter = linear_compare_supported ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
		shadow_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadow_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadow_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		shadow_info.anisotropy = false;
		shadow_info.max_lod = 0.f;
		shadow_info.compare = true;
		shadow_info.compare_op = RENDER_SHADOW_COMPARE_OP;
		samplers.shadow_compare = device->SamplerCreate(shadow_info);

		if (!linear_compare_supported)
			DebugLogW(log_channel, "Shadow map format doesn't support linear filtering, so we're going to fall back to the point-sampled PCF!");
	}

	// Init skybox mesh.
	{
		static const Vec3 vertices[] = {
			Vec3(-1.f, 1.f, 1.f),
			Vec3(-1.f, 1.f, -1.f),
			Vec3(1.f, 1.f, -1.f),
			Vec3(1.f, 1.f, 1.f),
			Vec3(-1.f, -1.f, 1.f),
			Vec3(-1.f, -1.f, -1.f),
			Vec3(1.f, -1.f, -1.f),
			Vec3(1.f, -1.f, 1.f)
		};

		static const unsigned short indices[] = {
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

		skybox_mesh.Alloc(sizeof(Vec3), gfx::IndexType_U16, ArraySize(vertices), ArraySize(indices));
		skybox_mesh.UploadBlocking(vertices, indices);
	}

	// IBL
	{
		brdf_lut = device->TextureAlloc2D(512, 512, VK_FORMAT_R32G32_SFLOAT, 1);

		environment_cubemap = device->TextureAllocCubemap(512, VK_FORMAT_R32G32B32A32_SFLOAT, 8);
		irradiance_cubemap = device->TextureAllocCubemap(32, VK_FORMAT_R32G32B32A32_SFLOAT, 1);
		prefilter_cubemap = device->TextureAllocCubemap(128, VK_FORMAT_R32G32B32A32_SFLOAT, RENDER_SYSTEM_PREFILTER_MIPS);

		environment_hdr_handle = assets->RequireAssetBlocking("assets://environment_map_1.hdr");
	}

	Select(this);

	DebugLogI(log_channel, "Initialized.");
}

rnd::System::~System()
{
	gfx::Device *device = gfx::Device::Get();

	device->TextureDestroy(brdf_lut);
	device->TextureDestroy(environment_cubemap);
	device->TextureDestroy(irradiance_cubemap);
	device->TextureDestroy(prefilter_cubemap);

	skybox_mesh.Destroy();

	device->SamplerDestroy(samplers.linear);
	device->SamplerDestroy(samplers.nearest);
	device->SamplerDestroy(samplers.material);
	device->SamplerDestroy(samplers.shadow_compare);

	device->BufferDestroy(cubemap_capture_transform_buffer);

	upload_ring.Destroy();

	DebugLogI(log_channel, "Destroyed.");

	if (selected_system == this)
		selected_system = nullptr;
}

rnd::System *rnd::System::Get()
{
	return selected_system;
}

void rnd::System::Select(System *system)
{
	selected_system = system;

	if (system)
		DebugRenderer::Select(&system->debug_renderer);
}

gfx::RingBuffer *rnd::System::BeginFrameUploads(unsigned frame_slot_index)
{
	gfx::RingBuffer *slice = &upload_slices[frame_slot_index];
	slice->Reset();
	return slice;
}

void rnd::System::BakeLookupsAndMaps(Graph *graph, const gfx::Swapchain *swapchain)
{
	gfx::Device *device = gfx::Device::Get();

	FrameParams params;
	params.BuildShared(this);

	if (!params.ResolveShaders(this)) {
		DebugLogE(log_channel, "IBL bake skipped: Shaders not loaded.");
		return;
	}

	GenerateLookupsAndMaps(graph, &params);
	graph->Compile(swapchain);

	gfx::CmdBuffer cmd = device->SubmitImBegin();
	graph->Execute(cmd);
	device->SubmitImEnd(cmd);

	graph->Reset();

	DebugLogI(log_channel, "Baked lookups and maps.");
}

void rnd::System::GenerateLookupsAndMaps(Graph *graph, const FrameParams *frame_params)
{
	const unsigned prefilter_mips = RENDER_SYSTEM_PREFILTER_MIPS;

	gfx::Device *device = gfx::Device::Get();

	gfx::ResourceKey hdr_texture_gfx = ast::AssetSystem::Get()->GetOrBreak(environment_hdr_handle)->texture.key;

	// Generate BRDF Lookup Table.
	{
		BRDFLutPassData *data = graph->NewPassData<BRDFLutPassData>();
		data->frame_params = frame_params;

		Pass *pass = graph->Add("BRDF LUT", PassType_Graphics);
		pass->SetRecord(BRDFLutPassFn, data);
		pass->WriteColour(graph->ImportTexture(brdf_lut), nullptr);
	}

	// Generate Environment Cubemap.
	{
		HdrToEnvPassData *data = graph->NewPassData<HdrToEnvPassData>();
		data->frame_params = frame_params;
		data->hdr_view = device->TextureViewAuto(hdr_texture_gfx);

		Pass *pass = graph->Add("HDR -> Environment Map", PassType_Graphics);
		pass->SetRecord(HdrToEnvPassFn, data);
		pass->SetMultiViewMask(0b111111);
		pass->WriteColour(graph->ImportTexture(environment_cubemap), nullptr);

		GenerateMipsPassData *mips_data = graph->NewPassData<GenerateMipsPassData>();
		mips_data->texture = environment_cubemap;

		Pass *pass_mipmaps = graph->Add("Environment Map Mipmapping", PassType_Transfer);
		pass_mipmaps->SetRecord(GenerateMipsPassFn, mips_data);
		pass_mipmaps->BlitTextureDst(graph->ImportTexture(environment_cubemap));
	}

	// Irradiance.
	{
		IBLPassIrradianceData *data = graph->NewPassData<IBLPassIrradianceData>();
		data->frame_params = frame_params;
		data->env_view = device->TextureViewAuto(environment_cubemap);

		Pass *pass = graph->Add("Irradiance", PassType_Graphics);
		pass->SetRecord(IBLPassIrradianceFn, data);
		pass->SetMultiViewMask(0b111111);
		pass->WriteColour(graph->ImportTexture(irradiance_cubemap), nullptr);
	}

	// Prefilter.
	{
		const int mipmap_count = prefilter_mips;

		for (int i = 0; i < mipmap_count; i++) {
			IBLPassPrefilterData *data = graph->NewPassData<IBLPassPrefilterData>();
			data->frame_params = frame_params;
			data->env_view = device->TextureViewAuto(environment_cubemap);
			data->roughness = (float)i / (float)(mipmap_count - 1);

			gfx::SubresourceRange range = {};
			range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
			range.base_mip = i;
			range.mips = 1;
			range.base_layer = 0;
			range.layers = 6;

			Pass *pass = graph->Add("Prefilter", PassType_Graphics);
			pass->SetRecord(IBLPassPrefilterFn, data);
			pass->SetMultiViewMask(0b111111);
			pass->WriteColourEx(graph->ImportTexture(prefilter_cubemap), nullptr, range);
		}
	}

	DebugLogI(log_channel, "Generated lookups and maps.");
}

/*
 * 1. CLEAR
 * 2. SHADOWS
 * 3. FRUSTUM CULL
 * 4. FORWARD - OPAQUE
 * 5. SKYBOX
 * 6. FORWARD - TRANSLUCENT
 * 7. POST PROCESSING
 * 8. DEBUG LINES
 * 9. PRESENT / BLIT
 */
void rnd::System::Render(Graph *graph, const FrameParams *frame_params)
{
	gfx::Device *device = gfx::Device::Get();

	FrustumVolume frustum = frame_params->camera.Frustum();

	Blackboard bb;

	Clear colour_clear = Clear::Colour(0.f, 0.f, 0.f, 1.f);
	Clear depth_clear = Clear::DepthStencil(1.f, 0);

	TextureInfo lighting_info = TextureInfo::Swapchain(VK_FORMAT_R16G16B16A16_SFLOAT, Vec3(1.f));
	lighting_info.flags = gfx::TextureAllocFlag_Storage;
	lighting_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.lighting_resolve = graph->CreateTexture(lighting_info);
	lighting_info.flags = gfx::TextureAllocFlag_None;
	lighting_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.lighting_msaa = graph->CreateTexture(lighting_info);

	TextureInfo depth_info = TextureInfo::Swapchain(device->GetDefaultDepthStencilFormat(), Vec3(1.f));
	depth_info.samples = VK_SAMPLE_COUNT_1_BIT;
	bb.depth_resolve = graph->CreateTexture(depth_info);
	depth_info.samples = VK_SAMPLE_COUNT_4_BIT;
	bb.depth_msaa = graph->CreateTexture(depth_info);

	// todo: fold the clear into the first pass that writes each target.
	Pass *clear_pass = graph->Add("Clear", PassType_Graphics);
	bb.lighting_msaa = clear_pass->WriteColour(bb.lighting_msaa, &colour_clear);
	bb.depth_msaa = clear_pass->WriteDepth(bb.depth_msaa, &depth_clear);

	// shadows + forward opaque
	if (frame_params->object_count > 0) {
		shadow_renderer.Render(graph, frame_params, &bb);
		DrawStream draw_stream = CullFrustum(graph, frame_params, CullFilter_OpaqueOnly, frustum);
		DepthPrepass(graph, frame_params, &bb, draw_stream);
		ForwardRender(graph, frame_params, &bb, draw_stream);
	}

	// skybox
	{
		SkyboxPassData *data = graph->NewPassData<SkyboxPassData>();
		data->frame_params = frame_params;
		data->cubemap = device->TextureViewAuto(environment_cubemap);

		Pass *skybox_pass = graph->Add("Skybox", PassType_Graphics);
		bb.lighting_msaa = skybox_pass->WriteColour(bb.lighting_msaa, nullptr);
		bb.depth_msaa = skybox_pass->WriteDepth(bb.depth_msaa, nullptr);
		skybox_pass->ReadTextureGraphics(graph->ImportTexture(environment_cubemap));
		skybox_pass->SetRecord(SkyboxPassFn, data);
	}

	// forward translucent + MSAA resolve
	ForwardRenderTranslucent(graph, frame_params, &bb);

	// post processing
	{
		PostProcessingPassData *data = graph->NewPassData<PostProcessingPassData>();
		data->frame_params = frame_params;
		data->exposure = exposure;
		data->input = bb.lighting_resolve;
		data->output = bb.lighting_resolve;
		data->encode_srgb = encode_rgb_output;

		Pass *pp_pass = graph->Add("Post Processing", PassType_Compute);
		bb.lighting_resolve = pp_pass->WriteTextureCompute(bb.lighting_resolve);
		pp_pass->ReadTextureCompute(bb.lighting_resolve);
		pp_pass->SetRecord(PostProcessingPassFn, data);
	}

	debug_renderer.Render(graph, frame_params, bb.lighting_resolve, bb.depth_resolve);

	graph->SetBackbuffer(bb.lighting_resolve);
	graph->SetPresentFilter(VK_FILTER_NEAREST);
}
