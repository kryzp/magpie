#ifndef TECHNIQUE_H_
#define TECHNIQUE_H_

#include "shader.h"

namespace llt
{
	/**
	 * Different passes for selecting the type of shader to use during
	 * a stage of the rendering process.
	 */
	enum ShaderPass
	{
		SHADER_PASS_FORWARD,
		SHADER_PASS_SHADOW,
		SHADER_PASS_MAX_ENUM
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

		void setPass(ShaderPass pass, ShaderEffect* effect)
		{
			m_passes[pass] = effect;
		}

		ShaderEffect* getPass(ShaderPass pass) const
		{
			return m_passes[pass];
		}

	private:
		ShaderEffect* m_passes[SHADER_PASS_MAX_ENUM];
	};
}

#endif // TECHNIQUE_H_
