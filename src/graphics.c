
internal VkSemaphore GetCurrentImageAvailableSemaphore()
{
	return graphics_device->frames[graphics_device->current_frame_index].image_available_semaphore;
}

internal VkSemaphore GetCurrentRenderFinishedSemaphore()
{
	return graphics_device->frames[graphics_device->current_frame_index].render_finished_semaphore;
}

#include "bindless.c"
#include "vertex_format.c"
#include "sync.c"
#include "sampler.c"
#include "image.c"
#include "gpu_buffer.c"
#include "shader.c"
#include "blend.c"
#include "pipeline.c"
#include "swapchain.c"
#include "command_buffer.c"
#include "command_pool.c"
#include "graphics_device.c"
