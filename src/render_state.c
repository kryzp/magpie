
internal void RenderStateInit(RenderState *rs, GPUBuffer *material_buffer)
{
	rs->material_buffer = material_buffer;
}

internal void RenderStateDestroy(RenderState *rs)
{
	GPUBufferDestroy(&rs->merged_vertex_buffer);
	GPUBufferDestroy(&rs->merged_index_buffer);
}

internal u32 RenderStateUploadMesh(RenderState *rs, Mesh *mesh)
{
	for (i32 i = 0; i < rs->mesh_count; i++) {
		if (MeshesEqual(rs->meshes[i].original, mesh))
			return i;
	}

	PassMesh pass_mesh = {0};
	pass_mesh.original = mesh;
	pass_mesh.is_merged = false;
	pass_mesh.first_vertex = 0;
	pass_mesh.first_index = 0;
	pass_mesh.vertex_count = mesh->vertex_count;
	pass_mesh.index_count = mesh->index_count;

	rs->meshes[rs->mesh_count] = pass_mesh;

	return rs->mesh_count++;
}

internal u32 RenderStateUploadMaterial(RenderState *rs, Assets *assets, Material *material)
{
	for (i32 i = 0; i < rs->material_count; i++) {
		if (MaterialsEqual(&rs->materials[i], material))
			return i;
	}

	rs->materials[rs->material_count] = *material;
	
	GPU_Material gpu_material = {0};
	gpu_material.diffuse_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->diffuse_texture_handle))->resource_id;
	gpu_material.normal_texture             = FetchStandardImageView(AssetsImageFromHandle(assets, material->normal_texture_handle))->resource_id;
	gpu_material.emissive_texture           = FetchStandardImageView(AssetsImageFromHandle(assets, material->emissive_texture_handle))->resource_id;
	gpu_material.metallic_roughness_texture = FetchStandardImageView(AssetsImageFromHandle(assets, material->metallic_roughness_texture_handle))->resource_id;
	gpu_material.ambient_texture            = FetchStandardImageView(AssetsImageFromHandle(assets, material->ambient_texture_handle))->resource_id;

	GPUBufferWrite(rs->material_buffer, &gpu_material,
		       sizeof(GPU_Material),
		       sizeof(GPU_Material) * rs->material_count);

	return rs->material_count++;
}

internal u32 RenderStateMakeLight(RenderState *rs, Light *light)
{
	rs->lights[rs->light_count] = *light;
	return rs->light_count++;
}

internal void RenderStateMergeMeshes(RenderState *rs)
{
	if (rs->mesh_count <= 0)
		return;

	// All meshes in the list *should* have the same vertex format.
	// If they don't we have a bit of a problem :/.
	VertexFormat *vertex_format = rs->meshes->original->vertex_format;

	u32 total_vertices = 0;
	u32 total_indices = 0;

	for (i32 i = 0; i < rs->mesh_count; i++) {
		PassMesh *mesh = &rs->meshes[i];

		mesh->first_vertex = total_vertices;
		mesh->first_index = total_indices;

		total_vertices += mesh->vertex_count;
		total_indices += mesh->index_count;

		mesh->is_merged = true;
	}

	u64 vb_size = total_vertices * vertex_format->vertex_size;
	u64 ib_size  = total_indices * sizeof(u16);
	
	if (rs->merged_vertex_buffer.size < vb_size ||
	    rs->merged_index_buffer.size < ib_size) {
		
		GPUBufferDestroy(&rs->merged_vertex_buffer);
		GPUBufferDestroy(&rs->merged_index_buffer);
		
		rs->merged_vertex_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_VERTEX_BUFFER_BIT |
							  VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							  vb_size);
	
		rs->merged_index_buffer = GPUBufferAlloc(VK_BUFFER_USAGE_2_INDEX_BUFFER_BIT |
							 VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
							 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
							 ib_size);
	}
	
	CommandBuffer cmd = BeginGraphicsInstantSubmit();

	for (i32 i = 0; i < rs->mesh_count; i++) {
		PassMesh *mesh = &rs->meshes[i];

		VkBufferCopy vertex_copy = {0};
		vertex_copy.srcOffset = 0;
		vertex_copy.dstOffset = mesh->first_vertex * vertex_format->vertex_size;
		vertex_copy.size      = mesh->vertex_count * vertex_format->vertex_size;

		CmdCopyBufferToBuffer(&cmd,
				      &mesh->original->vertex_buffer,
				      &rs->merged_vertex_buffer,
				      1, &vertex_copy);
		
		VkBufferCopy index_copy = {0};
		index_copy.srcOffset = 0;
		index_copy.dstOffset = mesh->first_index * sizeof(u16);
		index_copy.size      = mesh->index_count * sizeof(u16);

		CmdCopyBufferToBuffer(&cmd,
				      &mesh->original->index_buffer,
				      &rs->merged_index_buffer,
				      1, &index_copy);
	}

	EndGraphicsInstantSubmit(&cmd);
}

