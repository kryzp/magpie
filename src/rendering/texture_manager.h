#pragma once

#include <string>
#include <unordered_map>

namespace mgp
{
	class VulkanCore;
	class Image;
	class Sampler;

	class TextureManager
	{
	public:
		TextureManager();
		~TextureManager();

		void init(VulkanCore *core);
		void destroy();
		
		Image *getTexture(const std::string &name);
		Image *loadTexture(const std::string &name, const std::string &path);
		
		Sampler *getLinearSampler();
		Sampler *getNearestSampler();

		Image *getFallbackDiffuse();
		Image *getFallbackAmbient();
		Image *getFallbackRoughnessMetallic();
		Image *getFallbackNormals();
		Image *getFallbackEmissive();

	private:
		void loadTextures();

		VulkanCore *m_core;

		std::unordered_map<std::string, Image *> m_loadedImageCache;

		Sampler *m_linearSampler;
		Sampler *m_nearestSampler;
	};
}
