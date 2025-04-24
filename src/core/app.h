#pragma once

#include <functional>
#include <unordered_map>

#include "third_party/volk.h"

#include "common.h"

#include "rendering/camera.h"
#include "rendering/scene.h"

#include "vulkan/bindless.h"
#include "vulkan/pipeline_cache.h"

namespace mgp
{
	enum WindowMode
	{
		WINDOW_MODE_NONE = 0,

		WINDOW_MODE_WINDOWED_BIT	= 1 << 0,
		WINDOW_MODE_BORDERLESS_BIT	= 1 << 1,
		WINDOW_MODE_FULLSCREEN_BIT	= 1 << 2,

//		WINDOW_MODE_BORDERLESS_FULLSCREEN_BIT = WINDOW_MODE_BORDERLESS_BIT | WINDOW_MODE_FULLSCREEN_BIT
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

		WindowMode windowMode = WINDOW_MODE_WINDOWED_BIT;

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
	class Sampler;
	struct Material;
	struct MaterialData;
	class Mesh;
	class Technique;
	class GPUBuffer;
	class Image;

	struct EnvironmentProbe
	{
		Image *prefilter;
		Image *irradiance;
	};

	class App
	{
		friend class DeferredPass; // todo: temporary

	public:
		App(const Config &config);
		~App();

		void run();
		void exit();

		VkDescriptorSet allocateSet(const std::vector<VkDescriptorSetLayout> &layouts);

		Image *loadTexture(const std::string &name, const std::string &path);
		Material *buildMaterial(MaterialData &data);

		Shader *getShader(const std::string &name);

	private:
		void tick(float dt);
		void tickFixed(float dt);
		void render(Swapchain *swapchain);

		Config m_config;

		VulkanCore *m_vulkanCore;
		Platform *m_platform;

		InputState *m_inputSt;

		bool m_running;

		Camera m_camera;

		Image *m_targetColour;
		Image *m_targetDepth;

		ShadowMapAtlas m_shadowAtlas;
		GPUBuffer *m_lightBuffer;

		void loadTextures();
		void loadShaders();
		void loadTechniques();

		ShaderStage *getShaderStage(const std::string &name);
		ShaderStage *loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType);
		Shader *createShader(const std::string &name);

		void addTechnique(const std::string &name, const Technique &technique);

		Sampler *m_linearSampler;
		Sampler *m_nearestSampler;

		std::unordered_map<std::string, Image *> m_loadedImageCache;

		std::unordered_map<std::string, ShaderStage *> m_shaderStageCache;
		std::unordered_map<std::string, Shader *> m_shaderCache;

		std::unordered_map<uint64_t, Material *> m_materials;
		std::unordered_map<std::string, Technique> m_techniques;

		GPUBuffer *m_frameConstantsBuffer;
		GPUBuffer *m_transformDataBuffer;

		GPUBuffer *m_bindlessMaterialTable;
		bindless::Handle m_materialHandle_UID;

		GraphicsPipelineDef m_textureUVPipeline;
		VkDescriptorSet m_textureUVSet;

		ComputePipelineDef m_hdrTonemappingPipeline;
		VkDescriptorSet m_hdrTonemappingSet;

		Scene m_scene;

		void createSkyboxMesh();
		void createSkybox();

		Mesh *m_skyboxMesh;
		Shader *m_skyboxShader;
		VkDescriptorSet m_skyboxSet;
		GraphicsPipelineDef m_skyboxPipeline;

		DescriptorPoolDynamic m_descriptorPool;

		void generateEnvironmentProbe();

		Image *m_environmentMap;
		EnvironmentProbe m_environmentProbe;
	};
}
