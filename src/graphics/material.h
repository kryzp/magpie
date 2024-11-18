#ifndef MATERIAL_H_
#define MATERIAL_H_

#include "../common.h"

#include "../container/array.h"
#include "../container/string.h"

#include "technique.h"
#include "texture.h"
#include "shader.h"

namespace llt
{
	/**
	 * Compact representation of a material.
	 */
	struct MaterialData
	{
		Array<SampledTexture, mgc::MAX_BOUND_TEXTURES> textures;
		ShaderParameters parameters;
		String technique;
	};

	/**
	 * Material that packages together the required shaders, textures, and other
	 * information required for drawing a material.
	 */
	class Material
	{
	public:
		Material();
		virtual ~Material();

		uint64_t hash() const;

		void setTexture(int idx, const Texture* texture, TextureSampler* sampler);

		Technique technique;
		ShaderParameters parameters;
		Array<SampledTexture, mgc::MAX_BOUND_TEXTURES> textures;
	};
}

#endif // MATERIAL_H_
