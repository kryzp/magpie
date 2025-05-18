#pragma once

#include <functional>
#include <unordered_map>

#include <Volk/volk.h>

#include "common.h"

#include "rendering/camera.h"
#include "rendering/scene.h"
#include "rendering/texture_manager.h"
#include "rendering/shader_manager.h"
#include "rendering/renderer.h"

#include "vulkan/bindless.h"

namespace mgp
{
	// yes
	class Platform;
	class VulkanCore;
	class Swapchain;
	class InputState;
	class CommandBuffer;
	class ShaderStage;
	class Shader;
	class Sampler;
	class Material;
	struct MaterialData;
	class Mesh;
	class Technique;
	class GPUBuffer;
	class Image;

	enum WindowMode
	{
		WINDOW_MODE_NONE = 0,

		WINDOW_MODE_WINDOWED,
		WINDOW_MODE_BORDERLESS,
		WINDOW_MODE_FULLSCREEN,
		WINDOW_MODE_BORDERLESS_FULLSCREEN
	};

	enum ConfigFlag
	{
		CONFIG_FLAG_NONE					= 0,

		CONFIG_FLAG_RESIZABLE_BIT			= 1 << 0,
		CONFIG_FLAG_VSYNC_BIT				= 1 << 1,
		CONFIG_FLAG_CURSOR_INVISIBLE_BIT	= 1 << 2,
		CONFIG_FLAG_CENTRE_WINDOW_BIT		= 1 << 3,
		CONFIG_FLAG_HIGH_PIXEL_DENSITY_BIT	= 1 << 4,
		CONFIG_FLAG_LOCK_CURSOR_BIT			= 1 << 5
	};

	struct Config
	{
		struct Version
		{
			unsigned variant = 0;
			unsigned major = 1;
			unsigned minor = 0;
			unsigned patch = 0;
		};

		const char *windowName = nullptr;
		const char *engineName = nullptr;

		Version appVersion;
		Version engineVersion;

		unsigned width = 1280;
		unsigned height = 720;
		unsigned targetFPS = 60;
		float opacity = 1.0f;
		int flags = 0;
		bool vsync = false;

		WindowMode windowMode = WINDOW_MODE_WINDOWED;

		constexpr bool hasFlag(ConfigFlag flag) const { return flags & flag; }
	};

	class App
	{
	public:
		App(const Config &config);
		~App();

		void run();
		void exit();

		TextureManager &getTextures() { return m_textures; }
		ShaderManager &getShaders() { return m_shaders; }

		Renderer &getRenderer() { return m_renderer; }

		Platform *getPlatform() { return m_platform; }
		VulkanCore *getVkCore() { return m_vulkanCore; }
		InputState *getInput() { return m_inputSt; }
		
		VkDescriptorSet allocateSet(const std::vector<VkDescriptorSetLayout> &layouts);

	private:
		void tick(float dt);
		void tickFixed(float dt);
		void render(Swapchain *swapchain);

		void applyConfig(const Config &config);

		Config m_config;

		Platform *m_platform;
		VulkanCore *m_vulkanCore;
		InputState *m_inputSt;

		bool m_running;

		Renderer m_renderer;
		Scene m_scene;
		Camera m_camera;

		TextureManager m_textures;
		ShaderManager m_shaders;

		DescriptorPoolDynamic m_descriptorPool;
	};
}
