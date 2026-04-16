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

internal String8 AST_ContextSystemFilePath(const AST_Context *context);

typedef struct AST_Serializer AST_Serializer;
struct AST_Serializer
{
	AST_LoadData (*Load)      (const AST_Context *ctx);
	void         (*Finalize)  (const AST_Context *ctx, AST_LoadData *data, GFX_Device *device);
	void         (*HotReload) (const AST_Context *ctx, AST_LoadData *data, AST_Asset *existing, GFX_Device *device);
	void         (*Upload)    (const AST_Context *ctx, AST_LoadData *data, AST_Asset *asset, GFX_CmdBuffer *cmd, GFX_Buffer *stage, u64 stage_base);
	void         (*Dispose)   (AST_LoadData *data);
};

#endif // ASSET_SERIALIZER_H
