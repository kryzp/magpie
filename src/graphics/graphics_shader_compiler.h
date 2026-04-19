#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

typedef struct GFX_ShaderCompiledStages GFX_ShaderCompiledStages;
struct GFX_ShaderCompiledStages
{
	b32 failed;
	u32 count;
	GFX_ShaderBytecode *bytecodes;
};

typedef struct GFX_ShaderCompiler GFX_ShaderCompiler;
struct GFX_ShaderCompiler
{
	b32 temp;
};

internal void GFX_ShaderCompilerInit(GFX_ShaderCompiler *compiler);
internal void GFX_ShaderCompilerShutdown(GFX_ShaderCompiler *compiler);

internal GFX_ShaderCompiledStages GFX_ShaderCompilerCompile(GFX_ShaderCompiler *compiler,
															Arena *arena,
															String8 source_path,
															u32 search_path_count, const String8 *search_paths);

#endif // GRAPHICS_SHADER_COMPILER_H
