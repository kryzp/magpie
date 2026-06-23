#ifndef ASSET_SERIALIZER_H
#define ASSET_SERIALIZER_H

typedef struct A_Registry A_Registry;

typedef struct A_MetaData A_MetaData;
struct A_MetaData
{
	String8 path;
};

typedef struct A_Context A_Context;
struct A_Context
{
	A_Registry *assets;
	A_MetaData metadata;
	LOG_Channel log_channel;
};

typedef struct A_SerializerPipelineData A_SerializerPipelineData;
struct A_SerializerPipelineData
{
	void *data;
	
	u64 stage_size;
	b32 failed;
	
	u32 dependency_count;
	A_Handle *dependencies;

	u32 watch_path_count;
	String8 *watch_paths;
};

static String8 A_ContextSystemFilePath(const A_Context *context, Arena *arena);

typedef struct A_Serializer A_Serializer;
struct A_Serializer
{
	A_SerializerPipelineData (*Cpu)      (const A_Context *ctx, Arena *load_scope);
	void                       (*Alloc)    (const A_Context *ctx, A_SerializerPipelineData *data, A_Asset *out, Arena *arena);
	void                       (*Reload)   (const A_Context *ctx, A_SerializerPipelineData *data, A_Asset *existing);
	void                       (*Gpu)      (const A_Context *ctx, A_SerializerPipelineData *data, A_Asset *asset, G_CmdBuffer *cmd, G_BufferKey stage, u64 stage_base);
	void                       (*End)      (A_SerializerPipelineData *data);
	void                       (*Dispose)  (A_Asset *asset, A_Registry *assets);
};

#endif // ASSET_SERIALIZER_H
