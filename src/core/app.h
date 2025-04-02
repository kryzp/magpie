#pragma once

#include <functional>
#include <unordered_map>

#include "third_party/volk.h"

#include "common.h"

#include "rendering/camera.h"

#include "vulkan/pipeline_cache.h"

namespace mgp
{
	enum WindowMode
	{
		WINDOW_MODE_WINDOWED_BIT	= 0 << 0,
		WINDOW_MODE_BORDERLESS_BIT	= 1 << 0,
		WINDOW_MODE_FULLSCREEN_BIT	= 1 << 1,

//		WINDOW_MODE_BORDERLESS_FULLSCREEN_BIT = WINDOW_MODE_BORDERLESS_BIT | WINDOW_MODE_FULLSCREEN_BIT
	};

	enum ConfigFlag
	{
		CONFIG_FLAG_NONE_BIT				= 0 << 0,
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

		WindowMode windowMode = WINDOW_MODE_WINDOWED_BIT;

		std::function<void(void)> onInit = nullptr;
		std::function<void(void)> onExit = nullptr;
		std::function<void(void)> onDestroy = nullptr;

		constexpr bool hasFlag(ConfigFlag flag) const { return flags & flag; }
	};

	// yes
	class Platform;
	class VulkanCore;
	class Swapchain;
	class InputState;
	class CommandBuffer;
	class ShaderStage;
	class Shader;
	class Image;
	class Sampler;
	class Material;
	class MaterialData;
	class Technique;

	class EnvironmentProbe
	{
		Image *prefilter;
		Image *irradiance;
	};

	class App
	{
	public:
		App(const Config &config);
		~App();

		void run();
		void exit();

	private:
		void tick(float dt);
		void tickFixed(float dt);
		void render(CommandBuffer &inFlightCmd);

		void loadTextures();
		void loadShaders();
		void loadTechniques();

//		void createMaterialIdBuffer();

		Image *loadTexture(const std::string &name, const std::string &path);

		ShaderStage *getShaderStage(const std::string &name);
		Shader *getShader(const std::string &name);
		ShaderStage *loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType);
		Shader *createShader(const std::string &name);

		Material *buildMaterial(MaterialData &data);
		void addTechnique(const std::string &name, const Technique &technique);

		Sampler *m_linearSampler;
		std::unordered_map<std::string, Image *> m_imageCache;

		std::unordered_map<std::string, ShaderStage *> m_shaderStageCache;
		std::unordered_map<std::string, Shader *> m_shaderCache;

		std::unordered_map<uint64_t, Material *> m_materials;
		std::unordered_map<std::string, Technique> m_techniques;

		void generateEnvironmentProbe(CommandBuffer &cmd);
		void precomputeBRDF(CommandBuffer &cmd);

		Image *m_environmentMap;
		EnvironmentProbe m_environmentProbe;
		Image *m_brdfLUT;

//		DescriptorPoolDynamic m_descriptorPoolAllocator;

		GraphicsPipelineDefinition m_equirectangularToCubemapPipeline;
		GraphicsPipelineDefinition m_irradianceGenerationPipeline;
		GraphicsPipelineDefinition m_prefilterGenerationPipeline;
		GraphicsPipelineDefinition m_brdfIntegrationPipeline;

//		GPUBuffer *m_materialIdBuffer;
//		bindless::Handle m_material_UID;

		void initImGui();

		Config m_config;
		
		VulkanCore *m_vulkanCore;
		Platform *m_platform;

		InputState *m_inputSt;

		bool m_running;

		Camera m_camera;
	};
}
