#include "mesh_loader.h"
#include "texture_mgr.h"
#include "material_system.h"

llt::MeshLoader* llt::g_meshLoader = nullptr;

using namespace llt;

MeshLoader::MeshLoader()
	: m_meshCache()
	, m_importer()
{
}

MeshLoader::~MeshLoader()
{
	for (auto& [name, mesh] : m_meshCache) {
		delete mesh;
	}

	m_meshCache.clear();
}

Mesh* MeshLoader::loadMesh(const String& name, const String& path)
{
	if (m_meshCache.contains(name)) {
		return m_meshCache.get(name);
	}

	const aiScene* scene = m_importer.ReadFile(path.cstr(), aiProcess_Triangulate | aiProcess_FlipWindingOrder | aiProcess_CalcTangentSpace);

	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		LLT_ERROR("[MESHLOADER] Failed to load mesh at path: %s, Error: %s", path.cstr(), m_importer.GetErrorString());
		return nullptr;
	}

	Mesh* mesh = new Mesh();

	processNodes(mesh, scene->mRootNode, scene);

	m_meshCache.insert(name, mesh);
	return mesh;
}

void MeshLoader::processNodes(Mesh* mesh, aiNode* node, const aiScene* scene)
{
	for(int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
		processSubMesh(mesh->createSubmesh(), assimpMesh, scene);
	}

	for(int i = 0; i < node->mNumChildren; i++)
	{
		processNodes(mesh, node->mChildren[i], scene);
	}
}

void MeshLoader::processSubMesh(llt::SubMesh* submesh, aiMesh* assimpMesh, const aiScene* scene)
{
	Vector<ModelVertex> vertices;
	Vector<uint16_t> indices;

	for (int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		const aiVector3D& pos = assimpMesh->mVertices[i];
		const aiVector3D& nml = assimpMesh->mNormals[i];

		ModelVertex vertex;
		vertex.pos = { pos.x, pos.y, pos.z };
		vertex.norm = { nml.x, nml.y, nml.z };

		if (assimpMesh->mColors[0])
		{
			const aiColor4D& col = assimpMesh->mColors[0][i];

			vertex.col = { col.r, col.g, col.b };
		}
		else
		{
			vertex.col = { 1.0f, 1.0f, 1.0f };
		}

		if (assimpMesh->mTextureCoords[0])
		{
			const aiVector3D& uv = assimpMesh->mTextureCoords[0][i];

			vertex.uv = { uv.x, uv.y };
		}
		else
		{
			vertex.uv = { 0.0f, 0.0f };
		}

		if (assimpMesh->mTangents)
		{
			const aiVector3D& tangent = assimpMesh->mTangents[i];

			vertex.tangent = { tangent.x, tangent.y, tangent.z };
		}
		else
		{
			vertex.tangent = { 0.0f, 0.0f, 0.0f };
		}

		vertices.pushBack(vertex);
	}

	for (int i = 0; i < assimpMesh->mNumFaces; i++)
	{
		const aiFace& face = assimpMesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.pushBack(face.mIndices[j]);
		}
	}

	submesh->build(
		vertices.data(), vertices.size(),
		sizeof(ModelVertex),
		indices.data(), indices.size()
	);

	if (assimpMesh->mMaterialIndex >= 0)
	{
		const aiMaterial* assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];

		Vector<Texture*> diffuseMaps = loadMaterialTextures(assimpMaterial, aiTextureType_DIFFUSE);
		Vector<Texture*> specularMaps = loadMaterialTextures(assimpMaterial, aiTextureType_SPECULAR);
		Vector<Texture*> normalMaps = loadMaterialTextures(assimpMaterial, aiTextureType_HEIGHT);

		MaterialData data;
		data.technique = "texturedPBR_opaque";

		for (auto& tex : diffuseMaps)
		{
			BoundTexture boundTexture = {};
			boundTexture.texture = tex;
			boundTexture.sampler = g_textureManager->getSampler("linear");

			data.textures.pushBack(boundTexture);
		}

		for (auto& tex : specularMaps)
		{
			BoundTexture boundTexture = {};
			boundTexture.texture = tex;
			boundTexture.sampler = g_textureManager->getSampler("linear");

			data.textures.pushBack(boundTexture);
		}

		for (auto& tex : normalMaps)
		{
			BoundTexture boundTexture = {};
			boundTexture.texture = tex;
			boundTexture.sampler = g_textureManager->getSampler("linear");

			data.textures.pushBack(boundTexture);
		}

		Material* material = g_materialSystem->buildMaterial(data);

		submesh->setMaterial(material);
	}
}

Vector<Texture*> MeshLoader::loadMaterialTextures(const aiMaterial* material, aiTextureType type)
{
	Vector<Texture*> result;

	for (int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString texturePath;
		material->GetTexture(type, i, &texturePath);

		Texture* tex = g_textureManager->getTexture(texturePath.C_Str());

		if (!tex) {
			tex = g_textureManager->createFromImage(texturePath.C_Str(), Image(texturePath.C_Str()));
		}

		result.pushBack(tex);
	}

	return result;
}
