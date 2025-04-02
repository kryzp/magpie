#include "app.h"

#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "platform.h"
#include "debug_ui.h"

#include "vulkan/core.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/swapchain.h"
#include "vulkan/sync.h"
#include "vulkan/shader.h"
#include "vulkan/image.h"
#include "vulkan/sampler.h"
#include "vulkan/bitmap.h"
#include "vulkan/vertex_format.h"

#include "rendering/material.h"
#include "rendering/mesh_loader.h"
#include "rendering/mesh.h"

#include "input/input.h"

#include "math/timer.h"
#include "math/calc.h"
#include "math/colour.h"

using namespace mgp;

App::App(const Config &config)
	: m_config(config)
	, m_vulkanCore(nullptr)
	, m_platform(nullptr)
	, m_inputSt(nullptr)
	, m_running(false)
	, m_camera(config.width, config.height, 75.0f, 0.01f, 100.0f)
	, m_linearSampler(nullptr)
	, m_imageCache()
	, m_shaderStageCache()
	, m_shaderCache()
{
	m_platform = new Platform(config);
	m_vulkanCore = new VulkanCore(config, m_platform);

	m_inputSt = new InputState();

	vtx::initVertexTypes();

	initImGui();

	m_platform->setWindowName(m_config.windowName);
	m_platform->setWindowSize({ m_config.width, m_config.height });
	m_platform->setWindowMode(m_config.windowMode);
	m_platform->setWindowOpacity(m_config.opacity);
	m_platform->setCursorVisible(!m_config.hasFlag(CONFIG_FLAG_CURSOR_INVISIBLE_BIT));
	m_platform->toggleWindowResizable(m_config.hasFlag(CONFIG_FLAG_RESIZABLE_BIT));
	m_platform->lockCursor(m_config.hasFlag(CONFIG_FLAG_LOCK_CURSOR_BIT));

	if (m_config.hasFlag(CONFIG_FLAG_CENTRE_WINDOW_BIT))
	{
		glm::ivec2 screenSize = m_platform->getScreenSize();

		m_platform->setWindowPosition({
			(int)((screenSize.x - m_config.width) * 0.5f),
			(int)((screenSize.y - m_config.height) * 0.5f)
		});
	}

	m_camera.position = glm::vec3(0.0f, 2.75f, 3.5f);
	m_camera.setPitch(-glm::radians(30.0f));
	m_camera.update(m_inputSt, m_platform, 0.0f);

	dbgui::init();

	loadTextures();
	loadShaders();
	loadTechniques();

//	createMaterialIdBuffer();

	m_running = true;

	if (m_config.onInit)
		m_config.onInit();

	m_platform->onExit = [this]() { exit(); };
}

App::~App()
{
	if (m_config.onDestroy)
		m_config.onDestroy();

	for (auto &[id, material] : m_materials)
		delete material;

	delete m_linearSampler;

	for (cauto &[name, image] : m_imageCache)
		delete image;

	for (cauto &[name, stage] : m_shaderStageCache)
		delete stage;

	for (cauto &[name, shader] : m_shaderCache)
		delete shader;

	delete m_inputSt;

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	delete m_vulkanCore;
	delete m_platform;
}

struct FrameConstants
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec4 cameraPosition;
};

struct TransformData
{
	glm::mat4 model;
	glm::mat4 normalMatrix;
};

