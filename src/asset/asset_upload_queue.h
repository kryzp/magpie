#ifndef ASSET_UPLOAD_QUEUE_H
#define ASSET_UPLOAD_QUEUE_H

#define AST_UPLOAD_QUEUE_MAX_ELEMENTS 64

typedef struct AST_Upload AST_Upload;
struct AST_Upload
{
	u32 load_arena_index;
	AST_MetaData metadata;
	AST_Handle handle;
	AST_Type type;
	AST_LoadData load_data;
};

typedef struct AST_UploadQueue AST_UploadQueue;
struct AST_UploadQueue
{
	AST_Upload elements[AST_UPLOAD_QUEUE_MAX_ELEMENTS];
	u32 count;
};

internal void AST_UploadQueuePush  (AST_UploadQueue *queue, const AST_Upload *upload);
internal void AST_UploadQueueClear (AST_UploadQueue *queue);

#endif // ASSET_UPLOAD_QUEUE_H
