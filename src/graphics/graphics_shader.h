#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H

#define GFX_MAX_SHADER_STAGES 4

typedef struct GFX_ShaderBytecode GFX_ShaderBytecode;
struct GFX_ShaderBytecode
{
	u8 *bytes;
	u64 size;
};

typedef struct GFX_ShaderStage GFX_ShaderStage;
struct GFX_ShaderStage
{
	GFX_ShaderBytecode bytecode;
	VkShaderStageFlagBits flags;
	u32 push_constant_size;
};

typedef struct GFX_ShaderProgram GFX_ShaderProgram;
struct GFX_ShaderProgram
{
	u32 cookie;
	u32 push_constant_size;
	u32 stage_count;
	GFX_ShaderStage stages[GFX_MAX_SHADER_STAGES];
};

internal inline b32
GFX_ShaderProgramIsCompute(const GFX_ShaderProgram *program)
{
	return (program->stage_count == 1) && (program->stages[0].flags & VK_SHADER_STAGE_COMPUTE_BIT);
}

#endif // GRAPHICS_SHADER_H
