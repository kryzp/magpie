#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "core/common.h"

#include "container/array.h"
#include "container/string.h"

#include "vulkan/texture.h"
#include "vulkan/shader.h"
#include "vulkan/shader_buffer.h"
#include "vulkan/pipeline.h"

namespace llt
{
	class SubMesh;

	enum ShaderPassType
	{
		SHADER_PASS_FORWARD,
		SHADER_PASS_SHADOW,
		SHADER_PASS_MAX_ENUM
	};

	struct ShaderPass
	{
		ShaderEffect *shader;
		VkDescriptorSet set;
		GraphicsPipelineDefinition pipeline;

		ShaderPass()
			: shader(nullptr)
			, set()
			, pipeline()
		{
		}
	};

	class Technique
	{
	public:
		Technique() = default;
		~Technique() = default;

		VertexFormat vertexFormat;

		ShaderEffect *passes[SHADER_PASS_MAX_ENUM];
		// todo: default parameters

		bool depthTest = true;
		bool depthWrite = true;
	};

	struct MaterialData
	{
		Vector<BoundTexture> textures;
		String technique;

		void *parameters;
		uint64_t parameterSize;

		MaterialData()
			: textures()
			, technique("UNDEFINED")
			, parameters(nullptr)
			, parameterSize(0)
		{
		}

		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto &t : textures) {
				hash::combine(&result, &t);
			}

			hash::combine(&result, &technique);

			return result;
		}
	};

	class Material
	{
	public:
		Material() = default;
		~Material() = default;

		uint64_t getHash() const;

		GraphicsPipelineDefinition &getPipelineDef(ShaderPassType pass);

		void bindDescriptorSets(CommandBuffer &cmd, ShaderPassType pass, VkPipelineLayout layout);

		VertexFormat m_vertexFormat;
		Vector<BoundTexture> m_textures;
		DynamicShaderBuffer *m_parameterBuffer;
		ShaderPass m_passes[SHADER_PASS_MAX_ENUM];
	};
}

#endif // MATERIAL_H_
