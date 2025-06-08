#include "renderer.h"

#include <glm/glm.hpp>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "core/app.h"
#include "core/platform.h"

#include "vulkan/core.h"
#include "vulkan/bindless.h"
#include "vulkan/gpu_buffer.h"
#include "vulkan/image.h"
#include "vulkan/image_view.h"
#include "vulkan/bindless.h"
#include "vulkan/sampler.h"
#include "vulkan/shader.h"
#include "vulkan/vertex_format.h"
#include "vulkan/command_buffer.h"
#include "vulkan/render_graph.h"
#include "vulkan/sync.h"
#include "vulkan/swapchain.h"

#include "scene.h"
#include "model.h"
#include "camera.h"
#include "material.h"

using namespace mgp;

glm::mat4 CUBEMAP_CAPTURE_VIEW_MATRICES[] =
{
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
	glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
};

glm::vec3 CUBEMAP_CAPTURE_VIEW_DIRECTIONS[] =
{
	 glm::vec3( 1.0f, 0.0f, 0.0f),
	 glm::vec3(-1.0f, 0.0f, 0.0f),
	 glm::vec3( 0.0f, 1.0f, 0.0f),
	 glm::vec3( 0.0f,-1.0f, 0.0f),
	-glm::vec3( 0.0f, 0.0f, 1.0f),
	-glm::vec3( 0.0f, 0.0f,-1.0f)
};

glm::vec3 CUBEMAP_CAPTURE_VIEW_UP[] =
{
	glm::vec3(0.0f, 1.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 1.0f),
	glm::vec3(0.0f, 0.0f,-1.0f),
	glm::vec3(0.0f, 1.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f)
};

struct GPU_FrameData
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec4 viewPosition;
};

struct GPU_TransformData
{
	glm::mat4 model;
	glm::mat4 normalMatrix;
};

struct GPU_BindlessMaterial
{
	uint32_t diffuse_id;
	uint32_t ambient_id;
	uint32_t material_id;
	uint32_t normal_id;
	uint32_t emissive_id;
};

struct GPU_BufferPointers
{
	VkDeviceAddress frame_data;
	VkDeviceAddress transforms;
	VkDeviceAddress materials;
	VkDeviceAddress pointLights;
};

struct GPU_ModelPushConstants
{
	VkDeviceAddress buffers;
	uint32_t irradianceMap_id;
	uint32_t prefilterMap_id;
	uint32_t brdfLUT_id;
	uint32_t material_id;
	uint32_t textureSampler_id;
	uint32_t cubemapSampler_id;
};

struct GPU_DeferredLightingPushConstants
{
	uint32_t position_id;
	uint32_t albedo_id;
	uint32_t normal_id;
	uint32_t material_id;
	uint32_t emissive_id;

	uint32_t irradianceMap_id;
	uint32_t prefilterMap_id;
	uint32_t brdfLUT_id;
	
	uint32_t textureSampler_id;
	uint32_t cubemapSampler_id;

	uint32_t _padding[2];

	glm::vec4 cameraPosition;
};

