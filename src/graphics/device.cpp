#include "device.h"

#include "ext/spirv/spirv_reflect.h"

#include "ext/imgui/imgui.h"
#include "ext/imgui/imgui_impl_vulkan.h"

#include "core/scratch.h"
#include "platform/platform.h"
#include "math/calc.h"

#include "vk_check.h"

using namespace gfx;

static VkSurfaceFormatKHR _choose_swapchain_surface_format(const Vector<VkSurfaceFormatKHR> &available_surface_formats)
{
	for (auto &format : available_surface_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
		    format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			debug_log("Found desired swapchain swap surface format and colour space.");
			return format;
		}
	}

	debug_log("Could not find desired swapchain swap surface format and colour space, falling back...");

	return available_surface_formats[0];
}

static VkPresentModeKHR _choose_swapchain_present_mode(const Vector<VkPresentModeKHR> &available_present_modes, bool enable_vsync)
{
	if (!enable_vsync)
		return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for (auto &mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D _choose_swapchain_extent(const VkSurfaceCapabilitiesKHR *capabilities)
{
	if (capabilities->currentExtent.width != -1u &&
	    capabilities->currentExtent.height != -1u)
		return capabilities->currentExtent;

	int window_width, window_height;
	platform::get_window_size(&window_width, &window_height);

	VkExtent2D actual_extent = {
		(u32)window_width,
		(u32)window_height
	};

	actual_extent.width = CalcU::clamp(
		actual_extent.width,
		capabilities->minImageExtent.width,
		capabilities->maxImageExtent.width
	);

	actual_extent.height = CalcU::clamp(
		actual_extent.height,
		capabilities->minImageExtent.height,
		capabilities->maxImageExtent.height
	);

	return actual_extent;
}

Device::Device()
	: context()
	, current_frame_index(0)
	, pipeline_process_cache(VK_NULL_HANDLE)
	, graphics_timeline_semaphore(VK_NULL_HANDLE)
	, graphics_timeline_value(0)
	, bindless()
	, imgui_pool(VK_NULL_HANDLE)
{
}

Device::~Device()
{
}

void Device::init()
{
	context.init();

	VkPipelineCacheCreateInfo pipeline_cache_create_info = {};
	pipeline_cache_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipeline_cache_create_info.pNext = nullptr;
	pipeline_cache_create_info.flags = 0;
	pipeline_cache_create_info.initialDataSize = 0;
	pipeline_cache_create_info.pInitialData = nullptr;

	GFX_VK_CHECK(
		vkCreatePipelineCache(
			context.get_device(),
			&pipeline_cache_create_info, nullptr,
			&pipeline_process_cache
		),
		"Failed to process pipeline cache."
	);

	debug_log("Created graphics pipeline process cache.");

	create_sync_resources();
	create_bindless();
	init_imgui();
}

void Device::destroy()
{
	destroy_sync_resources();
	destroy_bindless();
	destroy_imgui();

	vkDestroyPipelineCache(context.get_device(), pipeline_process_cache, nullptr);

	context.destroy();
}

void Device::wait_idle()
{
	vkDeviceWaitIdle(context.get_device());
}

void Device::wait_for_fence(VkFence fence) const
{
	vkWaitForFences(context.get_device(), 1, &fence, VK_TRUE, UINT64_MAX);
}

void Device::reset_fence(VkFence fence) const
{
	vkResetFences(context.get_device(), 1, &fence);
}

void Device::destroy_fence(VkFence fence) const
{
	vkDestroyFence(context.get_device(), fence, nullptr);
}

VkSemaphore Device::create_timeline_semaphore(u64 initial_value) const
{
	VkSemaphoreTypeCreateInfo timeline_type_create_info = {};
	timeline_type_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	timeline_type_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timeline_type_create_info.initialValue = initial_value;

	VkSemaphoreCreateInfo timeline_semaphore_create_info = {};
	timeline_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	timeline_semaphore_create_info.flags = 0;
	timeline_semaphore_create_info.pNext = &timeline_type_create_info;
		
	VkSemaphore semaphore = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateSemaphore(
			context.get_device(),
			&timeline_semaphore_create_info, nullptr,
			&semaphore
		),
		"Failed to create timeline semaphore."
	);

	return semaphore;
}

void Device::wait_for_timeline_semaphore(VkSemaphore semaphore, u64 value) const
{
	VkSemaphoreWaitInfo wait_info = {};
	wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores = &semaphore;
	wait_info.pValues = &value;

	GFX_VK_CHECK(
		vkWaitSemaphores(
			context.get_device(),
			&wait_info, UINT64_MAX
		),
		"Failed to wait on timeline semaphore"
	);
}

u64 Device::get_timeline_semaphore_value(VkSemaphore semaphore) const
{
	u64 result = 0;

	vkGetSemaphoreCounterValue(
		context.get_device(),
		semaphore,
		&result
	);

	return result;
}

void Device::destroy_semaphore(VkSemaphore semaphore) const
{
	vkDestroySemaphore(context.get_device(), semaphore, nullptr);
}

void Device::destroy_query_pool(VkQueryPool pool) const
{
	vkDestroyQueryPool(context.get_device(), pool, nullptr);
}

void Device::PerFrameData::flush(VkDevice vk_device, VmaAllocator vma_allocator, BindlessResources &bindless)
{
	for (auto &sampler : destroyed_samplers)
		vkDestroySampler(vk_device, sampler, nullptr);

	for (auto &image : destroyed_images)
		vmaDestroyImage(vma_allocator, image.handle, image.allocation);

	for (auto &view : destroyed_views)
		vkDestroyImageView(vk_device, view, nullptr);

	for (auto &buffer : destroyed_buffers)
		vmaDestroyBuffer(vma_allocator, buffer.handle, buffer.allocation);

	for (auto &bs : destroyed_bindless_samplers)
		bindless.free_sampler(bs);

	for (auto &bv : destroyed_bindless_views)
		bindless.free_view(bv);

	destroyed_samplers.clear();
	destroyed_images.clear();
	destroyed_views.clear();
	destroyed_buffers.clear();
	destroyed_bindless_samplers.clear();
	destroyed_bindless_views.clear();
}

CommandBuffer Device::begin_frame(Swapchain &swapchain)
{
	PerFrameData &frame_data = per_frame_data[current_frame_index];

	frame_data.flush(context.get_device(), context.get_allocator(), bindless);

	if (frame_data.expected_timeline_value > 0)
		wait_for_timeline_semaphore(graphics_timeline_semaphore, frame_data.expected_timeline_value);

	VkAcquireNextImageInfoKHR acquire_next_image_info = {};
	acquire_next_image_info.sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR;
	acquire_next_image_info.swapchain = swapchain.get_handle();
	acquire_next_image_info.timeout = UINT64_MAX;
	acquire_next_image_info.semaphore = frame_data.image_available_semaphore;
	acquire_next_image_info.fence = VK_NULL_HANDLE;
	acquire_next_image_info.deviceMask = 1;

	VkResult result = vkAcquireNextImage2KHR(context.get_device(), &acquire_next_image_info, &swapchain.current_texture_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		debug_log_crash("Failed to acquire next image in swapchain.");

	reset_command_pool(frame_data.command_pool);

	CommandBuffer cmd = fetch_free_buffer(frame_data.command_pool);
	cmd.begin();

	return cmd;
}

void Device::end_frame(const Swapchain &swapchain, CommandBuffer &cmd)
{
	PerFrameData &frame_data = per_frame_data[current_frame_index];

	apply_bindless_updates();

	VkSemaphoreSubmitInfo image_available_semaphore = {};
	image_available_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	image_available_semaphore.semaphore = frame_data.image_available_semaphore;
	image_available_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	VkSemaphoreSubmitInfo render_finished_semaphore = {};
	render_finished_semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	render_finished_semaphore.semaphore = frame_data.render_finished_semaphore;
	render_finished_semaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	frame_data.expected_timeline_value = submit_graphics(
		cmd,
		{ image_available_semaphore },
		{ render_finished_semaphore },
		VK_NULL_HANDLE
	);

	present(swapchain, { render_finished_semaphore.semaphore });

	current_frame_index = (current_frame_index + 1) % FRAMES_IN_FLIGHT;
}

u64 Device::submit_graphics(
	CommandBuffer &cmd,
	const Vector<VkSemaphoreSubmitInfo> &waits,
	const Vector<VkSemaphoreSubmitInfo> &signals,
	VkFence fence
)
{
	cmd.end();

	graphics_timeline_value++;

	VkSemaphoreSubmitInfo timeline_signal_info = {};
	timeline_signal_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	timeline_signal_info.semaphore = graphics_timeline_semaphore;
	timeline_signal_info.value = graphics_timeline_value; // Signal the N+1 value!!
	timeline_signal_info.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

	// TODO: THIS FUCKING SUCKS!!!
	Vector<VkSemaphoreSubmitInfo> all_signals = signals;
	all_signals.push_back(timeline_signal_info);
	
	VkCommandBufferSubmitInfo buffer_info = {};
	buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	buffer_info.deviceMask = 0;
	buffer_info.commandBuffer = cmd.get_handle();

	VkSubmitInfo2 submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submit_info.flags = 0;

	submit_info.commandBufferInfoCount = 1;
	submit_info.pCommandBufferInfos = &buffer_info;

	submit_info.waitSemaphoreInfoCount = waits.size();
	submit_info.pWaitSemaphoreInfos = waits.data();

	submit_info.signalSemaphoreInfoCount = all_signals.size();
	submit_info.pSignalSemaphoreInfos = all_signals.data();

	GFX_VK_CHECK(
		vkQueueSubmit2(context.graphics().handle, 1, &submit_info, fence),
		"Failed to submit command to queue."
	);

	return graphics_timeline_value;
}

void Device::submit_graphics_immediate(
	const std::function<void(CommandBuffer &cmd)> &record
)
{
	PerFrameData &frame_data = per_frame_data[current_frame_index];

	vkQueueWaitIdle(context.graphics().handle); // TODO: is wait_idle() necessary?
	
	CommandBuffer cmd = fetch_free_buffer(frame_data.command_pool);
	cmd.begin();
	
	record(cmd);

	u64 t = submit_graphics(cmd, {}, {}, VK_NULL_HANDLE);

	wait_for_timeline_semaphore(graphics_timeline_semaphore, t);
}

void Device::present(
	const Swapchain &swapchain,
	const Vector<VkSemaphore> &waits
)
{
	u32 image_index = swapchain.get_current_texture_index();
	VkSwapchainKHR swapchain_handle = swapchain.get_handle();

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pResults = nullptr;
	present_info.pImageIndices = &image_index;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain_handle;
	present_info.waitSemaphoreCount = waits.size();
	present_info.pWaitSemaphores = waits.data();

	VkResult result = vkQueuePresentKHR(context.graphics().handle, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		debug_log_crash("TODO We need to rebuild the entire swapchain here.");
	else if (result != VK_SUCCESS)
		debug_log_crash("Failed to present swapchain image.");
}

CommandPool Device::create_command_pool(u32 family_index)
{
	const u32 initial_buffer_count = 64;

	VkCommandPoolCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = family_index;
	
	CommandPool pool;

	GFX_VK_CHECK(
		vkCreateCommandPool(
			context.get_device(),
			&create_info, nullptr,
			&pool.handle
		),
		"Failed to create command pool."
	);

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = initial_buffer_count;
	alloc_info.commandPool = pool.handle;

	pool.buffers.resize(initial_buffer_count);

	GFX_VK_CHECK(
		vkAllocateCommandBuffers(
			context.get_device(),
			&alloc_info,
			pool.buffers.data()
		),
		"Failed to allocate command pool command buffers."
	);

	return pool;
}

void Device::destroy_command_pool(const CommandPool &pool)
{
	vkDestroyCommandPool(context.get_device(), pool.get_handle(), nullptr);
}

void Device::reset_command_pool(CommandPool &pool)
{
	pool.used_count = 0;
	vkResetCommandPool(context.get_device(), pool.get_handle(), 0);
}

CommandBuffer Device::fetch_free_buffer(CommandPool &pool)
{
	if (pool.used_count < pool.buffers.size())
		return CommandBuffer(pool.buffers[pool.used_count++]);

	const u32 alloc_count = 32;

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = pool.handle;
	alloc_info.commandBufferCount = alloc_count;

	Vector<VkCommandBuffer> new_buffers(alloc_count);

	GFX_VK_CHECK(
		vkAllocateCommandBuffers(
			context.get_device(),
			&alloc_info,
			new_buffers.data()
		),
		"Failed to allocate command pool command buffers when expanding pool."
	);

	for (auto &b : new_buffers)
		pool.buffers.push_back(b);

	return CommandBuffer(pool.buffers[pool.used_count++]);
}

Swapchain Device::create_swapchain()
{
	ScratchScope scratch = scratch::get();

	SwapchainSupportDetails details = context.get_swapchain_details();

	VkSurfaceFormatKHR surface_format = _choose_swapchain_surface_format(details.surface_formats);
	VkPresentModeKHR present_mode = _choose_swapchain_present_mode(details.present_modes, false); // TODO: for now disable vsync
	VkExtent2D extent = _choose_swapchain_extent(&details.capabilities);

	Swapchain swapchain;

	swapchain.width = extent.width;
	swapchain.height = extent.height;
	swapchain.format = surface_format.format;

	u32 texture_count = details.capabilities.minImageCount + 1;

	if (details.capabilities.maxImageCount > 0 && texture_count > details.capabilities.maxImageCount)
		texture_count = details.capabilities.maxImageCount;

	constexpr VkImageUsageFlags swapchain_texture_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkSwapchainCreateInfoKHR create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = context.get_surface();
	create_info.minImageCount = texture_count;
	create_info.imageFormat = surface_format.format;
	create_info.imageColorSpace = surface_format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers = 1;
	create_info.imageUsage = swapchain_texture_usage;
	create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.queueFamilyIndexCount = 0;
	create_info.pQueueFamilyIndices = nullptr;
	create_info.preTransform = details.capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateSwapchainKHR(
			context.get_device(),
			&create_info, nullptr,
			&swapchain.handle
		),
		"Failed to create swapchain."
	);

	vkGetSwapchainImagesKHR(context.get_device(), swapchain.handle, &texture_count, nullptr);

	if (texture_count <= 0)
		debug_log_crash("Failed to find any images in swapchain.");

	VkImage *vk_images = scratch.arena().array<VkImage>(texture_count);

	vkGetSwapchainImagesKHR(context.get_device(), swapchain.handle, &texture_count, vk_images);

	swapchain.textures.resize(texture_count);
	swapchain.views.resize(texture_count);

	for (int i = 0; i < texture_count; i++) {
		Texture &texture = swapchain.textures[i];

		texture.handle = vk_images[i];

		texture.width = swapchain.width;
		texture.height = swapchain.height;
		texture.depth = 1;

		texture.is_depth_texture     = false;
		texture.is_cubemap_texture   = false;
		texture.is_storage_texture   = false;
		texture.is_swapchain_texture = true;

		texture.format = swapchain.format;
		texture.type   = VK_IMAGE_TYPE_2D;
		texture.tiling = VK_IMAGE_TILING_OPTIMAL;

		texture.usage = swapchain_texture_usage;

		texture.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
		texture.aspect_count = 1;

		texture.layer_count = 1;
		texture.mipmap_count = 1;
		texture.sample_count = VK_SAMPLE_COUNT_1_BIT;

		SubresourceRange range = {};
		range.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
		range.base_mip = 0;
		range.mips = 1;
		range.base_layer = 0;
		range.layers = 1;

		swapchain.views[i] = create_texture_view(&texture, VK_IMAGE_VIEW_TYPE_2D, range);
	}

	debug_log("Swapchain created.");

	return swapchain;
}

void Device::destroy_swapchain(const Swapchain &swapchain)
{
	wait_idle();

	for (int i = 0; i < swapchain.views.size(); i++)
		destroy_texture_view(swapchain.views[i]);

	vkDestroySwapchainKHR(context.get_device(), swapchain.get_handle(), nullptr);
}

VkPipelineLayout Device::create_pipeline_layout(const ShaderProgram *program)
{
	VkShaderStageFlags stage = program->is_compute()
		? VK_SHADER_STAGE_COMPUTE_BIT
		: VK_SHADER_STAGE_ALL_GRAPHICS;

	VkPushConstantRange push_constants = {};
	push_constants.offset = 0;
	push_constants.size = program->get_push_constant_size();
	push_constants.stageFlags = stage;

	VkPipelineLayoutCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.setLayoutCount = BINDLESS_SET_MAX_ENUM;
	create_info.pSetLayouts = bindless.get_layouts();

	if (push_constants.size > 0) {
		create_info.pushConstantRangeCount = 1;
		create_info.pPushConstantRanges = &push_constants;
	} else {
		create_info.pushConstantRangeCount = 0;
		create_info.pPushConstantRanges = nullptr;
	}

	VkPipelineLayout layout = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreatePipelineLayout(
			context.get_device(),
			&create_info, nullptr,
			&layout
		),
		"Failed to create pipeline layout."
	);

	return layout;
}

void Device::destroy_pipeline_layout(VkPipelineLayout layout)
{
	vkDestroyPipelineLayout(context.get_device(), layout, nullptr);
}

VkPipeline Device::create_pipeline(const GraphicsPipelineDef &def, VkPipelineLayout layout)
{
	assert(!def.program->is_compute());

	static const VkDynamicState graphics_pipeline_dynamic_states[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		//VK_DYNAMIC_STATE_BLEND_CONSTANTS // TODO: Add dynamic blend constants.
	};

	// We use vertex pulling in shaders so explicitly defined vertex formats aren't used.
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {};
	vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_state_create_info.vertexBindingDescriptionCount = 0;
	vertex_input_state_create_info.pVertexBindingDescriptions = nullptr;
	vertex_input_state_create_info.vertexAttributeDescriptionCount = 0;
	vertex_input_state_create_info.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {};
	input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_state_create_info.topology = def.topology;
	input_assembly_state_create_info.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;
	viewport_state_create_info.pViewports = nullptr; // Using dynamic viewport.
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = nullptr; // Using dynamic scissor.

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {};
	rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization_state_create_info.depthClampEnable = VK_FALSE;
	rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterization_state_create_info.lineWidth = 1.f;
	rasterization_state_create_info.cullMode = def.cull_mode;
	rasterization_state_create_info.frontFace = def.front_face;
	rasterization_state_create_info.depthBiasEnable = VK_FALSE;
	rasterization_state_create_info.depthBiasConstantFactor = 0.f;
	rasterization_state_create_info.depthBiasClamp = 0.f;
	rasterization_state_create_info.depthBiasSlopeFactor = 0.f;

	VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {};
	multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_state_create_info.sampleShadingEnable = def.min_sample_shading_enabled;
	multisample_state_create_info.minSampleShading = def.min_sample_shading;
	multisample_state_create_info.rasterizationSamples = def.samples;
	multisample_state_create_info.pSampleMask = nullptr;
	multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_state_create_info.alphaToOneEnable = VK_FALSE;
	
	ScratchScope scratch = scratch::get();

	VkPipelineColorBlendAttachmentState *blend_states = scratch.arena().array<VkPipelineColorBlendAttachmentState>(def.colour_attachment_formats.size());

	VkPipelineColorBlendAttachmentState *blend_state = blend_states;

	for (int i = 0; i < def.colour_attachment_formats.size(); i++, blend_state++) {

		blend_state->blendEnable = def.blend_state.enabled;

		blend_state->srcColorBlendFactor = def.blend_state.colour.src;
		blend_state->dstColorBlendFactor = def.blend_state.colour.dst;
		blend_state->colorBlendOp = def.blend_state.colour.op;

		blend_state->srcAlphaBlendFactor = def.blend_state.alpha.src;
		blend_state->dstAlphaBlendFactor = def.blend_state.alpha.dst;
		blend_state->alphaBlendOp = def.blend_state.alpha.op;

		if (def.blend_state.write_mask[0]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
		if (def.blend_state.write_mask[1]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
		if (def.blend_state.write_mask[2]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
		if (def.blend_state.write_mask[3]) blend_state->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo colour_blend_state_create_info = {};
	colour_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colour_blend_state_create_info.logicOpEnable = def.blend_state.logic_op_enabled;
	colour_blend_state_create_info.logicOp = def.blend_state.logic_op;
	colour_blend_state_create_info.attachmentCount = def.colour_attachment_formats.size();
	colour_blend_state_create_info.pAttachments = blend_states;
	colour_blend_state_create_info.blendConstants[0] = def.blend_state.constants[0];
	colour_blend_state_create_info.blendConstants[1] = def.blend_state.constants[1];
	colour_blend_state_create_info.blendConstants[2] = def.blend_state.constants[2];
	colour_blend_state_create_info.blendConstants[3] = def.blend_state.constants[3];

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {};
	depth_stencil_state_create_info.depthTestEnable       = def.depth_stencil_state.depth_test_enabled;
	depth_stencil_state_create_info.depthWriteEnable      = def.depth_stencil_state.depth_write_enabled;
	depth_stencil_state_create_info.depthCompareOp        = def.depth_stencil_state.depth_compare_op;
	depth_stencil_state_create_info.depthBoundsTestEnable = def.depth_stencil_state.depth_bounds_test_enabled;
	depth_stencil_state_create_info.minDepthBounds        = def.depth_stencil_state.depth_bounds_min;
	depth_stencil_state_create_info.maxDepthBounds        = def.depth_stencil_state.depth_bounds_max;
	depth_stencil_state_create_info.stencilTestEnable     = def.depth_stencil_state.stencil_test_enabled;
	depth_stencil_state_create_info.front.failOp          = def.depth_stencil_state.stencil_front.fail_op;
	depth_stencil_state_create_info.front.passOp          = def.depth_stencil_state.stencil_front.pass_op;
	depth_stencil_state_create_info.front.depthFailOp     = def.depth_stencil_state.stencil_front.depth_fail_op;
	depth_stencil_state_create_info.front.compareOp       = def.depth_stencil_state.stencil_front.compare_op;
	depth_stencil_state_create_info.front.writeMask       = def.depth_stencil_state.stencil_front.write_mask;
	depth_stencil_state_create_info.front.reference       = def.depth_stencil_state.stencil_front.reference;
	depth_stencil_state_create_info.back.failOp           = def.depth_stencil_state.stencil_back.fail_op;
	depth_stencil_state_create_info.back.passOp           = def.depth_stencil_state.stencil_back.pass_op;
	depth_stencil_state_create_info.back.depthFailOp      = def.depth_stencil_state.stencil_back.depth_fail_op;
	depth_stencil_state_create_info.back.compareOp        = def.depth_stencil_state.stencil_back.compare_op;
	depth_stencil_state_create_info.back.writeMask        = def.depth_stencil_state.stencil_back.write_mask;
	depth_stencil_state_create_info.back.reference        = def.depth_stencil_state.stencil_back.reference;

	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = array_size(graphics_pipeline_dynamic_states);
	dynamic_state_create_info.pDynamicStates = graphics_pipeline_dynamic_states;

	VkFormat depth_stencil_format = def.has_depth_attachment
		? context.get_depth_format()
		: VK_FORMAT_UNDEFINED;

	VkPipelineRenderingCreateInfo pipeline_rendering_create_info = {};
	pipeline_rendering_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	pipeline_rendering_create_info.viewMask = def.multi_view_mask;
	pipeline_rendering_create_info.colorAttachmentCount = def.colour_attachment_formats.size();
	pipeline_rendering_create_info.pColorAttachmentFormats = def.colour_attachment_formats.data();
	pipeline_rendering_create_info.depthAttachmentFormat = depth_stencil_format;
	pipeline_rendering_create_info.stencilAttachmentFormat = depth_stencil_format;

	const ShaderProgram *program = def.program;

	VkShaderModuleCreateInfo module_infos[MAX_SHADER_STAGES] = {};
	VkPipelineShaderStageCreateInfo shader_stages[MAX_SHADER_STAGES] = {};

	for (int i = 0; i < program->get_stage_count(); i++) {
		module_infos[i].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		module_infos[i].codeSize = program->get_stage_bytecode(i).size;
		module_infos[i].pCode = (u32 *)program->get_stage_bytecode(i).bytes;
		module_infos[i].flags = 0;

		shader_stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shader_stages[i].stage = (VkShaderStageFlagBits)program->get_stage_flags(i);
		shader_stages[i].module = VK_NULL_HANDLE;
		shader_stages[i].pName = "main";
		shader_stages[i].pNext = &module_infos[i];
	}

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = program->get_stage_count();
	graphics_pipeline_create_info.pStages = shader_stages;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_state_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
	graphics_pipeline_create_info.pColorBlendState = &colour_blend_state_create_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	graphics_pipeline_create_info.layout = layout;
	graphics_pipeline_create_info.renderPass = VK_NULL_HANDLE;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;
	graphics_pipeline_create_info.pNext = &pipeline_rendering_create_info;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateGraphicsPipelines(
			context.get_device(),
			pipeline_process_cache,
			1, &graphics_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create graphics pipeline."
	);

	return pipeline;
}

VkPipeline Device::create_pipeline(const ComputePipelineDef &def, VkPipelineLayout layout)
{
	const ShaderProgram *program = def.program;

	assert(program->is_compute());

	VkShaderModuleCreateInfo module_info = {};
	module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	module_info.codeSize = program->get_stage_bytecode(0).size;
	module_info.pCode = (u32 *)program->get_stage_bytecode(0).bytes;
	module_info.flags = 0;

	VkPipelineShaderStageCreateInfo shader_stage = {};
	shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stage.stage = (VkShaderStageFlagBits)program->get_stage_flags(0);
	shader_stage.module = VK_NULL_HANDLE;
	shader_stage.pName = "main";
	shader_stage.pNext = &module_info;

	VkComputePipelineCreateInfo compute_pipeline_create_info = {};
	compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	compute_pipeline_create_info.layout = layout;
	compute_pipeline_create_info.stage = shader_stage;

	VkPipeline pipeline = VK_NULL_HANDLE;

	GFX_VK_CHECK(
		vkCreateComputePipelines(
			context.get_device(),
			pipeline_process_cache,
			1, &compute_pipeline_create_info,
			nullptr, &pipeline
		),
		"Failed to create compute pipeline."
	);

	return pipeline;
}

void Device::destroy_pipeline(VkPipeline pipeline)
{
	vkDestroyPipeline(context.get_device(), pipeline, nullptr);
}

Sampler *Device::create_sampler(const SamplerCreateInfo &info)
{
	VkPhysicalDeviceProperties properties =	context.get_physical_properties();

	VkSamplerCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	create_info.minFilter = info.filter;
	create_info.magFilter = info.filter;
	create_info.addressModeU = info.wrap_x;
	create_info.addressModeV = info.wrap_y;
	create_info.addressModeW = info.wrap_z;
	create_info.anisotropyEnable = VK_TRUE;
	create_info.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	create_info.borderColor = info.border_colour;
	create_info.unnormalizedCoordinates = VK_FALSE;
	create_info.compareEnable = VK_FALSE;
	create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	create_info.mipLodBias = 0.f;
	create_info.minLod = 0.f;
	create_info.maxLod = VK_LOD_CLAMP_NONE;

	Sampler *sampler = new Sampler();
	sampler->filter = info.filter;
	sampler->wrap_x = info.wrap_x;
	sampler->wrap_y = info.wrap_y;
	sampler->wrap_z = info.wrap_z;
	sampler->border_colour = info.border_colour;

	GFX_VK_CHECK(
		vkCreateSampler(
			context.get_device(),
			&create_info, nullptr,
			&sampler->handle
		),
		"Failed to create texture sampler."
	);

	sampler->bindless_handle = bindless.register_sampler(sampler->handle);

	return sampler;
}

Sampler *Device::create_sampler(VkFilter filter)
{
	SamplerCreateInfo create_info = {};
	create_info.filter = filter;
	create_info.wrap_x = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_y = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.wrap_z = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	create_info.border_colour = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	return create_sampler(create_info);
}

void Device::destroy_sampler(const Sampler *sampler)
{
	assert(sampler);
	per_frame_data[current_frame_index].destroyed_samplers.push_back(sampler->handle);
	per_frame_data[current_frame_index].destroyed_bindless_samplers.push_back(sampler->bindless_handle);
	delete sampler;
}

static u32 clamp_mimap_count(u32 mipmaps, u32 w, u32 h, u32 d)
{
	return CalcF::min(mipmaps, 1u + (u32)CalcF::log2(CalcF::max(w, CalcF::max(h, d))));
}

Texture *Device::alloc_texture(const TextureAllocInfo &alloc_info)
{
	Texture *texture = new Texture();

	texture->width = alloc_info.width;
	texture->height = alloc_info.height;
	texture->depth = alloc_info.depth;

	texture->format = alloc_info.format;
	texture->type = alloc_info.type;
	texture->tiling = alloc_info.tiling;

	texture->mipmap_count = clamp_mimap_count(alloc_info.mipmaps, alloc_info.width, alloc_info.height, alloc_info.depth);
	texture->layer_count = alloc_info.layers;
	texture->sample_count = alloc_info.samples;

	if (alloc_info.is_transient)
		texture->usage =
			VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	else
		texture->usage =
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	texture->is_transient_texture = alloc_info.is_transient;
	texture->is_depth_texture = alloc_info.format == context.get_depth_format();
	texture->is_cubemap_texture = alloc_info.is_cubemap;
	texture->is_storage_texture = alloc_info.is_storage;
	texture->is_swapchain_texture = false;

	if (alloc_info.is_storage)
		texture->usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (texture->is_depth())
		texture->usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	else
		texture->usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	texture->aspect_flags = texture->is_depth()
		? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
		: VK_IMAGE_ASPECT_COLOR_BIT;

	texture->aspect_count = 0;

	for (VkImageAspectFlags b = 1; b <= texture->aspect_flags; b <<= 1) {
		if (texture->aspect_flags & b)
			texture->aspect_count++;
	}

	VkImageCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	create_info.imageType = alloc_info.type;
	create_info.extent.width = texture->width;
	create_info.extent.height = texture->height;
	create_info.extent.depth = texture->depth;
	create_info.mipLevels = texture->mipmap_count;
	create_info.arrayLayers = texture->layer_count;
	create_info.format = texture->format;
	create_info.tiling = texture->tiling;
	create_info.usage = texture->usage;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.samples = (VkSampleCountFlagBits)texture->sample_count;
	create_info.flags = 0;

	if (alloc_info.is_cubemap)
		create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.priority = 1.f;

	if (alloc_info.is_storage)
		vma_alloc_info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

	GFX_VK_CHECK(
		vmaCreateImage(
			context.get_allocator(),
			&create_info,
			&vma_alloc_info, &texture->handle,
			&texture->allocation, &texture->allocation_info
		),
		"Failed to allocate texture."
	);

	return texture;
}

Texture *Device::alloc_texture_2d(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	TextureAllocInfo alloc_info = {};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.is_cubemap = false;
	alloc_info.is_storage = false;
	alloc_info.is_transient = false;

	return alloc_texture(alloc_info);
}

Texture *Device::alloc_texture_2d_rw(u32 width, u32 height, VkFormat format, u32 mipmaps)
{
	TextureAllocInfo alloc_info = {};
	alloc_info.width = width;
	alloc_info.height = height;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 1;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.is_cubemap = false;
	alloc_info.is_storage = true;
	alloc_info.is_transient = false;

	return alloc_texture(alloc_info);
}

Texture *Device::alloc_texture_2d_depth(u32 width, u32 height, u32 mipmaps)
{
	return alloc_texture_2d(
		width, height,
		context.get_depth_format(),
		mipmaps
	);
}

Texture *Device::alloc_texture_2d_rw_depth(u32 width, u32 height, u32 mipmaps)
{
	return alloc_texture_2d_rw(
		width, height,
		context.get_depth_format(),
		mipmaps
	);
}

Texture *Device::alloc_texture_cubemap(u32 resolution, VkFormat format, u32 mipmaps)
{
	TextureAllocInfo alloc_info = {};
	alloc_info.width = resolution;
	alloc_info.height = resolution;
	alloc_info.depth = 1;
	alloc_info.format = format;
	alloc_info.type = VK_IMAGE_TYPE_2D;
	alloc_info.tiling = VK_IMAGE_TILING_OPTIMAL;
	alloc_info.mipmaps = mipmaps;
	alloc_info.layers = 6;
	alloc_info.samples = VK_SAMPLE_COUNT_1_BIT;
	alloc_info.is_cubemap = true;
	alloc_info.is_storage = false;
	alloc_info.is_transient = false;

	return alloc_texture(alloc_info);
}

Texture *Device::alloc_texture_cubemap_depth(u32 resolution, u32 mipmaps)
{
	return alloc_texture_cubemap(
		resolution,
		context.get_depth_format(),
		mipmaps
	);
}

void Device::destroy_texture(const Texture *texture)
{
	assert(texture);
	per_frame_data[current_frame_index].destroyed_images.push_back({ texture->handle, texture->allocation });
	delete texture;
}

TextureView *Device::create_texture_view(
	const Texture *texture,
	VkImageViewType type,
	const SubresourceRange &range
)
{
	VkImageViewCreateInfo view_create_info = {};
	view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_create_info.image = texture->get_handle();
	view_create_info.viewType = type;
	view_create_info.format = texture->get_format();
	
	SubresourceRange r = range.of_texture(texture);

	view_create_info.subresourceRange.aspectMask = r.aspects;
	view_create_info.subresourceRange.baseMipLevel = r.base_mip;
	view_create_info.subresourceRange.levelCount = r.mips;
	view_create_info.subresourceRange.baseArrayLayer = r.base_layer;
	view_create_info.subresourceRange.layerCount = r.layers;

	view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	TextureView *view = new TextureView();
	view->type = type;
	view->range = r;

	GFX_VK_CHECK(
		vkCreateImageView(
			context.get_device(),
			&view_create_info, nullptr,
			&view->handle
		),
		"Failed to create texture image view."
	);

	// Swapchain images are omitted from being accessible bindlessly.
	if (!texture->is_swapchain())
		view->bindless_handle = bindless.register_view(view->handle, texture->is_storage());

	return view;
}

void Device::destroy_texture_view(const TextureView *texture_view)
{
	assert(texture_view);
	per_frame_data[current_frame_index].destroyed_views.push_back(texture_view->handle);
	per_frame_data[current_frame_index].destroyed_bindless_views.push_back(texture_view->bindless_handle);
	delete texture_view;
}

GpuBuffer *Device::alloc_buffer(const BufferAllocInfo &alloc_info)
{
	GpuBuffer *buffer = new GpuBuffer();
	buffer->usage = alloc_info.usage;
	buffer->size = alloc_info.size;
	buffer->allocator = context.get_allocator();
	buffer->allocation_flags = alloc_info.flags;

	// All storage buffers automatically get BDA because its the big '26.
	if (buffer->is_storage())
		buffer->usage |= VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo buffer_create_info = {};
	buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_create_info.size = buffer->size;
	buffer_create_info.usage = buffer->usage;
	buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_create_info.queueFamilyIndexCount = 0;
	buffer_create_info.pQueueFamilyIndices = nullptr;

	VmaAllocationCreateInfo vma_alloc_info = {};
	vma_alloc_info.usage = VMA_MEMORY_USAGE_AUTO;
	vma_alloc_info.flags = alloc_info.flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	GFX_VK_CHECK(
		vmaCreateBuffer(
			context.get_allocator(),
			&buffer_create_info,
			&vma_alloc_info,
			&buffer->handle,
			&buffer->allocation,
			&buffer->allocation_info
		),
		"Failed to allocate buffer."
	);

	// All storage buffers automatically get BDA because its the big '26.
	if (buffer->is_storage()) {
		VkBufferDeviceAddressInfo address_info = {};
		address_info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		address_info.buffer = buffer->handle;

		buffer->device_address = vkGetBufferDeviceAddress(context.get_device(), &address_info);
	}

	return buffer;
}

GpuBuffer *Device::alloc_stage(u64 size)
{
	BufferAllocInfo alloc_info = {};
	alloc_info.usage = VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT;
	alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	alloc_info.size = size;

	return alloc_buffer(alloc_info);
}

void Device::destroy_buffer(const GpuBuffer *buffer)
{
	assert(buffer);
	per_frame_data[current_frame_index].destroyed_buffers.push_back({ buffer->handle, buffer->allocation });
	delete buffer;
}

static ShaderStage create_shader_stage(const ShaderBytecode &data)
{
	SpvReflectShaderModule reflect_module = {};
	SpvReflectResult reflect_result = spvReflectCreateShaderModule(data.size, data.bytes, &reflect_module);

	if (reflect_result != SPV_REFLECT_RESULT_SUCCESS)
		debug_log_crash("Failed to reflect SPIR-V module: %d\n", reflect_result);
	
	ScratchScope scratch = scratch::get();

	ShaderStage stage = {};

	if (reflect_module.entry_point_count >= 1) {
		stage.flags = (VkShaderStageFlags)reflect_module.entry_points[0].shader_stage;

		u32 push_constant_count = 0;
		reflect_result = spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, nullptr);

		if (reflect_result == SPV_REFLECT_RESULT_SUCCESS && push_constant_count > 0) {
			SpvReflectBlockVariable **pcs = scratch.arena().array<SpvReflectBlockVariable *>(push_constant_count);
			spvReflectEnumeratePushConstantBlocks(&reflect_module, &push_constant_count, pcs);

			for (u32 i = 0; i < push_constant_count; i++) {
				SpvReflectBlockVariable *pc = pcs[i];

				u32 alignment = 4;

				for (u32 j = 0; j < pc->member_count; j++)
					alignment = CalcU::max(alignment, pc->members[j].size);

				u32 padded = memory_align_up(pc->size, alignment);
				stage.push_constant_size = CalcU::max(stage.push_constant_size, padded);
			}
		}

		stage.bytecode.size = data.size;
		stage.bytecode.bytes = (u8 *)malloc(data.size);
		memcpy(stage.bytecode.bytes, data.bytes, data.size);
	} else {
		debug_log_crash("No entry points found in SPIR-V.\n");
	}

	spvReflectDestroyShaderModule(&reflect_module);

	return stage;
}

static void destroy_shader_stage(const ShaderStage &stage)
{
	free(stage.bytecode.bytes);
}

ShaderProgram *Device::create_shader_program(const Vector<ShaderBytecode> &stages)
{
	ShaderProgram *program = new ShaderProgram();
	program->stage_count = stages.size();
	program->push_constant_size = 0;

	for (int i = 0; i < stages.size(); i++) {
		program->stages[i] = create_shader_stage(stages[i]);
		program->push_constant_size = CalcU::max(program->push_constant_size, program->stages[i].push_constant_size);
	}

	static u32 shader_cookie = 0;

	program->cookie = ++shader_cookie;

	return program;
}

void Device::destroy_shader_program(const ShaderProgram *program)
{
	for (int i = 0; i < program->stage_count; i++)
		destroy_shader_stage(program->stages[i]);

	delete program;
}

static VkDescriptorType get_descriptor_type_from_bindless_set(BindlessSetKind kind)
{
	switch (kind) {
		case BINDLESS_SET_SAMPLER:  return VK_DESCRIPTOR_TYPE_SAMPLER;
		case BINDLESS_SET_SAMPLED:  return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case BINDLESS_SET_STORAGE:  return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	}

	debug_log_crash("Could not find descriptor type from bindless binding type.");

	return (VkDescriptorType)0;
}

void Device::create_sync_resources()
{
	graphics_timeline_value = 0;
	graphics_timeline_semaphore = create_timeline_semaphore(graphics_timeline_value);

	HardwareQueue graphics = context.graphics();

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		VkFenceCreateInfo in_flight_fence_create_info = {};
		in_flight_fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		in_flight_fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		per_frame_data[i].expected_timeline_value = 0;

		per_frame_data[i].command_pool = create_command_pool(graphics.family_index);

		VkSemaphoreCreateInfo binary_semaphore_create_info = {};
		binary_semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		binary_semaphore_create_info.flags = 0;
		binary_semaphore_create_info.pNext = nullptr;

		GFX_VK_CHECK(
			vkCreateSemaphore(
				context.get_device(),
				&binary_semaphore_create_info, nullptr,
				&per_frame_data[i].image_available_semaphore
			),
			"Failed to create image available semaphore."
		);

		GFX_VK_CHECK(
			vkCreateSemaphore(
				context.get_device(),
				&binary_semaphore_create_info, nullptr,
				&per_frame_data[i].render_finished_semaphore
			),
			"Failed to create render finished semaphore."
		);
	}

	debug_log("Created frame sync objects.");
}

void Device::destroy_sync_resources()
{
	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		destroy_semaphore(per_frame_data[i].image_available_semaphore);
		destroy_semaphore(per_frame_data[i].render_finished_semaphore);
		destroy_command_pool(per_frame_data[i].command_pool);
		per_frame_data[i].flush(context.get_device(), context.get_allocator(), bindless);
	}

	destroy_semaphore(graphics_timeline_semaphore);
}

void Device::create_bindless()
{
	VkDescriptorPoolSize pool_sizes[BINDLESS_SET_MAX_ENUM] = {};

	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++) {
		pool_sizes[i].type = get_descriptor_type_from_bindless_set((BindlessSetKind)i);
		pool_sizes[i].descriptorCount = BindlessResources::MAX_RESOURCES;
	}

	VkDescriptorPoolCreateInfo pool_create_info = {};
	pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
	pool_create_info.maxSets = array_size(pool_sizes) * BindlessResources::MAX_RESOURCES;
	pool_create_info.poolSizeCount = array_size(pool_sizes);
	pool_create_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(
		vkCreateDescriptorPool(
			context.get_device(),
			&pool_create_info, nullptr,
			&bindless.pool
		),
		"Failed to create bindless descriptor pool."
	);

	VkDescriptorBindingFlags bindless_flags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;

	VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags = {};
	binding_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
	binding_flags.bindingCount = 1;
	binding_flags.pBindingFlags = &bindless_flags;

	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++) {
		VkDescriptorSetLayoutBinding binding = {};
		binding.descriptorType = get_descriptor_type_from_bindless_set((BindlessSetKind)i);
		binding.descriptorCount = BindlessResources::MAX_RESOURCES;
		binding.binding = 0;
		binding.stageFlags = VK_SHADER_STAGE_ALL;
		binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layout_create_info = {};
		layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_create_info.bindingCount = 1;
		layout_create_info.pBindings = &binding;
		layout_create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
		layout_create_info.pNext = &binding_flags;

		GFX_VK_CHECK(
			vkCreateDescriptorSetLayout(
				context.get_device(),
				&layout_create_info, nullptr,
				&bindless.layouts[i]
			),
			"Failed to create bindless descriptor layout."
		);
	}

	VkDescriptorSetAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = bindless.pool;
	alloc_info.descriptorSetCount = BINDLESS_SET_MAX_ENUM;
	alloc_info.pSetLayouts = bindless.layouts;

	GFX_VK_CHECK(
		vkAllocateDescriptorSets(
			context.get_device(),
			&alloc_info,
			bindless.sets
		),
		"Failed to allocate bindless descriptor set."
	);

	debug_log("Bindless resources created.");
}

