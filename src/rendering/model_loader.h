#pragma once

#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "vulkan/bindless.h"

namespace mgp
{
	class VulkanCore;
	class Model;
	class Mesh;
	class Image;
	class ImageView;
	class App;

	class ModelLoader
	{
	public:
		ModelLoader(VulkanCore *core, App *app);
		~ModelLoader();

		Model *loadMesh(const std::string &path);

	private:
		void processNodes(Model *mesh, aiNode *node, const aiScene *scene, const aiMatrix4x4& transform);
		void processSubMesh(Mesh *submesh, aiMesh *assimpMesh, const aiScene *scene, const aiMatrix4x4& transform);

		void fetchMaterialBoundTextures(std::vector<bindless::Handle> &textures, const std::string &localPath, const aiMaterial *material, aiTextureType type, Image *fallback);
		std::vector<Image *> loadMaterialTextures(const aiMaterial *material, aiTextureType type, const std::string &localPath);

		Assimp::Importer m_importer;

		VulkanCore *m_core;
		App *m_app;
	};
}