void Renderer::init(App *app, Swapchain *swapchain)
{
	m_app = app;

	m_descriptorPool.init(app->getVkCore(), 64 * Queue::FRAMES_IN_FLIGHT, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 					(uint32_t)(0.5f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 	(uint32_t)(4.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 			(uint32_t)(4.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 			(uint32_t)(1.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 		(uint32_t)(1.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 		(uint32_t)(1.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			(uint32_t)(2.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 			(uint32_t)(2.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,	(uint32_t)(1.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,	(uint32_t)(1.0f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 			(uint32_t)(0.5f * 64.0f * (float)Queue::FRAMES_IN_FLIGHT) }
	});

	loadTechniques();

//	m_shadowAtlas.init(app->getVkCore());

	m_frameConstantsBuffer = new GPUBuffer(
		app->getVkCore(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_FrameData)
	);

	m_transformDataBuffer = new GPUBuffer(
		app->getVkCore(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_TransformData)
	);

	m_bindlessMaterialTable = new GPUBuffer(
		app->getVkCore(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_BindlessMaterial) * 128
	);

	m_pointLightBuffer = new GPUBuffer(
		app->getVkCore(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_PointLight) * MAX_POINT_LIGHTS
	);
	
	m_bufferPointersBuffer = new GPUBuffer(
		app->getVkCore(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_BufferPointers)
	);

	GPU_BufferPointers pointers = {};
	pointers.frame_data = m_frameConstantsBuffer->getGPUAddress();
	pointers.transforms = m_transformDataBuffer->getGPUAddress();
	pointers.materials = m_bindlessMaterialTable->getGPUAddress();
	pointers.pointLights = m_pointLightBuffer->getGPUAddress();

	m_bufferPointersBuffer->writeType(pointers);

	createSkyboxMesh();
	precomputeBRDF();
	generateEnvironmentProbe();
	
	m_skyboxSet = allocateSet(m_app->getShaders().getShader("skybox")->getDescriptorSetLayouts());

	DescriptorWriter(m_app->getVkCore())
		.writeCombinedImage(0, *m_environmentMap->getStandardView(), *m_app->getTextures().getLinearSampler())
		.writeTo(m_skyboxSet);

	createGBuffer(swapchain);

	m_textureUVSet = allocateSet(app->getShaders().getShader("texture_uv")->getDescriptorSetLayouts());
	m_hdrTonemappingSet = allocateSet(app->getShaders().getShader("hdr_tonemapping")->getDescriptorSetLayouts());
	
	DescriptorWriter(m_app->getVkCore())
		.writeCombinedImage(0, *m_gBuffer.lighting->getStandardView(), *m_app->getTextures().getLinearSampler())
		.writeTo(m_textureUVSet);

	DescriptorWriter(m_app->getVkCore())
		.writeStorageImage(0, *m_gBuffer.lighting->getStandardView())
		.writeTo(m_hdrTonemappingSet);
}

void Renderer::createGBuffer(Swapchain *swapchain)
{
	m_gBuffer.position = new Image();
	m_gBuffer.albedo = new Image();
	m_gBuffer.normal = new Image();
	m_gBuffer.material = new Image();
	m_gBuffer.emissive = new Image();
	m_gBuffer.lighting = new Image();
	m_gBuffer.depth = new Image();

	m_gBuffer.position->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.albedo->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.normal->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.material->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.emissive->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.lighting->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		true
	);

	m_gBuffer.depth->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		m_app->getVkCore()->getDepthFormat(),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);
}

void Renderer::destroy()
{
	m_descriptorPool.cleanUp();

	delete m_brdfLUT;

	delete m_skyboxMesh;

	delete m_frameConstantsBuffer;
	delete m_transformDataBuffer;
	delete m_pointLightBuffer;
	delete m_bindlessMaterialTable;
	delete m_bufferPointersBuffer;

	for (auto &[id, material] : m_materials)
		delete material;

	delete m_environmentMap;

	delete m_environmentProbe.irradiance;
	delete m_environmentProbe.prefilter;

//	m_shadowAtlas.destroy();

	delete m_gBuffer.position;
	delete m_gBuffer.albedo;
	delete m_gBuffer.normal;
	delete m_gBuffer.material;
	delete m_gBuffer.emissive;
	delete m_gBuffer.lighting;
	delete m_gBuffer.depth;
}

void Renderer::render(Scene &scene, const Camera &camera, Swapchain *swapchain)
{
	GPU_FrameData constants = {
		.proj = camera.getProj(),
		.view = camera.getView(),
		.viewPosition = glm::vec4(camera.position, 1.0f)
	};

	m_frameConstantsBuffer->writeType(constants);

	GPU_TransformData transform = {
		.model = scene.getRenderObjects()[0].transform.getMatrix(),
		.normalMatrix = glm::transpose(glm::inverse(transform.model))
	};

	m_transformDataBuffer->writeType(transform);

	shadowPass(scene, camera);

	deferredPass(scene, camera);

	lightingPass(scene, camera);

	// tonemapping
	{
		static float EXPOSURE = 1.12f;
		static bool ENABLED = true;

		ImGui::Begin("Tonemapping");
		{
			ImGui::Checkbox("Enabled", &ENABLED);
			ImGui::SliderFloat("Exposure", &EXPOSURE, 1.0f, 2.0f);
		}
		ImGui::End();

		// compute tonemapping
		if (ENABLED)
		{
			tonemappingPass(EXPOSURE);
		}
	}

	// draw colour target to swapchain
	GraphicsPipelineDef textureUVPipeline;
	textureUVPipeline.setShader(m_app->getShaders().getShader("texture_uv"));
	textureUVPipeline.setDepthTest(false);
	textureUVPipeline.setDepthWrite(false);

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setAttachments({ { swapchain->getCurrentSwapchainImageView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } } })
		.setInputViews({ m_gBuffer.lighting->getStandardView() })
		.setBuildFn([&, textureUVPipeline](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			// render target
			{
				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(textureUVPipeline, info);

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
			}

			// imgui
			{
				ImGui::Render();

				ImGui_ImplVulkan_RenderDrawData(
					ImGui::GetDrawData(),
					cmd.getHandle()
				);
			}
		})
	);
}

void Renderer::shadowPass(Scene &scene, const Camera &camera)
{
	/*
	// this is bad but im doing it to get stuff running
	// ideally shadow maps would be drawn once and only updated when they
	// actually have to change
	// also they shouldn't allocate a new space every time
	m_shadowAtlas.clear();

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setAttachments({ { m_shadowAtlas.getAtlasView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .depthStencil = { 1.0f, 0 } } } })
		.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			GraphicsPipelineDef shadowMapPipeline;
			shadowMapPipeline.setShader(m_app->getShaders().getShader("shadow_map"));
			shadowMapPipeline.setVertexFormat(&vtx::MODEL_VERTEX_FORMAT);
			shadowMapPipeline.setCullMode(VK_CULL_MODE_FRONT_BIT);

			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(shadowMapPipeline, info);

			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

			for (int i = 0; i < scene.getPointLightCount(); i++)
			{
				auto &light = scene.getPointLights()[i];

				glm::vec3 pos = light.getPosition();
				glm::vec3 col = light.getColour().getDisplayColour();
				glm::vec3 dir = light.getDirection();

				GPU_PointLight gpuLight = {};
				gpuLight.position = { pos.x, pos.y, pos.z, 0.0f };
				gpuLight.colour = { col.x, col.y, col.z, light.getIntensity() };
				gpuLight.attenuation = { 1.0f, 0.0f, 0.0f, light.isShadowCaster() };

				if (light.isShadowCaster())
				{
					for (int i = 0; i < 6; i++)
					{
						ShadowMapAtlas::AtlasRegion region;

						if (m_shadowAtlas.adaptiveAlloc(&region, 3, 6))
						{
							light.setShadowAtlasRegion(region);

							gpuLight.lightSpaceMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.01f, 25.0f) * glm::lookAt(pos, pos + CUBEMAP_CAPTURE_VIEW_DIRECTIONS[i], CUBEMAP_CAPTURE_VIEW_UP[i]);

							cmd.setViewport({ (float)region.area.x, (float)region.area.y, (float)region.area.w, (float)region.area.h });
							cmd.setScissor({ { (int)region.area.x, (int)region.area.y }, { region.area.w, region.area.h } });

							scene.foreachMesh([&](uint32_t meshIndex, Mesh *mesh) -> bool
							{
								glm::mat4 transform = gpuLight.lightSpaceMatrix * mesh->getParent()->getOwner()->transform.getMatrix();

								cmd.pushConstants(
									pipelineData.layout,
									VK_SHADER_STAGE_ALL_GRAPHICS,
									sizeof(glm::mat4),
									&transform
								);

								mesh->bind(cmd);

								cmd.drawIndexed(mesh->getIndexCount());

								return true; // continue
							});

							float regionX = (float)region.area.x / (float)ShadowMapAtlas::ATLAS_SIZE;
							float regionY = (float)region.area.y / (float)ShadowMapAtlas::ATLAS_SIZE;
							float regionW = (float)region.area.w / (float)ShadowMapAtlas::ATLAS_SIZE;
							float regionH = (float)region.area.h / (float)ShadowMapAtlas::ATLAS_SIZE;

							gpuLight.atlasRegion = { regionX, regionY, regionW, regionH };
						}
						else
						{
							MGP_ERROR("Couldn't allocate enough room on shadow map atlas for point light");
						}
					}
				}

				m_pointLightBuffer->writeType(gpuLight, i);
			}
		})
	);
	*/
}

void Renderer::deferredPass(Scene &scene, const Camera &camera)
{
	for (int i = 0; i < scene.getPointLightCount(); i++)
	{
		auto &light = scene.getPointLights()[i];

		glm::vec3 pos = light.getPosition();
		glm::vec3 col = light.getColour().getDisplayColour();
		glm::vec3 dir = light.getDirection();

		GPU_PointLight gpuLight = {};
		gpuLight.position		= { pos.x, pos.y, pos.z, 0.0f };
		gpuLight.colour			= { col.x, col.y, col.z, light.getIntensity() };
		gpuLight.attenuation	= { 1.0f, 0.0f, 0.0f, light.isShadowCaster() };

		m_pointLightBuffer->writeType(gpuLight, i);
	}

	std::vector<RenderGraph::AttachmentInfo> attachments;
	attachments.push_back({ m_gBuffer.position->getStandardView(),	VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } });
	attachments.push_back({ m_gBuffer.albedo->getStandardView(),	VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } });
	attachments.push_back({ m_gBuffer.normal->getStandardView(),	VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } });
	attachments.push_back({ m_gBuffer.material->getStandardView(),	VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } });
	attachments.push_back({ m_gBuffer.emissive->getStandardView(),	VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } });
	attachments.push_back({ m_gBuffer.depth->getStandardView(),		VK_ATTACHMENT_LOAD_OP_CLEAR, { .depthStencil = { 1.0f, 0 } } });

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setAttachments(attachments)
		.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			uint64_t currentPipelineHash = 0;

			scene.foreachMesh([&](uint32_t meshIndex, Mesh *mesh) -> bool
			{
				Material *mat = mesh->getMaterial();

				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(mat->passes[SHADER_PASS_DEFERRED], info);

				if (meshIndex == 0)
				{
					cmd.bindDescriptorSets(
						0,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.layout,
						{ m_app->getVkCore()->getBindlessResources().getSet() },
						{}
					);
				}

				if (meshIndex == 0 || currentPipelineHash != mat->getHash())
				{
					cmd.bindPipeline(
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.pipeline
					);

					currentPipelineHash = mat->getHash();
				}

				GPU_ModelPushConstants pushConstants = {};
				pushConstants.buffers							= m_bufferPointersBuffer->getGPUAddress();
				pushConstants.irradianceMap_id					= m_environmentProbe.irradiance->getStandardView()->getBindlessHandle();
				pushConstants.prefilterMap_id					= m_environmentProbe.prefilter->getStandardView()->getBindlessHandle();
				pushConstants.brdfLUT_id						= m_brdfLUT->getStandardView()->getBindlessHandle();
				pushConstants.material_id						= mat->bindlessHandle;
				pushConstants.cubemapSampler_id					= m_app->getTextures().getLinearSampler()->getBindlessHandle();
				pushConstants.textureSampler_id					= m_app->getTextures().getLinearSampler()->getBindlessHandle();

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(GPU_ModelPushConstants),
					&pushConstants
				);

				mesh->bind(cmd);

				int id = 0;

				cmd.drawIndexed(mesh->getIndexCount(), 1, 0, 0, id);

				return true; // continue
			});
		})
	);
}

