#include "assets.h"

#include "core/core_scratch.h"

#include "rendering/gpu_types.h"
#include "rendering/sync.h"

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "ext/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ext/stb_image_write.h"

struct bitmap_image bitmap_image_load_from_file(struct string8 path)
{
	struct bitmap_image image = {0};

	if (stbi_is_hdr((char *)path.str)) {
		image.pixels = stbi_loadf((char *)path.str,
					  &image.width, &image.height, &image.channels, 4);

		if (!image.pixels)
			debug_log_crash("Couldn't load Bitmap HDR: %s", path.str);

		image.format = BITMAP_IMAGE_FORMAT_RGBAF;
	} else {
		image.pixels = stbi_load((char *)path.str,
					 &image.width, &image.height, &image.channels, 4);

		if (!image.pixels)
			debug_log_crash("Couldn't load Bitmap LDR: %s", path.str);

		image.format = BITMAP_IMAGE_FORMAT_RGBA8;
	}

	return image;
}

void bitmap_image_destroy(struct bitmap_image *bitmap)
{
	stbi_image_free(bitmap->pixels);
}

u64 bitmap_image_get_memory_size(const struct bitmap_image *bitmap)
{
	u64 unit = 0;
	
	switch (bitmap->format) {
	case BITMAP_IMAGE_FORMAT_RGBA8:
		unit = sizeof(u8);
		break;
	case BITMAP_IMAGE_FORMAT_RGBAF:
		unit = sizeof(float);
		break;
	}
	
	return bitmap->width * bitmap->height * 4 * unit;
}

struct gfx_texture bitmap_create_gfx_texture(struct bitmap_image *bitmap,
					     struct gfx_device *device)
{
	VkFormat format = VK_FORMAT_UNDEFINED;

	switch (bitmap->format) {
	case BITMAP_IMAGE_FORMAT_RGBA8:
		format = VK_FORMAT_R8G8B8A8_UNORM;
		break;
	case BITMAP_IMAGE_FORMAT_RGBAF:
		format = VK_FORMAT_R32G32B32A32_SFLOAT;
		break;
	}

	u64 memory_size = bitmap_image_get_memory_size(bitmap);

	struct gfx_texture texture = gfx_device_texture_alloc(device,
							      bitmap->width, bitmap->height, 1,
							      format,
							      VK_IMAGE_VIEW_TYPE_2D,
							      VK_IMAGE_TILING_OPTIMAL,
							      4,
							      VK_SAMPLE_COUNT_1_BIT,
							      false, false);

	struct gfx_buffer staging_buffer = gfx_device_buffer_alloc(device,
								   VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT,
								   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
								   memory_size);
	
	gfx_buffer_write(&staging_buffer, bitmap->pixels, memory_size, 0);

	struct gfx_command_buffer cmd = gfx_device_begin_instant_submit(device);
	
	VkImageMemoryBarrier2 copy_barrier = gfx_sync_texture_memory_barrier(&texture,
									     gfx_sync_get_src_texture_access(GFX_TEXTURE_ACCESS_TYPE_undefined),
									     gfx_sync_get_dst_texture_access(GFX_TEXTURE_ACCESS_TYPE_copy_dst),
									     0, texture.mipmap_count,
									     0, gfx_texture_layer_count(&texture));
	
	gfx_cmd_pipeline_barrier(&cmd, 0,
				 0, NULL,
				 0, NULL,
				 1, &copy_barrier);
	
	gfx_cmd_copy_buffer_to_texture(&cmd, &staging_buffer, &texture);

	VkImageMemoryBarrier2 blit_barrier = gfx_sync_texture_memory_barrier(&texture,
									     gfx_sync_get_src_texture_access(GFX_TEXTURE_ACCESS_TYPE_copy_dst),
									     gfx_sync_get_dst_texture_access(GFX_TEXTURE_ACCESS_TYPE_blit_dst),
									     0, texture.mipmap_count,
									     0, gfx_texture_layer_count(&texture));
	
	gfx_cmd_pipeline_barrier(&cmd, 0,
				 0, NULL,
				 0, NULL,
				 1, &blit_barrier);
	
	gfx_cmd_generate_mipmaps(&cmd, &texture);
	
	gfx_device_end_instant_submit(device, &cmd);

	gfx_device_wait_idle(device);
	gfx_device_buffer_destroy(device, &staging_buffer);
	
	return texture;
}

void asset_store_init(struct asset_store *assets, struct memory_arena *arena)
{
	assets->arena = arena;
}

void asset_store_destroy(struct asset_store *assets, struct gfx_device *device)
{
	for (int i = 0; i < assets->texture_count; i++)
		gfx_device_texture_destroy(device, &assets->textures[i].texture);

	for (int i = 0; i < assets->model_count; i++)
		gfx_model_destroy(&assets->models[i].model, device);
}

