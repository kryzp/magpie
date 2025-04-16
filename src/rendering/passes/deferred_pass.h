#pragma once

namespace mgp
{
	class Scene;
	class Camera;
	class Image;
	class ImageView;
	class VulkanCore;
	class CommandBuffer;
	class RenderInfo;
	class App;

	class GBuffer
	{
	};

	/*
	 * Still just a forward renderer lmao
	 * Will be deferred at some point!!!
	 */
	class DeferredPass
	{
	public:
		static void init(VulkanCore *core, App *app);
		static void destroy();

		static void render(CommandBuffer &cmd, const RenderInfo &info, const Camera& camera, Scene& scene, const ImageView *shadowAtlas);

	private:
		static void precomputeBRDF();

		static VulkanCore *m_core;
		static App *m_app;

		static Image *m_brdfLUT;
	};
}
