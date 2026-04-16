
internal String8
AST_ContextSystemFilePath(const AST_Context *context, Arena *arena)
{
	return AST_GetSystemFilePath(context->assets, arena, context->metadata.path);
}
