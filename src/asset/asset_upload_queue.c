
internal void
A_UploadQueuePush(A_UploadQueue *queue, const A_Upload *upload)
{
	AssertTrue(queue->count < ArraySize(queue->elements));
	
	queue->elements[queue->count] = *upload;
	queue->count++;
}

internal void
A_UploadQueueClear(A_UploadQueue *queue)
{
	queue->count = 0;
}
