#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

typedef struct GFX_ShaderCompiledStages GFX_ShaderCompiledStages;
struct GFX_ShaderCompiledStages
{
	b32 failed;
	u32 count;
	GFX_ShaderBytecode *bytecodes;
};

typedef struct GFX_ShaderCompilerAPI GFX_ShaderCompilerAPI;
struct GFX_ShaderCompilerAPI
{
	void (*Init)(void);
	void (*Shutdown)(void);
	
	GFX_ShaderCompiledStages (*Compile)(Arena *arena,
										String8 source_path,
										u32 search_path_count, const String8 *search_paths);
};

internal const GFX_ShaderCompilerAPI *GFX_GetShaderCompilerAPI(void);

#endif // GRAPHICS_SHADER_COMPILER_H
