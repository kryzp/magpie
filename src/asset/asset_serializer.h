#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

typedef struct AST_Assets AST_Assets;

typedef struct AST_Context AST_Context;
struct AST_Context
{
	Arena *scope;
	AST_Assets *assets;
	AST_MetaData metadata;
};

typedef struct AST_LoadData AST_LoadData;
struct AST_LoadData
{
	void *data;
	
	u64 stage_size;
	b32 failed;
	
	u32 dependency_count;
	AST_Handle *dependencies;

	u32 watch_path_count;
	String8 *watch_paths;
};

internal String8 AST_ContextSystemFilePath(const AST_Context *context, Arena *arena);

typedef struct AST_Serializer AST_Serializer;
struct AST_Serializer
{
	AST_LoadData (*Cpu)     (const AST_Context *ctx);
	void         (*Alloc)   (const AST_Context *ctx, AST_LoadData *data, GFX_Device *device, AST_Asset *out);
	void         (*Reload)  (const AST_Context *ctx, AST_LoadData *data, GFX_Device *device, AST_Asset *existing);
	void         (*Gpu)     (const AST_Context *ctx, AST_LoadData *data, AST_Asset *asset, GFX_Device *device, GFX_CmdBuffer *cmd, GFX_Buffer *stage, u64 stage_base);
	void         (*End)     (AST_LoadData *data);
	void         (*Dispose) (AST_Asset *asset, GFX_Device *device);
};

#endif // ASSET_SERIALIZER_H
