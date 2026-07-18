
typedef struct A_ScriptLoadData A_ScriptLoadData;
struct A_ScriptLoadData
{
	S_Ref ref;
};

static A_LoadResult A_ScriptLoaderLoad(const A_LCTX *ctx,
									   Arena *result_arena)
{
	ScratchArena scratch = ScratchBegin(NULL, 0);
	
	String8 file_path = A_GetSystemFilePath(scratch.arena, ctx->metadata.path);

	A_ScriptLoadData *script = ArenaPushArray(result_arena, A_ScriptLoadData, 1);
	script->ref = S_RefNull();
	
	IO_ByteSpan source_bytes = IO_ReadEntireFile(scratch.arena, file_path);
	S_Ref chunk_ref = S_Compile(source_bytes, file_path);

	if (!S_RefIsNull(chunk_ref))
	{
		script->ref = S_ExecuteModule(chunk_ref);
		S_Release(chunk_ref);
	}
	
	A_LoadResult result = {0};
	result.user_data = script;
	result.stage_size = 0;
	result.failed = false;

	ScratchRelease(&scratch);
	
	return result;
}

static void A_ScriptLoaderAlloc(const A_LCTX *ctx,
								A_LoadResult *result,
								Arena *asset_arena,
								A_Asset *asset)
{
	A_ScriptLoadData *script = result->user_data;

	asset->script.ref = script->ref;
}

static A_LoaderAPI A_GetScriptLoaderAPI(void)
{
	static A_LoaderAPI script_loader_api = {
		.Load = A_ScriptLoaderLoad,
		.Alloc = A_ScriptLoaderAlloc,
		.UploadGPU = NULL,
		.DestroyIntermediateResources = NULL,
		.DestroyAsset = NULL,
	};

	return script_loader_api;
}
