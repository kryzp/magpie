#include "renderer.h"

#include <glm/glm.hpp>

#include "core/app.h"
#include "core/platform.h"

#include "vulkan/core.h"
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

struct GPU_FrameData
{
	glm::mat4 proj;
	glm::mat4 view;
	glm::vec4 view_position;
};

struct GPU_TransformData
{
	glm::mat4 model;
	glm::mat4 normal_matrix;
};

struct GPU_BindlessMaterial
{
    uint32_t diffuse_id;
    uint32_t ambient_id;
    uint32_t ambient_rough_metallic_id;
    uint32_t normal_id;
    uint32_t emissive_id;
};

struct GPU_BufferPointers
{
	VkDeviceAddress frame_data;
	VkDeviceAddress transforms;
	VkDeviceAddress materials;
	VkDeviceAddress point_lights;
};

struct GPU_ModelPushConstants
{
	VkDeviceAddress buffers;
    uint32_t transform_id;
    uint32_t irradiance_map_id;
    uint32_t prefilter_map_id;
    uint32_t brdf_lut_id;
    uint32_t material_id;
    uint32_t shadow_atlas_id;
    uint32_t cubemap_sampler_id;
    uint32_t texture_sampler_id;
    uint32_t shadow_atlas_sampler_id;
};

void Renderer::init(App *app, Swapchain *swapchain)
{
	m_app = app;

	loadTechniques();

	m_shadowAtlas.init(app->getVkCore());

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
	pointers.point_lights = m_pointLightBuffer->getGPUAddress();

	m_bufferPointersBuffer->writeType(pointers);

	createSkyboxMesh();

	precomputeBRDF();
	generateEnvironmentProbe();
	
	createSkybox();

	m_targetColour = new Image();
	m_targetColour->allocate(
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

	m_targetDepth = new Image();
	m_targetDepth->allocate(
		m_app->getVkCore(),
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		app->getVkCore()->getDepthFormat(),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	Shader *textureUVShader = app->getShaders().getShader("texture_uv");
	m_textureUVPipeline.setShader(textureUVShader);
	m_textureUVPipeline.setDepthTest(false);
	m_textureUVPipeline.setDepthWrite(false);
	m_textureUVSet = app->allocateSet(textureUVShader->getDescriptorSetLayouts());
	
	DescriptorWriter(m_app->getVkCore())
		.writeCombinedImage(0, *m_targetColour->getStandardView(), *m_app->getTextures().getLinearSampler())
		.writeTo(m_textureUVSet);

	Shader *hdrTonemappingShader = app->getShaders().getShader("hdr_tonemapping");
	m_hdrTonemappingPipeline.setShader(hdrTonemappingShader);

	m_hdrTonemappingSet = app->allocateSet(hdrTonemappingShader->getDescriptorSetLayouts());
	//m_hdrTonemappingSet.writeStorageImage(0, *m_targetColour->getStandardView());

	DescriptorWriter(m_app->getVkCore())
		.writeStorageImage(0, *m_targetColour->getStandardView())
		.writeTo(m_hdrTonemappingSet);
}

void Renderer::destroy()
{
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

	m_shadowAtlas.destroy();

	delete m_targetColour;
	delete m_targetDepth;
}

void Renderer::render(Scene &scene, const Camera &camera, Swapchain *swapchain)
{
	GPU_FrameData constants = {
		.proj = camera.getProj(),
		.view = camera.getView(),
		.view_position = glm::vec4(camera.position, 1.0f)
	};

	m_frameConstantsBuffer->writeType(constants);

	GPU_TransformData transform = {
		.model = scene.getRenderObjects()[0].transform.getMatrix(),
		.normal_matrix = glm::transpose(glm::inverse(transform.model))
	};

	m_transformDataBuffer->writeType(transform);

	shadowPass(scene, camera);

	deferredPass(scene, camera);

	// compute tonemapping
	if (m_app->getInput()->alt())
	{
		m_app->getVkCore()->getRenderGraph().addTask(RenderGraph::ComputeTaskDefinition()
			.setStorageViews({ m_targetColour->getStandardView() })
			.setBuildFn([&](CommandBuffer &cmd) -> void
			{
				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchComputePipeline(m_hdrTonemappingPipeline);
			
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
	//				float blurIntensity;
				}
				pc;

				pc.width = (uint32_t)m_app->getPlatform()->getWindowSizeInPixels().x;
				pc.height = (uint32_t)m_app->getPlatform()->getWindowSizeInPixels().y;
				pc.exposure = 2.25f;

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_COMPUTE_BIT,
					sizeof(pc),
					&pc
				);

				cmd.dispatch(pc.width, pc.height, 1);
			}));
	}

	// draw colour target to swapchain
	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setOutputAttachments({ { swapchain->getCurrentSwapchainImageView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } } })
		.setInputViews({ m_targetColour->getStandardView() })
		.setBuildFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
			PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(m_textureUVPipeline, info);

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
}

