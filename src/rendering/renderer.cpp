#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "core/profiler.h"

#include "input/input.h"

#include "vulkan/core.h"
#include "vulkan/swapchain.h"
#include "vulkan/command_buffer.h"
#include "vulkan/texture.h"
#include "vulkan/descriptor_builder.h"
#include "vulkan/render_target.h"

#include "material.h"
#include "material_system.h"
#include "mesh_loader.h"
#include "shader_mgr.h"
#include "texture_mgr.h"
#include "render_target_mgr.h"

#include "./passes/forward_pass.h"
#include "./passes/post_process_pass.h"
#include "./passes/shadow_pass.h"

#include "math/colour.h"

using namespace llt;

Renderer::Renderer()
	: m_target(nullptr)
	, m_currentScene()
	, m_skyboxMesh()
	, m_skyboxPipeline()
	, m_skyboxSet()
	, m_descriptorPool()
	, m_descriptorLayoutCache()
{
}

void Renderer::init()
{
	g_gpuBufferManager->createGlobalStagingBuffers();

	m_descriptorPool.init(64 * mgc::FRAMES_IN_FLIGHT, {
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

	initVertexTypes();

	g_meshLoader = new MeshLoader();

	g_materialSystem = new MaterialSystem();
	g_materialSystem->init();

	g_textureManager->loadDefaultTexturesAndSamplers();
	g_shaderManager->loadDefaultShaders();

	g_materialSystem->finalise();

	m_target = g_renderTargetManager->createTarget(
		"target",
		g_vkCore->m_swapchain->getWidth(),
		g_vkCore->m_swapchain->getHeight(),
		{
			VK_FORMAT_R32G32B32A32_SFLOAT
		},
		VK_SAMPLE_COUNT_8_BIT,
		1
	);

	m_target->createDepthAttachment();
	m_target->toggleClear(true);
	m_target->setClearColours(Colour::black());

	createSkyboxResources();

	g_forwardPass.init();
	g_postProcessPass.init(m_descriptorPool, m_target);
	g_shadowPass.init();

	auto assimpModel = m_currentScene.createRenderObject();
	assimpModel->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	assimpModel->transform.setRotation(glm::radians(0.0f), { 1.0f, 0.0f, 0.0f });
	assimpModel->transform.setScale({ 1.0f, 1.0f, 1.0f });
	assimpModel->transform.setOrigin({ 0.0f, 0.0f, 0.0f });
	assimpModel->mesh = g_meshLoader->loadMesh("model", "../../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");
	assimpModel->mesh->setOwner(&(*assimpModel));
}

void Renderer::cleanUp()
{
	g_shadowPass.dispose();
	g_postProcessPass.dispose();
	g_forwardPass.dispose();

	m_descriptorPool.cleanUp();
	m_descriptorLayoutCache.cleanUp();

	delete g_meshLoader;
	delete g_materialSystem;
}

void Renderer::setScene(const Scene &scene)
{
	m_currentScene = scene;
}

void Renderer::render(const Camera &camera, float deltaTime)
{
	m_currentScene.updatePrevMatrices();

	CommandBuffer cmd = CommandBuffer::fromGraphics();

	cmd.beginRecording();
	cmd.beginRendering(m_target);
	{
		auto &renderList = m_currentScene.getRenderList();
		
		g_forwardPass.render(cmd, camera, renderList);
	}
	cmd.endRendering();
	cmd.submit();

	m_target->toggleClear(false);

	cmd.beginRecording();
	cmd.beginRendering(m_target);
	renderSkybox(cmd, camera);
	cmd.endRendering();
	cmd.submit();

	m_target->toggleClear(true);

	g_postProcessPass.render(cmd);

	ImGui::Render();

	renderImGui(cmd);

	g_vkCore->swapBuffers();
}

void Renderer::renderImGui(CommandBuffer &cmd)
{
	RenderInfo renderInfo;
	
	renderInfo.setSize(
		g_vkCore->m_swapchain->getWidth(),
		g_vkCore->m_swapchain->getHeight()
	);
	
	renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		g_vkCore->m_swapchain->getCurrentSwapchainImageView()
	);

	cmd.beginRecording();
	cmd.beginRendering(renderInfo);

	ImDrawData *drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, cmd.getHandle());

	cmd.endRendering();
	cmd.submit();
}

void Renderer::createSkyboxResources()
{
	Vector<PrimitiveVertex> skyboxVertices =
	{
		{ { -0.5f,  0.5f,  0.5f } },
		{ { -0.5f, -0.5f,  0.5f } },
		{ {  0.5f, -0.5f,  0.5f } },
		{ {  0.5f,  0.5f,  0.5f } },
		{ { -0.5f,  0.5f, -0.5f } },
		{ { -0.5f, -0.5f, -0.5f } },
		{ {  0.5f, -0.5f, -0.5f } },
		{ {  0.5f,  0.5f, -0.5f } }
	};

	Vector<uint16_t> skyboxIndices =
	{
		0, 1, 2,
		2, 3, 0,

		5, 4, 7,
		7, 6, 5,

		1, 5, 6,
		6, 2, 1,

		4, 0, 3,
		3, 7, 4,

		4, 5, 1,
		1, 0, 4,

		3, 2, 6,
		6, 7, 3
	};

	m_skyboxMesh.build(
		g_primitiveVertexFormat,
		skyboxVertices.data(), skyboxVertices.size(),
		skyboxIndices.data(), skyboxIndices.size()
	);

	ShaderEffect *skyboxShader = g_shaderManager->getEffect("skybox");

	BoundTexture skyboxCubemap(
		g_textureManager->getTexture("environment_map"),
		g_textureManager->getSampler("linear")
	);

	m_skyboxSet = m_descriptorPool.allocate(skyboxShader->getDescriptorSetLayouts());

	DescriptorWriter()
		.writeCombinedImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, skyboxCubemap.getStandardImageInfo())
		.updateSet(m_skyboxSet);

	m_skyboxPipeline.setShader(skyboxShader);
	m_skyboxPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_skyboxPipeline.setDepthTest(true);
	m_skyboxPipeline.setDepthWrite(false);
	m_skyboxPipeline.setDepthOp(VK_COMPARE_OP_LESS_OR_EQUAL);
}

void Renderer::renderSkybox(CommandBuffer &cmd, const Camera &camera)
{
	PipelineData pipelineData = g_vkCore->getPipelineCache().fetchGraphicsPipeline(m_skyboxPipeline, cmd.getCurrentRenderInfo());

	struct
	{
		glm::mat4 proj;
		glm::mat4 view;
	}
	params;

	params.proj = camera.getProj();
	params.view = camera.getRotationMatrix();

	cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

	cmd.bindDescriptorSets(0, pipelineData.layout, { m_skyboxSet }, {});

	cmd.pushConstants(
		pipelineData.layout,
		VK_SHADER_STAGE_ALL_GRAPHICS,
		sizeof(params),
		&params
	);

	m_skyboxMesh.render(cmd);
}

/*
void Renderer::renderParticles(CommandBuffer &cmd, const Camera &camera, const GenericRenderTarget *target, float deltaTime)
{
	m_gBuffer->toggleClear(false);

	m_pushConstants.set("time", deltaTime);
	g_vkCore->setPushConstants(m_pushConstants);
	m_gpuParticles.dispatchCompute(camera);

	glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
	m_shaderParams.set("currModelMatrix", particleMatrix);
	m_shaderParams.set("prevModelMatrix", particleMatrix);
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_gpuParticles.render();
}
*/
