#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

typedef struct GFX_ShaderCompiler GFX_ShaderCompiler;
struct GFX_ShaderCompiler
{
	void (*Init)(void);
	void (*Shutdown)(void);
	GFX_ShaderProgram (*Compile)(String8 source_path, u32 search_path_count, const String8 *search_paths, b32 *failed);
};

GFX_ShaderCompiler GFX_GetShaderCompiler();

#endif // GRAPHICS_SHADER_COMPILER_H
