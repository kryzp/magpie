#include "gpu_particles.h"

#include "graphics/backend.h"
#include "graphics/shader_buffer_mgr.h"
#include "renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace llt;

struct Particle
{
	glm::vec3 pos;
	float _padding0;
	glm::vec3 vel;
	float _padding1;
};

GPUParticles::GPUParticles()
	: m_particleBuffer()
	, m_computeProgram(nullptr)
	, m_computeParams()
	, m_computeParamsBuffer()
	, m_particleMesh()
	, m_particleVertexFormat()
{
}

GPUParticles::~GPUParticles()
{
}

void GPUParticles::init()
{
	m_computeProgram = g_shaderManager->create("particleCompute", "../../res/shaders/compute/particles.spv", VK_SHADER_STAGE_COMPUTE_BIT);

	uint64_t particleDataSize = sizeof(Particle) * PARTICLE_COUNT;
	Particle* particleData = new Particle[PARTICLE_COUNT];

	for (int i = 0; i < PARTICLE_COUNT; i++)
	{
		particleData[i].pos.x = i;
		particleData[i].pos.y = 0.0f;
		particleData[i].pos.z = 0.0f;

		particleData[i].vel.x = 0.0f;
		particleData[i].vel.y = 0.0f;
		particleData[i].vel.z = 0.0f;
	}

	m_particleBuffer = g_shaderBufferManager->createSSBO();
	m_particleBuffer->pushData(particleData, particleDataSize);

	delete[] particleData;

	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, pos));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32_SFLOAT, offsetof(MyVertex, uv));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, col));
	m_particleVertexFormat.addAttribute(0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(MyVertex, norm));
	m_particleVertexFormat.addBinding(0, sizeof(MyVertex), VK_VERTEX_INPUT_RATE_VERTEX);

	m_particleVertexFormat.addAttribute(1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Particle, pos));
	m_particleVertexFormat.addBinding(1, sizeof(Particle), VK_VERTEX_INPUT_RATE_INSTANCE);

	Vector<MyVertex> particleVtx =
	{
		// front face
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },

		// right face
		{ {  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },

		// back face
		{ {  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },

		// left face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },

		// top face
		{ { -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ {  1.0f,  1.0f,  1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
		{ { -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },

		// bottom face
		{ { -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ {  1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
		{ { -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
	};

	Vector<uint16_t> particleIdx = {
		0, 1, 2,
		0, 2, 3,

		4, 5, 6,
		4, 6, 7,

		8, 9, 10,
		8, 10, 11,

		12, 13, 14,
		12, 14, 15,

		16, 17, 18,
		16, 18, 19,

		20, 21, 22,
		20, 22, 23
	};

	m_particleMesh.build(particleVtx.data(), particleVtx.size(), sizeof(MyVertex), particleIdx.data(), particleIdx.size());

	m_computeParams.set("viewProjMatrix", glm::identity<glm::mat4>());
	m_computeParamsBuffer = g_shaderBufferManager->createUBO();
	m_computeParamsBuffer->pushData(m_computeParams);
}

void GPUParticles::dispatchCompute(const Camera& camera)
{
	auto pixelSampler = g_textureManager->getSampler("nearest");
	auto gBuffer = g_renderTargetManager->get("gBuffer");

	g_vulkanBackend->beginCompute();

	m_computeParams.set("viewProjMatrix", camera.getProj() * camera.getView());
	m_computeParamsBuffer->pushData(m_computeParams);
	m_computeParamsBuffer->bind(0);

	m_particleBuffer->bind(1);

	g_vulkanBackend->setTexture(0, gBuffer->getAttachment(1), pixelSampler); // motion
	g_vulkanBackend->setTexture(0, gBuffer->getAttachment(2), pixelSampler); // normals
	g_vulkanBackend->setTexture(0, gBuffer->getDepthAttachment(), pixelSampler); // depth

	g_vulkanBackend->bindShader(m_computeProgram);

	g_vulkanBackend->dispatchCompute(1, 1, 1);

	g_vulkanBackend->endCompute();

	m_computeParamsBuffer->unbind();
	m_particleBuffer->unbind();
}

void GPUParticles::render()
{
	g_vulkanBackend->syncComputeWithNextRender();

	g_vulkanBackend->setVertexDescriptor(m_particleVertexFormat);

	auto vert = g_shaderManager->get("vertexInstanced");
	auto frag = g_shaderManager->get("fragment");

	g_vulkanBackend->bindShader(vert);
	g_vulkanBackend->bindShader(frag);

	RenderOp pass;
	pass.setInstanceData(PARTICLE_COUNT, 0, m_particleBuffer->getBuffer());
	pass.setMesh(m_particleMesh);

	g_vulkanBackend->render(pass);
}
