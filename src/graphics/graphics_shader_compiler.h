#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

typedef struct GFX_CompiledShaderStage GFX_CompiledShaderStage;
struct GFX_CompiledShaderStage
{
	GFX_ShaderBytecode bytecode;
	u32 push_constant_size;
	VkShaderStageFlags stage;
};

typedef struct GFX_CompiledShaderProgram GFX_CompiledShaderProgram;
struct GFX_CompiledShaderProgram
{
	u32 stage_count;
	GFX_CompiledShaderStage *stages;
	b32 failed;
};

typedef struct GFX_ShaderCompilerAPI GFX_ShaderCompilerAPI;
struct GFX_ShaderCompilerAPI
{
	void (*Init)(void);
	void (*Shutdown)(void);
	GFX_ShaderProgram (*Compile)(String8 source_path, u32 search_path_count, const String8 *search_paths, b32 *failed);
};

internal GFX_ShaderCompilerAPI GFX_GetShaderCompilerAPI(void);

#endif // GRAPHICS_SHADER_COMPILER_H
