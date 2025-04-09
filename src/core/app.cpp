#include "app.h"

#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_sdl3.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "platform.h"
#include "editor_ui.h"

#include "vulkan/core.h"
#include "vulkan/toolbox.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/swapchain.h"
#include "vulkan/sync.h"
#include "vulkan/shader.h"
#include "vulkan/image.h"
#include "vulkan/sampler.h"
#include "vulkan/bitmap.h"
#include "vulkan/vertex_format.h"

#include "rendering/material.h"
#include "rendering/model_loader.h"
#include "rendering/model.h"

#include "input/input.h"

#include "math/timer.h"
#include "math/calc.h"
#include "math/colour.h"

using namespace mgp;

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

struct BindlessMaterialHandles
{
	bindless::Handle diffuseTexture_ID;
	bindless::Handle aoTexture_ID;
	bindless::Handle armTexture_ID;
	bindless::Handle normalTexture_ID;
	bindless::Handle emissiveTexture_ID;
};

struct BindlessPushConstants // todo: this should use instance id instead
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
};

App::App(const Config &config)
	: m_linearSampler(nullptr)
	, m_loadedImageCache()
	, m_shaderStageCache()
	, m_shaderCache()
	, m_materials()
	, m_techniques()
	, m_frameConstantsBuffer(nullptr)
	, m_transformDataBuffer(nullptr)
	, m_bindlessMaterialTable()
	, m_materialHandle_UID()
	, m_textureUVPipeline()
	, m_textureUVSet(VK_NULL_HANDLE)
	, m_hdrTonemappingPipeline()
	, m_hdrTonemappingSet(VK_NULL_HANDLE)
	, m_skyboxMesh(nullptr)
	, m_skyboxShader(nullptr)
	, m_skyboxSet(VK_NULL_HANDLE)
	, m_skyboxPipeline()
	, m_brdfLUT(nullptr)
	, m_environmentMap(nullptr)
	, m_environmentProbe()
	, m_descriptorPool()
	, m_config(config)
	, m_vulkanCore(nullptr)
	, m_platform(nullptr)
	, m_inputSt(nullptr)
	, m_running(false)
	, m_camera(config.width, config.height, 75.0f, 0.01f, 100.0f)
	, m_targetColour(nullptr)
	, m_targetDepth(nullptr)
{
	m_platform = new Platform(config);
	m_vulkanCore = new VulkanCore(config, m_platform);

	m_inputSt = new InputState();

	vtx::initVertexTypes();

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

	loadTextures();
	loadShaders();
	loadTechniques();

	m_descriptorPool.init(m_vulkanCore, 64 * Queue::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			0.5f }
	});

	createSkyboxMesh();

	precomputeBRDF();
	generateEnvironmentProbe();

	createSkybox();

	m_platform->onExit = [this]() { exit(); };

	m_frameConstantsBuffer = new GPUBuffer(
		m_vulkanCore,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(FrameConstants)
	);

	m_transformDataBuffer = new GPUBuffer(
		m_vulkanCore,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(TransformData)
	);

	m_bindlessMaterialTable = new GPUBuffer(
		m_vulkanCore,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(BindlessMaterialHandles)
	);

	Shader *textureUVShader = getShader("texture_uv");
	m_textureUVPipeline.setShader(textureUVShader);
	m_textureUVPipeline.setDepthTest(false);
	m_textureUVPipeline.setDepthWrite(false);
	m_textureUVSet = m_descriptorPool.allocate(textureUVShader->getDescriptorSetLayouts());

	Shader *hdrTonemappingShader = getShader("hdr_tonemapping");
	m_hdrTonemappingPipeline.setShader(hdrTonemappingShader);
	m_hdrTonemappingSet = m_descriptorPool.allocate(hdrTonemappingShader->getDescriptorSetLayouts());

	m_running = true;
}

