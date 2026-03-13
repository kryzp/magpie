#include "resource_cache.h"
#include "vk_check.h"
#include "context.h"
#include "device.h"
#include "bindless.h"

#include "core/hash.h"

using namespace gfx;

ResourceCache::ResourceCache()
	: device(nullptr)
	, pipeline_layout_cache()
	, pipeline_cache()
	, texture_view_cache()
{
}

ResourceCache::~ResourceCache()
{
}

void ResourceCache::init(Device *device)
{
	this->device = device;
}

void ResourceCache::destroy()
{
	for (auto &[key, view] : texture_view_cache)
		device->destroy_texture_view(view);

	for (auto &[key, pipeline] : pipeline_cache)
		device->destroy_pipeline(pipeline);

	for (auto &[key, layout] : pipeline_layout_cache)
		device->destroy_pipeline_layout(layout);

	texture_view_cache.clear();
	pipeline_cache.clear();
	pipeline_layout_cache.clear();
}

VkPipelineLayout ResourceCache::fetch_pipeline_layout(const ShaderProgram *program)
{
	u64 hash = hash::generic(&program, sizeof(ShaderProgram *));

	if (pipeline_layout_cache.find(hash) == pipeline_layout_cache.end())
		pipeline_layout_cache[hash] = device->create_pipeline_layout(program);

	return pipeline_layout_cache[hash];
}

PipelineState ResourceCache::fetch_pipeline(const GraphicsPipelineDef &def)
{
	VkPipelineLayout layout = fetch_pipeline_layout(def.program);

	u64 hash = 0;

	hash = hash::generic_combine(hash, &def.program,                    sizeof(ShaderProgram *));
	hash = hash::generic_combine(hash, &def.cull_mode,                  sizeof(VkCullModeFlags));
	hash = hash::generic_combine(hash, &def.front_face,                 sizeof(VkFrontFace));
	hash = hash::generic_combine(hash, &def.blend_state,                sizeof(BlendState));
	hash = hash::generic_combine(hash, &def.depth_stencil_state,        sizeof(DepthStencilState));
	hash = hash::generic_combine(hash, &def.has_depth_attachment,       sizeof(bool));
	hash = hash::generic_combine(hash, &def.samples,                    sizeof(VkSampleCountFlagBits));
	hash = hash::generic_combine(hash, &def.min_sample_shading_enabled, sizeof(bool));
	hash = hash::generic_combine(hash, &def.min_sample_shading,         sizeof(float));
	hash = hash::generic_combine(hash, &def.multi_view_mask,                  sizeof(u32));

	for (auto &format : def.colour_attachment_formats)
		hash = hash::generic_combine(hash, &format, sizeof(format));

	if (pipeline_cache.find(hash) == pipeline_cache.end())
		pipeline_cache[hash] = device->create_pipeline(def, layout);

	PipelineState st = {};
	st.pipeline = pipeline_cache[hash];
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;

	return st;
}

PipelineState ResourceCache::fetch_pipeline(const ComputePipelineDef &def)
{
	VkPipelineLayout layout = fetch_pipeline_layout(def.program);

	u64 hash = hash::generic(&def.program, sizeof(ShaderProgram *));

	if (pipeline_cache.find(hash) == pipeline_cache.end())
		pipeline_cache[hash] = device->create_pipeline(def, layout);

	PipelineState st = {};
	st.pipeline = pipeline_cache[hash];
	st.layout = layout;
	st.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;

	return st;
}

TextureView *ResourceCache::fetch_texture_view(
	const Texture *texture,
	VkImageViewType type,
	const SubresourceRange &range
)
{
	u64 hash = 0;

	hash = hash::generic_combine(hash, &texture->get_handle(),  sizeof(VkImage)); // TODO: texture.get_cookie() function
	hash = hash::generic_combine(hash, &type,                   sizeof(VkImageViewType));
	hash = hash::generic_combine(hash, &range,                  sizeof(SubresourceRange));

	if (texture_view_cache.find(hash) == texture_view_cache.end())
		texture_view_cache[hash] = device->create_texture_view(texture, type, range);

	return texture_view_cache[hash];
}

TextureView *ResourceCache::fetch_texture_view_std(const Texture *texture)
{
	SubresourceRange range = {};
	range.aspects = texture->is_depth() ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
	range.base_mip = 0;
	range.mips = texture->get_mipmap_count();
	range.base_layer = 0;
	range.layers = texture->get_layer_count();

	return fetch_texture_view(
		texture, texture->get_default_view_type(),
		range
	);
}
