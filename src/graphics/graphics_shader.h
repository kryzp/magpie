#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H

#define G_MAX_SHADER_STAGES 4

// TODO: replace with IO_ByteSpan??? one part of me
//       wants to since they're the same data-wise
//       but in reality I think it might actually
//       be better to keep them seperate, since shader
//       bytecode is compiled rather than generated
//       from a file.

typedef struct G_ShaderBytecode G_ShaderBytecode;
struct G_ShaderBytecode
{
	u8 *bytes;
	u64 size;
};

typedef struct G_ShaderStage G_ShaderStage;
struct G_ShaderStage
{
	G_ShaderBytecode bytecode;
	VkShaderStageFlagBits flags;
	u32 push_constant_size;
};

typedef struct G_ShaderProgram G_ShaderProgram;
struct G_ShaderProgram
{
	u32 cookie;
	u32 push_constant_size;
	u32 stage_count;
	G_ShaderStage stages[G_MAX_SHADER_STAGES];
};

inline b32 G_ShaderProgramIsCompute(const G_ShaderProgram *program)
{
	return (program->stage_count == 1) && (program->stages[0].flags & VK_SHADER_STAGE_COMPUTE_BIT);
}

#endif // GRAPHICS_SHADER_H