void App::run()
{
	double accumulator = 0.0;
	const double fixedDeltaTime = 1.0 / static_cast<double>(CalcU::min(m_config.targetFPS, m_platform->getWindowRefreshRate()));
	
	Timer deltaTimer(m_platform);
	deltaTimer.start();

	InFlightSync inFlightSync(m_vulkanCore);

	m_platform->onWindowResize = [&inFlightSync](int w, int h) -> void {
		MGP_LOG("Detected window resize!");
		inFlightSync.onWindowResize(w, h);
	};

	InstantSubmitSync instantSubmit(m_vulkanCore);
	CommandBuffer &instantCmd = instantSubmit.begin();
	{
		generateEnvironmentProbe(instantCmd);
		precomputeBRDF(instantCmd);
	}
	instantSubmit.submit();

	MeshLoader meshLoader(m_vulkanCore);
	Mesh *mesh = meshLoader.loadMesh("../../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");

	MaterialData materialData;
	materialData.technique = "whitePBR_opaque";

	Material *material = buildMaterial(materialData);

	GPUBuffer frameConstants(
		m_vulkanCore, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT/* | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT*/,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(FrameConstants)
	);

	GPUBuffer transformData(
		m_vulkanCore,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT/* | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT*/,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(TransformData)
	);

	while (m_running)
	{
		m_platform->pollEvents(m_inputSt);
		m_inputSt->update();

		if (m_inputSt->isPressed(KB_KEY_ESCAPE))
			exit();

		double deltaTime = deltaTimer.reset();

		tick(deltaTime);

		accumulator += CalcD::min(deltaTime, fixedDeltaTime);

		while (accumulator >= fixedDeltaTime)
		{
			tickFixed(fixedDeltaTime);

			accumulator -= fixedDeltaTime;
		}

		CommandBuffer &cmd = inFlightSync.begin();
		{
			render(cmd);

			FrameConstants constants;
			constants.proj = m_camera.getProj();
			constants.view = m_camera.getView();
			constants.cameraPosition = glm::vec4(m_camera.position, 1.0f);

			frameConstants.writeDataToMe(&constants, sizeof(FrameConstants), 0);

			TransformData transform;
			transform.model = glm::identity<glm::mat4>();
			transform.normalMatrix = glm::transpose(glm::inverse(transform.model));

			transformData.writeDataToMe(&transform, sizeof(TransformData), 0);

			cmd.beginRendering(inFlightSync.getSwapchain()->getRenderInfo());
			{
				PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(material->passes[SHADER_PASS_FORWARD], inFlightSync.getSwapchain()->getRenderInfo());

				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, pipelineData.layout, { m_vulkanCore->getBindlessResources().getSet() }, {});

				struct
				{
					int frameDataBuffer_ID;
					int transformBuffer_ID;
					int materialDataBuffer_ID;

					int transform_ID;

					int irradianceMap_ID;
					int prefilterMap_ID;
					int brdfLUT_ID;

					int material_ID;

					int cubemapSampler_ID;
					int textureSampler_ID;
				}
				pushConstants;
				{
					pushConstants.frameDataBuffer_ID	= frameConstants.getBindlessHandle();
					pushConstants.transformBuffer_ID	= transformData.getBindlessHandle();
					pushConstants.materialDataBuffer_ID = 0;

					pushConstants.transform_ID			= 0;

					pushConstants.irradianceMap_ID		= 0;//g_materialSystem->getIrradianceMap()->getStandardView().getBindlessHandle().id;
					pushConstants.prefilterMap_ID		= 0;//g_materialSystem->getPrefilterMap()->getStandardView().getBindlessHandle().id;
					pushConstants.brdfLUT_ID			= 0;//g_materialSystem->getBRDFLUT()->getStandardView().getBindlessHandle().id;

					pushConstants.material_ID			= 0;//mat->getBindlessHandle().id;

					pushConstants.cubemapSampler_ID		= m_linearSampler->getBindlessHandle();
					pushConstants.textureSampler_ID		= m_linearSampler->getBindlessHandle();
				}

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pushConstants),
					&pushConstants
				);

				mesh->getSubmesh(0)->render(cmd);
			}
			cmd.endRendering();
		}
		inFlightSync.present();

		m_vulkanCore->nextFrame();
	}

	m_vulkanCore->deviceWaitIdle();

	delete mesh;

	inFlightSync.destroy();
}

void App::exit()
{
	MGP_LOG("Detected window close event, quitting...");

	m_running = false;

	if (m_config.onExit)
		m_config.onExit();
}

void App::tick(float dt)
{
	//ImGui_ImplVulkan_NewFrame();
	//ImGui_ImplSDL3_NewFrame();
	//ImGui::NewFrame();

	//dbgui::update();
}

void App::tickFixed(float dt)
{
	if (m_inputSt->isDown(KB_KEY_F))
	{
		m_platform->setCursorVisible(false);
		m_platform->setCursorPosition(m_config.width / 2, m_config.height / 2, m_inputSt);

		m_camera.update(m_inputSt, m_platform, dt);
	}
	else
	{
		m_platform->setCursorVisible(true);
	}
}

void App::render(CommandBuffer &inFlightCmd)
{
}