void Renderer::lightingPass(Scene &scene, const Camera &camera)
{
	std::vector<RenderGraph::AttachmentInfo> attachments = {
		{ m_gBuffer.lighting->getStandardView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } },
		{ m_gBuffer.depth->getStandardView(), VK_ATTACHMENT_LOAD_OP_LOAD, {} }
	};

	std::vector<ImageView *> inputViews = {
		m_gBuffer.position->getStandardView(),
		m_gBuffer.albedo->getStandardView(),
		m_gBuffer.normal->getStandardView(),
		m_gBuffer.material->getStandardView(),
		m_gBuffer.emissive->getStandardView()
	};

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setAttachments(attachments)
		.setInputViews(inputViews)
		.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			GraphicsPipelineDef ambientDeferredLightingPipeline;
			ambientDeferredLightingPipeline.setShader(m_app->getShaders().getShader("deferred_lighting_ambient"));
			ambientDeferredLightingPipeline.setDepthTest(false);
			ambientDeferredLightingPipeline.setDepthWrite(false);

			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(ambientDeferredLightingPipeline, info);

			cmd.bindPipeline(
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.pipeline
			);

			cmd.bindDescriptorSets(
				0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.layout,
				{ m_app->getVkCore()->getBindlessResources().getSet() },
				{}
			);

			GPU_DeferredLightingPushConstants pc = {};
			pc.position_id						 = m_gBuffer.position->getStandardView()->getBindlessHandle();
			pc.albedo_id						 = m_gBuffer.albedo->getStandardView()->getBindlessHandle();
			pc.normal_id						 = m_gBuffer.normal->getStandardView()->getBindlessHandle();
			pc.material_id						 = m_gBuffer.material->getStandardView()->getBindlessHandle();
			pc.emissive_id						 = m_gBuffer.emissive->getStandardView()->getBindlessHandle();
			pc.irradianceMap_id					 = m_environmentProbe.irradiance->getStandardView()->getBindlessHandle();
			pc.prefilterMap_id					 = m_environmentProbe.prefilter->getStandardView()->getBindlessHandle();
			pc.brdfLUT_id						 = m_brdfLUT->getStandardView()->getBindlessHandle();
			pc.textureSampler_id				 = m_app->getTextures().getLinearSampler()->getBindlessHandle();
			pc.cubemapSampler_id				 = m_app->getTextures().getLinearSampler()->getBindlessHandle();
			pc.cameraPosition					 = { camera.position.x, camera.position.y, camera.position.z, 0.0f };
			
			cmd.pushConstants(
				pipelineData.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(GPU_DeferredLightingPushConstants),
				&pc
			);

			cmd.draw(3);

			// skybox
			{
				GraphicsPipelineDef skyboxPipeline;
				skyboxPipeline.setShader(m_app->getShaders().getShader("skybox"));
				skyboxPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
				skyboxPipeline.setDepthTest(true);
				skyboxPipeline.setDepthWrite(false);
				skyboxPipeline.setDepthOp(VK_COMPARE_OP_LESS_OR_EQUAL);

				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(skyboxPipeline, info);

				glm::mat4 viewProj = camera.getProj() * camera.getRotationMatrix();

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
					sizeof(glm::mat4),
					&viewProj
				);

				m_skyboxMesh->bind(cmd);

				cmd.drawIndexed(m_skyboxMesh->getIndexCount());
			}
		})
	);
}

