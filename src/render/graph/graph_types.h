#ifndef RENDER_GRAPH_TYPES_H
#define RENDER_GRAPH_TYPES_H

#include "core/types.h"
#include "core/vec3.h"
#include "graphics/texture.h"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

// NOTE: Yes, we could just use one graph handle here, and everything
//       would work, in some cases it would even make code cleaner,
//       but it would also remove type safety and I don't wanna bother
//       with debugging the fact I accidentally passed a buffer into
//       a texture slot and vice versa. It also makes it obvious when
//       reading code what resource we are talking about.

// NOTE: Helps prevent bugs like reading a texture before
//       writing to it, which won't cause a crash but it will
//       result in complete garbage data being rendered.
//
//           Pass *bloom = graph->Add("Bloom", PassType_Compute);
//           bloom->ReadTextureCompute(hdr); // Reads before lighting writes to it!
//                                              This won't cause a crash but it will be garbage.
//
//           Pass *lighting = graph->Add("Lighting", PassType_Graphics);
//           lighting->WriteColour(hdr, &clear); // Actually writes to it.

// NOTE: 0 is reserved for the invalid handle so indices start at 1.
//       Graph::CreateTexture allocates version index 1+ for the initial version.

#define RENDER_GRAPH_INVALID_INDEX 0xFFFFFFFFu

namespace rnd {

struct GraphTexHandle {
	unsigned value = 0;

	bool IsNull() const
	{
		return value == 0;
	}

	bool operator==(const GraphTexHandle &other) const
	{
		return value == other.value;
	}

	bool operator!=(const GraphTexHandle &other) const
	{
		return value != other.value;
	}
};

struct GraphBufHandle {
	unsigned value = 0;

	bool IsNull() const
	{
		return value == 0;
	}

	bool operator==(const GraphBufHandle &other) const
	{
		return value == other.value;
	}

	bool operator!=(const GraphBufHandle &other) const
	{
		return value != other.value;
	}
};

enum SizeClass {
	SizeClass_Absolute,
	SizeClass_SwapchainRelative,
	SizeClass_Relative,
	SizeClass_MaxEnum
};

struct TextureInfo {
	VkFormat format = VK_FORMAT_UNDEFINED;
	SizeClass size_class = SizeClass_Absolute;
	GraphTexHandle relative_to;

	float size_x = 0.f;
	float size_y = 0.f;
	float size_z = 0.f;

	unsigned mips = 1;
	unsigned layers = 1;

	VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
	unsigned flags = 0;

	bool operator==(const TextureInfo &other) const;

	static TextureInfo Absolute(VkFormat format, const Vec3 &size);
	static TextureInfo Swapchain(VkFormat format, const Vec3 &factor);
	static TextureInfo Relative(VkFormat format, const Vec3 &factor, GraphTexHandle relative_to);
};

struct BufferInfo {
	u64 size = 0;
	VmaAllocationCreateFlags flags = 0;
	VkBufferUsageFlags2 usage = 0;

	BufferInfo() = default;

	BufferInfo(u64 size_, VmaAllocationCreateFlags flags_, VkBufferUsageFlags2 usage_)
		: size(size_)
		, flags(flags_)
		, usage(usage_)
	{
	}

	bool operator==(const BufferInfo &other) const;
};

struct ResourceState {
	VkPipelineStageFlags2 write_stage = 0; // stage of last unsynced write
	VkAccessFlags2 write_access = 0;       // WRITE access pending flush (0 = no pending writes)
	VkPipelineStageFlags2 read_stages = 0; // accumulated reader stages since last flush (for WAR)
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

}

#endif // RENDER_GRAPH_TYPES_H
