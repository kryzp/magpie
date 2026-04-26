
internal void
GFX_ShaderCompilerLogCallback(const char *context,
							  const char *source,
							  const char *message,
							  void *user_data)
{
	GFX_ShaderCompiler *self = user_data;
	
	DebugLogD(self->log_channel, "%s --> %s: %s", source, context, message);
}

internal void
GFX_ShaderCompilerInit(GFX_ShaderCompiler *compiler, LOG_Channel log_channel)
{
	SLANG_Init(&compiler->global_session);

	compiler->mutex = osapi->MutexCreate();
	
	compiler->log_channel = log_channel;

	if (compiler->global_session)
		DebugLogI(compiler->log_channel, "Initialized.");
	else
		DebugLogB(compiler->log_channel, "Failed to initialize.");
}

internal void
GFX_ShaderCompilerShutdown(GFX_ShaderCompiler *compiler)
{
	DebugLogI(compiler->log_channel, "Shutting down...");

	osapi->MutexDestroy(compiler->mutex);
	
	SLANG_Shutdown(compiler->global_session);

	compiler->global_session = NULL;
}

internal GFX_ShaderCompiledStages
GFX_ShaderCompilerCompile(GFX_ShaderCompiler *compiler,
						  Arena *arena,
						  String8 source_path,
						  u32 search_path_count, const String8 *search_paths)
{
	osapi->MutexLock(compiler->mutex);
	
	GFX_ShaderCompiledStages compiled = {0};
	compiled.failed = true;

	AssertTrue(compiler->global_session);

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

	SLANG_CompileResult bridge_result = SLANG_Compile(compiler->global_session,
													  source_cstr,
													  search_path_count,
													  search_path_cstrs,
													  GFX_ShaderCompilerLogCallback,
													  compiler);

	if (bridge_result.failed)
		goto end;

	compiled.count = bridge_result.stage_count;
	compiled.bytecodes = ArenaPushArray(arena, GFX_ShaderBytecode, compiled.count);

	for (u32 i = 0; i < compiled.count; i++)
	{
		SLANG_StageResult *src = &bridge_result.stages[i];

		compiled.bytecodes[i].size = src->size;
		compiled.bytecodes[i].bytes = ArenaPushArray(arena, u8, compiled.bytecodes[i].size);

		MemCopy(compiled.bytecodes[i].bytes, src->bytes, compiled.bytecodes[i].size);
	}

	compiled.failed = false;

	DebugLogD(compiler->log_channel,
			  "Compiled shader: %.*s.",
			  (i32)source_path.len, source_path.str);

end:
	SLANG_FreeResult(&bridge_result);
	ScratchRelease(&scratch);

	osapi->MutexUnlock(compiler->mutex);
	
	return compiled;
}
