#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "../common.h"

#include "../container/array.h"
#include "../container/string.h"

#include "technique.h"
#include "texture.h"
#include "shader.h"
#include "shader_buffer.h"
#include "graphics_pipeline.h"

namespace llt
{
	struct BoundTexture
	{
		Texture* texture;
		TextureSampler* sampler;
	};

	struct MaterialData
	{
		Vector<BoundTexture> textures;
		String technique;
		ShaderParameters parameters;

		bool depthTest = true;
		bool depthWrite = true;

		uint64_t getHash() const
		{
			uint64_t result = 0;

			for (auto& t : textures) {
				hash::combine(&result, &t);
			}

			hash::combine(&result, &technique);
			hash::combine(&result, &depthTest);
			hash::combine(&result, &depthWrite);

			return result;
		}
	};

	class Material
	{
	public:
		Material();
		virtual ~Material();

		uint64_t getHash() const;

		Vector<BoundTexture> textures;
		ShaderBuffer* parameterBuffer;
		GraphicsPipeline pipeline;
	};
}

#endif // MATERIAL_H_