void Renderer::tonemappingPass(float exposure)
{
	Shader *hdrTonemappingShader = m_app->getShaders().getShader("hdr_tonemapping");
	
	ComputePipelineDef hdrTonemappingPipeline;
	hdrTonemappingPipeline.setShader(hdrTonemappingShader);

	m_app->getVkCore()->getRenderGraph().addTask(RenderGraph::ComputeTaskDefinition()
		.setStorageViews({ m_gBuffer.lighting->getStandardView() })
		.setBuildFn([&, hdrTonemappingPipeline, exposure](CommandBuffer &cmd) -> void
		{
			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchComputePipeline(hdrTonemappingPipeline);
			
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
				float exposure;
			}
			pc;

			pc.width = (uint32_t)m_app->getPlatform()->getWindowSizeInPixels().x;
			pc.height = (uint32_t)m_app->getPlatform()->getWindowSizeInPixels().y;
			pc.exposure = exposure;

			cmd.pushConstants(
				pipelineData.layout,
				VK_SHADER_STAGE_COMPUTE_BIT,
				sizeof(pc),
				&pc
			);

			cmd.dispatch(pc.width, pc.height, 1);
		})
	);
}

void Renderer::precomputeBRDF()
{
	InstantSubmitSync instantSubmit(m_app->getVkCore());
	CommandBuffer &cmd = instantSubmit.begin();
	{
		MGP_LOG("Precomputing BRDF...");

		const int BRDF_RESOLUTION = 512;

		m_brdfLUT = new Image();
		m_brdfLUT->allocate(
			m_app->getVkCore(),
			BRDF_RESOLUTION, BRDF_RESOLUTION, 1,
			VK_FORMAT_R32G32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_TILING_OPTIMAL,
			1,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		RenderInfo info(m_app->getVkCore());
		info.setSize(BRDF_RESOLUTION, BRDF_RESOLUTION);
		info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *m_brdfLUT->getStandardView(), nullptr);

		Shader *brdfLUTShader = m_app->getShaders().getShader("brdf_lut");

		GraphicsPipelineDef brdfIntegrationPipeline;
		brdfIntegrationPipeline.setShader(brdfLUTShader);
		brdfIntegrationPipeline.setDepthTest(false);
		brdfIntegrationPipeline.setDepthWrite(false);

		PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(brdfIntegrationPipeline, info);

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd.beginRendering(info);
		{
			cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);
			cmd.draw(3);
		}
		cmd.endRendering();

		cmd.transitionLayout(*m_brdfLUT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();
}

void Renderer::generateEnvironmentProbe()
{
	const int ENVIRONMENT_MAP_RESOLUTION = 1024;
	const int IRRADIANCE_MAP_RESOLUTION = 32;

	const int PREFILTER_MAP_RESOLUTION = 128;
	const int PREFILTER_MAP_MIP_LEVELS = 5;

	glm::mat4 captureProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	glm::mat4 view_proj_pc = glm::identity<glm::mat4>();

	InstantSubmitSync instantSubmit(m_app->getVkCore());

	// generate environment map / skybox from 2D HDR image
	CommandBuffer &cmd = instantSubmit.begin();
	{
		MGP_LOG("Generating environment map...");

		m_environmentMap = new Image();
		m_environmentMap->allocate(
			m_app->getVkCore(),
			ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			4,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		Shader *eqrToCbmShader = m_app->getShaders().getShader("equirectangular_to_cubemap");
		Image *environmentHDRImage = m_app->getTextures().getTexture("environmentHDR");

		VkDescriptorSet eqrToCbmSet = allocateSet(eqrToCbmShader->getDescriptorSetLayouts());

		DescriptorWriter(m_app->getVkCore())
			.writeCombinedImage(0, *environmentHDRImage->getStandardView(), *m_app->getTextures().getLinearSampler())
			.writeTo(eqrToCbmSet);

		GraphicsPipelineDef equirectangularToCubemapPipeline;
		equirectangularToCubemapPipeline.setShader(eqrToCbmShader);
		equirectangularToCubemapPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
		equirectangularToCubemapPipeline.setDepthTest(false);
		equirectangularToCubemapPipeline.setDepthWrite(false);

		cmd.transitionLayout(*m_environmentMap, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			view_proj_pc = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

			ImageView *view = m_environmentMap->createView(1, i, 0);

			RenderInfo targetInfo(m_app->getVkCore());
			targetInfo.setSize(ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(equirectangularToCubemapPipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { eqrToCbmSet }, {});

				cmd.setViewport({ 0, 0, ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(glm::mat4),
					&view_proj_pc
				);

				m_skyboxMesh->bind(cmd);

				cmd.drawIndexed(m_skyboxMesh->getIndexCount());
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
		m_app->getVkCore(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(prefilterParams) * PREFILTER_MAP_MIP_LEVELS
	);

	// generate irradiance and prefilter maps from environment map
	cmd = instantSubmit.begin();
	{
		MGP_LOG("Generating irradiance map...");

		m_environmentProbe.irradiance = new Image();
		m_environmentProbe.irradiance->allocate(
			m_app->getVkCore(),
			IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			4,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		Shader *irradianceGenShader = m_app->getShaders().getShader("irradiance_convolution");

		VkDescriptorSet irradianceSet = allocateSet(irradianceGenShader->getDescriptorSetLayouts());

		DescriptorWriter(m_app->getVkCore())
			.writeCombinedImage(0, *m_environmentMap->getStandardView(), *m_app->getTextures().getLinearSampler())
			.writeTo(irradianceSet);

		GraphicsPipelineDef irradiancePipeline;
		irradiancePipeline.setShader(irradianceGenShader);
		irradiancePipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
		irradiancePipeline.setDepthTest(false);
		irradiancePipeline.setDepthWrite(false);

		cmd.transitionLayout(*m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		for (int i = 0; i < 6; i++)
		{
			view_proj_pc = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

			ImageView *view = m_environmentProbe.irradiance->createView(1, i, 0);

			RenderInfo targetInfo(m_app->getVkCore());
			targetInfo.setSize(IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(irradiancePipeline, targetInfo);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd.beginRendering(targetInfo);
			{
				cmd.bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.pipeline);

				cmd.bindDescriptorSets(0, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineData.layout, { irradianceSet }, {});

				cmd.setViewport({ 0, 0, IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION });

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(glm::mat4),
					&view_proj_pc
				);

				m_skyboxMesh->bind(cmd);

				cmd.drawIndexed(m_skyboxMesh->getIndexCount());
			}
			cmd.endRendering();
		}

		cmd.transitionLayout(*m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd.generateMipmaps(*m_environmentProbe.irradiance);

		// ---

		MGP_LOG("Generating prefilter map...");

		m_environmentProbe.prefilter = new Image();
		m_environmentProbe.prefilter->allocate(
			m_app->getVkCore(),
			PREFILTER_MAP_RESOLUTION, PREFILTER_MAP_RESOLUTION, 1,
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VK_IMAGE_TILING_OPTIMAL,
			PREFILTER_MAP_MIP_LEVELS,
			VK_SAMPLE_COUNT_1_BIT,
			false,
			false
		);

		Shader *prefilterShader = m_app->getShaders().getShader("prefilter_convolution");

		VkDescriptorSet prefilterSet = allocateSet(prefilterShader->getDescriptorSetLayouts());

		DescriptorWriter(m_app->getVkCore())
			.writeBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pfParameterBuffer->getDescriptorInfoRange(sizeof(prefilterParams)))
			.writeCombinedImage(1, *m_environmentMap->getStandardView(), *m_app->getTextures().getLinearSampler())
			.writeTo(prefilterSet);

		GraphicsPipelineDef prefilterPipeline;
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

			pfParameterBuffer->write(&prefilterParams, sizeof(prefilterParams), dynamicOffset);

			for (int i = 0; i < 6; i++)
			{
				view_proj_pc = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

				ImageView *view = m_environmentProbe.prefilter->createView(1, i, mipLevel);

				RenderInfo info(m_app->getVkCore());
				info.setSize(width, height);
				info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, *view, nullptr);

				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(prefilterPipeline, info);

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
						sizeof(glm::mat4),
						&view_proj_pc
					);

					m_skyboxMesh->bind(cmd);

					cmd.drawIndexed(m_skyboxMesh->getIndexCount());
				}
				cmd.endRendering();
			}
		}

		cmd.transitionLayout(*m_environmentProbe.prefilter, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	instantSubmit.submit();

	m_app->getVkCore()->deviceWaitIdle(); // deleting the pfParameterBuffer
	// fixme

	delete pfParameterBuffer;
}

void Renderer::createSkyboxMesh()
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

	m_skyboxMesh = new Mesh(m_app->getVkCore());

	m_skyboxMesh->build(
		vtx::PRIMITIVE_VERTEX_FORMAT,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

void Renderer::loadTechniques()
{
	// PBR
	{
		Technique texturedPBR_gbuffer;
		texturedPBR_gbuffer.passes[SHADER_PASS_DEFERRED] = m_app->getShaders().getShader("texturedPBR_gbuffer");
		texturedPBR_gbuffer.passes[SHADER_PASS_FORWARD] = nullptr;
		texturedPBR_gbuffer.vertexFormat = &vtx::MODEL_VERTEX_FORMAT;
		addTechnique("texturedPBR_gbuffer_opaque", texturedPBR_gbuffer);
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

Material *Renderer::buildMaterial(MaterialData &data)
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

	GPU_BindlessMaterial handles = {};
	handles.diffuse_id = material->textures[0];
	handles.ambient_id = material->textures[1];
	handles.material_id = material->textures[2];
	handles.normal_id = material->textures[3];
	handles.emissive_id = material->textures[4];

	material->bindlessHandle = m_materialHandle_UID++;

	m_bindlessMaterialTable->write(
		&handles,
		sizeof(GPU_BindlessMaterial),
		sizeof(GPU_BindlessMaterial) * material->bindlessHandle
	);

	return material;
}

void Renderer::addTechnique(const std::string &name, const Technique &technique)
{
	m_techniques.insert({ name, technique });
}

VkDescriptorSet Renderer::allocateSet(const std::vector<VkDescriptorSetLayout> &layouts)
{
	return m_descriptorPool.allocate(layouts);
}
