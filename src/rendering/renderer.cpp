#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_vulkan.h"

#include "graphics/graphics_core.h"
#include "graphics/gpu_buffer.h"
#include "graphics/render_info.h"
#include "graphics/swapchain.h"

#include "core/app.h"

#include "vertex_types.h"
#include "camera.h"
#include "light.h"
#include "model.h"
#include "material.h"
#include "scene.h"

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

Renderer::Renderer()
	: m_app(nullptr)
	, m_renderGraph(nullptr)
	, m_gBuffer()
	, m_frameConstantsBuffer(nullptr)
	, m_transformDataBuffer(nullptr)
	, m_bindlessMaterialTable(nullptr)
	, m_pointLightBuffer(nullptr)
	, m_bufferPointersTable(nullptr)
	, m_descriptorPool(nullptr)
	, m_textureUV_descriptor(nullptr)
	, m_hdrTonemapping_descriptor(nullptr)
	, m_brdfLUT(nullptr)
	, m_environmentMap(nullptr)
	, m_environmentProbe()
	, m_skyboxMesh(nullptr)
	, m_skybox_descriptor(nullptr)
	, m_materials()
	, m_techniques()
	, m_materialFreeIndex(0)
{
}

void Renderer::init(App *app)
{
	m_app = app;

	m_renderGraph = new RenderGraph(app->getGraphics());

	m_descriptorPool = m_app->getGraphics()->createDescriptorPool(64 * gfx_constants::FRAMES_IN_FLIGHT, 0, {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 						0.5f },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		4.0f },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 				4.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 				1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 			1.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 				2.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 				2.0f },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,		1.0f },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,		1.0f },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 				0.5f }
	});

	loadTechniques();

	m_frameConstantsBuffer = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_FrameData)
	);

	m_transformDataBuffer = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_TransformData)
	);

	m_bindlessMaterialTable = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_BindlessMaterial) * 128
	);

	m_pointLightBuffer = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_PointLight) * MAX_POINT_LIGHTS
	);

	m_bufferPointersTable = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(GPU_BufferPointers)
	);
	
	GPU_BufferPointers pointers = {};
	{
		pointers.frame_data		= m_frameConstantsBuffer	->getDeviceAddress();
		pointers.transforms		= m_transformDataBuffer		->getDeviceAddress();
		pointers.materials		= m_bindlessMaterialTable	->getDeviceAddress();
		pointers.pointLights	= m_pointLightBuffer		->getDeviceAddress();
	}
	m_bufferPointersTable->writeType<GPU_BufferPointers>(pointers);

	createSkyboxResources();
	precomputeBRDF_LUT();
	generateEnvironmentMaps();

	createGBuffer();

	m_skybox_descriptor				= allocateDescriptor	(m_app->getShaders().getShader("skybox")				->getLayouts());
	m_textureUV_descriptor			= allocateDescriptor	(m_app->getShaders().getShader("texture_uv")			->getLayouts());
	m_hdrTonemapping_descriptor		= allocateDescriptor	(m_app->getShaders().getShader("hdr_tonemapping")		->getLayouts());

	m_skybox_descriptor				->writeCombinedImage	(0, stdView(m_environmentMap),			m_app->getTextures().getLinearSampler());
	m_textureUV_descriptor			->writeCombinedImage	(0, stdView(m_gBuffer.lighting),		m_app->getTextures().getLinearSampler());
	m_hdrTonemapping_descriptor		->writeStorageImage		(0, stdView(m_gBuffer.lighting));
}

void Renderer::destroy()
{
	delete m_renderGraph;

	delete m_descriptorPool;

	delete m_brdfLUT;

	delete m_skyboxMesh;

	delete m_frameConstantsBuffer;
	delete m_transformDataBuffer;
	delete m_pointLightBuffer;
	delete m_bindlessMaterialTable;
	delete m_bufferPointersTable;

	for (auto &[id, material] : m_materials)
		delete material;

	delete m_environmentMap;

	delete m_environmentProbe.irradiance;
	delete m_environmentProbe.prefilter;

	delete m_gBuffer.position;
	delete m_gBuffer.albedo;
	delete m_gBuffer.normal;
	delete m_gBuffer.material;
	delete m_gBuffer.emissive;
	delete m_gBuffer.lighting;
	delete m_gBuffer.depth;
}

