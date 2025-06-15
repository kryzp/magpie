#pragma once

#include <inttypes.h>

#include <string>
#include <vector>

#include "core/common.h"

#include "graphics/pipeline.h"

#include "bindless.h"

namespace mgp
{
	class VertexFormat;

	class GPUBuffer;
	class Shader;

	enum ShaderPassType
	{
		SHADER_PASS_DEFERRED,
		SHADER_PASS_FORWARD,
		SHADER_PASS_MAX_ENUM
	};

	struct Technique
	{
		VertexFormat *vertexFormat;
		Shader *passes[SHADER_PASS_MAX_ENUM];
	};

	struct MaterialData
	{
		std::string technique;
		std::vector<BindlessHandle> textures;

		void *parameters;
		uint64_t parameterSize;

		MaterialData()
			: technique("UNDEFINED")
			, textures()
			, parameters(nullptr)
			, parameterSize(0)
		{
		}

		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto &t : textures)
				hash::combine(&result, &t);

			hash::combine(&result, &technique);

			return result;
		}
	};

	class Material
	{
	public:
		Material(uint32_t tableIndex, const std::vector<BindlessHandle> &textures, const std::array<GraphicsPipelineDef, SHADER_PASS_MAX_ENUM> &pipelines, GPUBuffer *parameters)
			: m_textures(textures)
			, m_passes(pipelines)
			, m_parameterBuffer(parameters)
			, m_tableIndex(tableIndex)
		{
		}

		~Material() = default;
		
		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto &t : m_textures)
				hash::combine(&result, &t);

			for (auto &pass : m_passes)
				hash::combine(&result, &pass);

			return result;
		}

		const std::vector<BindlessHandle> &getTextures() const { return m_textures; }
		const BindlessHandle &getTexture(uint32_t index) const { return m_textures[index]; }

		const GPUBuffer *getParameterBuffer() const { return m_parameterBuffer; }
		const GraphicsPipelineDef &getPipeline(ShaderPassType pass) const { return m_passes[pass]; }

		uint32_t getTableIndex() const { return m_tableIndex; }

	private:
		std::vector<BindlessHandle> m_textures;
		std::array<GraphicsPipelineDef, SHADER_PASS_MAX_ENUM> m_passes;
		GPUBuffer *m_parameterBuffer;
		uint32_t m_tableIndex;
	};
}
