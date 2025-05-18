#include "model_loader.h"

#include <filesystem>

#include "core/app.h"
#include "core/common.h"

#include "vulkan/vertex_format.h"
#include "vulkan/image.h"
#include "vulkan/image_view.h"

#include "model.h"
#include "material.h"

using namespace mgp;

ModelLoader::ModelLoader(VulkanCore *core, App *app)
	: m_importer()
	, m_core(core)
	, m_app(app)
{
}

ModelLoader::~ModelLoader()
{
}

Model *ModelLoader::loadModel(const std::string &path)
{
	const aiScene *scene = m_importer.ReadFile(path.c_str(),
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs
	);

	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		MGP_ERROR("Failed to load mesh at path: %s, Error: %s", path.c_str(), m_importer.GetErrorString());
		return nullptr;
	}

	std::filesystem::path filePath(path);
	std::string directory = filePath.parent_path().string() + "/";

	Model *mesh = new Model(m_core);
	mesh->setDirectory(directory);

	aiMatrix4x4 identity(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	processNodes(mesh, scene->mRootNode, scene, identity);

	return mesh;
}

void ModelLoader::processNodes(Model *mesh, aiNode *node, const aiScene *scene, const aiMatrix4x4& transform)
{
	for(int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *assimpMesh = scene->mMeshes[node->mMeshes[i]];
		processSubMesh(mesh->createMesh(), assimpMesh, scene, node->mTransformation * transform);
	}

	for(int i = 0; i < node->mNumChildren; i++)
	{
		processNodes(mesh, node->mChildren[i], scene, node->mTransformation * transform);
	}
}

void ModelLoader::processSubMesh(Mesh *submesh, aiMesh *assimpMesh, const aiScene *scene, const aiMatrix4x4& transform)
{
	std::vector<vtx::ModelVertex> vertices(assimpMesh->mNumVertices);
	std::vector<uint16_t> indices;

	for (int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		const aiVector3D &vtx = transform * assimpMesh->mVertices[i];

		vtx::ModelVertex vertex = {};

		vertex.position = { vtx.x, vtx.y, vtx.z };

		if (assimpMesh->HasTextureCoords(0))
		{
			const aiVector3D &uv = assimpMesh->mTextureCoords[0][i];

			vertex.uv = { uv.x, uv.y };
		}
		else
		{
			vertex.uv = { 0.0f, 0.0f };
		}

		if (assimpMesh->HasVertexColors(0))
		{
			const aiColor4D &col = assimpMesh->mColors[0][i];

			vertex.colour = { col.r, col.g, col.b };
		}
		else
		{
			vertex.colour = { 1.0f, 1.0f, 1.0f };
		}

		if (assimpMesh->HasNormals())
		{
			const aiVector3D &nml = transform * assimpMesh->mNormals[i]; // this literally wont work lmao

			vertex.normal = { nml.x, nml.y, nml.z };
		}
		else
		{
			vertex.normal = { 0.0f, 0.0f, 1.0f };
		}

		if (assimpMesh->HasTangentsAndBitangents())
		{
			const aiVector3D &tangent = transform * assimpMesh->mTangents[i];
			const aiVector3D &bitangent = transform * assimpMesh->mBitangents[i];

			vertex.tangent = { tangent.x, tangent.y, tangent.z };
			vertex.bitangent = { bitangent.x, bitangent.y, bitangent.z };
		}
		else
		{
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
			vertex.bitangent = { 0.0f, 0.0f, 0.0f };
		}

		vertices[i] = vertex;
	}

	for (int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		const aiFace &face = assimpMesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	submesh->build(
		vtx::MODEL_VERTEX_FORMAT,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);

	if (assimpMesh->mMaterialIndex >= 0)
	{
		const aiMaterial *assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];

		MaterialData data;
		data.technique = "texturedPBR_opaque"; // temporarily just the forced material type

		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE,				m_app->getTextures().getFallbackDiffuse());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_LIGHTMAP,				m_app->getTextures().getFallbackAmbient());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS,	m_app->getTextures().getFallbackRoughnessMetallic());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_NORMALS,				m_app->getTextures().getFallbackNormals());
		fetchMaterialBoundTextures(data.textures, submesh->getParent()->getDirectory(), assimpMaterial, aiTextureType_EMISSIVE,				m_app->getTextures().getFallbackEmissive());

		submesh->setMaterial(m_app->getRenderer().buildMaterial(data));
	}
}

void ModelLoader::fetchMaterialBoundTextures(std::vector<bindless::Handle> &textures, const std::string &localPath, const aiMaterial *material, aiTextureType type, Image *fallback)
{
	std::vector<Image *> maps = loadMaterialTextures(material, type, localPath);

	if (maps.size() >= 1)
	{
		textures.push_back(maps[0]->getStandardView()->getBindlessHandle());
	}
	else
	{
		if (fallback)
		{
			textures.push_back(fallback->getStandardView()->getBindlessHandle());
		}
	}
}

std::vector<Image *> ModelLoader::loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &localPath)
{
	std::vector<Image *> result;

	for (int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString texturePath;
		material->GetTexture(type, i, &texturePath);

		aiString basePath = aiString(localPath.c_str());
		basePath.Append(texturePath.C_Str());

		result.push_back(m_app->getTextures().loadTexture(basePath.C_Str(), basePath.C_Str()));
	}

	return result;
}
