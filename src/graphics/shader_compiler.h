#pragma once

#include <volk/volk.h>

#include "shader.h"

namespace gfx
{
	struct CompiledShaderStage {
		ShaderBytecode bytecode;
		u32 push_constant_size;
		VkShaderStageFlags stage;
		bool failed;
	};

	class IShaderCompiler {
	public:
		virtual ~IShaderCompiler() = default;
		virtual void start() = 0;
		virtual void shutdown() = 0;
		virtual CompiledShaderStage compile(const char *path, const char *entry_point, VkShaderStageFlags stage_type) = 0;
	};

	IShaderCompiler *get_shader_compiler();
}
