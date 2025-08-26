
internal IndirectBatch *
MeshPassCompactDrawsToBatches(MeshPass *pass, Scene *scene, MemoryArena *arena)
{
	SceneObject *render_object = SceneObjectFromHandle(scene, pass->direct_batches->object_id);
	
	IndirectBatch *batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
	batch->mesh_id = render_object->mesh_id;
	batch->material_id = render_object->material_id;
	batch->first = 0;
	batch->count = 1;
	
	u32 i = 1;
	for(DirectBatch *curr = pass->direct_batches->next; curr; curr = curr->next, i++)
	{
		render_object = SceneObjectFromHandle(scene, curr->object_id);
		
		b32 are_same_mesh     = render_object->mesh_id     == batch->mesh_id;
		b32 are_same_material = render_object->material_id == batch->material_id;
		
		if(are_same_mesh && are_same_material)
		{
			batch->count++;
		}
		else
		{
			IndirectBatch *new_batch = MemoryArenaPush(arena, sizeof(IndirectBatch));
			new_batch->next = batch;
			new_batch->mesh_id = render_object->mesh_id;
			new_batch->material_id = render_object->material_id;
			new_batch->first = i;
			new_batch->count = 1u;
			
			batch = new_batch;
		}
	}
	
	return batch;
}

internal MeshPass
MeshPassInit(Scene *scene,
			 MemoryArena *arena)
{
	MeshPass pass = {0};
	
	// NOTE(kp): RENDER BATCHES.
	{
		for(i32 i = 0; i < scene->object_count; i++)
		{
			DirectBatch *direct_batch = MemoryArenaPush(arena, sizeof(DirectBatch));
			direct_batch->next = pass.direct_batches;
			direct_batch->object_id = i;
			pass.direct_batches = direct_batch;
		}
	}
	
	// NOTE(kp): BATCHES.
	{
		pass.batches = MeshPassCompactDrawsToBatches(&pass, scene, arena);
	}
	
	// NOTE(kp): MULTI BATCHES.
	{
		/*
		MultiBatch *multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
		multi_batch->next = 0;
		multi_batch->count = 1;
		multi_batch->first = 0;
		pass.multi_batches = multi_batch;
		
		u32 i = 1;
		for(IndirectBatch *batch = pass.batches->next; batch; batch = batch->next, i++)
		{
			// NOTE(kp): Iterate up to the multi_batch->first'th batch.
			IndirectBatch *join_batch = pass.batches;
			for(i32 j = 0; j < multi_batch->first; j++, join_batch = join_batch->next);
			
			b32 compatible_mesh = SceneMeshFromHandle(scene, join_batch->mesh_id)->is_merged;
			b32 same_material = join_batch->material_id == batch->material_id;
			
			// NOTE(kp): As long as the materials are the same and the mesh
			//           has been merged then we can combine the rendering
			//           calls together.
			if(compatible_mesh && same_material)
			{
				multi_batch->count++;
			}
			else
			{
				multi_batch = MemoryArenaPush(arena, sizeof(MultiBatch));
				multi_batch->next = pass.multi_batches;
				multi_batch->count = 1u;
				multi_batch->first = i;
				pass.multi_batches = multi_batch;
			}
		}
*/
	}
	
	return pass;
}
