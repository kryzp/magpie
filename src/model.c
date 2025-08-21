
internal Mesh
MeshInit(VertexFormat *format,
		 u32 vertex_count, void *vertices,
		 u32 index_count, u16 *indices)
{
	Mesh mesh = {0};
	mesh.vertex_format = format;
	mesh.vertex_count = vertex_count;
	mesh.index_count = index_count;
	
	u64 vertex_buffer_size = vertex_count * format->vertex_size;
	u64 index_buffer_size = index_count * sizeof(u16);
	
	mesh.vertex_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										   vertex_buffer_size);
	
	mesh.index_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
										  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
										  index_buffer_size);
	
	GPUBuffer staging_buffer = GPUBufferAllocate(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
												 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
												 vertex_buffer_size + index_buffer_size);
	{
		GPUBufferWrite(&staging_buffer, vertices, vertex_buffer_size, 0);
		GPUBufferWrite(&staging_buffer, indices, index_buffer_size, vertex_buffer_size);
		
		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			VkBufferCopy stage_to_vertex_copy = {0};
			stage_to_vertex_copy.srcOffset = 0;
			stage_to_vertex_copy.dstOffset = 0;
			stage_to_vertex_copy.size = vertex_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.vertex_buffer,
								  1, &stage_to_vertex_copy);
			
			VkBufferCopy stage_to_index_copy = {0};
			stage_to_index_copy.srcOffset = vertex_buffer_size;
			stage_to_index_copy.dstOffset = 0;
			stage_to_index_copy.size = index_buffer_size;
			
			CmdCopyBufferToBuffer(&cmd,
								  &staging_buffer,
								  &mesh.index_buffer,
								  1, &stage_to_index_copy);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);
	
	return mesh;
}

internal void
MeshDestroy(Mesh *mesh)
{
	GPUBufferDestroy(&mesh->vertex_buffer);
	GPUBufferDestroy(&mesh->index_buffer);
}

internal void
MeshBindCmd(Mesh *mesh, CommandBuffer *cmd)
{
	CmdBindVertexBuffer(cmd, 0, &mesh->vertex_buffer, 0);
	CmdBindIndexBuffer(cmd, &mesh->index_buffer, 0);
}

internal void
MeshDrawCmd(Mesh *mesh, CommandBuffer *cmd)
{
	CmdDrawIndexed(cmd, mesh->index_count, 1, 0, 0, 0);
}

// ---

internal SubModel *
ModelCreateSubModel(Model *model)
{
	SubModel *sub_model = MemoryArenaPush(model->arena, sizeof(SubModel));
	sub_model->next = model->sub_models;
	sub_model->parent = model;
	
	model->sub_models = sub_model;
	model->sub_model_count++;
	
	return sub_model;
}