void Device::destroy_bindless()
{
	for (int i = 0; i < BINDLESS_SET_MAX_ENUM; i++)
		vkDestroyDescriptorSetLayout(context.get_device(), bindless.layouts[i], nullptr);

	vkDestroyDescriptorPool(context.get_device(), bindless.pool, nullptr);
}

void Device::apply_bindless_updates()
{
	if (bindless.updates.empty())
		return;

	Vector<VkWriteDescriptorSet> writes(bindless.updates.size());
	Vector<VkDescriptorImageInfo> infos(bindless.updates.size());

	for (int i = 0; i < bindless.updates.size(); i++) {
		BindlessResources::BindlessUpdate &update = bindless.updates[i];

		VkDescriptorImageInfo *info = &infos[i];
		info->sampler = update.sampler;
		info->imageView = update.view;
		info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet *write = &writes[i];
		write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write->descriptorCount = 1;
		write->dstArrayElement = update.slot;
		write->descriptorType = get_descriptor_type_from_bindless_set(update.kind);
		write->dstSet = bindless.sets[update.kind];
		write->dstBinding = 0;
		write->pImageInfo = info;
	}

	vkUpdateDescriptorSets(
		context.get_device(),
		writes.size(), writes.data(),
		0, nullptr
	);

	bindless.updates.clear();
}

