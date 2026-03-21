#pragma once

#include <volk/volk.h>

#include "core/types.h"
#include "container/vector.h"
#include "container/string.h"

#include "shader.h"

namespace gfx
{
	struct CompiledShaderStage {
		ShaderBytecode bytecode;
		u32 push_constant_size;
		VkShaderStageFlags stage;
	};

	struct CompiledShaderProgram {
		Vector<CompiledShaderStage> stages;
		bool failed;
	};

	class IShaderCompiler {
	public:
		virtual ~IShaderCompiler() = default;
		virtual void init() = 0;
		virtual void shutdown() = 0;
		virtual CompiledShaderProgram compile(const char *source_path, const Vector<String> &search_paths) = 0;
	};

	IShaderCompiler *get_shader_compiler();
}
