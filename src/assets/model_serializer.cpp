#include "model_serializer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "io/filesystem.h"
#include "graphics/gpu_types.h"

using namespace ast;

// This is a coordanate transformation converting
// Assimp's coordinate system, which is right-handed Y+ up
// into our coordinate system, which is right-handed Z+ up.
const static aiMatrix4x4 assimp_basis = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 0.f,-1.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

static Mat4 aiMatrix4x4_to_Mat4(const aiMatrix4x4 &m)
{
	return Mat4(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
}

static Assimp::Importer ai_importer;

struct ParsedMesh {
	u32 target_sub_model;
	Vector<gfx::gpu_types::GpuModelVertex> vertices;
	Vector<gfx::IndexType> indices;
};

struct ModelLoadData {
	gfx::Model model;
	Vector<ParsedMesh> meshes;
	u64 total_vertex_size = 0;
	u64 total_index_size = 0;
};

static AssetHandle try_fetch_assimp_material_texture(
	const AssetLoadContext &ctx,
	const String &directory,
	const aiMaterial *ai_material,
	aiTextureType type
)
{
	if (ai_material->GetTextureCount(type) <= 0)
		return AssetHandle::invalid();

	aiString texture_path = {};
	ai_material->GetTexture(type, 0, &texture_path);

	String path = directory + String(texture_path.C_Str());

	AssetHandle handle = ctx.assets.from_file_path(path);
	ctx.assets.load_async(handle, ast::ASSET_TYPE_TEXTURE);

	return handle;
}

static gfx::Material load_material_from_assimp(
	const AssetLoadContext &ctx,
	const String &directory,
	const aiMaterial *ai_material
)
{
	/*
	debug_log("PROPERTIES OF %s:", ai_material->GetName().C_Str());

	for (int i = 0; i < ai_material->mNumProperties; i++) {
		auto property = ai_material->mProperties[i];

		debug_log("* %s: %f", property->mKey.C_Str(), *(float *)property->mData);
	}

	debug_log();
	*/

	gfx::Material material = {};
	material.albedo             = try_fetch_assimp_material_texture(ctx, directory, ai_material, aiTextureType_DIFFUSE);
	material.normal             = try_fetch_assimp_material_texture(ctx, directory, ai_material, aiTextureType_NORMALS);
	material.emissive           = try_fetch_assimp_material_texture(ctx, directory, ai_material, aiTextureType_EMISSIVE);
	material.metallic_roughness = try_fetch_assimp_material_texture(ctx, directory, ai_material, aiTextureType_DIFFUSE_ROUGHNESS);
	material.ambient            = try_fetch_assimp_material_texture(ctx, directory, ai_material, aiTextureType_LIGHTMAP);

	return material;
}

static void process_sub_model(
	const AssetLoadContext &ctx,
	ModelLoadData &load_data,
	gfx::SubModel &sub_model, u32 sub_model_index,
	const String &directory,
	const aiMesh *assimp_mesh,
	const aiScene *scene,
	const aiMatrix4x4 &transform
)
{
	Vector<gfx::gpu_types::GpuModelVertex> vertices;

	// TODO: Transforms should be applied when rendering (so be a member of a Sub_model)
	//       rather than being directly applied to vertices when loading them in.
	for (int i = 0; i < assimp_mesh->mNumVertices; i++) {
		gfx::gpu_types::GpuModelVertex vertex = {};

		if (assimp_mesh->HasPositions()) {
			aiVector3D position = assimp_basis * assimp_mesh->mVertices[i];
			vertex.position = Vec3(position.x, position.y, position.z);
		} else {
			vertex.position = Vec3(0.f, 0.f, 0.f);
		}

		if (assimp_mesh->HasTextureCoords(0)) {
			aiVector3D uv = assimp_mesh->mTextureCoords[0][i];
			vertex.texcoord = Vec2(uv.x, uv.y);
		} else {
			vertex.texcoord = Vec2(0.f, 0.f);
		}

		if (assimp_mesh->HasVertexColors(0)) {
			aiColor4D colour = assimp_mesh->mColors[0][i];
			vertex.colour = Vec3(colour.r, colour.g, colour.b);
		} else {
			vertex.colour = Vec3(1.f, 1.f, 1.f);
		}

		if (assimp_mesh->HasNormals()) {
			aiVector3D normal = assimp_basis * assimp_mesh->mNormals[i];
			vertex.normal = Vec3(normal.x, normal.y, normal.z);
		} else {
			vertex.normal = Vec3(0.f, 0.f, 0.f);
		}

		if (assimp_mesh->HasTangentsAndBitangents()) {
			aiVector3D tangent = assimp_basis * assimp_mesh->mTangents[i];
			aiVector3D bitangent = assimp_basis * assimp_mesh->mBitangents[i];

			vertex.tangent   = Vec3(tangent.x, tangent.y, tangent.z);
			vertex.bitangent = Vec3(bitangent.x, bitangent.y, bitangent.z);
		} else {
			vertex.tangent   = Vec3(0.f, 0.f, 0.f);
			vertex.bitangent = Vec3(0.f, 0.f, 0.f);
		}

		vertices.push_back(vertex);
	}
	
	Vector<gfx::IndexType> indices;

	for (int i = 0; i < assimp_mesh->mNumFaces; i++) {
		struct aiFace *face = assimp_mesh->mFaces + i;

		for (int j = 0; j < face->mNumIndices; j++)
			indices.push_back(face->mIndices[j]);
	}
	
	ParsedMesh parsed_mesh = {};
	parsed_mesh.target_sub_model = sub_model_index;
	parsed_mesh.vertices = vertices;
	parsed_mesh.indices = indices;

	load_data.meshes.push_back(parsed_mesh);

	load_data.total_vertex_size += parsed_mesh.vertices.size() * sizeof(gfx::gpu_types::GpuModelVertex);
	load_data.total_index_size += parsed_mesh.indices.size() * sizeof(gfx::IndexType);

	const aiMaterial *assimp_material = scene->mMaterials[assimp_mesh->mMaterialIndex];
	sub_model.material = load_material_from_assimp(ctx, directory, assimp_material);

	sub_model.transform = aiMatrix4x4_to_Mat4(transform);
}

static void process_nodes(
	const AssetLoadContext &ctx,
	ModelLoadData &load_data,
	const String &directory,
	const aiNode *node,
	const aiScene *scene,
	const aiMatrix4x4 &parent_transform
)
{
	aiMatrix4x4 node_transform = node->mTransformation;
	aiMatrix4x4 current_transform = parent_transform * node_transform;

	for (int i = 0; i < node->mNumMeshes; i++) {
		const aiMesh *assimp_mesh = scene->mMeshes[node->mMeshes[i]];

		float opacity = 1.f;

		if (scene->mMaterials[assimp_mesh->mMaterialIndex]->Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS)
			opacity = 1.f;

		if (opacity <= 0.9f)
			continue;

		load_data.model.sub_models.emplace_back();
		gfx::SubModel &sub_model = load_data.model.sub_models.back();
		process_sub_model(ctx, load_data, sub_model, load_data.model.sub_models.size() - 1, directory, assimp_mesh, scene, current_transform);
	}

	for (int i = 0; i < node->mNumChildren; i++) {
		const aiNode *child = node->mChildren[i];
		process_nodes(ctx, load_data, directory, child, scene, current_transform);
	}
}

static AssetLoadResult model_load(const AssetLoadContext &ctx)
{
	String system_file_path = ctx.system_file_path();

	const aiScene *scene = ai_importer.ReadFile(
		system_file_path,
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_PreTransformVertices
	);

	bool failed_to_load =
		!scene ||
		(scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0 ||
		!scene->mRootNode;

	ModelLoadData *load_data = new ModelLoadData();

	AssetLoadResult result = {};
	result.data = load_data;
	result.stage_size = 0;
	result.failed = failed_to_load;

	if (!failed_to_load) {
		String directory = io::path::get_file_directory(ctx.metadata.file_path) + "/";

		const static aiMatrix4x4 identity = {
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 0.f, 0.f, 1.f
		};

		process_nodes(ctx, *load_data, directory, scene->mRootNode, scene, identity);

		result.stage_size =
			load_data->total_vertex_size +
			load_data->total_index_size;
	}

	return result;
}

static Asset *model_finalize(
	const AssetLoadContext &ctx, const AssetLoadResult &result,
	gfx::Device &device, gfx::CommandBuffer &cmd,
	gfx::GpuBuffer *stage, u64 stage_base
)
{
	ModelLoadData *load_data = (ModelLoadData *)result.data;

	u64 sub_offset = 0;

	for (auto &parsed : load_data->meshes) {
		gfx::Mesh &mesh = load_data->model.sub_models[parsed.target_sub_model].mesh;

		mesh.create_buffers(
			&device,
			sizeof(gfx::gpu_types::GpuModelVertex),
			parsed.vertices.size(), parsed.indices.size()
		);

		mesh.write_to_staging_buffer(
			stage, stage_base + sub_offset,
			parsed.vertices.data(), parsed.indices.data()
		);

		sub_offset += mesh.batch_upload(
			cmd, stage, stage_base + sub_offset
		);
	}

	return new ModelAsset(load_data->model);
}

static void model_clean_up(void *data)
{
	ModelLoadData *load_data = (ModelLoadData *)data;
	delete load_data;
}

AssetSerializer ast::get_model_serializer()
{
	AssetSerializer model_serializer = {};
	model_serializer.load = model_load;
	model_serializer.finalize = model_finalize;
	model_serializer.clean_up = model_clean_up;

	return model_serializer;
}
