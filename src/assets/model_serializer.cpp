#include "model_serializer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "core/scratch.h"
#include "io/filesystem.h"
#include "graphics/gpu_types.h"

using namespace ast;

static void serialize(AssetManager &assets, const FileStream &fs, const AssetMetaData &metadata, const AssetHandle &handle);
static Asset *try_load_data(AssetManager &assets, const AssetMetaData &metadata);
static void process_nodes(AssetManager &assets, gfx::Model &model, const String &directory, gfx::Device *device, const aiNode *node, const aiScene *scene, const aiMatrix4x4 &transform);
static void process_sub_model(AssetManager &assets, gfx::Model &model, gfx::SubModel &sub_model, const String &directory, gfx::Device *device, const aiMesh *assimp_mesh, const aiScene *scene, const aiMatrix4x4 &transform);
static gfx::Material load_material_from_assimp(AssetManager &assets, const String &directory, const aiMaterial *ai_material);

static void serialize(AssetManager &assets, const FileStream &fs, const AssetMetaData &metadata, const AssetHandle &handle)
{
}

static Asset *try_load_data(AssetManager &assets, const AssetMetaData &metadata)
{
	String system_file_path = assets.get_system_file_path(metadata.file_path);

	// TODO: Move this out into the global scope, creating an
	//       Importer every time a new model is loaded is super slow.
	Assimp::Importer ai_importer;

	const aiScene *scene = ai_importer.ReadFile(
		system_file_path,
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs
	);

	ModelAsset *asset = new ModelAsset();

	if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 || !scene->mRootNode) {
		asset->set_flag(ASSET_FLAG_INVALID, true);
	} else {
		// This is a coordanate transformation converting
		// Assimp's coordinate system, which is right-handed Y+ up
		// into our coordinate system, which is right-handed Z+ up.
		aiMatrix4x4 identity = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 0.f,-1.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};

		String directory = io::path::get_file_directory(metadata.file_path) + "/";

		process_nodes(assets, asset->model, directory, assets.get_device(), scene->mRootNode, scene, identity);
	}

	return asset;
}

static void process_nodes(AssetManager &assets, gfx::Model &model, const String &directory, gfx::Device *device, const aiNode *node, const aiScene *scene, const aiMatrix4x4 &transform)
{
	aiMatrix4x4 node_transform = node->mTransformation * transform;

	for (int i = 0; i < node->mNumMeshes; i++) {
		const aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];
		gfx::SubModel &sub_model = model.add_sub_model();
		process_sub_model(assets, model, sub_model, directory, device, assimp_mesh, scene, node_transform);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		const aiNode *child = node->mChildren[i];
		process_nodes(assets, model, directory, device, child, scene, node_transform);
	}
}

