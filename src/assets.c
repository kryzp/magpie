
internal BitmapImage BitmapImageLoadFromFile(String8 path)
{
	BitmapImage image = { 0 };

	if (stbi_is_hdr((char *)path.str)) {
		image.pixels = stbi_loadf((char *)path.str, &image.width,
					  &image.height, &image.channels, 4);

		if (!image.pixels) {
			DebugLogCrash("Couldn't load Bitmap HDR: %s", path.str);
		}

		image.format = BitmapImageFormat_RGBAF;
	} else {
		image.pixels = stbi_load((char *)path.str, &image.width,
					 &image.height, &image.channels, 4);

		if (!image.pixels) {
			DebugLogCrash("Couldn't load Bitmap LDR: %s", path.str);
		}

		image.format = BitmapImageFormat_RGBA8;
	}

	return image;
}

internal void BitmapImageDestroy(BitmapImage *image)
{
	stbi_image_free(image->pixels);
	image->pixels = 0;
}

internal u64 GetBitmapImageMemorySize(BitmapImage *image)
{
	u64 unit = (image->format == BitmapImageFormat_RGBA8) ? sizeof(u8) :
								sizeof(f32);
	return image->width * image->height * 4 * unit;
}

internal Image BitmapCreateImage(BitmapImage *bitmap)
{
	VkFormat format = VK_FORMAT_UNDEFINED;

	switch (bitmap->format) {
	case BitmapImageFormat_RGBA8: {
		format = VK_FORMAT_R8G8B8A8_UNORM;
	} break;

	case BitmapImageFormat_RGBAF: {
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
	} break;
	}

	u64 memory_size = GetBitmapImageMemorySize(bitmap);

	Image image = ImageAlloc(bitmap->width, bitmap->height, 1, format,
				 VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
				 4, VK_SAMPLE_COUNT_1_BIT, false, false);

	GPUBuffer staging_buffer = GPUBufferAlloc(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		memory_size);
	{
		GPUBufferWrite(&staging_buffer, bitmap->pixels, memory_size, 0);

		CommandBuffer cmd = BeginGraphicsInstantSubmit();
		{
			CmdTransitionImageLayout(
				&cmd, &image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CmdCopyBufferToImage(&cmd, &staging_buffer, &image);
			CmdGenerateMipmaps(&cmd, &image);
		}
		EndGraphicsInstantSubmit(&cmd);
	}
	GraphicsWaitIdle();
	GPUBufferDestroy(&staging_buffer);

	return image;
}

internal void AssetsInit(Assets *assets, MemoryArena *arena)
{
	assets->arena = arena;

	assets->texture_count = 0;
	assets->model_count = 0;
}

internal void AssetsDestroy(Assets *assets)
{
	for (i32 i = 0; i < assets->texture_count; i++) {
		ImageDestroy(&assets->textures[i].image);
	}

	for (i32 i = 0; i < assets->model_count; i++) {
		ModelDestroy(&assets->models[i].model);
	}
}

internal Image *AssetsImageFromHandle(Assets *assets, u32 handle)
{
	return &assets->textures[handle].image;
}

internal Model *AssetsModelFromHandle(Assets *assets, u32 handle)
{
	return &assets->models[handle].model;
}

internal u32 AssetsLoadTexture(Assets *assets, String8 path)
{
	BitmapImage bitmap = BitmapImageLoadFromFile(path);
	Image image = BitmapCreateImage(&bitmap);
	BitmapImageDestroy(&bitmap);

	u32 handle = assets->texture_count;

	assets->textures[handle].path = path;
	assets->textures[handle].image = image;

	assets->texture_count++;

	return handle;
}

internal b32 AssimpMeshHasPositions(struct aiMesh *mesh)
{
	return mesh->mVertices && mesh->mNumVertices > 0;
}

internal b32 AssimpMeshHasFaces(struct aiMesh *mesh)
{
	return mesh->mFaces && mesh->mNumFaces > 0;
}

internal b32 AssimpMeshHasNormals(struct aiMesh *mesh)
{
	return mesh->mNormals && mesh->mNumVertices > 0;
}

internal b32 AssimpMeshHasTangentsAndBitangents(struct aiMesh *mesh)
{
	return mesh->mTangents && mesh->mBitangents && mesh->mNumVertices > 0;
}

internal b32 AssimpMeshHasTextureCoords(struct aiMesh *mesh, u32 index)
{
	return mesh->mTextureCoords[index] && mesh->mNumVertices > 0;
}

internal b32 AssimpMeshHasVertexColours(struct aiMesh *mesh, u32 index)
{
	return mesh->mColors[index] && mesh->mNumVertices > 0;
}

internal u32 AssetsTryFetchAssimpMaterialTexture(
	Assets *assets, String8 directory, const struct aiMaterial *material,
	enum aiTextureType type, u32 fallback)
{
	if (aiGetMaterialTextureCount(material, type) <= 0) {
		return fallback;
	}

	ScratchArena scratch = GetScratch(assets->arena, 1);

	struct aiString texture_path = { 0 };
	aiGetMaterialTexture(material, type, 0, &texture_path, 0, 0, 0, 0, 0,
			     0);

	String8 final_path = MemoryArenaAllocateString8(
		scratch.arena, directory.len + texture_path.length);
	MemoryCopy(final_path.str, directory.str, directory.len);
	MemoryCopy(final_path.str + directory.len, texture_path.data,
		   texture_path.length);

	u32 handle = AssetsLoadTexture(assets, final_path);

	ReleaseScratch(&scratch);
	return handle;
}

internal Material
AssetsLoadMaterialFromAssimp(Assets *assets, String8 directory,
			     const struct aiMaterial *assimp_material)
{
	Material material = { 0 };

	material.diffuse_texture_handle = AssetsTryFetchAssimpMaterialTexture(
		assets, directory, assimp_material, aiTextureType_DIFFUSE, 0);
	material.normal_texture_handle = AssetsTryFetchAssimpMaterialTexture(
		assets, directory, assimp_material, aiTextureType_NORMALS, 0);
	material.emissive_texture_handle = AssetsTryFetchAssimpMaterialTexture(
		assets, directory, assimp_material, aiTextureType_EMISSIVE, 0);
	material.metallic_roughness_texture_handle =
		AssetsTryFetchAssimpMaterialTexture(
			assets, directory, assimp_material,
			aiTextureType_DIFFUSE_ROUGHNESS, 0);
	material.ambient_texture_handle = AssetsTryFetchAssimpMaterialTexture(
		assets, directory, assimp_material, aiTextureType_LIGHTMAP, 0);

	return material;
}

internal void AssetsProcessSubModel(Assets *assets, SubModel *sub_model,
				    String8 path, struct aiMesh *assimp_mesh,
				    const struct aiScene *scene,
				    struct aiMatrix4x4 transform)
{
	ScratchArena scratch = GetScratch(assets->arena, 1);

	ModelVertex *vertices = MemoryArenaPush(
		scratch.arena, sizeof(ModelVertex) * assimp_mesh->mNumVertices);

	// TODO(kp): Transforms should be applied when rendering (so be a member of a SubModel)
	//           rather than being directly applied to vertices when loading them in.
	for (i32 i = 0; i < assimp_mesh->mNumVertices; i++) {
		ModelVertex *vertex = vertices + i;

		if (AssimpMeshHasPositions(assimp_mesh)) {
			struct aiVector3D position = assimp_mesh->mVertices[i];
			aiTransformVecByMatrix4(&position, &transform);

			vertex->position =
				v3(position.x, position.y, position.z);
		} else {
			vertex->position = v3(0.f, 0.f, 0.f);
		}

		if (AssimpMeshHasTextureCoords(assimp_mesh, 0)) {
			struct aiVector3D uv =
				assimp_mesh->mTextureCoords[0][i];

			vertex->texcoord = v2(uv.x, uv.y);
		} else {
			vertex->texcoord = v2(0.f, 0.f);
		}

		if (AssimpMeshHasVertexColours(assimp_mesh, 0)) {
			struct aiColor4D colour = assimp_mesh->mColors[0][i];

			vertex->colour = v3(colour.r, colour.g, colour.b);
		} else {
			vertex->colour = v3(1.f, 1.f, 1.f);
		}

		if (AssimpMeshHasNormals(assimp_mesh)) {
			// TODO(kp): This won't work.
			//           Need to use a corrected transformation matrix for normals!
			//           Unless assimp transformations are orthonormal?
			//           --> Investigate this.

			struct aiVector3D normal = assimp_mesh->mNormals[i];
			aiTransformVecByMatrix4(&normal, &transform);

			vertex->normal = v3(normal.x, normal.y, normal.z);
		} else {
			vertex->normal = v3(0.f, 0.f, 1.f);
		}

		if (AssimpMeshHasTangentsAndBitangents(assimp_mesh)) {
			struct aiVector3D tangent = assimp_mesh->mTangents[i];
			struct aiVector3D bitangent =
				assimp_mesh->mBitangents[i];

			aiTransformVecByMatrix4(&tangent, &transform);
			aiTransformVecByMatrix4(&bitangent, &transform);

			vertex->tangent = v3(tangent.x, tangent.y, tangent.z);
			vertex->bitangent =
				v3(bitangent.x, bitangent.y, bitangent.z);
		} else {
			vertex->tangent = v3(1.f, 0.f, 0.f);
			vertex->bitangent = v3(0.f, 1.f, 0.f);
		}
	}

	u32 index_count = 0;

	for (i32 i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (i32 j = 0; j < face->mNumIndices; j++) {
			index_count++;
		}
	}

	u16 *indices =
		MemoryArenaPush(scratch.arena, sizeof(u16) * index_count);

	index_count = 0;

	for (i32 i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (i32 j = 0; j < face->mNumIndices; j++) {
			indices[index_count] = face->mIndices[j];
			index_count++;
		}
	}

	sub_model->mesh = MeshInit(&vertex_formats->model,
				   assimp_mesh->mNumVertices, vertices,
				   index_count, indices);

	if (assimp_mesh->mMaterialIndex >= 0) {
		const struct aiMaterial *assimp_material =
			scene->mMaterials[assimp_mesh->mMaterialIndex];
		String8 directory =
			String8BeforeFirstSubstringFromBackInclusive(path,
								     str8("/"));
		sub_model->material = AssetsLoadMaterialFromAssimp(
			assets, directory, assimp_material);
	}

	ReleaseScratch(&scratch);
}

internal void AssetsProcessModelNodes(Assets *assets, Model *model,
				      String8 path, struct aiNode *node,
				      const struct aiScene *scene,
				      struct aiMatrix4x4 transform)
{
	struct aiMatrix4x4 node_transform = node->mTransformation;
	aiMultiplyMatrix4(&node_transform, &transform);

	for (i32 i = 0; i < node->mNumMeshes; i++) {
		struct aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];

		SubModel *sub_model = ModelCreateSubModel(model);

		AssetsProcessSubModel(assets, sub_model, path, assimp_mesh,
				      scene, node_transform);
	}

	for (i32 i = 0; i < node->mNumChildren; i++) {
		AssetsProcessModelNodes(assets, model, path, node->mChildren[i],
					scene, node_transform);
	}
}

internal u32 AssetsLoadModel(Assets *assets, String8 path)
{
	const struct aiScene *scene = aiImportFile(
		(char *)path.str,
		aiProcess_Triangulate | aiProcess_FlipWindingOrder |
			aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
	    !scene->mRootNode) {
		DebugLogCrash("Failed to load model.");
	}

	u32 handle = assets->model_count;

	assets->models[handle].path = path;
	assets->models[handle].model.arena = assets->arena;

	// NOTE(kp): This is a coordanate transformation converting
	//           Assimp's coordinate system, which is right handed Y-up, into
	//           our coordinate system, which is right handed Z-up.
	struct aiMatrix4x4 identity = {
		1.f, 0.f, 0.f, 0.f, 0.f, 0.f, -1.f, 0.f,
		0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,  1.f
	};

	DebugLog("Loading model...");

	AssetsProcessModelNodes(assets, &assets->models[handle].model, path,
				scene->mRootNode, scene, identity);

	aiReleaseImport(scene);

	assets->model_count++;

	return handle;
}
