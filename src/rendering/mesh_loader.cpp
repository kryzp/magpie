#include "mesh_loader.h"
#include "texture_mgr.h"
#include "material_system.h"

llt::MeshLoader *llt::g_meshLoader = nullptr;

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

Mesh *MeshLoader::loadMesh(const String &name, const String &path)
{
	if (m_meshCache.contains(name)) {
		return m_meshCache.get(name);
	}

	const aiScene *scene = m_importer.ReadFile(path.cstr(),
		aiProcess_Triangulate |
		aiProcess_FlipWindingOrder |
		aiProcess_CalcTangentSpace |
		aiProcess_FlipUVs
	);

	if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		LLT_ERROR("Failed to load mesh at path: %s, Error: %s", path.cstr(), m_importer.GetErrorString());
		return nullptr;
	}

	Mesh *mesh = new Mesh();

	aiMatrix4x4 identity(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	processNodes(mesh, scene->mRootNode, scene, identity);

	m_meshCache.insert(name, mesh);
	return mesh;
}

void MeshLoader::processNodes(Mesh *mesh, aiNode *node, const aiScene *scene, const aiMatrix4x4& transform)
{
	for(int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh *assimpMesh = scene->mMeshes[node->mMeshes[i]];
		processSubMesh(mesh->createSubmesh(), assimpMesh, scene, node->mTransformation * transform);
	}

	for(int i = 0; i < node->mNumChildren; i++)
	{
		processNodes(mesh, node->mChildren[i], scene, node->mTransformation * transform);
	}
}

void MeshLoader::processSubMesh(SubMesh *submesh, aiMesh *assimpMesh, const aiScene *scene, const aiMatrix4x4& transform)
{
	Vector<ModelVertex> vertices;
	Vector<uint16_t> indices;

	for (int i = 0; i < assimpMesh->mNumVertices; i++)
	{
		const aiVector3D &vtx = transform * assimpMesh->mVertices[i];

		ModelVertex vertex = {};

		vertex.pos = { vtx.x, vtx.y, vtx.z };

		if (assimpMesh->HasNormals())
		{
			const aiVector3D &nml = transform * assimpMesh->mNormals[i]; // this literally wont work lmao

			vertex.norm = { nml.x, nml.y, nml.z };
		}
		else
		{
			vertex.norm = { 0.0f, 0.0f, 1.0f };
		}

		if (assimpMesh->HasVertexColors(0))
		{
			const aiColor4D &col = assimpMesh->mColors[0][i];

			vertex.col = { col.r, col.g, col.b };
		}
		else
		{
			vertex.col = { 1.0f, 1.0f, 1.0f };
		}


		if (assimpMesh->HasTextureCoords(0))
		{
			const aiVector3D &uv = assimpMesh->mTextureCoords[0][i];

			vertex.uv = { uv.x, uv.y };
		}
		else
		{
			vertex.uv = { 0.0f, 0.0f };
		}

		if (assimpMesh->HasTangentsAndBitangents())
		{
			const aiVector3D &tangent = assimpMesh->mTangents[i];

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
		const aiFace &face = assimpMesh->mFaces[i];

		for (int j = 0; j < face.mNumIndices; j++)
		{
			indices.pushBack(face.mIndices[j]);
		}
	}

	submesh->build(
		g_modelVertexFormat,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);

	if (assimpMesh->mMaterialIndex >= 0)
	{
		const aiMaterial *assimpMaterial = scene->mMaterials[assimpMesh->mMaterialIndex];

		MaterialData data;
		data.technique = "texturedPBR_opaque"; // temporarily just the forced material type

		fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_DIFFUSE);
		fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_LIGHTMAP);
		fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS);
		fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_NORMALS);
		fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_EMISSIVE);
		//fetchMaterialBoundTextures(data.textures, assimpMaterial, aiTextureType_DIFFUSE_ROUGHNESS);

		Material *material = g_materialSystem->buildMaterial(data);

		submesh->setMaterial(material);
	}
}

void MeshLoader::fetchMaterialBoundTextures(Vector<BoundTexture>& textures, const aiMaterial *material, aiTextureType type)
{
	Vector<Texture*> maps = loadMaterialTextures(material, type);

	for (auto &tex : maps)
	{
		BoundTexture boundTexture = {};
		boundTexture.texture = tex;
		boundTexture.sampler = g_textureManager->getSampler("linear");

		textures.pushBack(boundTexture);
		return; // for now only want one of each!
	}
}

Vector<Texture*> MeshLoader::loadMaterialTextures(const aiMaterial *material, aiTextureType type)
{
	Vector<Texture*> result;

	for (int i = 0; i < material->GetTextureCount(type); i++)
	{
		aiString texturePath;
		material->GetTexture(type, i, &texturePath);

		aiString basePath = aiString("../res/models/GLTF/DamagedHelmet/"); // todo????
		basePath.Append(texturePath.C_Str());

		Texture *tex = g_textureManager->getTexture(basePath.C_Str());

		if (!tex)
			tex = g_textureManager->load(basePath.C_Str(), basePath.C_Str());

		result.pushBack(tex);
	}

	return result;
}
