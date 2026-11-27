#ifndef GRAPHICS_DEVICE_H
#define GRAPHICS_DEVICE_H

#include "core/types.h"
#include "core/log.h"
#include "os/spin_lock.h"
#include "os/job.h"
#include "graphics/resource_key.h"
#include "graphics/resource_list.h"
#include "graphics/buffer.h"
#include "graphics/texture.h"
#include "graphics/sampler.h"
#include "graphics/shader.h"
#include "graphics/pipeline.h"
#include "graphics/accel_struct.h"
#include "graphics/capabilities.h"
#include "graphics/bindless.h"
#include "graphics/semaphore.h"
#include "graphics/swapchain.h"
#include "graphics/cmd_buffer.h"
#include "container/deque.h"
#include "container/vector.h"

#include <atomic>
#include "container/hash_map.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace os {
class Window;
}

namespace gfx {
class CmdPool;

struct SwapchainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities = {};
	Vector<VkSurfaceFormatKHR> surface_formats;
	Vector<VkPresentModeKHR> present_modes;
};

struct HardwareQueue {
	VkQueue vk_handle = VK_NULL_HANDLE;
	unsigned family_index = 0;
};

/*
 * ALRIGHTY:
 * Anything a recorded frame may still read is queued here instead of being destroyed.
 * Each entry is tagged with the newest frame/packet number, that could reference it
 * aka: the device's retire frame
 * this is a number that the top level (app) advances whenever it starts a new frame to build.
 * because the game always runs at or ahead of the rendering, that tag is conservative for destroys issued from either side.
 * when the render side has waited for frame N's slot, then every frame <= N - GRAPHICS_FRAMES_IN_FLIGHT has finished on the GPU, so it is released.
 */
enum PendingDestroyKind {
	PendingDestroyKind_Image,
	PendingDestroyKind_Buffer,
	PendingDestroyKind_Sampler,
	PendingDestroyKind_Shader,
	PendingDestroyKind_AccelStruct,
	PendingDestroyKind_ImGuiTexture,
	PendingDestroyKind_MaxEnum
};

struct PendingDestroy {
	PendingDestroyKind kind = PendingDestroyKind_MaxEnum;
	u64 retire_frame = 0;
	ResourceKey key;

	VkImage image = VK_NULL_HANDLE;
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation allocation = VK_NULL_HANDLE;

	VkSampler sampler = VK_NULL_HANDLE;
	unsigned sampler_bindless = 0;

	VkAccelerationStructureKHR accel = VK_NULL_HANDLE;

	VkDescriptorSet imgui_set = VK_NULL_HANDLE;
};

struct FrameInFlight {
	u64 frame_number = 0;
	TimelinePoint completion_point;
	CmdPool *command_pool = nullptr;
	VkSemaphore image_available_semaphore = VK_NULL_HANDLE; // Wait until the OS gives us an image.
};

// todo: make some kinda limits struct / expose ?

class Device {
	friend class Swapchain;

public:
	Device(os::Window *window, LogChannel log_channel, const Vector<FeatureRequest> &requested_features);
	~Device();

	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;

	static Device *Get();
	static void Select(Device *device);

	bool FeatureEnabled(FeatureType type) const;

	bool SupportsTextureCompressionBC() const;
	bool SupportsTextureCompressionASTC() const;

	bool FormatSupports(VkFormat format, VkFormatFeatureFlags required) const;

	void SetRetiredFrame(u64 frame_number);

	void FrameWait(u64 frame_number);
	unsigned FrameSlotIndex() const;

	CmdBuffer BeginFrame(Swapchain &swapchain);
	void EndFrame(const Swapchain &swapchain, const CmdBuffer &cmd);

	void ReleaseRetired(u64 completed_frame, bool release_all);

	TimelinePoint Submit(const CmdBuffer &cmd);

	TimelinePoint SubmitEx(
		const CmdBuffer &cmd,
		unsigned wait_count, const VkSemaphoreSubmitInfo *waits,
		unsigned signal_count, const VkSemaphoreSubmitInfo *signals
	);

	CmdBuffer SubmitImBegin();
	void SubmitImEnd(const CmdBuffer &cmd);


	/* ==================================================
	   QUERY
	   ================================================== */

	VkQueryPool QueryPoolCreate(unsigned query_count, VkQueryType type, VkQueryPipelineStatisticFlags pipeline_stat_flags);
	void QueryPoolDestroy(VkQueryPool pool);


