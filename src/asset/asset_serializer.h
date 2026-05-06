#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

typedef struct AST_Assets AST_Assets;

typedef struct AST_Context AST_Context;
struct AST_Context
{
	Arena *scope;
	AST_Assets *assets;
	AST_MetaData metadata;
	LOG_Channel log_channel;
};

typedef struct AST_SerializerPipelineData AST_SerializerPipelineData;
struct AST_SerializerPipelineData
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
	AST_SerializerPipelineData (*Cpu)      (const AST_Context *ctx);
	void                       (*Alloc)    (const AST_Context *ctx, AST_SerializerPipelineData *data, AST_Asset *out);
	void                       (*Reload)   (const AST_Context *ctx, AST_SerializerPipelineData *data, AST_Asset *existing);
	void                       (*Gpu)      (const AST_Context *ctx, AST_SerializerPipelineData *data, AST_Asset *asset, GFX_CmdBuffer *cmd, GFX_BufferKey stage, u64 stage_base);
	void                       (*End)      (AST_SerializerPipelineData *data);
	void                       (*Dispose)  (AST_Asset *asset, AST_Assets *assets);
};

#endif // ASSET_SERIALIZER_H
