#pragma once

namespace mgp
{
	class Scene;
	class Camera;
	class Image;
	class VulkanCore;
	class CommandBuffer;
	class RenderInfo;
	class App;

	class DeferredRenderer
	{
	public:
		static void init(VulkanCore *core, App *app);
		static void destroy();

		static void render(CommandBuffer &cmd, const RenderInfo &info, const Camera& camera, Scene& scene);

	private:
		static void precomputeBRDF();

		static VulkanCore *m_core;
		static App *m_app;

		static Image *m_brdfLUT;
	};
}