void Renderer::render(const RenderContext &context)
{
	m_frameConstantsBuffer->writeType<GPU_FrameData>({
		.proj = context.camera->getProj(),
		.view = context.camera->getView(),
		.viewPosition = glm::vec4(context.camera->position, 1.0f)
	});

	glm::mat4 transformMatrix = glm::identity<glm::mat4>();//context.scene->getRenderObjects()[0].transform.getMatrix();

	m_transformDataBuffer->writeType<GPU_TransformData>({
		.model = transformMatrix,
		.normalMatrix = glm::transpose(glm::inverse(transformMatrix))
	});

	shadowPass(context);
	deferredPass(context);
	lightingPass(context);

	/*
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
	*/

	tonemappingPass(1.12f);
	
	m_renderGraph->addPass(RenderPassDef()
		.setAttachments({ RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, context.swapchain->getCurrentView(), nullptr, Colour::black()) })
		.setInputViews({ stdView(m_gBuffer.lighting) })
		.setRecordFn([&](CommandBuffer *cmd, const RenderInfo &info) -> void
		{
			// render target
			{
				GraphicsPipelineDef textureUVPipeline;
				textureUVPipeline.setShader(m_app->getShaders().getShader("texture_uv"));
				textureUVPipeline.setDepthTest(false);
				textureUVPipeline.setDepthWrite(false);

				PipelineState pipelineData = m_app->getPipelines().fetchGraphicsPipeline(textureUVPipeline, info);

				cmd->bindPipeline(
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineData.pipeline
				);

				cmd->bindDescriptors(
					0,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineData.layout,
					{ m_textureUV_descriptor },
					{}
				);

				cmd->draw(3);
			}

			// imgui
			{
				ImGui::Render();

				ImGui_ImplVulkan_RenderDrawData(
					ImGui::GetDrawData(),
					cmd->getHandle()
				);
			}
		})
	);

	m_renderGraph->recordTo(context.cmd, context.swapchain);
}

