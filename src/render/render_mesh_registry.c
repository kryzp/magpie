
static void R_MeshRegistryInit(R_MeshRegistry *r, Arena *arena, LOG_Channel log_channel)
{
	r->arena = arena;
	r->log_channel = log_channel;

	for (u32 i = 0; i < ArraySize(r->mesh_slots); i++)
		r->mesh_slots[i].generation = 1;

	for (i32 i = ArraySize(r->mesh_slots) - 1; i > 0; i--)
		r->mesh_free_list[r->mesh_free_count++] = i - 1;

	G_BufferAllocInfo mesh_buffer_alloc_info = {0};
	mesh_buffer_alloc_info.usage = VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT;
	mesh_buffer_alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	mesh_buffer_alloc_info.size = sizeof(R_GPU_RenderMesh) * ArraySize(r->mesh_slots);

	r->mesh_buffer = G_DeviceBufferAlloc(&mesh_buffer_alloc_info);

	r->mesh_buffer_dirty = true;
}

static void R_MeshRegistryDestroy(R_MeshRegistry *r)
{
	G_DeviceBufferDestroy(r->mesh_buffer);

	for (u32 i = 0; i < r->geometry_page_count; i++)
	{
		G_DeviceBufferDestroy(r->geometry_pages[i].vertex_buffer);
		G_DeviceBufferDestroy(r->geometry_pages[i].index_buffer);
	}
}

static R_SceneHandle R_MeshRegistryCreateMesh(R_MeshRegistry *r, G_CmdBuffer *cmd, const R_MeshDesc *desc)
{
	DebugLogAssert(r->log_channel,
				   r->mesh_free_count > 0,
				   "Ran out of free mesh slots.");

	r->mesh_free_count--;
	u32 slot_index = r->mesh_free_list[r->mesh_free_count];
	R_MeshSlot *slot = &r->mesh_slots[slot_index];

	u32 page_index = R_MeshRegistryFindSuitablePage(r, desc->vertex_count, desc->index_count);
	R_GeometryPage *page = &r->geometry_pages[page_index];

	u64 vertex_offset = 0;
	u64 index_offset = 0;
	
	b32 vok = R_GeometryFreeListTryAlloc(&page->vertex_free, desc->vertex_count, &vertex_offset);
	b32 iok = R_GeometryFreeListTryAlloc(&page->index_free,  desc->index_count,  &index_offset);

	DebugLogAssert(r->log_channel, vok, "Vertex region allocation failed after R_GeometryFreeListAvailable returned true.");
	DebugLogAssert(r->log_channel, iok, "Index region allocation failed after R_GeometryFreeListAvailable returned true.");
	
	const u64 vertex_stride = sizeof(R_GPU_ModelVertex);
	const u64 index_stride  = sizeof(A_ModelIndex);

	G_BufferCopy vc = {0};
	vc.src_offset = 0;
	vc.dst_offset = vertex_offset * vertex_stride;
	vc.size = desc->vertex_count * vertex_stride;

	G_BufferCopy ic = {0};
	ic.src_offset = 0;
	ic.dst_offset = index_offset * index_stride;
	ic.size = desc->index_count * index_stride;
	
	G_CmdCopyBufferToBuffer(cmd, desc->vertex_buffer, page->vertex_buffer, 1, &vc);
	G_CmdCopyBufferToBuffer(cmd, desc->index_buffer,  page->index_buffer,  1, &ic);

	R_GPU_RenderMesh *gpu_mesh = &r->mesh_gpus[slot_index];
	gpu_mesh->index_count = desc->index_count;
	gpu_mesh->first_index = index_offset;
	gpu_mesh->vertex_buffer = G_DeviceBufferAddress(page->vertex_buffer) + (vertex_offset * sizeof(R_GPU_ModelVertex));

	if (!G_BufferKeyIsNull(desc->skin_buffer))
		gpu_mesh->skin_buffer = G_DeviceBufferAddress(desc->skin_buffer);
	else
		gpu_mesh->skin_buffer = 0;
	
	page->vertex_count += desc->vertex_count;
	page->index_count += desc->index_count;

	slot->page_index = page_index;
	slot->vertex_offset = vertex_offset;
	slot->vertex_count = desc->vertex_count;
	slot->index_offset = index_offset;
	slot->index_count = desc->index_count;
	slot->active = true;
	
	r->mesh_count++;
	r->mesh_buffer_dirty = true;

	R_SceneHandle handle = {0};
	handle.index = slot_index;
	handle.generation = slot->generation;
	
	return handle;
}

