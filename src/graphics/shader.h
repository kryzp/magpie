#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{
	constexpr static u32 MAX_SHADER_STAGES = 4;

	struct ShaderBytecode {
		u8 *bytes;
		u64 size;
	};

	struct ShaderStage {
		VkShaderStageFlags flags;
		u32 push_constant_size;
		ShaderBytecode bytecode;
	};

	/*
	 * Shaders are assumed to be fully bindless.
	 */
	class ShaderProgram {
		friend class Device;

	public:
		ShaderProgram()
			: push_constant_size(0)
			, stage_count(0)
			, stages{}
			, cookie(0)
		{
		}

		~ShaderProgram() = default;

		bool is_compute() const
		{
			return stage_count == 1;
		}

		u32 get_push_constant_size() const
		{
			return push_constant_size;
		}

		u32 get_stage_count() const
		{
			return stage_count;
		}

		VkShaderStageFlags get_stage_flags(u32 stage) const
		{
			return stages[stage].flags;
		}

		ShaderBytecode get_stage_bytecode(u32 stage) const
		{
			return stages[stage].bytecode;
		}

		u32 get_cookie() const
		{
			return cookie;
		}

	private:
		u32 push_constant_size;

		u32 stage_count;
		ShaderStage stages[MAX_SHADER_STAGES];

		u32 cookie;
	};
}