void Renderer::shadowPass(const RenderContext &context)
{
	/*
	// this is bad but im doing it to get stuff running
	// ideally shadow maps would be drawn once and only updated when they
	// actually have to change
	// also they shouldn't allocate a new space every time
	m_shadowAtlas.clear();

	m_app->getVkCore()->getRenderGraph().addPass(RenderGraph::RenderPassDefinition()
		.setAttachments({ { m_shadowAtlas.getAtlasView(), VK_ATTACHMENT_LOAD_OP_CLEAR, { .depthStencil = { 1.0f, 0 } } } })
		.setRecordFn([&](CommandBuffer &cmd, const RenderInfo &info) -> void
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

void Renderer::deferredPass(const RenderContext &context)
{
	for (int i = 0; i < context.scene->getPointLightCount(); i++)
	{
		auto &light = context.scene->getPointLights()[i];

		glm::vec3 pos = light.getPosition();
		glm::vec3 col = light.getColour().getDisplayColour();
		glm::vec3 dir = light.getDirection();

		GPU_PointLight gpuLight = {};
		gpuLight.position		= { pos.x, pos.y, pos.z, 0.0f };
		gpuLight.colour			= { col.x, col.y, col.z, light.getIntensity() };
		gpuLight.attenuation	= { 1.0f, 0.0f, 0.0f, 0.0f };

		m_pointLightBuffer->writeType(gpuLight, i);
	}

	std::vector<RenderGraphAttachment> attachments = {
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.position), nullptr, Colour::black()),
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.albedo), nullptr, Colour::black()),
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.normal), nullptr, Colour::black()),
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.material), nullptr, Colour::black()),
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.emissive), nullptr, Colour::black()),
		RenderGraphAttachment::getDepth(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.depth), nullptr, 1.0f, 0),
	};

	m_renderGraph->addPass(RenderPassDef()
		.setAttachments(attachments)
		.setRecordFn([&](CommandBuffer *cmd, const RenderInfo &info) -> void
		{
			uint64_t currentPipelineHash = 0;

			context.scene->foreachMesh([&](uint32_t meshIndex, Mesh *mesh) -> bool
			{
				Material *mat = mesh->getMaterial();

				PipelineState pipelineData = m_app->getPipelines().fetchGraphicsPipeline(mat->getPipeline(SHADER_PASS_DEFERRED), info);

				if (meshIndex == 0)
				{
					cmd->bindDescriptors(
						0,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.layout,
						{ m_app->getBindlessResources()->getDescriptor() },
						{}
					);
				}

				if (meshIndex == 0 || currentPipelineHash != mat->getHash())
				{
					cmd->bindPipeline(
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineData.pipeline
					);

					currentPipelineHash = mat->getHash();
				}

				GPU_ModelPushConstants pushConstants = {};
				pushConstants.buffers							= m_bufferPointersTable->getDeviceAddress();
				pushConstants.irradianceMap_id					= cbmIdx(stdView(m_environmentProbe.irradiance));
				pushConstants.prefilterMap_id					= cbmIdx(stdView(m_environmentProbe.prefilter));
				pushConstants.brdfLUT_id						= tex2DIdx(stdView(m_brdfLUT));
				pushConstants.material_id						= mat->getTableIndex();
				pushConstants.cubemapSampler_id					= smpIdx(m_app->getTextures().getLinearSampler());
				pushConstants.textureSampler_id					= smpIdx(m_app->getTextures().getLinearSampler());

				cmd->pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(GPU_ModelPushConstants),
					&pushConstants
				);

				mesh->bind(cmd);

				int id = 0;

				cmd->drawIndexed(mesh->getIndexCount(), 1, 0, 0, id);

				return true; // continue
			});
		})
	);
}

void Renderer::lightingPass(const RenderContext &context)
{
	std::vector<RenderGraphAttachment> attachments = {
		RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_CLEAR, stdView(m_gBuffer.lighting), nullptr, Colour::black()),
		RenderGraphAttachment::getDepth(VK_ATTACHMENT_LOAD_OP_LOAD, stdView(m_gBuffer.depth), nullptr, 1.0f, 0)
	};

	std::vector<ImageView *> inputViews = {
		stdView(m_gBuffer.position),
		stdView(m_gBuffer.albedo),
		stdView(m_gBuffer.normal),
		stdView(m_gBuffer.material),
		stdView(m_gBuffer.emissive),
		stdView(m_environmentMap)
	};

	m_renderGraph->addPass(RenderPassDef()
		.setAttachments(attachments)
		.setInputViews(inputViews)
		.setRecordFn([&](CommandBuffer *cmd, const RenderInfo &info) -> void
		{
			GraphicsPipelineDef ambientDeferredLightingPipeline;
			ambientDeferredLightingPipeline.setShader(m_app->getShaders().getShader("deferred_lighting_ambient"));
			ambientDeferredLightingPipeline.setDepthTest(false);
			ambientDeferredLightingPipeline.setDepthWrite(false);

			PipelineState pipelineData = m_app->getPipelines().fetchGraphicsPipeline(ambientDeferredLightingPipeline, info);

			cmd->bindPipeline(
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.pipeline
			);

			cmd->bindDescriptors(
				0,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineData.layout,
				{ m_app->getBindlessResources()->getDescriptor() },
				{}
			);

			GPU_DeferredLightingPushConstants pc = {};
			pc.position_id						 = tex2DIdx(stdView(m_gBuffer.position));
			pc.albedo_id						 = tex2DIdx(stdView(m_gBuffer.albedo));
			pc.normal_id						 = tex2DIdx(stdView(m_gBuffer.normal));
			pc.material_id						 = tex2DIdx(stdView(m_gBuffer.material));
			pc.emissive_id						 = tex2DIdx(stdView(m_gBuffer.emissive));
			pc.irradianceMap_id					 = cbmIdx(stdView(m_environmentProbe.irradiance));
			pc.prefilterMap_id					 = cbmIdx(stdView(m_environmentProbe.prefilter));
			pc.brdfLUT_id						 = tex2DIdx(stdView(m_brdfLUT));
			pc.textureSampler_id				 = smpIdx(m_app->getTextures().getLinearSampler());
			pc.cubemapSampler_id				 = smpIdx(m_app->getTextures().getLinearSampler());
			pc.cameraPosition					 = { context.camera->position.x, context.camera->position.y, context.camera->position.z, 0.0f };
			
			cmd->pushConstants(
				pipelineData.layout,
				VK_SHADER_STAGE_ALL_GRAPHICS,
				sizeof(GPU_DeferredLightingPushConstants),
				&pc
			);

			cmd->draw(3);

			// skybox
			{
				GraphicsPipelineDef skyboxPipeline;
				skyboxPipeline.setShader(m_app->getShaders().getShader("skybox"));
				skyboxPipeline.setVertexFormat(&vertex_types::PRIMITIVE_VERTEX_FORMAT);
				skyboxPipeline.setDepthTest(true);
				skyboxPipeline.setDepthWrite(false);
				skyboxPipeline.setDepthOp(VK_COMPARE_OP_LESS_OR_EQUAL);

				PipelineState pipelineData = m_app->getPipelines().fetchGraphicsPipeline(skyboxPipeline, info);

				glm::mat4 viewProj = context.camera->getProj() * context.camera->getRotationMatrix();

				cmd->bindPipeline(
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineData.pipeline
				);

				cmd->bindDescriptors(
					0,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineData.layout,
					{ m_skybox_descriptor },
					{}
				);

				cmd->pushConstants(
					pipelineData.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(glm::mat4),
					&viewProj
				);

				m_skyboxMesh->bind(cmd);

				cmd->drawIndexed(m_skyboxMesh->getIndexCount());
			}
		})
	);
}

void Renderer::tonemappingPass(float exposure)
{
	m_renderGraph->addTask(ComputeTaskDef()
		.setStorageViews({ stdView(m_gBuffer.lighting) })
		.setRecordFn([&, exposure](CommandBuffer *cmd) -> void
		{
			Shader *hdrTonemappingShader = m_app->getShaders().getShader("hdr_tonemapping");
	
			ComputePipelineDef hdrTonemappingPipeline;
			hdrTonemappingPipeline.setShader(hdrTonemappingShader);

			PipelineState pipelineState = m_app->getPipelines().fetchComputePipeline(hdrTonemappingPipeline);
			
			cmd->bindPipeline(
				VK_PIPELINE_BIND_POINT_COMPUTE,
				pipelineState.pipeline
			);

			cmd->bindDescriptors(
				0,
				VK_PIPELINE_BIND_POINT_COMPUTE,
				pipelineState.layout,
				{ m_hdrTonemapping_descriptor },
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

			cmd->pushConstants(
				pipelineState.layout,
				VK_SHADER_STAGE_COMPUTE_BIT,
				sizeof(pc),
				&pc
			);

			cmd->dispatch(pc.width, pc.height, 1);
		})
	);
}

void Renderer::createGBuffer()
{
	Swapchain *swapchain = m_app->getGraphics()->getSwapchain();

	m_gBuffer.position = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_gBuffer.albedo = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_gBuffer.normal = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_gBuffer.material = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_gBuffer.emissive = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_gBuffer.lighting = m_app->getGraphics()->createImage(
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

	m_gBuffer.depth = m_app->getGraphics()->createImage(
		swapchain->getWidth(),
		swapchain->getHeight(),
		1,
		m_app->getGraphics()->getDepthFormat(),
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);
}

void Renderer::createSkyboxResources()
{
	std::vector<PrimitiveVertex> vertices =
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

	m_skyboxMesh = new Mesh(m_app->getGraphics());

	m_skyboxMesh->build(
		&vertex_types::PRIMITIVE_VERTEX_FORMAT,
		vertices.data(), vertices.size(),
		indices.data(), indices.size()
	);
}

void Renderer::precomputeBRDF_LUT()
{
	const int BRDF_RESOLUTION = 512;

	m_brdfLUT = m_app->getGraphics()->createImage(
		BRDF_RESOLUTION, BRDF_RESOLUTION, 1,
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_TILING_OPTIMAL,
		1,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	MGP_LOG("Precomputing BRDF...");

	m_renderGraph->addPass(RenderPassDef()
		.setAttachments({ RenderGraphAttachment::getColour(VK_ATTACHMENT_LOAD_OP_LOAD, stdView(m_brdfLUT), nullptr, Colour::black()) })
		.setRecordFn([&](CommandBuffer *cmd, const RenderInfo &info) -> void
		{
			GraphicsPipelineDef brdfIntegrationPipeline;
			brdfIntegrationPipeline.setShader(m_app->getShaders().getShader("brdf_lut"));
			brdfIntegrationPipeline.setDepthTest(false);
			brdfIntegrationPipeline.setDepthWrite(false);

			PipelineState pipelineState = m_app->getPipelines().fetchGraphicsPipeline(brdfIntegrationPipeline, info);

			cmd->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineState.pipeline);
			cmd->draw(3);
		})
	);
}

void Renderer::generateEnvironmentMaps()
{
	const int ENVIRONMENT_MAP_RESOLUTION = 1024;
	const int IRRADIANCE_MAP_RESOLUTION = 32;

	const int PREFILTER_MAP_RESOLUTION = 128;
	const int PREFILTER_MAP_MIP_LEVELS = 5;

	glm::mat4 captureProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

	m_environmentMap = m_app->getGraphics()->createImage(
		ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION, 1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		VK_IMAGE_TILING_OPTIMAL,
		4,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_environmentProbe.irradiance = m_app->getGraphics()->createImage(
		IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION, 1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		VK_IMAGE_TILING_OPTIMAL,
		4,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);

	m_environmentProbe.prefilter = m_app->getGraphics()->createImage(
		PREFILTER_MAP_RESOLUTION, PREFILTER_MAP_RESOLUTION, 1,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		VK_IMAGE_TILING_OPTIMAL,
		PREFILTER_MAP_MIP_LEVELS,
		VK_SAMPLE_COUNT_1_BIT,
		false,
		false
	);
	
	Shader *eqrToCbmShader = m_app->getShaders().getShader("equirectangular_to_cubemap");
	Image *environmentHDRImage = m_app->getTextures().getTexture("environmentHDR");

	Descriptor *eqrToCbmSet = allocateDescriptor(eqrToCbmShader->getLayouts());
	eqrToCbmSet->writeCombinedImage(0, stdView(environmentHDRImage), m_app->getTextures().getLinearSampler());

	CommandBuffer *cmd = m_app->getGraphics()->beginInstantSubmit();
	{
		MGP_LOG("Generating environment map...");

		GraphicsPipelineDef equirectangularToCubemapPipeline;
		equirectangularToCubemapPipeline.setShader(eqrToCbmShader);
		equirectangularToCubemapPipeline.setVertexFormat(&vertex_types::PRIMITIVE_VERTEX_FORMAT);
		equirectangularToCubemapPipeline.setDepthTest(false);
		equirectangularToCubemapPipeline.setDepthWrite(false);

		cmd->transitionLayout(m_environmentMap, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int i = 0; i < 6; i++)
		{
			ImageView *view = m_app->getImageViews().fetchView(m_environmentMap, 1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view, nullptr);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd->beginRendering(targetInfo);
			{
				PipelineState pipelineSt = m_app->getPipelines().fetchGraphicsPipeline(equirectangularToCubemapPipeline, targetInfo);

				cmd->bindPipeline(
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineSt.pipeline
				);

				cmd->bindDescriptors(
					0,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineSt.layout,
					{ eqrToCbmSet },
					{}
				);

				cmd->setViewport({
					0, 0,
					ENVIRONMENT_MAP_RESOLUTION, ENVIRONMENT_MAP_RESOLUTION
				});

				glm::mat4 viewProj = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

				cmd->pushConstants(
					pipelineSt.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(glm::mat4),
					&viewProj
				);

				m_skyboxMesh->bind(cmd);

				cmd->drawIndexed(m_skyboxMesh->getIndexCount());
			}
			cmd->endRendering();
		}

		cmd->transitionLayout(m_environmentMap, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd->generateMipmaps(m_environmentMap);
	}
	m_app->getGraphics()->submit(cmd);

	struct
	{
		float roughness = 0.0f;
		float _padding0 = 0.0f;
		float _padding1 = 0.0f;
		float _padding2 = 0.0f;
	}
	prefilterParams;

	GPUBuffer *prefilterParameters = m_app->getGraphics()->createGPUBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		sizeof(prefilterParams) * PREFILTER_MAP_MIP_LEVELS
	);
	
	cmd = m_app->getGraphics()->beginInstantSubmit();
	{
		MGP_LOG("Generating irradiance map...");

		Shader *irradianceGenShader = m_app->getShaders().getShader("irradiance_convolution");

		Descriptor *irradianceSet = allocateDescriptor(irradianceGenShader->getLayouts());
		irradianceSet->writeCombinedImage(0, stdView(m_environmentMap), m_app->getTextures().getLinearSampler());

		GraphicsPipelineDef irradiancePipeline;
		irradiancePipeline.setShader(irradianceGenShader);
		irradiancePipeline.setVertexFormat(&vertex_types::PRIMITIVE_VERTEX_FORMAT);
		irradiancePipeline.setDepthTest(false);
		irradiancePipeline.setDepthWrite(false);

		cmd->transitionLayout(m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		
		for (int i = 0; i < 6; i++)
		{
			ImageView *view = m_app->getImageViews().fetchView(m_environmentProbe.irradiance, 1, i, 0);

			RenderInfo targetInfo;
			targetInfo.setSize(IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION);
			targetInfo.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view, nullptr);

			// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
			cmd->beginRendering(targetInfo);
			{
				PipelineState pipelineSt = m_app->getPipelines().fetchGraphicsPipeline(irradiancePipeline, targetInfo);

				cmd->bindPipeline(
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineSt.pipeline
				);

				cmd->bindDescriptors(
					0,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineSt.layout,
					{ irradianceSet },
					{}
				);

				cmd->setViewport({
					0, 0,
					IRRADIANCE_MAP_RESOLUTION, IRRADIANCE_MAP_RESOLUTION
				});

				glm::mat4 viewProj = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

				cmd->pushConstants(
					pipelineSt.layout,
					VK_SHADER_STAGE_ALL_GRAPHICS,
					sizeof(glm::mat4),
					&viewProj
				);

				m_skyboxMesh->bind(cmd);

				cmd->drawIndexed(m_skyboxMesh->getIndexCount());
			}
			cmd->endRendering();
		}

		cmd->transitionLayout(m_environmentProbe.irradiance, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		cmd->generateMipmaps(m_environmentProbe.irradiance);

		// ---

		MGP_LOG("Generating prefilter map...");

		Shader *prefilterShader = m_app->getShaders().getShader("prefilter_convolution");

		Descriptor *prefilterSet = allocateDescriptor(prefilterShader->getLayouts());
		prefilterSet->writeBufferRange(0, prefilterParameters, true, sizeof(prefilterParams));
		prefilterSet->writeCombinedImage(1, stdView(m_environmentMap), m_app->getTextures().getLinearSampler());

		GraphicsPipelineDef prefilterPipeline;
		prefilterPipeline.setShader(prefilterShader);
		prefilterPipeline.setVertexFormat(&vertex_types::PRIMITIVE_VERTEX_FORMAT);
		prefilterPipeline.setDepthTest(false);
		prefilterPipeline.setDepthWrite(false);

		cmd->transitionLayout(m_environmentProbe.prefilter, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		for (int mipLevel = 0; mipLevel < PREFILTER_MAP_MIP_LEVELS; mipLevel++)
		{
			int width = PREFILTER_MAP_RESOLUTION >> mipLevel;
			int height = PREFILTER_MAP_RESOLUTION >> mipLevel;

			float roughness = (float)mipLevel / (float)(PREFILTER_MAP_MIP_LEVELS - 1);

			prefilterParams.roughness = roughness;

			uint32_t dynamicOffset = sizeof(prefilterParams) * mipLevel;

			prefilterParameters->write(&prefilterParams, sizeof(prefilterParams), dynamicOffset);

			for (int i = 0; i < 6; i++)
			{
				ImageView *view = m_app->getImageViews().fetchView(m_environmentProbe.prefilter, 1, i, mipLevel);

				RenderInfo info;
				info.setSize(width, height);
				info.addColourAttachment(VK_ATTACHMENT_LOAD_OP_LOAD, view, nullptr);

				// todo: this is unoptimal: https://www.reddit.com/r/vulkan/comments/17rhrrc/question_about_rendering_to_cubemaps/
				cmd->beginRendering(info);
				{
					PipelineState pipelineSt = m_app->getPipelines().fetchGraphicsPipeline(prefilterPipeline, info);

					cmd->bindPipeline(
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineSt.pipeline
					);

					cmd->bindDescriptors(
						0,
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						pipelineSt.layout,
						{ prefilterSet },
						{ dynamicOffset }
					);

					cmd->setViewport({
						0, 0,
						(float)width, (float)height
					});

					glm::mat4 viewProj = captureProjectionMatrix * CUBEMAP_CAPTURE_VIEW_MATRICES[i];

					cmd->pushConstants(
						pipelineSt.layout,
						VK_SHADER_STAGE_ALL_GRAPHICS,
						sizeof(glm::mat4),
						&viewProj
					);

					m_skyboxMesh->bind(cmd);

					cmd->drawIndexed(m_skyboxMesh->getIndexCount());
				}
				cmd->endRendering();
			}
		}

		cmd->transitionLayout(m_environmentProbe.prefilter, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
	m_app->getGraphics()->submit(cmd);

	m_app->getGraphics()->waitIdle();
	delete prefilterParameters;
}
		
Material *Renderer::buildMaterial(const MaterialData &data)
{
	if (m_materials.contains(data.getHash()))
		return m_materials.at(data.getHash());

	cauto &technique = m_techniques.at(data.technique);

	std::array<GraphicsPipelineDef, SHADER_PASS_MAX_ENUM> passPipelines;

	for (int i = 0; i < SHADER_PASS_MAX_ENUM; i++)
	{
		if (!technique.passes[i])
			continue;

		passPipelines[i].setShader(technique.passes[i]);
		passPipelines[i].setVertexFormat(technique.vertexFormat);
	}

	GPU_BindlessMaterial handles = {};
	handles.diffuse_id		= data.textures[0].id;
	handles.ambient_id		= data.textures[1].id;
	handles.material_id		= data.textures[2].id;
	handles.normal_id		= data.textures[3].id;
	handles.emissive_id		= data.textures[4].id;

	uint32_t tableIndex = m_materialFreeIndex++;

	m_bindlessMaterialTable->writeType<GPU_BindlessMaterial>(handles, tableIndex);

	Material *material = new Material(tableIndex, data.textures, passPipelines, nullptr);

	m_materials.insert({ data.getHash(), material });

	return material;
}

void Renderer::loadTechniques()
{
	// PBR
	{
		Technique texturedPBR_gbuffer;
		texturedPBR_gbuffer.passes[SHADER_PASS_DEFERRED] = m_app->getShaders().getShader("texturedPBR_gbuffer");
		texturedPBR_gbuffer.passes[SHADER_PASS_FORWARD] = nullptr;
		texturedPBR_gbuffer.vertexFormat = &vertex_types::MODEL_VERTEX_FORMAT;
		addTechnique("texturedPBR_gbuffer_opaque", texturedPBR_gbuffer);
	}
}

void Renderer::addTechnique(const std::string &name, const Technique &technique)
{
	m_techniques.insert({ name, technique });
}

Descriptor *Renderer::allocateDescriptor(const std::vector<DescriptorLayout *> &layouts)
{
	return m_descriptorPool->allocate(layouts);
}

ImageView *Renderer::stdView(Image *image)
{
	return m_app->getImageViews().fetchStdView(image);
}

uint32_t Renderer::smpIdx(Sampler *sampler)
{
	return m_app->getBindlessResources()->fromSampler(sampler).id;
}

uint32_t Renderer::tex2DIdx(ImageView *view)
{
	return m_app->getBindlessResources()->fromTexture2D(view).id;
}

uint32_t Renderer::cbmIdx(ImageView *cubemap)
{
	return m_app->getBindlessResources()->fromCubemap(cubemap).id;
}
