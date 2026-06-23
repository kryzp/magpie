
static void A_UploadQueuePush(A_UploadQueue *queue, const A_Upload *upload)
{
	AssertTrue(queue->count < ArraySize(queue->elements));
	
	queue->elements[queue->count] = *upload;
	queue->count++;
}

static void A_UploadQueueClear(A_UploadQueue *queue)
{
	queue->count = 0;
}
