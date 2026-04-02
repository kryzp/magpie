
internal void
AST_Init(AST_Assets *assets, GFX_Device *device)
{
	// TODO
}

internal void
AST_Destroy(AST_Assets *assets)
{
	// TODO
}

internal void
AST_PollHotReloads(AST_Assets *assets)
{
	// TODO
}

internal void
AST_WaitForAsyncUploads(AST_Assets *assets)
{
	// TODO
}

internal void
AST_FlushUploads(AST_Assets *assets)
{
	// TODO
}

internal void
AST_Mount(AST_Assets *assets, String8 prefix, String8 directory)
{
	// TODO
}

internal String8
AST_GetSystemFilePath(AST_Assets *assets, Arena *arena, String8 path)
{
	// TODO
}

internal b32
AST_IsLoaded(const AST_Assets *assets, AST_Handle handle)
{
	// TODO
}

internal b32
AST_IsLoading(const AST_Assets *assets, AST_Handle handle)
{
	// TODO
}

internal b32
AST_IsValid(const AST_Assets *assets, AST_Handle handle)
{
	// TODO
}

internal void
AST_LoadNow(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	// TODO
}

internal void
AST_LoadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	// TODO
}

internal void
AST_ReloadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	// TODO
}

internal void
AST_LoadDirectEx(AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter)
{
	// TODO
}

internal void
AST_PushUpload(AST_Assets *assets, const AST_Upload *upload)
{
	// TODO
}

internal void
AST_PushForDependencyResolution(AST_Assets *assets, const AST_Upload *upload)
{
	// TODO
}

internal AST_Asset *
AST_AssetCreate(AST_Assets *assets, String8 path)
{
	// TODO
}

internal void
AST_AssetDestroy(AST_Handle handle)
{
	// TODO
}

internal AST_Asset *
AST_AssetGet(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	// TODO
}

internal void
AST_AssetUnload(AST_Asset *asset)
{
	// TODO
}

internal AST_Handle
AST_FromFilePath(AST_Assets *assets, String8 path)
{
	// TODO
}
