#pragma once

#include <string>
#include <unordered_map>

namespace mgp
{
	class GraphicsCore;

	class Image;
	class Sampler;

	class TextureManager
	{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		void init(GraphicsCore *gfx);
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
		GraphicsCore *m_gfx;

		void loadTextures();

		std::unordered_map<std::string, Image *> m_loadedImageCache;

		Sampler *m_linearSampler;
		Sampler *m_nearestSampler;
	};
}
