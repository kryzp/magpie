#ifndef ASSET_UPLOAD_QUEUE_H
#define ASSET_UPLOAD_QUEUE_H

#define A_UPLOAD_QUEUE_MAX_ELEMENTS 512

typedef struct A_Upload A_Upload;
struct A_Upload
{
	u32 load_arena_index;
	A_MetaData metadata;
	A_Handle handle;
	A_SerializerPipelineData load_data;
};

typedef struct A_UploadQueue A_UploadQueue;
struct A_UploadQueue
{
	A_Upload elements[A_UPLOAD_QUEUE_MAX_ELEMENTS];
	u32 count;
};

internal void A_UploadQueuePush  (A_UploadQueue *queue, const A_Upload *upload);
internal void A_UploadQueueClear (A_UploadQueue *queue);

#endif // ASSET_UPLOAD_QUEUE_H
