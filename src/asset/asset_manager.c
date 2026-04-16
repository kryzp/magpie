
internal void
AST_Init(AST_Assets *assets, Arena *arena, GFX_Device *device)
{
	assets->arena = arena;
	assets->graphics_device = device;

	asset->async_upload_counter = osapi->JobCounterAlloc(arena, 0);

	DebugLogF("Assets Initialized.");
}

internal void
AST_Destroy(AST_Assets *assets)
{	
	DebugLogF("Assets Destroyed.");
}

internal void
AST_PollHotReloads(AST_Assets *assets)
{
}

internal void
AST_WaitForAsyncUploads(AST_Assets *assets)
{
	osapi->JobYield(assets->async_upload_counter, 0);
}

internal void
AST_FlushUploads(AST_Assets *assets)
{
}

internal u32
AST_AllocSlot(AST_Assets *assets)
{
	if (assets->free_index_count > 0)
		return assets->free_indices[--assets->free_index_count];

	AssertTrue(assets->record_count < ArraySize(assets->records));

	return assets->record_count++;
}

internal AST_Record *
AST_GetRecord(AST_Assets *assets, AST_Handle handle)
{
	return &assets->records[handle.index];
}

internal void
AST_Mount(AST_Assets *assets, String8 prefix, String8 directory)
{
	AST_MountPoint *mp = &assets->mount_points[assets->mount_point_count++];

	mp->prefix    = String8Clone(assets->arena, prefix);
	mp->directory = String8Clone(assets->arena, directory);
}

internal String8
AST_GetSystemFilePath(AST_Assets *assets, Arena *arena, String8 path)
{
}

internal b32
AST_IsLoaded(const AST_Assets *assets, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));

	return AST_StateIsLoaded(AST_GetRecord(assets, handle)->state);
}

internal b32
AST_IsLoading(const AST_Assets *assets, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));

	return AST_StateIsLoading(AST_GetRecord(assets, handle)->state);
}

internal b32
AST_IsValid(const AST_Assets *assets, AST_Handle handle)
{
}

internal void
AST_LoadNow(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal void
AST_LoadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal void
AST_ReloadAsync(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal void
AST_LoadDirectEx(AST_Assets *assets, AST_Handle handle, AST_Type type, JOB_Counter *counter)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal void
AST_PushUpload(AST_Assets *assets, const AST_Upload *upload)
{
}

internal void
AST_PushForDependencyResolution(AST_Assets *assets, const AST_Upload *upload)
{
}

internal AST_Asset *
AST_AssetCreate(AST_Assets *assets, String8 path)
{
}

internal void
AST_AssetDestroy(AST_Assets *assets, AST_Handle handle)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal AST_Asset *
AST_AssetGet(AST_Assets *assets, AST_Handle handle, AST_Type type)
{
	AssertTrue(AST_IsValid(assets, handle));
}

internal void
AST_AssetUnload(AST_Assets *assets, AST_Asset *asset)
{
}

internal AST_Handle
AST_FromFilePath(AST_Assets *assets, String8 path)
{
}