	/* ==================================================
	   SYNCHRONISATION
	   ================================================== */

	void WaitIdle();

	void WaitForFence(VkFence fence);
	void ResetFence(VkFence fence);
	void DestroyFence(VkFence fence);

	Semaphore SemaphoreCreate(u64 value);
	void SemaphoreDestroy(const Semaphore &semaphore);
	u64 SemaphoreGPUCounterValue(const Semaphore &semaphore);

	void WaitUntil(TimelinePoint point);

	u64 LastSubmittedFrame() const
	{
		return graphics_semaphore.last_submitted_frame;
	}

	u64 GPUCompletedFrame()
	{
		return SemaphoreGPUCounterValue(graphics_semaphore);
	}


	/* ==================================================
	   PIPELINES
	   ================================================== */

	ResourceKey PipelineLayoutFetch(ResourceKey program);
	VkPipelineLayout PipelineLayoutFromKey(ResourceKey key);

	PipelineState FetchGraphicsPipeline(const GraphicsPipelineDef &def);
	PipelineState FetchComputePipeline(const ComputePipelineDef &def);

	VkPipeline PipelineFromKey(ResourceKey key);


	/* ==================================================
	   TEXTURES
	   ================================================== */

	ResourceKey TextureAlloc(const TextureAllocInfo &alloc_info);
	ResourceKey TextureAlloc2D(unsigned width, unsigned height, VkFormat format, unsigned mipmaps);
	ResourceKey TextureAlloc2DRW(unsigned width, unsigned height, VkFormat format, unsigned mipmaps);
	ResourceKey TextureAllocDepth2D(unsigned width, unsigned height, unsigned mipmaps);
	ResourceKey TextureAllocDepth2DRW(unsigned width, unsigned height, unsigned mipmaps);
	ResourceKey TextureAllocCubemap(unsigned resolution, VkFormat format, unsigned mipmaps);
	// VK_FORMAT_UNDEFINED falls bakc to the default depth-stencil format.
	ResourceKey TextureAllocCubemapDepth(unsigned resolution, unsigned mipmaps, VkFormat format = VK_FORMAT_UNDEFINED);

	void TextureDestroy(ResourceKey texture);

	Texture *TextureFromKey(ResourceKey key);


	/* ==================================================
	   VIEWS
	   ================================================== */

	ResourceKey TextureViewFetch(const TextureViewCreateInfo &create_info);
	ResourceKey TextureViewAuto(ResourceKey texture);

	TextureView *TextureViewFromKey(ResourceKey key);

	unsigned TextureViewBindless(ResourceKey key);


	/* ==================================================
	   BUFFERS
	   ================================================== */

	ResourceKey BufferAlloc(const BufferAllocInfo &alloc_info);
	ResourceKey StageAlloc(u64 size);

	void BufferDestroy(ResourceKey buffer);

	Buffer *BufferFromKey(ResourceKey key);

	void *BufferMap(ResourceKey key);
	u64 BufferAddress(ResourceKey key);

	void BufferRead(ResourceKey key, void *dst, u64 length, u64 offset);
	void BufferWrite(ResourceKey key, const void *src, u64 length, u64 offset);

	u64 BufferSize(ResourceKey key);


	/* ==================================================
	   SAMPLERS
	   ================================================== */

	ResourceKey SamplerCreate(const SamplerCreateInfo &create_info);
	ResourceKey SamplerCreateF(VkFilter filter);

	void SamplerDestroy(ResourceKey key);

	Sampler *SamplerFromKey(ResourceKey key);

	unsigned SamplerBindless(ResourceKey key);


	/* ==================================================
	   SHADERS
	   ================================================== */

	ShaderStage ShaderStageCreate(const ShaderBytecode &bytecode);

	ResourceKey ShaderProgramCreate(unsigned stage_count, const ShaderBytecode *stages);
	void ShaderProgramDestroy(ResourceKey key);

	ShaderProgram *ShaderProgramFromKey(ResourceKey key);


	/* ==================================================
	   ACCELERATION STRUCTURES
	   ================================================== */

	// Bottom-Level Acceleration Structure -- vertex / index data
	AccelStructReceipt BLASAlloc(const BLASGeometry *geometries, unsigned geometry_count);

	// Top-Level Acceleration Structure -- objects
	AccelStructReceipt TLASAlloc(unsigned max_instance_count);