App::~App()
{
	m_descriptorPool.cleanUp();

	delete m_skyboxMesh;

	delete m_frameConstantsBuffer;
	delete m_transformDataBuffer;

	delete m_bindlessMaterialTable;

	for (auto &[id, material] : m_materials)
		delete material;

	delete m_linearSampler;

	delete m_environmentMap;

	delete m_environmentProbe.irradiance;
	delete m_environmentProbe.prefilter;

	delete m_brdfLUT;

	delete m_targetColour;
	delete m_targetDepth;

	for (cauto &[name, image] : m_loadedImageCache)
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

void App::run()
{
	InFlightSync inFlightSync(m_vulkanCore);

	editor_ui::init(m_platform, m_vulkanCore);

	m_platform->onWindowResize = [&inFlightSync](int w, int h) -> void {
		MGP_LOG("Detected window resize!");
		inFlightSync.onWindowResize(w, h);
	};

	m_targetColour = new Image(
		m_vulkanCore,
		inFlightSync.getSwapchain()->getWidth(),
		inFlightSync.getSwapchain()->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_targetDepth = new Image(
		m_vulkanCore,
		inFlightSync.getSwapchain()->getWidth(),
		inFlightSync.getSwapchain()->getHeight(),
		1,
		vk_toolbox::findDepthFormat(m_vulkanCore->getPhysicalDevice()),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	DescriptorWriter()
		.writeCombinedImage(0, m_targetColour->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_linearSampler->getHandle())
		.updateSet(m_vulkanCore, m_textureUVSet);

	DescriptorWriter()
		.writeStorageImage(0, m_targetColour->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_GENERAL)
		.updateSet(m_vulkanCore, m_hdrTonemappingSet);

	ModelLoader meshLoader(m_vulkanCore, this);
	Model *mesh = meshLoader.loadMesh("../../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");

	double accumulator = 0.0;
	const double fixedDeltaTime = 1.0 / static_cast<double>(CalcU::min(m_config.targetFPS, m_platform->getWindowRefreshRate()));

	Timer deltaTimer(m_platform);
	deltaTimer.start();

	while (m_running)
	{
		m_platform->pollEvents(m_inputSt);
		m_inputSt->update();

		if (m_inputSt->isPressed(KB_KEY_ESCAPE))
			exit();

		editor_ui::update();

		double deltaTime = deltaTimer.reset();

		tick(deltaTime);

		accumulator += CalcD::min(deltaTime, fixedDeltaTime);

		while (accumulator >= fixedDeltaTime)
		{
			tickFixed(fixedDeltaTime);

			accumulator -= fixedDeltaTime;
		}

		inFlightSync.begin();
		{
			FrameConstants constants = {
				.proj = m_camera.getProj(),
				.view = m_camera.getView(),
				.cameraPosition = glm::vec4(m_camera.position, 1.0f)
			};

			TransformData transform = {
				.model = glm::identity<glm::mat4>(),
				.normalMatrix = glm::transpose(glm::inverse(transform.model))
			};

			m_frameConstantsBuffer->writeDataToMe(&constants, sizeof(FrameConstants), 0);
			m_transformDataBuffer->writeDataToMe(&transform, sizeof(TransformData), 0);

			RenderGraph::AttachmentInfo targetColourAttachment;
			targetColourAttachment.view = m_targetColour->getStandardView();
			targetColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			targetColourAttachment.clear = { .color = { 0.0f, 0.0f, 0.0f, 1.0f } };

			RenderGraph::AttachmentInfo targetDepthAttachment;
			targetDepthAttachment.view = m_targetDepth->getStandardView();
			targetDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			targetDepthAttachment.clear = { .depthStencil = { 1.0f, 0 } };

			RenderGraph::AttachmentInfo swapchainColourAttachment;
			swapchainColourAttachment.view = inFlightSync.getSwapchain()->getColourAttachment().getStandardView();
			swapchainColourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

			RenderGraph::AttachmentInfo swapchainDepthAttachment;
			swapchainDepthAttachment.view = inFlightSync.getSwapchain()->getDepthAttachment().getStandardView();
			swapchainDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

			RenderGraph::AttachmentInfo swapchainDirectAttachment;
			swapchainDirectAttachment.view = inFlightSync.getSwapchain()->getCurrentSwapchainImageView();
			swapchainDirectAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

			// forward render
			m_vulkanCore->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
				.setColourAttachments({ targetColourAttachment })
				.setDepthStencilAttachment(targetDepthAttachment)
				.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
				{
					// model
					{
						PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(mesh->getSubmesh(0)->getMaterial()->passes[SHADER_PASS_FORWARD], info);

						cmd.bindPipeline(
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineData.pipeline
						);

						cmd.bindDescriptorSets(
							0,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineData.layout,
							{ m_vulkanCore->getBindlessResources().getSet() },
							{}
						);

						BindlessPushConstants pc = {
							.frameDataBuffer_ID			= m_frameConstantsBuffer->getBindlessHandle(),
							.transformBuffer_ID			= m_transformDataBuffer->getBindlessHandle(),
							.materialDataBuffer_ID		= m_bindlessMaterialTable->getBindlessHandle(),

							.transform_ID				= 0,

							.irradianceMap_ID			= m_environmentProbe.irradiance->getStandardView()->getBindlessHandle(),
							.prefilterMap_ID			= m_environmentProbe.prefilter->getStandardView()->getBindlessHandle(),

							.brdfLUT_ID					= m_brdfLUT->getStandardView()->getBindlessHandle(),

							.material_ID				= mesh->getSubmesh(0)->getMaterial()->bindlessHandle,

							.cubemapSampler_ID			= m_linearSampler->getBindlessHandle(),
							.textureSampler_ID			= m_linearSampler->getBindlessHandle()
						};

						cmd.pushConstants(
							pipelineData.layout,
							VK_SHADER_STAGE_ALL_GRAPHICS,
							sizeof(BindlessPushConstants),
							&pc
						);

						mesh->getSubmesh(0)->render(cmd);
					}

					// skybox
					{
						PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(m_skyboxPipeline, info);

						struct
						{
							glm::mat4 proj;
							glm::mat4 view;
						}
						params;

						params.proj = m_camera.getProj();
						params.view = m_camera.getRotationMatrix();

						cmd.bindPipeline(
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineData.pipeline
						);

						cmd.bindDescriptorSets(
							0,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							pipelineData.layout,
							{ m_skyboxSet },
							{}
						);

						cmd.pushConstants(
							pipelineData.layout,
							VK_SHADER_STAGE_ALL_GRAPHICS,
							sizeof(params),
							&params
						);

						m_skyboxMesh->render(cmd);
					}
				}));

			// compute HDR pass
			m_vulkanCore->getRenderGraph().addTask(RenderGraph::ComputeTaskDefinition()
				.setInputStorageViews({ targetColourAttachment.view })
				.setBuildFn([&](CommandBuffer &cmd) -> void
			{
				PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchComputePipeline(m_hdrTonemappingPipeline);

				cmd.bindPipeline(
					VK_PIPELINE_BIND_POINT_COMPUTE,
					pipelineData.pipeline
				);

				cmd.bindDescriptorSets(
					0,
					VK_PIPELINE_BIND_POINT_COMPUTE,
					pipelineData.layout,
					{ m_hdrTonemappingSet },
					{}
				);

				struct
				{
					uint32_t width;
					uint32_t height;
					float exposure = 2.25f;
				}
				pc {
					.width = (uint32_t)m_platform->getWindowSizeInPixels().x,
					.height = (uint32_t)m_platform->getWindowSizeInPixels().y
				};

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_COMPUTE_BIT,
					sizeof(pc),
					&pc
				);

				cmd.dispatch(pc.width / 8, pc.height / 8, 1);
			}));

			// output to swapchain
			swapchainColourAttachment.resolve = inFlightSync.getSwapchain()->getCurrentSwapchainImageView();

			m_vulkanCore->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
				.setColourAttachments({ swapchainDirectAttachment })
				.setDepthStencilAttachment(targetDepthAttachment)
				.setInputViews({ targetColourAttachment.view })
				.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
				{
					PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(m_textureUVPipeline, info);

					cmd.bindPipeline(
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.pipeline
					);

					cmd.bindDescriptorSets(
						0,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.layout,
						{ m_textureUVSet },
						{}
					);

					cmd.draw(3);
				}));

			// editor ui pass
			m_vulkanCore->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
				.setColourAttachments({ swapchainDirectAttachment })
				.setBuildFn(editor_ui::render));
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
}

