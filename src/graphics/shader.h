#pragma once

#include <volk/volk.h>

#include "core/types.h"

namespace gfx
{
	struct ShaderBytecode {
		u8 *bytes;
		u64 size;
	};

	struct ShaderStage {
		VkShaderStageFlags type;
		VkShaderModule module;
		u32 push_constant_size;
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

		const ShaderStage &get_stage(u32 stage) const
		{
			return stages[stage];
		}

		u32 get_cookie() const
		{
			return cookie;
		}

	private:
		u32 push_constant_size;

		u32 stage_count;
		ShaderStage stages[2];

		u32 cookie;
	};
}
