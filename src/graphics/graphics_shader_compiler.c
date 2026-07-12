
static G_ShaderCompiler *g_shader_compiler = NULL;

static void G_ShaderCompilerLogCallback(const char *context,
							  const char *source,
							  const char *message,
							  void *user_data)
{
	DebugLogW(g_shader_compiler->log_channel,
			  "%s --> %s: %s",
			  source, context, message);
}

static void G_ShaderCompilerInitAndSelect(G_ShaderCompiler *compiler, LOG_Channel log_channel)
{
	compiler->log_channel = log_channel;

	compiler->mutex = osapi->FiberMutexCreate();

	SLANG_Init(&compiler->global_session);

	G_ShaderCompilerSelectContext(compiler);

	if (compiler->global_session)
		DebugLogI(compiler->log_channel, "Initialized.");
	else
		DebugLogB(compiler->log_channel, "Failed to initialize.");
}

static void G_ShaderCompilerShutdown(void)
{
	DebugLogI(g_shader_compiler->log_channel, "Shutting down...");
	
	SLANG_Shutdown(g_shader_compiler->global_session);

	osapi->FiberMutexDestroy(g_shader_compiler->mutex);

	g_shader_compiler = NULL;
}

static void G_ShaderCompilerSelectContext(G_ShaderCompiler *compiler)
{
	g_shader_compiler = compiler;
}

static G_ShaderCompiledStages G_ShaderCompilerCompile(Arena *arena,
													  String8 source_path,
													  u32 search_path_count, const String8 *search_paths)
{
	osapi->FiberMutexLock(g_shader_compiler->mutex);
	
	G_ShaderCompiledStages compiled = {0};
	compiled.failed = true;

	DebugLogAssert(g_shader_compiler->log_channel, g_shader_compiler->global_session, "Invalid global session.");

	ScratchArena scratch = ScratchBegin(&arena, 1);

	char *source_cstr = ArenaPushArray(scratch.arena, char, source_path.len + 1);
	MemCopy(source_cstr, source_path.str, source_path.len);
	source_cstr[source_path.len] = '\0';

	const char **search_path_cstrs = ArenaPushArray(scratch.arena, const char *, search_path_count);
	
	for (u32 i = 0; i < search_path_count; i++)
	{
		char *p = ArenaPushArray(scratch.arena, char, search_paths[i].len + 1);
		MemCopy(p, search_paths[i].str, search_paths[i].len);
		p[search_paths[i].len] = '\0';
		search_path_cstrs[i] = p;
	}

	SLANG_CompileResult bridge_result = SLANG_Compile(g_shader_compiler->global_session,
													  source_cstr,
													  search_path_count, search_path_cstrs,
													  G_ShaderCompilerLogCallback, NULL);

	if (bridge_result.failed)
		goto end;

	compiled.count = bridge_result.stage_count;
	compiled.bytecodes = ArenaPushArray(arena, G_ShaderBytecode, compiled.count);

	for (u32 i = 0; i < compiled.count; i++)
	{
		SLANG_StageResult *src = &bridge_result.stages[i];

		compiled.bytecodes[i].size = src->size;
		compiled.bytecodes[i].bytes = ArenaPushArray(arena, u8, src->size);

		MemCopy(compiled.bytecodes[i].bytes, src->bytes, src->size);
	}

	compiled.failed = false;

	/*
	DebugLogD(compiler->log_channel,
			  "Compiled shader: %.*s.",
			  String8VArg(source_path));
	*/

end:
	SLANG_FreeResult(&bridge_result);
	ScratchRelease(&scratch);

	osapi->FiberMutexUnlock(g_shader_compiler->mutex);
	
	return compiled;
}