internal b32
AssimpMeshHasPositions(struct aiMesh *mesh)
{
	return mesh->mVertices && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasFaces(struct aiMesh *mesh)
{
	return mesh->mFaces && mesh->mNumFaces > 0;
}

internal b32
AssimpMeshHasNormals(struct aiMesh *mesh)
{
	return mesh->mNormals && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasTangentsAndBitangents(struct aiMesh *mesh)
{
	return mesh->mTangents && mesh->mBitangents && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasTextureCoords(struct aiMesh *mesh, u32 index)
{
	return mesh->mTextureCoords[index] && mesh->mNumVertices > 0;
}

internal b32
AssimpMeshHasVertexColours(struct aiMesh *mesh, u32 index)
{
	return mesh->mColors[index] && mesh->mNumVertices > 0;
}

internal u32
AssimpTryFetchMaterialTexture(MemoryArena *arena,
							  String8 directory,
							  const struct aiMaterial *material,
							  enum aiTextureType type,
							  u32 fallback)
{
	if(aiGetMaterialTextureCount(material, type) <= 0)
	{
		return fallback;
	}
	
	ScratchArena scratch = GetScratch(arena);
	
	struct aiString texture_path = {0};
	aiGetMaterialTexture(material, type, 0, &texture_path, 0, 0, 0, 0, 0, 0);
	
	String8 final_path = MemoryArenaAllocateString8(scratch.arena, directory.len + texture_path.length);
	MemoryCopy(final_path.str, directory.str, directory.len);
	MemoryCopy(final_path.str + directory.len, texture_path.data, texture_path.length);
	
	// TODO(kp): Temporarily I just don't even bother storing the images.
	//           Memory leaks who?? Never heard of 'em.
	
	Image image = ImageLoadFromPath(final_path);
	u32 id = FetchStandardImageView(&image)->resource_id;
	
	ReleaseScratch(&scratch);
	return id;
}

internal void
ModelLoadProcessSubModel(MemoryArena *arena,
						 SubModel *sub_model,
						 struct aiMesh *assimp_mesh,
						 const struct aiScene *scene,
						 struct aiMatrix4x4 transform)
{
	ScratchArena scratch = GetScratch(arena);
	
	ModelVertex *vertices = MemoryArenaPush(scratch.arena, sizeof(ModelVertex) * assimp_mesh->mNumVertices);
	
	// TODO(kp): Transforms should be applied when rendering (so be a member of a SubModel)
	//           rather than being directly applied to vertices when loading them in.
	for(i32 i = 0; i < assimp_mesh->mNumVertices; i++)
	{
		ModelVertex *vertex = vertices + i;
		
		if(AssimpMeshHasPositions(assimp_mesh))
		{
			struct aiVector3D position = assimp_mesh->mVertices[i];
			aiTransformVecByMatrix4(&position, &transform);
			
			vertex->position = v3(position.x, position.y, position.z); 
		}
		else
		{
			vertex->position = v3(0.f, 0.f, 0.f);
		}
		
		if(AssimpMeshHasTextureCoords(assimp_mesh, 0))
		{
			struct aiVector3D uv = assimp_mesh->mTextureCoords[0][i];
			
			vertex->texcoord = v2(uv.x, uv.y);
		}
		else
		{
			vertex->texcoord = v2(0.f, 0.f);
		}
		
		if(AssimpMeshHasVertexColours(assimp_mesh, 0))
		{
			struct aiColor4D colour = assimp_mesh->mColors[0][i];
			
			vertex->colour = v3(colour.r, colour.g, colour.b);
		}
		else
		{
			vertex->colour = v3(1.f, 1.f, 1.f);
		}
		
		if(AssimpMeshHasNormals(assimp_mesh))
		{
			// TODO(kp): This won't work.
			//           Need to use a corrected transformation matrix for normals!
			//           Unless assimp transformations are orthonormal?
			//           --> Investigate this.
			
			struct aiVector3D normal = assimp_mesh->mNormals[i];
			aiTransformVecByMatrix4(&normal, &transform);
			
			vertex->normal = v3(normal.x, normal.y, normal.z);
		}
		else
		{
			vertex->normal = v3(0.f, 0.f, 1.f);
		}
		
		if(AssimpMeshHasTangentsAndBitangents(assimp_mesh))
		{
			struct aiVector3D tangent = assimp_mesh->mTangents[i];
			struct aiVector3D bitangent = assimp_mesh->mBitangents[i];
			
			aiTransformVecByMatrix4(&tangent, &transform);
			aiTransformVecByMatrix4(&bitangent, &transform);
			
			vertex->tangent = v3(tangent.x, tangent.y, tangent.z);
			vertex->bitangent = v3(bitangent.x, bitangent.y, bitangent.z);
		}
		else
		{
			vertex->tangent = v3(1.f, 0.f, 0.f);
			vertex->bitangent = v3(0.f, 1.f, 0.f);
		}
	}
	
	u32 index_count = 0;
	
	for(i32 i = 0; i < assimp_mesh->mNumFaces; i++)
	{
		struct aiFace *face = assimp_mesh->mFaces + i;
		
		for(i32 j = 0; j < face->mNumIndices; j++)
		{
			index_count++;
		}
	}
	
	u16 *indices = MemoryArenaPush(scratch.arena, sizeof(u16) * index_count);
	
	index_count = 0;
	
	for(i32 i = 0; i < assimp_mesh->mNumFaces; i++)
	{
		struct aiFace *face = assimp_mesh->mFaces + i;
		
		for(i32 j = 0; j < face->mNumIndices; j++)
		{
			indices[index_count] = face->mIndices[j];
			index_count++;
		}
	}
	
	sub_model->mesh = MeshInit(&vertex_formats->model_format,
							   assimp_mesh->mNumVertices, vertices,
							   index_count, indices);
	
	if(assimp_mesh->mMaterialIndex >= 0)
	{
		const struct aiMaterial *assimp_material = scene->mMaterials[assimp_mesh->mMaterialIndex];
		
		sub_model->material.diffuse  = AssimpTryFetchMaterialTexture(arena, sub_model->parent->directory, assimp_material, aiTextureType_DIFFUSE, 0);
		sub_model->material.normal   = AssimpTryFetchMaterialTexture(arena, sub_model->parent->directory, assimp_material, aiTextureType_NORMALS, 0);
		sub_model->material.emissive = AssimpTryFetchMaterialTexture(arena, sub_model->parent->directory, assimp_material, aiTextureType_EMISSIVE, 0);
		sub_model->material.mr       = AssimpTryFetchMaterialTexture(arena, sub_model->parent->directory, assimp_material, aiTextureType_DIFFUSE_ROUGHNESS, 0);
		sub_model->material.ambient  = AssimpTryFetchMaterialTexture(arena, sub_model->parent->directory, assimp_material, aiTextureType_LIGHTMAP, 0);
	}
	
	ReleaseScratch(&scratch);
}

internal void
ModelLoadProcessNodes(MemoryArena *arena,
					  Model *model,
					  struct aiNode *node,
					  const struct aiScene *scene,
					  struct aiMatrix4x4 transform)
{
	struct aiMatrix4x4 node_transform = node->mTransformation;
	aiMultiplyMatrix4(&node_transform, &transform);
	
	for(i32 i = 0; i < node->mNumMeshes; i++)
	{
		struct aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];
		
		SubModel *sub_model = ModelCreateSubModel(model);
		
		ModelLoadProcessSubModel(arena, sub_model, assimp_mesh, scene, node_transform);
	}
	
	for (i32 i = 0; i < node->mNumChildren; i++)
	{
		ModelLoadProcessNodes(arena, model, node->mChildren[i], scene, node_transform);
	}
}

internal Model
ModelLoadFromPath(MemoryArena *arena, String8 path)
{
	const struct aiScene *scene = aiImportFile((char *)path.str,
											   aiProcess_Triangulate |
											   aiProcess_FlipWindingOrder |
											   aiProcess_CalcTangentSpace |
											   aiProcess_FlipUVs);
	
	if(!scene ||
	   (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
	   !scene->mRootNode)
	{
		DebugLogCrash("Failed to load model.");
	}
	
	Model model = {0};
	model.arena = arena;
	model.directory = String8BeforeFirstSubstringFromBackInclusive(path, str8("/"));
	
	// NOTE(kp): This is a coordanate transformation converting
	//           Assimp's coordinate system, which is right handed Y-up, into
	//           our coordinate system, which is right handed Z-up.
	struct aiMatrix4x4 identity = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 0.f,-1.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	
	DebugLog("Loading model...");
	
	ModelLoadProcessNodes(arena, &model, scene->mRootNode, scene, identity);
	
	aiReleaseImport(scene);
	return model;
}

internal void
ModelDestroy(Model *model)
{
	for(i32 i = 0; i < model->sub_model_count; i++)
	{
		MeshDestroy(&model->sub_models[i].mesh);
	}
}
