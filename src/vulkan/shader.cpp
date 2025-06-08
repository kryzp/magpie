#include "shader.h"

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>

#include "core/common.h"

#include "core.h"
#include "bindless.h"

using namespace mgp;

const char *VERTEX_ENTRY_POINT = "vertexMain";
const char *FRAGMENT_ENTRY_POINT = "fragmentMain";
const char *COMPUTE_ENTRY_POINT = "computeMain";

const char *getShaderStageName(VkShaderStageFlagBits stage)
{
	switch (stage)
	{
		case VK_SHADER_STAGE_VERTEX_BIT:
			return VERTEX_ENTRY_POINT;

		case VK_SHADER_STAGE_FRAGMENT_BIT:
			return FRAGMENT_ENTRY_POINT;

		case VK_SHADER_STAGE_COMPUTE_BIT:
			return COMPUTE_ENTRY_POINT;

		default:
			MGP_ERROR("Unsupported shader stage");
			break;
	}

	return nullptr;
}

ShaderStage::ShaderStage(VulkanCore *core)
	: m_stage()
	, m_module(VK_NULL_HANDLE)
//	, m_compiler(nullptr)
//	, m_resources()
	, m_core(core)
{
}

ShaderStage::ShaderStage(VulkanCore *core, VkShaderStageFlagBits stage, const char *path)
	: m_stage(stage)
	, m_module(VK_NULL_HANDLE)
//	, m_compiler(nullptr)
//	, m_resources()
	, m_core(core)
{
	compileFromSource(path);
}

ShaderStage::~ShaderStage()
{
	vkDestroyShaderModule(m_core->getLogicalDevice(), m_module, nullptr);
	m_module = VK_NULL_HANDLE;
}

VkPipelineShaderStageCreateInfo ShaderStage::getShaderStageCreateInfo() const
{
	VkPipelineShaderStageCreateInfo shaderStage = {};
	shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStage.stage = m_stage;
	shaderStage.module = m_module;
	shaderStage.pName = "main";

	return shaderStage;
}

void ShaderStage::compileFromSource(const char *path)
{
	Slang::ComPtr<slang::IModule> slangModule;
	{
		slangModule = m_core->getSlangSession()->loadModule(path);
		
		if (!slangModule)
			MGP_ERROR("Error loading shader module: %s", path);
	}

	Slang::ComPtr<slang::IEntryPoint> entryPoint;
	{
		slangModule->findEntryPointByName(getShaderStageName(m_stage), entryPoint.writeRef());
		
		if (!entryPoint)
			MGP_ERROR("Error getting entry point from shader: %s", path);
	}

	std::array<slang::IComponentType *, 2> componentTypes =
	{
		slangModule,
		entryPoint
	};

	Slang::ComPtr<slang::IComponentType> composedProgram;
	{
		Slang::ComPtr<slang::IBlob> diagnosticsBlob;

		SlangResult result = m_core->getSlangSession()->createCompositeComponentType(
			componentTypes.data(),
			componentTypes.size(),
			composedProgram.writeRef(),
			diagnosticsBlob.writeRef()
		);

		if (SLANG_FAILED(result))
			MGP_ERROR("Failed to compose module and entry point into shader program: %s", path);
	}

	Slang::ComPtr<slang::IComponentType> linkedProgram;
	{
		SlangResult result = composedProgram->link(linkedProgram.writeRef());
		
		if (SLANG_FAILED(result))
			MGP_ERROR("Failed to link composed shader program: %s", path);
	}

	Slang::ComPtr<slang::IBlob> spirvCode;
	{
		SlangResult result = linkedProgram->getEntryPointCode(
			0, // entryPointIndex
			0, // targetIndex
			spirvCode.writeRef()
		);
		
		if (SLANG_FAILED(result))
			MGP_ERROR("Failed to compile composed shader program into SPIR-V: %s", path);
	}

	VkShaderModuleCreateInfo moduleCreateInfo = {};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = spirvCode->getBufferSize();
	moduleCreateInfo.pCode = (const uint32_t *)spirvCode->getBufferPointer();

	MGP_VK_CHECK(
		vkCreateShaderModule(m_core->getLogicalDevice(), &moduleCreateInfo, nullptr, &m_module),
		"Failed to create shader module"
	);

//	m_compiler = new spirv_cross::Compiler(moduleCreateInfo.pCode, moduleCreateInfo.codeSize / sizeof(uint32_t));
//	m_resources = m_compiler->get_shader_resources();
}

Shader::Shader(VulkanCore *core)
	: m_core(core)
	, m_stages()
	, m_descriptorSetLayouts()
	, m_pushConstantsSize(0)
{
}

void Shader::addStage(ShaderStage *stage)
{
	m_stages.push_back(stage);
}

void Shader::finalize()
{
	/*
	DescriptorLayoutBuilder layoutBuilder;

	std::vector<VkDescriptorBindingFlags> flags;

	auto processModule = [&](const spirv_cross::Compiler &compiler, const spirv_cross::ShaderResources &resources, VkShaderStageFlagBits stage) -> void
	{
		auto processResources = [&](const spirv_cross::SmallVector<spirv_cross::Resource> &res, VkDescriptorType descriptorType) -> void
		{
			for (cauto &r : res)
			{
				uint32_t binding = compiler.get_decoration(r.id, spv::DecorationBinding);
				auto type = compiler.get_type(r.type_id);
				
				int descriptorCount = 1;
				VkDescriptorBindingFlags f = 0;

				if (!type.array.empty())
				{
					if (type.array[0] == 0)
					{
						// bindless array

						descriptorCount = m_core->getBindlessResources().getMaxDescriptorSize(descriptorType);

						f |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
					}
					else if (type.array_size_literal[0])
					{
						// fixed-size array

						descriptorCount = type.array[0];
					}
					else
					{
						// specialization constant

						MGP_ERROR("Resource %s has array size specialization constant, not currently handled.", compiler.get_name(r.id).c_str());
					}
				}

				layoutBuilder.bind(binding, descriptorType, descriptorCount);
				flags.push_back(f);
			}
		};

		processResources(resources.uniform_buffers,		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		processResources(resources.storage_buffers,		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
		processResources(resources.separate_samplers,	VK_DESCRIPTOR_TYPE_SAMPLER);
		processResources(resources.separate_images,		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
		processResources(resources.sampled_images,		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	};

	for (auto &stage : m_stages) {
		processModule(*stage->getCompiler(), stage->getResources(), stage->getType());
	}
	
	VkDescriptorSetLayout layout = layoutBuilder.build(m_core, VK_SHADER_STAGE_ALL_GRAPHICS, flags.data(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

	m_descriptorSetLayouts.push_back(layout);
	*/
}

const std::vector<ShaderStage *> &Shader::getStages() const
{
	return m_stages;
}

const ShaderStage *Shader::getStage(int idx) const
{
	return m_stages[idx];
}

const std::vector<VkDescriptorSetLayout> &Shader::getDescriptorSetLayouts() const
{
	return m_descriptorSetLayouts;
}

void Shader::setDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> &layouts)
{
	m_descriptorSetLayouts = layouts;
}

void Shader::setPushConstantsSize(uint64_t size)
{
	m_pushConstantsSize = size;
}

uint64_t Shader::getPushConstantsSize() const
{
	return m_pushConstantsSize;
}
