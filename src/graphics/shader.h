#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H

#include "core/types.h"

#include "container/vector.h"

#include <volk/volk.h>

#define GRAPHICS_MAX_SHADER_STAGES 4

namespace gfx {

// spirv for a single stage.
struct ShaderBytecode {
	Vector<u8> bytes;
};

struct ShaderStage {
	ShaderBytecode bytecode;
	VkShaderStageFlagBits flags = static_cast<VkShaderStageFlagBits>(0);
	unsigned push_constant_size = 0;
};

struct ShaderProgram {
	unsigned push_constant_size = 0;
	unsigned stage_count = 0;
	ShaderStage stages[GRAPHICS_MAX_SHADER_STAGES];

	[[nodiscard]] bool IsCompute() const
	{
		return (stage_count == 1) && (stages[0].flags & VK_SHADER_STAGE_COMPUTE_BIT);
	}
};

}

#endif // GRAPHICS_SHADER_H