void App::loadTextures()
{
	m_linearSampler = new Sampler(m_vulkanCore, Sampler::Style(VK_FILTER_LINEAR));

	loadTexture("fallback_white",	"../../res/textures/standard/white.png");
	loadTexture("fallback_black",	"../../res/textures/standard/black.png");
	loadTexture("fallback_normals",	"../../res/textures/standard/normal_fallback.png");

	loadTexture("environmentHDR",	"../../res/textures/spruit_sunrise_greg_zaal.hdr");

	loadTexture("stone",			"../../res/textures/smooth_stone.png");
	loadTexture("wood",				"../../res/textures/wood.jpg");
}

void App::loadShaders()
{
	// vertex shaders
	loadShaderStage("primitive_vs",						"../../res/shaders/compiled/primitive_vs.spv",						VK_SHADER_STAGE_VERTEX_BIT);
	loadShaderStage("skybox_vs",						"../../res/shaders/compiled/skybox_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
	loadShaderStage("primitive_quad_vs",				"../../res/shaders/compiled/primitive_quad_vs.spv",					VK_SHADER_STAGE_VERTEX_BIT);
	loadShaderStage("model_vs",							"../../res/shaders/compiled/model_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);

	// pixel shaders
	loadShaderStage("equirectangular_to_cubemap_ps",	"../../res/shaders/compiled/equirectangular_to_cubemap_ps.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("irradiance_convolution_ps",		"../../res/shaders/compiled/irradiance_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("prefilter_convolution_ps",			"../../res/shaders/compiled/prefilter_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("brdf_integrator_ps",				"../../res/shaders/compiled/brdf_integrator_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("whitePBR_ps",						"../../res/shaders/compiled/whitePBR_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("texturedPBR_ps",					"../../res/shaders/compiled/texturedPBR_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("subsurface_refraction_ps",			"../../res/shaders/compiled/subsurface_refraction_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("skybox_ps",						"../../res/shaders/compiled/skybox_ps.spv",							VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("hdr_tonemapping_ps",				"../../res/shaders/compiled/hdr_tonemapping_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("bloom_downsample_ps",				"../../res/shaders/compiled/bloom_downsample_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
	loadShaderStage("bloom_upsample_ps",				"../../res/shaders/compiled/bloom_upsample_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);

	// compute shaders
	loadShaderStage("hdr_tonemapping_cs",				"../../res/shaders/compiled/hdr_tonemapping_cs.spv",				VK_SHADER_STAGE_COMPUTE_BIT);

	// White PBR
	{
		Shader *pbr = createShader("whitePBR");

		pbr->setDescriptorSetLayouts({ m_vulkanCore->getBindlessResources().getLayout()});
		pbr->setPushConstantsSize(sizeof(int) * 16);
		pbr->addStage(getShaderStage("model_vs"));
		pbr->addStage(getShaderStage("whitePBR_ps"));
	}

	// PBR
	{
		Shader *pbr = createShader("texturedPBR");

		pbr->setDescriptorSetLayouts({ m_vulkanCore->getBindlessResources().getLayout()});
		pbr->setPushConstantsSize(sizeof(int) * 16);
		pbr->addStage(getShaderStage("model_vs"));
		pbr->addStage(getShaderStage("texturedPBR_ps"));
	}

	// SUBSURFACE REFRACTION
	{
		/*
		DescriptorLayoutBuilder descriptorLayoutBuilder;

		// bind all buffers
		for (int i = 0; i <= 2; i++)
		descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);

		// bind all textures
		for (int i = 3; i <= 10; i++)
		descriptorLayoutBuilder.bind(i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

		VkDescriptorSetLayout layout = descriptorLayoutBuilder.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *pbr = createEffect("subsurface_refraction");
		pbr->setDescriptorSetLayout(layout);
		pbr->setPushConstantsSize(0);
		pbr->addStage(g_shaderManager->get("model_vs"));
		pbr->addStage(g_shaderManager->get("subsurface_refraction_ps"));
		*/
	}

	// SKYBOX
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *skybox = createShader("skybox");

		skybox->setDescriptorSetLayouts({ layout });
		skybox->setPushConstantsSize(sizeof(float)*16 * 2);
		skybox->addStage(getShaderStage("skybox_vs"));
		skybox->addStage(getShaderStage("skybox_ps"));
	}

	// EQUIRECTANGULAR TO CUBEMAP
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *equirectangularToCubemap = createShader("equirectangular_to_cubemap");

		equirectangularToCubemap->setDescriptorSetLayouts({ layout });
		equirectangularToCubemap->setPushConstantsSize(sizeof(float)*16 * 2);
		equirectangularToCubemap->addStage(getShaderStage("primitive_vs"));
		equirectangularToCubemap->addStage(getShaderStage("equirectangular_to_cubemap_ps"));
	}

	// IRRADIANCE CONVOLUTION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *irradianceConvolution = createShader("irradiance_convolution");

		irradianceConvolution->setDescriptorSetLayouts({ layout });
		irradianceConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
		irradianceConvolution->addStage(getShaderStage("primitive_vs"));
		irradianceConvolution->addStage(getShaderStage("irradiance_convolution_ps"));
	}

	// PREFILTER CONVOLUTION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
			.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *prefilterConvolution = createShader("prefilter_convolution");

		prefilterConvolution->setDescriptorSetLayouts({ layout });
		prefilterConvolution->setPushConstantsSize(sizeof(float)*16 * 2);
		prefilterConvolution->addStage(getShaderStage("primitive_vs"));
		prefilterConvolution->addStage(getShaderStage("prefilter_convolution_ps"));
	}

	// BRDF LUT GENERATION
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *brdfLUT = createShader("brdf_lut");

		brdfLUT->setDescriptorSetLayouts({ layout });
		brdfLUT->setPushConstantsSize(0);
		brdfLUT->addStage(getShaderStage("primitive_quad_vs"));
		brdfLUT->addStage(getShaderStage("brdf_integrator_ps"));
	}

	// HDR TONEMAPPING
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.bind(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *hdrTonemapping = createShader("hdr_tonemapping");

		hdrTonemapping->setDescriptorSetLayouts({ layout });
		hdrTonemapping->setPushConstantsSize(sizeof(float) * 2);
		hdrTonemapping->addStage(getShaderStage("primitive_quad_vs"));
		hdrTonemapping->addStage(getShaderStage("hdr_tonemapping_ps"));

		/*
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
		.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.bind(1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		.build(VK_SHADER_STAGE_ALL_GRAPHICS);

		ShaderEffect *hdrTonemapping = createEffect("hdr_tonemapping");
		hdrTonemapping->setDescriptorSetLayouts({ layout });
		hdrTonemapping->setPushConstantsSize(sizeof(unsigned) * 2);
		hdrTonemapping->addStage(get("hdr_tonemapping_cs"));
		*/
	}

	// BLOOM DOWNSAMPLE
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *bloomDownsample = createShader("bloom_downsample");

		bloomDownsample->setDescriptorSetLayouts({ layout });
		bloomDownsample->setPushConstantsSize(sizeof(int));
		bloomDownsample->addStage(getShaderStage("primitive_quad_vs"));
		bloomDownsample->addStage(getShaderStage("bloom_downsample_ps"));
	}

	// BLOOM UPSAMPLE
	{
		VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
			.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

		Shader *bloomUpsample = createShader("bloom_upsample");

		bloomUpsample->setDescriptorSetLayouts({ layout });
		bloomUpsample->setPushConstantsSize(sizeof(float));
		bloomUpsample->addStage(getShaderStage("primitive_quad_vs"));
		bloomUpsample->addStage(getShaderStage("bloom_upsample_ps"));
	}
}

void App::loadTechniques()
{
	// WHITE PBR
	{
		Technique whitePBR;
		whitePBR.passes[SHADER_PASS_FORWARD] = getShader("whitePBR");
		whitePBR.passes[SHADER_PASS_SHADOW] = nullptr;
		whitePBR.vertexFormat = vtx::MODEL_VERTEX_FORMAT;
		addTechnique("whitePBR_opaque", whitePBR);
	}

	// PBR
	{
		Technique texturedPBR;
		texturedPBR.passes[SHADER_PASS_FORWARD] = getShader("texturedPBR");
		texturedPBR.passes[SHADER_PASS_SHADOW] = nullptr;
		texturedPBR.vertexFormat = vtx::MODEL_VERTEX_FORMAT;
		addTechnique("texturedPBR_opaque", texturedPBR);
	}

	// SUBSURFACE REFRACTION
	{
		/*
		Technique subsurfaceRefractionTechnique;
		subsurfaceRefractionTechnique.passes[SHADER_PASS_FORWARD] = g_shaderManager->getEffect("subsurface_refraction");
		subsurfaceRefractionTechnique.passes[SHADER_PASS_SHADOW] = nullptr;
		subsurfaceRefractionTechnique.vertexFormat = g_modelVertexFormat;
		addTechnique("subsurface_refraction", subsurfaceRefractionTechnique);
		*/
	}
}

/*
void MaterialRegistry::createMaterialIdBuffer()
{
	// todo: max material count is 16 right now temporarily
	m_materialIdBuffer = g_gpuBufferManager->createStorageBuffer(sizeof(Material) * 16);

	//	m_material_UID = 0;
}
*/

Image *App::loadTexture(const std::string &name, const std::string &path)
{
	if (m_imageCache.contains(name))
		return m_imageCache.at(name);

	Bitmap bitmap(path);

	Image *image = new Image();

	image->create(
		m_vulkanCore,
		bitmap.getWidth(), bitmap.getHeight(), 1,
		bitmap.getFormat() == Bitmap::FORMAT_RGBA8 ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		4,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	GPUBuffer *stagingBuffer = new GPUBuffer(
		m_vulkanCore,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		bitmap.getMemorySize()
	);

	InstantSubmitSync instantSubmit(m_vulkanCore);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		cmd.transitionLayout(*image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		stagingBuffer->writeDataToMe(bitmap.getData(), bitmap.getMemorySize(), 0);
		stagingBuffer->writeToImage(cmd, image, bitmap.getMemorySize());

		cmd.generateMipmaps(*image);
	}
	instantSubmit.submit();

	m_vulkanCore->deviceWaitIdle();

	delete stagingBuffer;

	m_imageCache.insert({ name, image });

	return image;
}

ShaderStage *App::getShaderStage(const std::string &name)
{
	return m_shaderStageCache.at(name);
}

Shader *App::getShader(const std::string &name)
{
	return m_shaderCache.at(name);
}

ShaderStage *App::loadShaderStage(const std::string &name, const std::string &path, VkShaderStageFlagBits stageType)
{
	if (m_shaderStageCache.contains(name))
		return m_shaderStageCache.at(name);

	std::ifstream file(path, std::ios::binary | std::ios::ate);

	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> sourceData(size);
	file.read(sourceData.data(), size);

	ShaderStage *stage = new ShaderStage(m_vulkanCore);
	stage->setStage(stageType);
	stage->loadFromSource(sourceData.data(), sourceData.size());

	m_shaderStageCache.insert({ name, stage });

	return stage;
}

Shader *App::createShader(const std::string &name)
{
	if (m_shaderCache.contains(name))
		return m_shaderCache.at(name);

	Shader *s = new Shader();
	m_shaderCache.insert({ name, s });
	
	return s;
}

Material *App::buildMaterial(MaterialData &data)
{
	if (m_materials.contains(data.getHash()))
		return m_materials.at(data.getHash());

	cauto &technique = m_techniques.at(data.technique);

	Material *material = new Material();

	/*
	material->m_parameterBuffer = g_shaderBufferManager->createDynamicUBO(sizeof(float) * 32);

	if (data.parameterSize > 0)
	{
	material->m_parameterBuffer->pushData(
	data.parameters,
	data.parameterSize
	);
	}
	else
	{
	struct
	{
	int asdf = 0;
	}
	garbageDefaultParametersWhatIsEvenGoingOn;

	material->m_parameterBuffer->pushData(
	&garbageDefaultParametersWhatIsEvenGoingOn,
	sizeof(garbageDefaultParametersWhatIsEvenGoingOn)
	);
	}
	*/

	for (int i = 0; i < SHADER_PASS_MAX_ENUM; i++)
	{
		if (!technique.passes[i])
			continue;

		material->passes[i].setShader(technique.passes[i]);
		material->passes[i].setVertexFormat(technique.vertexFormat);
	}

	m_materials.insert({ data.getHash(), material });

	/*
	struct BindlessMaterialData
	{
	BindlessResourceID diffuseTexture_ID;
	BindlessResourceID aoTexture_ID;
	BindlessResourceID mrTexture_ID;
	BindlessResourceID normalTexture_ID;
	BindlessResourceID emissiveTexture_ID;
	};

	BindlessMaterialData materialData = {};
	materialData.diffuseTexture_ID		= material->getTextures()[0].id;
	materialData.aoTexture_ID			= material->getTextures()[1].id;
	materialData.mrTexture_ID			= material->getTextures()[2].id;
	materialData.normalTexture_ID		= material->getTextures()[3].id;
	materialData.emissiveTexture_ID		= material->getTextures()[4].id;

	material->m_bindlessHandle.id = m_material_UID++;

	m_materialIdBuffer->writeDataToMe(
	&materialData,
	sizeof(BindlessMaterialData),
	sizeof(BindlessMaterialData) * material->m_bindlessHandle.id
	);
	*/

	return material;
}

void App::addTechnique(const std::string &name, const Technique &technique)
{
	m_techniques.insert({ name, technique });
}

void App::generateEnvironmentProbe(CommandBuffer &cmd)
{
	/*
	glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	glm::mat4 captureViews[] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
	};

	SubMesh *cubeMesh = g_meshLoader->getCubeMesh();

	// ---

	LLT_LOG("Generating environment map...");

	const int ENVIRONMENT_RESOLUTION = 1024;

	m_environmentMap = g_textureManager->createCubemap(
		"environment_map",
		ENVIRONMENT_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture hdrImage(
		g_textureManager->getTexture("environmentHDR"),
		g_textureManager->getSampler("linear")
	);

	struct
	{
		glm::mat4 proj;
		glm::mat4 view;
	}
	pc;

	pc.proj = captureProjection;
	pc.view = glm::identity<glm::mat4>();

	ShaderEffect *equirectangularToCubemapShader = g_shaderManager->getEffect("equirectangular_to_cubemap");

	VkDescriptorSet etcDescriptorSet = m_descriptorPoolAllocator.allocate(equirectangularToCubemapShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, hdrImage.getStandardImageInfo())
		.updateSet(etcDescriptorSet);

	m_equirectangularToCubemapPipeline.setShader(equirectangularToCubemapShader);
	m_equirectangularToCubemapPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_equirectangularToCubemapPipeline.setDepthTest(false);
	m_equirectangularToCubemapPipeline.setDepthWrite(false);

	cmd.begin();
	{
		m_environmentMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViews[i];

			TextureView view = m_environmentMap->getView(1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_equirectangularToCubemapPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, pipelineData.layout, { etcDescriptorSet }, {});

				cmd.setViewport({ 0, 0, ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				cubeMesh->render(cmd);
			}
			cmd.endRendering();
		}

		m_environmentMap->generateMipmaps(cmd);
	}
	cmd.endAndSubmitGraphics();

	// ---

	LLT_LOG("Generating irradiance map...");

	const int IRRADIANCE_RESOLUTION = 32;

	m_irradianceMap = g_textureManager->createCubemap(
		"irradiance_map",
		IRRADIANCE_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		4
	);

	BoundTexture envMapImage(
		m_environmentMap,
		g_textureManager->getSampler("linear")
	);

	ShaderEffect *irradianceGenerationShader = g_shaderManager->getEffect("irradiance_convolution");

	VkDescriptorSet icDescriptorSet = m_descriptorPoolAllocator.allocate(irradianceGenerationShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getStandardImageInfo())
		.updateSet(icDescriptorSet);

	m_irradianceGenerationPipeline.setShader(irradianceGenerationShader);
	m_irradianceGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_irradianceGenerationPipeline.setDepthTest(false);
	m_irradianceGenerationPipeline.setDepthWrite(false);

	cmd.begin();
	{
		m_irradianceMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViews[i];

			TextureView view = m_irradianceMap->getView(1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

			PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_irradianceGenerationPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, pipelineData.layout, { icDescriptorSet }, {});

				cmd.setViewport({ 0, 0, IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				cubeMesh->render(cmd);
			}
			cmd.endRendering();
		}

		m_irradianceMap->generateMipmaps(cmd);
	}
	cmd.endAndSubmitGraphics();

	// ---

	LLT_LOG("Generating prefilter map...");

	const int PREFILTER_RESOLUTION = 128;
	const int PREFILTER_MIP_LEVELS = 5;

	m_prefilterMap = g_textureManager->createCubemap(
		"prefilter_map",
		PREFILTER_RESOLUTION,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		PREFILTER_MIP_LEVELS
	);

	struct
	{
		float roughness = 0.0f;
		float _padding0 = 0.0f;
		float _padding1 = 0.0f;
		float _padding2 = 0.0f;
	}
	prefilterParams;

	GPUBuffer *pfParameterBuffer = g_gpuBufferManager->createUniformBuffer(sizeof(prefilterParams) * PREFILTER_MIP_LEVELS);

	ShaderEffect *prefilterGenerationShader = g_shaderManager->getEffect("prefilter_convolution");

	VkDescriptorSet pfDescriptorSet = m_descriptorPoolAllocator.allocate(prefilterGenerationShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pfParameterBuffer->getDescriptorInfoRange(sizeof(prefilterParams)))
		.writeCombinedImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, envMapImage.getStandardImageInfo())
		.updateSet(pfDescriptorSet);

	m_prefilterGenerationPipeline.setShader(prefilterGenerationShader);
	m_prefilterGenerationPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_prefilterGenerationPipeline.setDepthTest(false);
	m_prefilterGenerationPipeline.setDepthWrite(false);

	cmd.begin();
	{
		m_prefilterMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int mipLevel = 0; mipLevel < PREFILTER_MIP_LEVELS; mipLevel++)
		{
			int width = PREFILTER_RESOLUTION >> mipLevel;
			int height = PREFILTER_RESOLUTION >> mipLevel;

			float roughness = (float)mipLevel / (float)(PREFILTER_MIP_LEVELS - 1);

			prefilterParams.roughness = roughness;
			uint32_t dynamicOffset = sizeof(prefilterParams) * mipLevel;
			pfParameterBuffer->writeDataToMe(&prefilterParams, sizeof(prefilterParams), dynamicOffset);

			for (int i = 0; i < 6; i++)
			{
				pc.view = captureViews[i];

				TextureView view = m_prefilterMap->getView(1, i, mipLevel);

				RenderInfo info;
				info.setSize(width, height);
				info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view);

				PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_prefilterGenerationPipeline, info);

				// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
				cmd.beginRendering(info);
				{
					cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

					cmd.bindDescriptorSets(
						0,
						pipelineData.layout,
						{ pfDescriptorSet },
						{ dynamicOffset }
					);

					cmd.setViewport({ 0, 0, (float)width, (float)height });

					cmd.pushConstants(
						pipelineData.layout,
						VK_SHADER_STAGE_ALL_GRAPHICS,
						sizeof(pc),
						&pc
					);

					cubeMesh->render(cmd);
				}
				cmd.endRendering();
			}
		}

		m_prefilterMap->transitionLayout(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	cmd.endAndSubmitGraphics();

	vkQueueWaitIdle(g_vkCore->m_graphicsQueue.getQueue());

	delete pfParameterBuffer;
	*/
}

void App::precomputeBRDF(CommandBuffer &cmd)
{
	/*
	MGP_LOG("Precomputing BRDF...");

	SubMesh *quadMesh = g_meshLoader->getQuadMesh();

	const int BRDF_RESOLUTION = 512;

	m_brdfLUT = g_textureManager->createAttachment(
		"brdfIntegration",
		BRDF_RESOLUTION, BRDF_RESOLUTION,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_TILING_LINEAR
	);

	RenderInfo info(m_vulkanCore);
	info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
	info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, m_brdfLUT->getStandardView());

	ShaderEffect *brdfLUTShader = g_shaderManager->getEffect("brdf_lut");

	VkDescriptorSet set = m_descriptorPoolAllocator.allocate(brdfLUTShader->getDescriptorSetLayouts());

	m_brdfIntegrationPipeline.setShader(brdfLUTShader);
	m_brdfIntegrationPipeline.setVertexFormat(g_primitiveUvVertexFormat);
	m_brdfIntegrationPipeline.setDepthTest(false);
	m_brdfIntegrationPipeline.setDepthWrite(false);

	PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_brdfIntegrationPipeline, info);

	cmd.begin();
	{
		cmd.beginRendering(info);
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			cmd.bindDescriptorSets(0, pipelineData.layout, { set }, {});

			cmd.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });

			quadMesh->render(cmd);
		}
		cmd.endRendering();
	}
	cmd.endAndSubmitGraphics();
	*/
}

void App::initImGui()
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	//ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsClassic();

	m_platform->initImGui();
	m_vulkanCore->initImGui();
}