void Renderer::shadowPass(Scene &scene, const Camera &camera)
{
	GraphicsPipelineDef shadowMapPipeline;
	shadowMapPipeline.setShader(m_app->getShaders().getShader("shadow_map"));
	shadowMapPipeline.setVertexFormat(&vtx::MODEL_VERTEX_FORMAT);
	shadowMapPipeline.setCullMode(VK_CULL_MODE_FRONT_BIT);

	// this is bad but im doing it to get stuff running
	// ideally shadow maps would be drawn once and only updated when they
	// actually have to change
	// also they shouldn't allocate a new space every time
	m_shadowAtlas.clear();

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setOutputAttachments({ { m_shadowAtlas.getAtlasView(), VK_ATTACHMENT_LOAD_OP_CLEAR, {.depthStencil = { 1.0f, 0 } } } })
		.setBuildFn([&, shadowMapPipeline](CommandBuffer &cmd, const RenderInfo &info) -> void
		{
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
				gpuLight.colour = { col.x * light.getIntensity(), col.y * light.getIntensity(), col.z * light.getIntensity(), 0.0f };

				if (light.isShadowCaster())
				{
					ShadowMapAtlas::AtlasRegion region;

					if (m_shadowAtlas.adaptiveAlloc(&region, 2, 6))
					{
						light.setShadowAtlasRegion(region);

						Camera lightCamera((float)region.area.w / (float)region.area.h, 75.0f, 0.01f, 25.0f);
						lightCamera.position = gpuLight.position;
						lightCamera.direction = dir;

						gpuLight.lightSpaceMatrix = lightCamera.getProj() * lightCamera.getView();

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
						MGP_ERROR("couldn't allocate enough room on shadowmap for light");
					}
				}

				m_pointLightBuffer->writeType(gpuLight, i);
			}
		})
	);
}

void Renderer::deferredPass(Scene &scene, const Camera &camera)
{
	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setOutputAttachments({
			{ m_targetColour->getStandardView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .color = { 0.0f, 0.0f, 0.0f, 1.0f } } },
			{ m_targetDepth->getStandardView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .depthStencil = { 1.0f, 0 } } }
		})
		.setInputViews({ m_shadowAtlas.getAtlasView() })
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
				pushConstants.buffers = m_bufferPointersBuffer->getGPUAddress();
				pushConstants.transform_id = 0;
				pushConstants.irradiance_map_id = m_environmentProbe.irradiance->getStandardView()->getBindlessHandle();
				pushConstants.prefilter_map_id = m_environmentProbe.prefilter->getStandardView()->getBindlessHandle();
				pushConstants.brdf_lut_id = m_brdfLUT->getStandardView()->getBindlessHandle();
				pushConstants.material_id = mat->bindlessHandle;
				pushConstants.shadow_atlas_id = m_shadowAtlas.getAtlasView()->getBindlessHandle();
				pushConstants.cubemap_sampler_id = m_app->getTextures().getLinearSampler()->getBindlessHandle();
				pushConstants.texture_sampler_id = m_app->getTextures().getLinearSampler()->getBindlessHandle();
				pushConstants.shadow_atlas_sampler_id = m_app->getTextures().getNearestSampler()->getBindlessHandle();

				cmd.pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(GPU_ModelPushConstants),
					&pushConstants
				);

				mesh->bind(cmd);

				cmd.drawIndexed(mesh->getIndexCount());

				return true; // continue
			});

			// skybox
			{
				PipelineState pipelineData = m_app->getVkCore()->getPipelineCache().fetchGraphicsPipeline(m_skyboxPipeline, info);

				glm::mat4 view_proj = camera.getProj() * camera.getRotationMatrix();

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
					&view_proj
				);

				m_skyboxMesh->bind(cmd);

				cmd.drawIndexed(m_skyboxMesh->getIndexCount());
			}
		}));
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

	glm::mat4 captureViewMatrices[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,-1.0f, 0.0f), glm::vec3(0.0f, 0.0f,-1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f),-glm::vec3( 0.0f, 0.0f,-1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
	};

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

		VkDescriptorSet eqrToCbmSet = m_app->allocateSet(eqrToCbmShader->getDescriptorSetLayouts());

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
			view_proj_pc = captureProjectionMatrix * captureViewMatrices[i];

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

		VkDescriptorSet irradianceSet = m_app->allocateSet(irradianceGenShader->getDescriptorSetLayouts());

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
			view_proj_pc = captureProjectionMatrix * captureViewMatrices[i];

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

		VkDescriptorSet prefilterSet = m_app->allocateSet(prefilterShader->getDescriptorSetLayouts());

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
				view_proj_pc = captureProjectionMatrix * captureViewMatrices[i];

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

void Renderer::createSkybox()
{
	m_skyboxShader = m_app->getShaders().getShader("skybox");

	m_skyboxSet = m_app->allocateSet(m_skyboxShader->getDescriptorSetLayouts());

	DescriptorWriter(m_app->getVkCore())
		.writeCombinedImage(0, *m_environmentMap->getStandardView(), *m_app->getTextures().getLinearSampler())
		.writeTo(m_skyboxSet);

	m_skyboxPipeline.setShader(m_skyboxShader);
	m_skyboxPipeline.setVertexFormat(&vtx::PRIMITIVE_VERTEX_FORMAT);
	m_skyboxPipeline.setDepthTest(true);
	m_skyboxPipeline.setDepthWrite(false);
	m_skyboxPipeline.setDepthOp(VK_COMPARE_OP_LESS_OR_EQUAL);
}

void Renderer::loadTechniques()
{
	// PBR
	{
		Technique texturedPBR;
		texturedPBR.passes[SHADER_PASS_DEFERRED] = m_app->getShaders().getShader("texturedPBR");
		texturedPBR.passes[SHADER_PASS_FORWARD] = nullptr;
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
	handles.ambient_rough_metallic_id = material->textures[2];
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

