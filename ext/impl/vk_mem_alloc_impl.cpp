// Vulkan Memory Allocator implementation.
// gfx::Device hands VMA every function pointer it needs (loaded by volk), so VMA
// must not try to find them itself.

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>