struct asset_handle asset_store_load_texture(struct asset_store *assets, struct gfx_device *device, struct string8 path)
{
	struct bitmap_image bitmap = bitmap_image_load_from_file(path);
	struct gfx_texture texture = bitmap_create_gfx_texture(&bitmap, device);
	bitmap_image_destroy(&bitmap);

	u32 index = assets->texture_count;

	assets->textures[index].path = path;
	assets->textures[index].texture = texture;

	assets->texture_count++;

	struct asset_handle handle = {0};
	handle.index = index;

	return handle;
}

static bool assimp_mesh_has_positions(struct aiMesh *mesh)
{
	return mesh->mVertices && mesh->mNumVertices > 0;
}

static bool assimp_mesh_has_faces(struct aiMesh *mesh)
{
	return mesh->mFaces && mesh->mNumFaces > 0;
}

static bool assimp_mesh_has_normals(struct aiMesh *mesh)
{
	return mesh->mNormals && mesh->mNumVertices > 0;
}

static bool assimp_mesh_has_tangents_and_bitangents(struct aiMesh *mesh)
{
	return mesh->mTangents && mesh->mBitangents && mesh->mNumVertices > 0;
}

static bool assimp_mesh_has_texture_coords(struct aiMesh *mesh, u32 index)
{
	return mesh->mTextureCoords[index] && mesh->mNumVertices > 0;
}

static bool assimp_mesh_has_vertex_colours(struct aiMesh *mesh, u32 index)
{
	return mesh->mColors[index] && mesh->mNumVertices > 0;
}

static struct asset_handle assets_try_fetch_assimp_material_texture(struct asset_store *assets,
								    struct gfx_device *device,
								    struct string8 directory,
								    const struct aiMaterial *material,
								    enum aiTextureType type)
{
	if (aiGetMaterialTextureCount(material, type) <= 0) {
		struct asset_handle fallback = {0};
		return fallback;
	}
	
	struct aiString texture_path = {0};
	aiGetMaterialTexture(material, type, 0, &texture_path, 0, 0, 0, 0, 0, 0);

	struct string8 final_path = memory_arena_allocate_string8(assets->arena, directory.len + texture_path.length);
	memory_copy(final_path.str, directory.str, directory.len);
	memory_copy(final_path.str + directory.len, texture_path.data, texture_path.length);

	return asset_store_load_texture(assets, device, final_path);
}

static struct gfx_material asset_store_load_material_from_assimp(struct asset_store *assets,
								 struct gfx_device *device,
								 struct string8 directory,
								 const struct aiMaterial *assimp_material)
{
	struct gfx_material material = {0};

	material.diffuse_texture_handle            = assets_try_fetch_assimp_material_texture(assets, device, directory, assimp_material, aiTextureType_DIFFUSE);
	material.normal_texture_handle             = assets_try_fetch_assimp_material_texture(assets, device, directory, assimp_material, aiTextureType_NORMALS);
	material.emissive_texture_handle           = assets_try_fetch_assimp_material_texture(assets, device, directory, assimp_material, aiTextureType_EMISSIVE);
	material.metallic_roughness_texture_handle = assets_try_fetch_assimp_material_texture(assets, device, directory, assimp_material, aiTextureType_DIFFUSE_ROUGHNESS);
	material.ambient_texture_handle            = assets_try_fetch_assimp_material_texture(assets, device, directory, assimp_material, aiTextureType_LIGHTMAP);

	return material;
}

static void asset_store_process_sub_model(struct asset_store *assets,
					  struct gfx_device *device,
					  struct gfx_sub_model *sub_model,
					  struct string8 path,
					  struct aiMesh *assimp_mesh,
					  const struct aiScene *scene,
					  struct aiMatrix4x4 transform)
{
	struct scratch_arena scratch = scratch_begin(assets->arena, 1);

	struct gfx_gpu_model_vertex *vertices = memory_arena_array(scratch.arena,
								   assimp_mesh->mNumVertices,
								   sizeof(struct gfx_gpu_model_vertex));

