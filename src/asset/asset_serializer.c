
internal String8
A_ContextSystemFilePath(const A_Context *context, Arena *arena)
{
	return A_GetSystemFilePath(context->assets, arena, context->metadata.path);
}
