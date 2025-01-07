#ifndef TECHNIQUE_H_
#define TECHNIQUE_H_

#include "shader.h"
#include "vertex_descriptor.h"
#include "graphics_pipeline.h"

namespace llt
{
	/**
	 * Different passes for selecting the type of shader to use during
	 * a stage of the rendering process.
	 */
	enum ShaderPassType
	{
		SHADER_PASS_FORWARD,
		SHADER_PASS_SHADOW,
		SHADER_PASS_MAX_ENUM
	};

	struct ShaderPass
	{
		ShaderEffect* effect;
		VkDescriptorSet set;
		GraphicsPipeline pipeline;
	};

	/**
	 * Represents how you should go about rendering a particular material, by
	 * packaging together all the different possible shader passes in the pipeline.
	 */
	class Technique
	{
	public:
		Technique() = default;
		~Technique() = default;

		VertexDescriptor vertexFormat;
		ShaderEffect* passes[SHADER_PASS_MAX_ENUM];
	};
}

#endif // TECHNIQUE_H_
