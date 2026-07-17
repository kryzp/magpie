#ifndef GRAPHICS_SHADER_COMPILER_H
#define GRAPHICS_SHADER_COMPILER_H

typedef struct G_ShaderCompiledStages G_ShaderCompiledStages;
struct G_ShaderCompiledStages
{
	b32 failed;
	u32 count;
	G_ShaderBytecode *bytecodes;
};

typedef struct G_ShaderCompiler G_ShaderCompiler;
struct G_ShaderCompiler
{
	LOG_Channel log_channel;
	OS_Handle mutex;
	void *global_session;
};

static void G_ShaderCompilerInitAndSelect(G_ShaderCompiler *compiler, LOG_Channel log_channel);
static void G_ShaderCompilerShutdown(void);
static void G_ShaderCompilerSelectContext(G_ShaderCompiler *compiler);

static G_ShaderCompiledStages G_ShaderCompilerCompile(Arena *arena,
													  String8 source_path,
													  u32 search_path_count, const String8 *search_paths);

#endif // GRAPHICS_SHADER_COMPILER_H