static void R_MeshRegistryDestroyMesh(R_MeshRegistry *r, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(r->mesh_slots))
		return;

	R_MeshSlot *slot = &r->mesh_slots[handle.index];

	if (!slot->active || slot->generation != handle.generation)
		return;

	R_GeometryPage *page = &r->geometry_pages[slot->page_index];

	R_GeometryFreeListRelease(&page->vertex_free, slot->vertex_offset, slot->vertex_count);
	R_GeometryFreeListRelease(&page->index_free,  slot->index_offset,  slot->index_count);

	slot->active = false;
	slot->generation++;

	MemZeroStruct(&r->mesh_gpus[handle.index]);
	r->mesh_buffer_dirty = true;

	r->mesh_free_list[r->mesh_free_count++] = handle.index;
	r->mesh_count--;
}

static u32 R_MeshRegistryCountOfMeshes(const R_MeshRegistry *r)
{
	return r->mesh_count;
}

static b32 R_MeshRegistryHandleIsValid(const R_MeshRegistry *r, R_SceneHandle handle)
{
	if (handle.index >= ArraySize(r->mesh_slots))
		return false;

	const R_MeshSlot *slot = &r->mesh_slots[handle.index];
	return slot->active && slot->generation == handle.generation;
}

static void R_MeshRegistryFlushIfDirty(R_MeshRegistry *r)
{
	if (!r->mesh_buffer_dirty)
		return;

	G_DeviceBufferWrite(r->mesh_buffer, r->mesh_gpus, sizeof(r->mesh_gpus), 0);

	r->mesh_buffer_dirty = false;
}

static u32 R_MeshRegistryFindSuitablePage(R_MeshRegistry *r, u32 vertex_count, u32 index_count)
{
	for (u32 i = 0 ; i < r->geometry_page_count; i++)
	{
		R_GeometryPage *page = &r->geometry_pages[i];

		if (R_GeometryFreeListAvailable(&page->vertex_free, vertex_count) &&
			R_GeometryFreeListAvailable(&page->index_free, index_count))
		{
			return i;
		}
	}
	
	DebugLogAssert(r->log_channel,
				   r->geometry_page_count < ArraySize(r->geometry_pages),
				   "Exhausted all possible geometry pages.");

	u32 new_index = r->geometry_page_count;

	r->geometry_pages[new_index] = R_MeshRegistryCreateNewPage(r);
	r->geometry_page_count++;

	return new_index;
}

static R_GeometryPage R_MeshRegistryCreateNewPage(R_MeshRegistry *r)
{
	DebugLogD(r->log_channel, "Creating new geometry page...");

	// We use vertex pulling so don't need to use VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT.
	G_BufferAllocInfo vb_info = {0};
	vb_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
		vb_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	vb_info.size = R_GEOMETRY_PAGE_VERTEX_BUFFER_SIZE;
 
	G_BufferAllocInfo ib_info = {0};
	ib_info.usage =
		VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
		VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
		VK_BUFFER_USAGE_2_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
	ib_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	ib_info.size = R_GEOMETRY_PAGE_INDEX_BUFFER_SIZE;

	u32 max_vertices = vb_info.size / sizeof(R_GPU_ModelVertex);
	u32 max_indices  = ib_info.size / sizeof(A_ModelIndex);
	
	R_GeometryPage page = {0};
	page.vertex_buffer = G_DeviceBufferAlloc(&vb_info);
	page.index_buffer = G_DeviceBufferAlloc(&ib_info);
	page.vertex_count = 0;
	page.index_count = 0;
	page.max_vertices = max_vertices;
	page.max_indices = max_indices;

	R_GeometryFreeListInit(&page.vertex_free, max_vertices);
	R_GeometryFreeListInit(&page.index_free, max_indices);
 
	return page;
}

static u32 R_MeshRegistryPageCount(const R_MeshRegistry *r)
{
	return r->geometry_page_count;
}