void App::tick(float dt)
{
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

void App::render(CommandBuffer &inFlightCmd, const Swapchain *swapchain)
{
}

void App::loadTextures()
{
	m_linearSampler = new Sampler(m_vulkanCore, Sampler::Style(VK_FILTER_LINEAR));

	loadTexture("fallback_white",	"../../res/textures/standard/white.png");
	loadTexture("fallback_black",	"../../res/textures/standard/black.png");
	loadTexture("fallback_normals",	"../../res/textures/standard/normal_fallback.png");

	loadTexture("environmentHDR",	"../../res/textures/rogland_clear_night_greg_zaal.hdr");

	loadTexture("stone",			"../../res/textures/smooth_stone.png");
	loadTexture("wood",				"../../res/textures/wood.jpg");
}

void App::loadShaders()
{
	// shader stages
	{
		// vertex shaders
		loadShaderStage("primitive_vs",						"../../res/shaders/compiled/primitive_vs.spv",						VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("skybox_vs",						"../../res/shaders/compiled/skybox_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("fullscreen_triangle_vs",			"../../res/shaders/compiled/fullscreen_triangle_vs.spv",			VK_SHADER_STAGE_VERTEX_BIT);
		loadShaderStage("model_vs",							"../../res/shaders/compiled/model_vs.spv",							VK_SHADER_STAGE_VERTEX_BIT);

		// pixel shaders
		loadShaderStage("equirectangular_to_cubemap_ps",	"../../res/shaders/compiled/equirectangular_to_cubemap_ps.spv",		VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("irradiance_convolution_ps",		"../../res/shaders/compiled/irradiance_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("prefilter_convolution_ps",			"../../res/shaders/compiled/prefilter_convolution_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("brdf_integrator_ps",				"../../res/shaders/compiled/brdf_integrator_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texturedPBR_ps",					"../../res/shaders/compiled/texturedPBR_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("subsurface_refraction_ps",			"../../res/shaders/compiled/subsurface_refraction_ps.spv",			VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("skybox_ps",						"../../res/shaders/compiled/skybox_ps.spv",							VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_downsample_ps",				"../../res/shaders/compiled/bloom_downsample_ps.spv",				VK_SHADER_STAGE_FRAGMENT_BIT);
//		loadShaderStage("bloom_upsample_ps",				"../../res/shaders/compiled/bloom_upsample_ps.spv",					VK_SHADER_STAGE_FRAGMENT_BIT);
		loadShaderStage("texture_uv_ps",					"../../res/shaders/compiled/texture_uv_ps.spv",						VK_SHADER_STAGE_FRAGMENT_BIT);

		// compute shaders
		loadShaderStage("hdr_tonemapping_cs",				"../../res/shaders/compiled/hdr_tonemapping_cs.spv",				VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// effects
	{
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
			brdfLUT->addStage(getShaderStage("fullscreen_triangle_vs"));
			brdfLUT->addStage(getShaderStage("brdf_integrator_ps"));
		}

		// HDR TONEMAPPING
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				.build(m_vulkanCore, VK_SHADER_STAGE_COMPUTE_BIT);

			Shader *hdrTonemapping = createShader("hdr_tonemapping");

			hdrTonemapping->setDescriptorSetLayouts({ layout });
			hdrTonemapping->setPushConstantsSize(sizeof(uint32_t)*2 + sizeof(float)*1);
			hdrTonemapping->addStage(getShaderStage("hdr_tonemapping_cs"));
		}

		// BLOOM DOWNSAMPLE
		/*
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *bloomDownsample = createShader("bloom_downsample");

			bloomDownsample->setDescriptorSetLayouts({ layout });
			bloomDownsample->setPushConstantsSize(sizeof(int));
			bloomDownsample->addStage(getShaderStage("fullscreen_triangle_vs"));
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
			bloomUpsample->addStage(getShaderStage("fullscreen_triangle_vs"));
			bloomUpsample->addStage(getShaderStage("bloom_upsample_ps"));
		}
		*/

		// TEXTURE UV
		{
			VkDescriptorSetLayout layout = DescriptorLayoutBuilder()
				.bind(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
				.build(m_vulkanCore, VK_SHADER_STAGE_ALL_GRAPHICS);

			Shader *textureUV = createShader("texture_uv");

			textureUV->setDescriptorSetLayouts({ layout });
			textureUV->setPushConstantsSize(0);
			textureUV->addStage(getShaderStage("fullscreen_triangle_vs"));
			textureUV->addStage(getShaderStage("texture_uv_ps"));
		}
	}
}

void App::loadTechniques()
{
	// PBR
	{
		Technique texturedPBR;
		texturedPBR.passes[SHADER_PASS_FORWARD] = getShader("texturedPBR");
		texturedPBR.passes[SHADER_PASS_SHADOW] = nullptr;
		texturedPBR.vertexFormat = &vtx::MODEL_VERTEX_FORMAT;
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

Image *App::loadTexture(const std::string &name, const std::string &path)
{
	if (m_loadedImageCache.contains(name))
		return m_loadedImageCache.at(name);

	Bitmap bitmap(path);

	Image *image = new Image(
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

	m_loadedImageCache.insert({ name, image });

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
	material->textures = data.textures;

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

	BindlessMaterialHandles handles = {};
	handles.diffuseTexture_ID		= material->textures[0];
	handles.aoTexture_ID			= material->textures[1];
	handles.armTexture_ID			= material->textures[2];
	handles.normalTexture_ID		= material->textures[3];
	handles.emissiveTexture_ID		= material->textures[4];

	material->bindlessHandle = m_materialHandle_UID++;

	m_bindlessMaterialTable->writeDataToMe(
		&handles,
		sizeof(BindlessMaterialHandles),
		sizeof(BindlessMaterialHandles) * material->bindlessHandle
	);

	return material;
}

void App::addTechnique(const std::string &name, const Technique &technique)
{
	m_techniques.insert({ name, technique });
}

void App::precomputeBRDF()
{
	InstantSubmitSync instantSubmit(m_vulkanCore);
	CommandBuffer &cmd = instantSubmit.begin();
	{
		MGP_LOG("Precomputing BRDF...");

		const int BRDF_RESOLUTION = 512;

		m_brdfLUT = new Image(
			m_vulkanCore,
			BRDF_RESOLUTION, BRDF_RESOLUTION, 1,
			VK_FORMAT_R32G32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_TILING_OPTIMAL,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		RenderInfo info(m_vulkanCore);
		info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
		info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *m_brdfLUT->getStandardView(), nullptr);

		Shader *brdfLUTShader = getShader("brdf_lut");

		VkDescriptorSet set = m_descriptorPool.allocate(brdfLUTShader->getDescriptorSetLayouts());

		GraphicsPipelineDefinition brdfIntegrationPipeline;
		brdfIntegrationPipeline.setShader(brdfLUTShader);
		brdfIntegrationPipeline.setDepthTest(false);
		brdfIntegrationPipeline.setDepthWrite(false);

		PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(brdfIntegrationPipeline, info);

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd.beginRendering(info);
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { set }, {});

			cmd.setViewport({ 0, 0, BRDF_RESOLUTION, BRDF_RESOLUTION });

			cmd.draw(3);
		}
		cmd.endRendering();

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();
}

void App::generateEnvironmentProbe()
{
	const int ENVIRONMENT_MAP_RESOLUTION = 1024;
	const int IRRADIANCE_MAP_RESOLUTION = 32;

	const int PREFILTER_MAP_RESOLUTION = 128;
	const int PREFILTER_MAP_MIP_LEVELS = 5;

	glm::mat4 captureProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	glm::mat4 captureViewMatrices[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
	};

	struct
	{
		glm::mat4 proj;
		glm::mat4 view;
	}
	pc;

	InstantSubmitSync instantSubmit(m_vulkanCore);

	// generate environment map / skybox from 2D HDR image
	CommandBuffer &cmd = instantSubmit.begin();
	{
		MGP_LOG("Generating environment map...");

		m_environmentMap = new Image(
			m_vulkanCore,
			ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			4,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		pc.proj = captureProjectionMatrix;
		pc.view = glm::identity<glm::mat4>();

		Shader *eqrToCbmShader = getShader("equirectangular_to_cubemap");

		VkDescriptorSet eqrToCbmSet = m_descriptorPool.allocate(eqrToCbmShader->getDescriptorSetLayouts());

		Image *environmentHDRImage = m_loadedImageCache.at("environmentHDR");

		DescriptorWriter()
			.writeCombinedImage(0, environmentHDRImage->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_linearSampler->getHandle())
			.updateSet(m_vulkanCore, eqrToCbmSet);

		GraphicsPipelineDefinition equirectangularToCubemapPipeline;
		equirectangularToCubemapPipeline.setShader(eqrToCbmShader);
		equirectangularToCubemapPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
		equirectangularToCubemapPipeline.setDepthTest(false);
		equirectangularToCubemapPipeline.setDepthWrite(false);

		cmd.transitionLayout(*m_environmentMap, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViewMatrices[i];

			ImageView *view = m_environmentMap->createView(1, i, 0);

			RenderInfo targetInfo(m_vulkanCore);
			targetInfo.setSize(ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

			PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(equirectangularToCubemapPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { eqrToCbmSet }, {});

				cmd.setViewport({ 0, 0, ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				m_skyboxMesh->render(cmd);
			}
			cmd.endRendering();
		}

		cmd.transitionLayout(*m_environmentMap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd.generateMipmaps(*m_environmentMap);
	}
	instantSubmit.submit();

	struct
	{
		float roughness = 0.0f;
		float _padding0 = 0.0f;
		float _padding1 = 0.0f;
		float _padding2 = 0.0f;
	}
	prefilterParams;

	GPUBuffer *pfParameterBuffer = new GPUBuffer(
		m_vulkanCore,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		sizeof(prefilterParams) * PREFILTER_MAP_MIP_LEVELS
	);

	// generate irradiance and prefilter maps from environment map
	cmd = instantSubmit.begin();
	{
		MGP_LOG("Generating irradiance map...");

		m_environmentProbe.irradiance = new Image(
			m_vulkanCore,
			IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			4,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		Shader *irradianceGenShader = getShader("irradiance_convolution");

		VkDescriptorSet irradianceSet = m_descriptorPool.allocate(irradianceGenShader->getDescriptorSetLayouts());

		DescriptorWriter()
			.writeCombinedImage(0, m_environmentMap->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_linearSampler->getHandle())
			.updateSet(m_vulkanCore, irradianceSet);

		GraphicsPipelineDefinition irradiancePipeline;
		irradiancePipeline.setShader(irradianceGenShader);
		irradiancePipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
		irradiancePipeline.setDepthTest(false);
		irradiancePipeline.setDepthWrite(false);

		cmd.transitionLayout(*m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		for (int i = 0; i < 6; i++)
		{
			pc.view = captureViewMatrices[i];

			ImageView *view = m_environmentProbe.irradiance->createView(1, i, 0);

			RenderInfo targetInfo(m_vulkanCore);
			targetInfo.setSize(IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

			PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(irradiancePipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { irradianceSet }, {});

				cmd.setViewport({ 0, 0, IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(pc),
					&pc
				);

				m_skyboxMesh->render(cmd);
			}
			cmd.endRendering();
		}

		cmd.transitionLayout(*m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd.generateMipmaps(*m_environmentProbe.irradiance);

		// ---

		MGP_LOG("Generating prefilter map...");

		m_environmentProbe.prefilter = new Image(
			m_vulkanCore,
			PREFILTER_MAP_RESOLUTION, PREFILTER_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			PREFILTER_MAP_MIP_LEVELS,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		Shader *prefilterShader = getShader("prefilter_convolution");

		VkDescriptorSet prefilterSet = m_descriptorPool.allocate(prefilterShader->getDescriptorSetLayouts());

		DescriptorWriter()
			.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pfParameterBuffer->getDescriptorInfoRange(sizeof(prefilterParams)))
			.writeCombinedImage(1, m_environmentMap->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_linearSampler->getHandle())
			.updateSet(m_vulkanCore, prefilterSet);

		GraphicsPipelineDefinition prefilterPipeline;
		prefilterPipeline.setShader(prefilterShader);
		prefilterPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
		prefilterPipeline.setDepthTest(false);
		prefilterPipeline.setDepthWrite(false);

		cmd.transitionLayout(*m_environmentProbe.prefilter, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int mipLevel = 0; mipLevel < PREFILTER_MAP_MIP_LEVELS; mipLevel++)
		{
			int width = PREFILTER_MAP_RESOLUTION >> mipLevel;
			int height = PREFILTER_MAP_RESOLUTION >> mipLevel;

			float roughness = (float)mipLevel / (float)(PREFILTER_MAP_MIP_LEVELS - 1);

			prefilterParams.roughness = roughness;
			uint32_t dynamicOffset = sizeof(prefilterParams) * mipLevel;
			pfParameterBuffer->writeDataToMe(&prefilterParams, sizeof(prefilterParams), dynamicOffset);

			for (int i = 0; i < 6; i++)
			{
				pc.view = captureViewMatrices[i];

				ImageView *view = m_environmentProbe.prefilter->createView(1, i, mipLevel);

				RenderInfo info(m_vulkanCore);
				info.setSize(width, height);
				info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

				PipelineData pipelineData = m_vulkanCore->getPipelineCache().fetchGraphicsPipeline(prefilterPipeline, info);

				// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
				cmd.beginRendering(info);
				{
					cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

					cmd.bindDescriptorSets(
						0,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.layout,
						{ prefilterSet },
						{ dynamicOffset }
					);

					cmd.setViewport({ 0, 0, (float)width, (float)height });

					cmd.pushConstants(
						pipelineData.layout,
						VK_SHADER_STAGE_ALL_GRAPHICS,
						sizeof(pc),
						&pc
					);

					m_skyboxMesh->render(cmd);
				}
				cmd.endRendering();
			}
		}

		cmd.transitionLayout(*m_environmentProbe.prefilter, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();

	vkQueueWaitIdle(m_vulkanCore->getGraphicsQueue().getHandle());

	delete pfParameterBuffer;
}

void App::createSkyboxMesh()
{
	std::vector<vtx::PrimitiveVertex> vertices =
	{
		{ { -1.0f,  1.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f } },
		{ {  1.0f,  1.0f, -1.0f } },
		{ { -1.0f,  1.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f } }
	};

	std::vector<uint16_t> indices =
	{
		0, 2, 1,
		2, 0, 3,

		7, 5, 6,
		5, 7, 4,

		4, 1, 5,
		1, 4, 0,

		3, 6, 2,
		6, 3, 7,

		1, 6, 5,
		6, 1, 2,

		4, 3, 0,
		3, 4, 7
	};

	m_skyboxMesh = new Mesh(m_vulkanCore);

	m_skyboxMesh->build(
		vtx::PRIMITIVE_VERTEX_FORMAT,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

void App::createSkybox()
{
	m_skyboxShader = getShader("skybox");

	m_skyboxSet = m_descriptorPool.allocate(m_skyboxShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, m_environmentMap->getStandardView()->getHandle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_linearSampler->getHandle())
		.updateSet(m_vulkanCore, m_skyboxSet);

	m_skyboxPipeline.setShader(m_skyboxShader);
	m_skyboxPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
	m_skyboxPipeline.setDepthTest(true);
	m_skyboxPipeline.setDepthWrite(false);
	m_skyboxPipeline.setDepthOp(VK_COMPARE_OP_LESS_OR_EQUAL);
}
