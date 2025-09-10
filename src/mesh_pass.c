
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