static void process_sub_model(AssetManager &assets, gfx::Model &model, gfx::SubModel &sub_model, const String &directory, gfx::Device *device, const aiMesh *assimp_mesh, const aiScene *scene, const aiMatrix4x4 &transform)
{
	ScratchArena scratch;

	gfx::gpu_types::GpuModelVertex *vertices = scratch.get_arena()
		.push_array<gfx::gpu_types::GpuModelVertex>(assimp_mesh->mNumVertices);

	// TODO: Transforms should be applied when rendering (so be a member of a Sub_model)
	//       rather than being directly applied to vertices when loading them in.
	for (int i = 0; i < assimp_mesh->mNumVertices; i++) {
		gfx::gpu_types::GpuModelVertex *vertex = &vertices[i];

		if (assimp_mesh->HasPositions()) {
			aiVector3D position = transform * assimp_mesh->mVertices[i];
			vertex->position = Vec3(position.x, position.y, position.z);
		} else {
			vertex->position = Vec3(0.f, 0.f, 0.f);
		}

		if (assimp_mesh->HasTextureCoords(0)) {
			aiVector3D uv = assimp_mesh->mTextureCoords[0][i];
			vertex->texcoord = Vec2(uv.x, uv.y);
		} else {
			vertex->texcoord = Vec2(0.f, 0.f);
		}

		if (assimp_mesh->HasVertexColors(0)) {
			aiColor4D colour = assimp_mesh->mColors[0][i];
			vertex->colour = Vec3(colour.r, colour.g, colour.b);
		} else {
			vertex->colour = Vec3(1.f, 1.f, 1.f);
		}

		if (assimp_mesh->HasNormals()) {
			// TODO: This won't work.
			//       Need to use a corrected transformation matrix for normals!
			//       Unless assimp transformations are orthonormal?
			//       --> Investigate!!

			aiVector3D normal = transform * assimp_mesh->mNormals[i];
			vertex->normal = Vec3(normal.x, normal.y, normal.z);
		} else {
			vertex->normal = Vec3(0.f, 0.f, 1.f);
		}

		if (assimp_mesh->HasTangentsAndBitangents()) {
			aiVector3D tangent = transform * assimp_mesh->mTangents[i];
			aiVector3D bitangent = transform * assimp_mesh->mBitangents[i];

			vertex->tangent   = Vec3(tangent.x, tangent.y, tangent.z);
			vertex->bitangent = Vec3(bitangent.x, bitangent.y, bitangent.z);
		} else {
			vertex->tangent   = Vec3(1.f, 0.f, 0.f);
			vertex->bitangent = Vec3(0.f, 1.f, 0.f);
		}
	}

	u32 index_count = 0;

	for (int i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (int j = 0; j < face->mNumIndices; j++)
			index_count++;
	}

	u16 *indices = scratch.get_arena().push_array<u16>(index_count);

	index_count = 0;

	for (int i = 0; i < assimp_mesh->mNumFaces; i++) {
		const aiFace &face = assimp_mesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++) {
			indices[index_count] = face.mIndices[j];
			index_count++;
		}
	}

	sub_model.mesh.init(device, sizeof(gfx::gpu_types::GpuModelVertex),
		assimp_mesh->mNumVertices, vertices,
		index_count, indices
	);

	if (assimp_mesh->mMaterialIndex >= 0) {
		const aiMaterial *assimp_material = scene->mMaterials[assimp_mesh->mMaterialIndex];
		sub_model.material = load_material_from_assimp(assets, directory, assimp_material);
	}
}

static AssetHandle try_fetch_assimp_material_texture(AssetManager &assets, const String &directory, const aiMaterial *ai_material, aiTextureType type)
{
	if (ai_material->GetTextureCount(type) <= 0) {
		AssetHandle fallback = {};
		return fallback;
	}

	aiString texture_path = {};
	ai_material->GetTexture(type, 0, &texture_path);

	String path = directory + String(texture_path.C_Str());

	return assets.from_file_path(path, ASSET_TYPE_TEXTURE);
}

static gfx::Material load_material_from_assimp(AssetManager &assets, const String &directory, const aiMaterial *ai_material)
{
	gfx::Material material = {};
	material.diffuse            = try_fetch_assimp_material_texture(assets, directory, ai_material, aiTextureType_DIFFUSE);
	material.normal             = try_fetch_assimp_material_texture(assets, directory, ai_material, aiTextureType_NORMALS);
	material.emissive           = try_fetch_assimp_material_texture(assets, directory, ai_material, aiTextureType_EMISSIVE);
	material.metallic_roughness = try_fetch_assimp_material_texture(assets, directory, ai_material, aiTextureType_DIFFUSE_ROUGHNESS);
	material.ambient            = try_fetch_assimp_material_texture(assets, directory, ai_material, aiTextureType_LIGHTMAP);

	return material;
}

AssetSerializer ast::get_model_serializer()
{
	AssetSerializer model_serializer = {};
	model_serializer.serialize = serialize;
	model_serializer.try_load_data = try_load_data;

	return model_serializer;
}
