#pragma once

#include "config.h"

#include "platform/platform_core.h"
#include "graphics/graphics_core.h"

#include "graphics/pipeline.h"
#include "graphics/descriptor.h"
#include "graphics/image_view.h"

#include "input/input.h"

#include "rendering/model_loader.h"
#include "rendering/texture_manager.h"
#include "rendering/shader_manager.h"
#include "rendering/renderer.h"
#include "rendering/camera.h"
#include "rendering/scene.h"
#include "rendering/bindless.h"

namespace mgp
{
	class CommandBuffer;

	class App
	{
	public:
		App() = default;
		~App() = default;

		void run(const Config &config);
		void exit();

		PlatformCore *getPlatform() { return m_platform; }
		GraphicsCore *getGraphics() { return m_graphics; }

		InputState &getInputSt() { return m_inputSt; }
		Renderer &getRenderer() { return m_renderer; }

		ModelLoader *getModelLoader() { return m_modelLoader; }
		BindlessResources *getBindlessResources() { return m_bindlessResources; }

		TextureManager &getTextures() { return m_textures; }
		ShaderManager &getShaders() { return m_shaders; }

		PipelineCache &getPipelines() { return m_pipelines; }
		DescriptorLayoutCache &getDescriptorLayouts() { return m_descriptorLayouts; }
		ImageViewCache &getImageViews() { return m_imageViews; }

	private:
		void init();
		void destroy();
		void configure(const Config &config);

		void tick(float dt);
		void tickFixed(float dt);
		void render(CommandBuffer *cmd);

		Config m_config;
		bool m_running;

		PlatformCore *m_platform;
		GraphicsCore *m_graphics;

		Scene m_scene;
		Camera m_camera;

		InputState m_inputSt;
		Renderer m_renderer;

		PipelineCache m_pipelines;
		DescriptorLayoutCache m_descriptorLayouts;
		ImageViewCache m_imageViews;

		ModelLoader *m_modelLoader;
		BindlessResources *m_bindlessResources;

		TextureManager m_textures;
		ShaderManager m_shaders;
	};
}
