#pragma once

#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "rendering/bindless.h"

namespace mgp
{
	class App;
	class Model;
	class Mesh;
	class Image;

	class ModelLoader
	{
	public:
		ModelLoader(App *app);
		~ModelLoader() = default;

		Model *loadModel(const std::string &path);

	private:
		App *m_app;

		void processNodes(Model *mesh, aiNode *node, const aiScene *scene, const aiMatrix4x4& transform);
		void processSubMesh(Mesh *submesh, aiMesh *assimpMesh, const aiScene *scene, const aiMatrix4x4& transform);

		void fetchMaterialBoundTextures(std::vector<BindlessHandle> &textures, const std::string &localPath, const aiMaterial *material, aiTextureType type, Image *fallback);
		std::vector<Image *> loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &localPath);

		Assimp::Importer m_importer;
	};
}
