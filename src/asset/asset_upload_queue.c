
internal void
AST_UploadQueuePush(AST_UploadQueue *queue, const AST_Upload *upload)
{
	AssertTrue(queue->count < ArraySize(queue->elements));
	
	queue->elements[queue->count] = *upload;
	queue->count++;
}

internal void
AST_UploadQueueClear(AST_UploadQueue *queue)
{
	queue->count = 0;
}
