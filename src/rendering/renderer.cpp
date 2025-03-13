#include "renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"

#include "core/profiler.h"

#include "input/input.h"

#include "vulkan/backend.h"
#include "vulkan/backbuffer.h"
#include "vulkan/command_buffer.h"
#include "vulkan/texture.h"

#include "rendering/material.h"
#include "rendering/material_system.h"
#include "rendering/mesh_loader.h"
#include "rendering/texture_mgr.h"
#include "rendering/render_target_mgr.h"

#include "math/colour.h"

using namespace llt;

Renderer::Renderer()
	: m_target(nullptr)
//	, m_gpuParticles()
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

	g_textureManager->loadDefaultTexturesAndSamplers();
	g_shaderManager->loadDefaultShaders();

	g_materialSystem = new MaterialSystem();
	g_materialSystem->loadDefaultTechniques();
	g_materialSystem->init();

	createSkyboxResources();

	addRenderObjects();

	m_target = g_renderTargetManager->createTarget(
		"target",
		g_vulkanBackend->m_backbuffer->getWidth(),
		g_vulkanBackend->m_backbuffer->getHeight(),
		{
			VK_FORMAT_R32G32B32A32_SFLOAT
		},
		VK_SAMPLE_COUNT_1_BIT,
		1
	);

	m_target->createDepthAttachment();
	m_target->toggleClear(true);
	m_target->setClearColours(Colour::black());

	g_forwardPass.init();
	g_postProcessPass.init(m_descriptorPool);

//	m_gpuParticles.init(m_shaderParamsBuffer);
}

void Renderer::cleanUp()
{
	g_forwardPass.cleanUp();
	g_postProcessPass.cleanUp();

	m_descriptorPool.cleanUp();
	m_descriptorLayoutCache.cleanUp();

	delete g_meshLoader;
	delete g_materialSystem;
}

void Renderer::addRenderObjects()
{
	auto assimpModel = m_currentScene.createRenderObject();
	assimpModel->transform.setPosition({ 0.0f, 0.0f, 0.0f });
	assimpModel->transform.setRotation(glm::radians(0.0f), { 1.0f, 0.0f, 0.0f });
	assimpModel->transform.setScale({ 4.0f, 4.0f, 4.0f });
	assimpModel->transform.setOrigin({ 0.0f, 0.0f, 0.0f });
	assimpModel->mesh = g_meshLoader->loadMesh("model", "../../res/models/GLTF/DamagedHelmet/DamagedHelmet.gltf");
	assimpModel->mesh->setOwner(&(*assimpModel));
}

void Renderer::setScene(const Scene &scene)
{
	m_currentScene = scene;
}

void Renderer::render(const Camera &camera, float deltaTime)
{
	m_currentScene.updatePrevMatrices();

	CommandBuffer cmd = CommandBuffer::fromGraphics();

	cmd.beginRendering(m_target);
	{
		renderSkybox(cmd, camera);

		auto &renderList = m_currentScene.getRenderList();
		
		g_forwardPass.render(cmd, camera, renderList);
	}
	cmd.endRendering();
	cmd.submit();

	g_postProcessPass.render(cmd);

	ImGui::Render();

	renderImGui(cmd);

	g_vulkanBackend->swapBuffers();
}

void Renderer::renderImGui(CommandBuffer &cmd)
{
	RenderInfo renderInfo;
	
	renderInfo.setSize(
		g_vulkanBackend->m_backbuffer->getWidth(),
		g_vulkanBackend->m_backbuffer->getHeight()
	);
	
	renderInfo.addColourAttachment(
		VK_ATTACHMENT_LOAD_OP_LOAD,
		g_vulkanBackend->m_backbuffer->getCurrentSwapchainImageView(),
		g_vulkanBackend->getImGuiAttachmentFormat()
	);

	cmd.beginRendering(renderInfo);

	ImDrawData *drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, cmd.getBuffer());

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
		g_textureManager->getTexture("environmentMap"),
		g_textureManager->getSampler("linear")
	);
	
	DescriptorWriter descriptorWriter;
	descriptorWriter.writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, skyboxCubemap.getStandardImageInfo());

	m_skyboxSet = m_descriptorPool.allocate(skyboxShader->getDescriptorSetLayout());
	descriptorWriter.updateSet(m_skyboxSet);

	m_skyboxPipeline.setShader(skyboxShader);
	m_skyboxPipeline.setVertexFormat(g_primitiveVertexFormat);
	m_skyboxPipeline.setDepthTest(false);
	m_skyboxPipeline.setDepthWrite(false);
}

void Renderer::renderSkybox(CommandBuffer &cmd, const Camera &camera)
{
	PipelineData pipelineData = g_vulkanBackend->getPipelineCache().fetchGraphicsPipeline(m_skyboxPipeline, cmd.getCurrentRenderInfo());

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
	g_vulkanBackend->setPushConstants(m_pushConstants);
	m_gpuParticles.dispatchCompute(camera);

	glm::mat4 particleMatrix = glm::scale(glm::identity<glm::mat4>(), { 0.1f, 0.1f, 0.1f });
	m_shaderParams.set("currModelMatrix", particleMatrix);
	m_shaderParams.set("prevModelMatrix", particleMatrix);
	m_shaderParamsBuffer->pushData(m_shaderParams);
	m_gpuParticles.render();
}

void Renderer::renderPostProcess()
{
	RenderPass pass;
	pass.setMesh(m_quadMesh);

	m_postProcessPipeline.bind();

	vkCmdBindDescriptorSets(
		g_vulkanBackend->graphicsQueue.getCurrentFrame().commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_postProcessPipeline.getPipelineLayout(),
		0,
		1, &m_postProcessDescriptorSet,
		0, nullptr
	);

	m_postProcessPipeline.render(pass);
}
*/