internal void RenderStateFillInstancesArray(RenderState *rs, MeshPass *pass, GPU_Instance *instances)
{
	i32 i = 0;
	i32 index = 0;
	for (IndirectBatch *b = pass->batches; b; b = b->next) {
		DirectBatch *direct_batch = pass->direct_batches;
	        for (i32 j = 0; j < b->first; j++, direct_batch = direct_batch->next); // Iterate up to b->first'th batch.
		for (i32 k = 0; k < b->count; k++) {
			instances[index].object_id = direct_batch->object_id;
			instances[index].batch_id = i;
			direct_batch = direct_batch->next;
			index++;
		}
		i++;
	}
}

internal void RenderStateFillIndirectArray(RenderState *rs, MeshPass *pass, GPU_Indirect *indirects)
{
	i32 i = 0;
	for (IndirectBatch *b = pass->batches; b; b = b->next) {
		PassMesh *mesh = rs->meshes + b->mesh_id;
		
		indirects[i].command.firstInstance = b->first;
		indirects[i].command.instanceCount = 0; // This gets filled-in in the compute shader.
		indirects[i].command.vertexOffset = mesh->first_vertex;
		indirects[i].command.firstIndex = mesh->first_index;
		indirects[i].command.indexCount = mesh->index_count;

		indirects[i].batch_id = i;
		
		i++;
	}
}

internal IndirectBatch *MeshPassCompactDrawsToBatches(MeshPass *mesh_pass,
						      MemoryArena *arena,
						      Scene *scene)
{
	SceneObject *object = SceneObjectFromHandle(scene, mesh_pass->direct_batches->object_id);

	IndirectBatch *batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
	batch->mesh_id = object->mesh_id;
	batch->material_id = object->material_id;
	batch->first = 0;
	batch->count = 1;

	mesh_pass->batch_count = 1;
	
	u32 i = 1;
	for (DirectBatch *curr = mesh_pass->direct_batches->next; curr; curr = curr->next, i++) {
		
		object = SceneObjectFromHandle(scene, curr->object_id);

		b32 are_same_mesh     = object->mesh_id     == batch->mesh_id;
		b32 are_same_material = object->material_id == batch->material_id;

		if (are_same_mesh && are_same_material) {
			batch->count++;
		} else {
			IndirectBatch *new_batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
			new_batch->next = batch;
			new_batch->mesh_id = object->mesh_id;
			new_batch->material_id = object->material_id;
			new_batch->first = i;
			new_batch->count = 1;

			mesh_pass->batch_count++;
		}
	}

	return batch;
}

internal void MeshPassPopulate(MeshPass *pass,
			       MemoryArena *arena,
			       RenderState *rs,
			       Scene *scene)
{
	// RENDER BATCHES.
	pass->direct_batch_count = 0;
	pass->direct_batches = NULL;
	
	for (SceneObject *s = scene->objects; s; s = s->next) {
		if (s->mesh_id == SCENE_INVALID_HANDLE)
			continue;

		DirectBatch *direct_batch = MemoryArenaPush(arena, sizeof(DirectBatch));
		direct_batch->next = pass->direct_batches;
		direct_batch->object_id = s->id;
		pass->direct_batches = direct_batch;

		pass->direct_batch_count++;
	}

	// BATCHES.
	pass->batches = MeshPassCompactDrawsToBatches(pass, arena, scene);

	// MULTI BATCHES.
	MultiBatch *multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
	multi_batch->next = 0;
	multi_batch->count = 1;
	multi_batch->first = 0;
	pass->multi_batches = multi_batch;
		
	u32 i = 1;
	for (IndirectBatch *batch = pass->batches->next; batch; batch = batch->next, i++) {

		// Iterate up to the (multi_batch->first)'th batch.
		IndirectBatch *join_batch = pass->batches;
		for (i32 j = 0; j < multi_batch->first; j++, join_batch = join_batch->next);

		b32 compatible_mesh = rs->meshes[join_batch->mesh_id].is_merged;
		b32 same_material = join_batch->material_id == batch->material_id;
			
		// As long as the materials are the same and the mesh
		// has been merged then we can combine the rendering
		// calls together.
		if (compatible_mesh && same_material) {
			multi_batch->count++;
		} else {
			multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
			multi_batch->next = pass->multi_batches;
			multi_batch->count = 1;
			multi_batch->first = i;
			pass->multi_batches = multi_batch;
		}
	}
}
