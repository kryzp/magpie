#pragma once

#include <volk/volk.h>

#include "core/types.h"

#include "container/hash_map.h"

#include "shader.h"
#include "texture.h"
#include "pipeline.h"

namespace gfx
{
	class Device;

	class ResourceCache {
	public:
		ResourceCache();
		~ResourceCache();

		void init(Device *device);
		void destroy();
		
		VkPipelineLayout fetch_pipeline_layout(const ShaderProgram *program);
		PipelineState fetch_pipeline(const GraphicsPipelineDef &def);
		PipelineState fetch_pipeline(const ComputePipelineDef &def);
		
		TextureView *fetch_texture_view(
			const Texture *texture,
			VkImageViewType type,
			const SubresourceRange &range
		);

		TextureView *fetch_texture_view_std(const Texture *texture);

	private:
		Device *device;
		
		HashMap<u64, VkPipelineLayout> pipeline_layout_cache;
		HashMap<u64, VkPipeline> pipeline_cache;
		HashMap<u64, TextureView *> texture_view_cache;
	};
}
