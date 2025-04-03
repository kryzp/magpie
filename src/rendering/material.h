#pragma once

#include <vector>
#include <string>

#include "vulkan/image_view.h"
#include "vulkan/vertex_format.h"
#include "vulkan/pipeline_cache.h"
#include "vulkan/bindless.h"

namespace mgp
{
	class Mesh;
	class Shader;
	class GPUBuffer;

	enum ShaderPassType
	{
		SHADER_PASS_FORWARD,
		SHADER_PASS_SHADOW,
		SHADER_PASS_MAX_ENUM
	};

	class Technique
	{
	public:
		Technique() = default;
		~Technique() = default;

		VertexFormat *vertexFormat;

		Shader *passes[SHADER_PASS_MAX_ENUM];

		// todo: default parameters
	};

	struct MaterialData
	{
		std::string technique;
		std::vector<bindless::Handle> textures;

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

	struct Material
	{
		Material() = default;
		~Material() = default;

		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto &t : textures)
				hash::combine(&result, &t);

			hash::combine(&result, &passes[SHADER_PASS_FORWARD]);
			hash::combine(&result, &passes[SHADER_PASS_SHADOW]);

			return result;
		}

		std::vector<bindless::Handle> textures;
		GPUBuffer *parameterBuffer;
		GraphicsPipelineDefinition passes[SHADER_PASS_MAX_ENUM];
		bindless::Handle bindlessHandle;
	};
}