	// TODO: Transforms should be applied when rendering (so be a member of a Sub_model)
	//       rather than being directly applied to vertices when loading them in.
	for (int i = 0; i < assimp_mesh->mNumVertices; i++) {
		struct gfx_gpu_model_vertex *vertex = vertices + i;

		if (assimp_mesh_has_positions(assimp_mesh)) {
			struct aiVector3D position = assimp_mesh->mVertices[i];
			aiTransformVecByMatrix4(&position, &transform);
			vertex->position = v3(position.x, position.y, position.z);
		} else {
			vertex->position = v3(0.f, 0.f, 0.f);
		}

		if (assimp_mesh_has_texture_coords(assimp_mesh, 0)) {
			struct aiVector3D uv = assimp_mesh->mTextureCoords[0][i];
			vertex->texcoord = v2(uv.x, uv.y);
		} else {
			vertex->texcoord = v2(0.f, 0.f);
		}

		if (assimp_mesh_has_vertex_colours(assimp_mesh, 0)) {
			struct aiColor4D colour = assimp_mesh->mColors[0][i];
			vertex->colour = v3(colour.r, colour.g, colour.b);
		} else {
			vertex->colour = v3(1.f, 1.f, 1.f);
		}

		if (assimp_mesh_has_normals(assimp_mesh)) {
			// TODO: This won't work.
			//       Need to use a corrected transformation matrix for normals!
			//       Unless assimp transformations are orthonormal?
			//       --> Investigate this.

			struct aiVector3D normal = assimp_mesh->mNormals[i];
			aiTransformVecByMatrix4(&normal, &transform);
			vertex->normal = v3(normal.x, normal.y, normal.z);
		} else {
			vertex->normal = v3(0.f, 0.f, 1.f);
		}

		if (assimp_mesh_has_tangents_and_bitangents(assimp_mesh)) {
			struct aiVector3D tangent = assimp_mesh->mTangents[i];
			struct aiVector3D bitangent = assimp_mesh->mBitangents[i];

			aiTransformVecByMatrix4(&tangent, &transform);
			aiTransformVecByMatrix4(&bitangent, &transform);

			vertex->tangent   = v3(tangent.x, tangent.y, tangent.z);
			vertex->bitangent = v3(bitangent.x, bitangent.y, bitangent.z);
		} else {
			vertex->tangent   = v3(1.f, 0.f, 0.f);
			vertex->bitangent = v3(0.f, 1.f, 0.f);
		}
	}

	u32 index_count = 0;

	for (int i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (int j = 0; j < face->mNumIndices; j++)
			index_count++;
	}

	u16 *indices = memory_arena_array(scratch.arena, index_count, sizeof(u16));

	index_count = 0;

	for (int i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (int j = 0; j < face->mNumIndices; j++) {
			indices[index_count] = face->mIndices[j];
			index_count++;
		}
	}

	gfx_mesh_init(&sub_model->mesh, device,
		      sizeof(struct gfx_gpu_model_vertex),
		      assimp_mesh->mNumVertices, vertices,
		      index_count, indices);

	if (assimp_mesh->mMaterialIndex >= 0) {
		const struct aiMaterial *assimp_material = scene->mMaterials[assimp_mesh->mMaterialIndex];
		struct string8 directory = string8_before_first_substring_from_back_inclusive(path, str8("/"));
		sub_model->material = asset_store_load_material_from_assimp(assets, device, directory, assimp_material);
	}

	scratch_release(&scratch);
}

static void asset_store_process_model_nodes(struct asset_store *assets,
					    struct gfx_device *device,
					    struct gfx_model *model,
					    struct string8 path,
					    struct aiNode *node,
					    const struct aiScene *scene,
					    struct aiMatrix4x4 transform)
{
	struct aiMatrix4x4 node_transform = node->mTransformation;
	aiMultiplyMatrix4(&node_transform, &transform);

	for (int i = 0; i < node->mNumMeshes; i++) {
		struct aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];
		struct gfx_sub_model *sub_model = gfx_model_add_sub_model(model);
		asset_store_process_sub_model(assets, device, sub_model, path, assimp_mesh, scene, node_transform);
	}

	for (int i = 0; i < node->mNumChildren; i++)
		asset_store_process_model_nodes(assets, device, model, path, node->mChildren[i], scene, node_transform);
}

struct asset_handle asset_store_load_model(struct asset_store *assets, struct gfx_device *device, struct string8 path)
{
	const struct aiScene *scene = aiImportFile((char *)path.str,
						   aiProcess_Triangulate |
						   aiProcess_FlipWindingOrder |
						   aiProcess_CalcTangentSpace |
						   aiProcess_FlipUVs);

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode)
		debug_log_crash("Failed to load model.");

	u32 index = assets->model_count;

	assets->models[index].path = path;
	assets->models[index].model.arena = assets->arena;

	// This is a coordanate transformation converting
	// Assimp's coordinate system, which is right handed Y-up, into
	// our coordinate system, which is right handed Z-up.
	struct aiMatrix4x4 identity = {
		1.f, 0.f, 0.f, 0.f,
		0.f, 0.f,-1.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};

	debug_log("Loading model...");

	asset_store_process_model_nodes(assets,
					device,
					&assets->models[index].model,
					path,
					scene->mRootNode,
					scene,
					identity);

	aiReleaseImport(scene);

	assets->model_count++;

	struct asset_handle handle = {0};
	handle.index = index;
	
	return handle;
}

struct gfx_texture *asset_store_texture_from_handle(struct asset_store *assets, struct asset_handle handle)
{
	return &assets->textures[handle.index].texture;
}

struct gfx_model *asset_store_model_from_handle(struct asset_store *assets, struct asset_handle handle)
{
	return &assets->models[handle.index].model;
}
