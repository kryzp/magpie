#include "render/graph/graph_types.h"

rnd::TextureInfo rnd::TextureInfo::Absolute(VkFormat format, const Vec3 &size)
{
	TextureInfo info;
	info.format = format;
	info.size_class = SizeClass_Absolute;
	info.relative_to = GraphTexHandle();
	info.size_x = size.x;
	info.size_y = size.y;
	info.size_z = size.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.flags = 0;

	return info;
}

rnd::TextureInfo rnd::TextureInfo::Swapchain(VkFormat format, const Vec3 &factor)
{
	TextureInfo info;
	info.format = format;
	info.size_class = SizeClass_SwapchainRelative;
	info.relative_to = GraphTexHandle();
	info.size_x = factor.x;
	info.size_y = factor.y;
	info.size_z = factor.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.flags = 0;

	return info;
}

rnd::TextureInfo rnd::TextureInfo::Relative(VkFormat format, const Vec3 &factor, GraphTexHandle relative_to)
{
	TextureInfo info;
	info.format = format;
	info.size_class = SizeClass_Relative;
	info.relative_to = relative_to;
	info.size_x = factor.x;
	info.size_y = factor.y;
	info.size_z = factor.z;
	info.mips = 1;
	info.layers = 1;
	info.samples = VK_SAMPLE_COUNT_1_BIT;
	info.flags = 0;

	return info;
}

bool rnd::TextureInfo::operator==(const TextureInfo &other) const
{
	return (
		format == other.format &&
		size_class == other.size_class &&
		relative_to == other.relative_to &&
		size_x == other.size_x &&
		size_y == other.size_y &&
		size_z == other.size_z &&
		mips == other.mips &&
		layers == other.layers &&
		samples == other.samples &&
		flags == other.flags
	);
}

bool rnd::BufferInfo::operator==(const BufferInfo &other) const
{
	return (
		size == other.size &&
		flags == other.flags &&
		usage == other.usage
	);
}