	void AccelStructDestroy(ResourceKey key);

	u64 AccelStructAddress(ResourceKey key);

	AccelStruct *AccelStructFromKey(ResourceKey key);


	/* ==================================================
	   IMGUI
	   ================================================== */

	bool ImGuiEnabled() const
	{
		return imgui_enabled;
	}

	void ImGuiNewFrame();
	void ImGuiRecord(const CmdBuffer &cmd) const;

	u64 ImGuiTextureIdForView(ResourceKey view); // The view must be in GENERAL layout whenever ImGui draws it.
	void ImGuiForgetTextureView(ResourceKey view);


	/* ==================================================
	   STATE
	   ================================================== */

	VkDevice GetVkDevice() const
	{
		return vk_device;
	}

	VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const
	{
		return vk_physical_device_properties.properties;
	}

	VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures() const
	{
		return vk_physical_device_features.features;
	}

	const Bindless &GetBindless() const
	{
		return bindless;
	}

	u64 GetMaxPushConstantsSize() const
	{
		return max_push_constants_size;
	}

	VkFormat GetDefaultDepthStencilFormat() const
	{
		return default_depth_stencil_format;
	}

	unsigned GetFrameInFlightIndex() const
	{
		return current_frame_in_flight_index;
	}

private:
	void ApplyBindlessUpdates();

	void CreateSyncResources();
	void DestroySyncResources();

	void CreateBindless();
	void DestroyBindless();

	void CreateImGui();
	void DestroyImGui();

	PendingDestroy &PushPendingDestroyLocked(PendingDestroyKind kind, ResourceKey key);
	void DestroyPendingNow(const PendingDestroy &d);

	PipelineState FetchGraphicsPipelineUnlocked(const GraphicsPipelineDef &def);
	PipelineState FetchComputePipelineUnlocked(const ComputePipelineDef &def);

	ResourceKey TextureViewFetchUnlocked(const TextureViewCreateInfo &info);

	os::Window *window;

public:
	LogChannel log_channel;
	LogChannel log_channel_general;
	LogChannel log_channel_validation;
	LogChannel log_channel_performance;
	LogChannel log_channel_cmd_buffer;

private:
	VkInstance vk_instance;
	VkDevice vk_device;

	VkPhysicalDevice vk_physical_device;
	VkPhysicalDeviceProperties2 vk_physical_device_properties;
	VkPhysicalDeviceFeatures2 vk_physical_device_features;

	VkSurfaceKHR vk_surface;

	VmaAllocator vma_allocator;

	HardwareQueue graphics_queue;

	VkPipelineCache pipeline_process_cache;

	bool has_validation_layers;
	VkDebugUtilsMessengerEXT debug_messenger;

	VkFormat default_depth_stencil_format;
	VkSampleCountFlagBits max_msaa_samples;
	u64 max_push_constants_size; // todo: move into a seperate limits struct

	ResolvedFeatures features;

	SwapchainSupportDetails swapchain_details;

	Bindless bindless;

	unsigned current_frame_in_flight_index;
	FrameInFlight frames_in_flight[GRAPHICS_FRAMES_IN_FLIGHT];

	std::atomic<i64> retire_frame;

	SpinLock destroy_queue_lock;
	Deque<PendingDestroy> destroy_queue;

	// this quards vkQueueXXX2() operations and the timeline value allocation,
	// as it must reach the queue in order!
	SpinLock queue_lock;

	// for blocking/immediate submits which must be carried out one at a time
	// and have their own command pool
	// so they never touch the render side's per-frame pools.
	job::FiberMutex immediate_mutex;
	CmdPool *immediate_pool;

	SpinLock bindless_apply_lock;

	SpinLock view_cache_lock;
	SpinLock pipeline_cache_lock;

	ResourceList<VkPipelineLayout> pipeline_layouts;
	ResourceList<VkPipeline> pipelines;
	ResourceList<Texture> textures;
	ResourceList<TextureView> texture_views;
	ResourceList<Buffer> buffers;
	ResourceList<Sampler> samplers;
	ResourceList<ShaderProgram> shaders;
	ResourceList<AccelStruct> accel_structures;

	Semaphore graphics_semaphore;

	bool imgui_enabled;
	VkDescriptorPool imgui_pool;
	HashMap<u64, VkDescriptorSet> imgui_textures;
};

}

#endif // GRAPHICS_DEVICE_H