void Device::init_imgui()
{
	const u32 max_sets = 1000;

	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                max_sets },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, max_sets },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         max_sets },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, max_sets },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       max_sets }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = max_sets;
	pool_info.poolSizeCount = array_size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;

	GFX_VK_CHECK(
		vkCreateDescriptorPool(
			context.get_device(),
			&pool_info, nullptr,
			&imgui_pool
		),
		"Failed to create ImGui descriptor pool."
	);

	VkFormat swapchain_image_format = VK_FORMAT_R32G32B32A32_SFLOAT;

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = context.get_instance();
	init_info.PhysicalDevice = context.get_physical_device();
	init_info.Device = context.get_device();
	init_info.QueueFamily = context.graphics().family_index;
	init_info.Queue = context.graphics().handle;
	init_info.PipelineCache = pipeline_process_cache;
	init_info.DescriptorPool = imgui_pool;
	init_info.Allocator = nullptr;
	init_info.MinImageCount = FRAMES_IN_FLIGHT;
	init_info.ImageCount = FRAMES_IN_FLIGHT;
	init_info.CheckVkResultFn = nullptr;
	init_info.UseDynamicRendering = true;
	init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
	init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &swapchain_image_format;

	ImGui_ImplVulkan_Init(&init_info);
}

void Device::destroy_imgui()
{
	ImGui_ImplVulkan_Shutdown();
	vkDestroyDescriptorPool(context.get_device(), imgui_pool, nullptr);
}

void Device::imgui_new_frame()
{
	ImGui_ImplVulkan_NewFrame();
}

void Device::imgui_record_draw_data(const CommandBuffer &cmd)
{
	ImDrawData *draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, cmd.get_handle());
}
