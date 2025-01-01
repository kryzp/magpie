#ifndef VK_SHADER_H_
#define VK_SHADER_H_

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "../container/hash_map.h"
#include "../container/vector.h"
#include "../container/string.h"
#include "../container/pair.h"

namespace llt
{
	class VulkanBackend;

	class ShaderParameters
	{
		struct ShaderParameter
		{
			void* data;
			uint64_t size;
			uint64_t offset;
		};

	public:
		// the packed data is really just a list of bytes so this helps legibility
		using PackedData = Vector<byte>;

		ShaderParameters() = default;

		~ShaderParameters()
		{
			reset();
		}

		/*
		* Returns the packed data.
		*/
		const PackedData& getPackedConstants();

		/*
		* Completely resets the data.
		*/
		void reset()
		{
			for (auto& [name, param] : m_constants)
			{
				free(param.data);
			}

			m_constants.clear();
			m_packedConstants.clear();
			m_dirtyConstants = true;
			m_offsetAccumulator = 0;
		}

		/*
		* Functions for setting different variables in shaders
		*/
		void setInt8		(const String& name, int8_t val)						{ _setBuffer(name, &val, sizeof(int8_t)); }
		void setInt16		(const String& name, int16_t val)						{ _setBuffer(name, &val, sizeof(int16_t)); }
		void setInt32		(const String& name, int32_t val)						{ _setBuffer(name, &val, sizeof(int32_t)); }
		void setInt64		(const String& name, int64_t val)						{ _setBuffer(name, &val, sizeof(int64_t)); }
		void setUInt8		(const String& name, uint8_t val)						{ _setBuffer(name, &val, sizeof(uint8_t)); }
		void setUInt16		(const String& name, uint16_t val)						{ _setBuffer(name, &val, sizeof(uint16_t)); }
		void setUInt32		(const String& name, uint32_t val)						{ _setBuffer(name, &val, sizeof(uint32_t)); }
		void setUInt64		(const String& name, uint64_t val)						{ _setBuffer(name, &val, sizeof(uint64_t)); }
		void setFloat		(const String& name, float val)							{ _setBuffer(name, &val, sizeof(float)); }
		void setDouble		(const String& name, double val)						{ _setBuffer(name, &val, sizeof(double)); }
		void setBool		(const String& name, bool val)							{ _setBuffer(name, &val, sizeof(bool)); }
		void setVec2		(const String& name, const glm::vec2& val)				{ _setBuffer(name, &val, sizeof(float) * 2); }
		void setVec3		(const String& name, const glm::vec3& val)				{ _setBuffer(name, &val, sizeof(float) * 3); }
		void setVec4		(const String& name, const glm::vec4& val)				{ _setBuffer(name, &val, sizeof(float) * 4); }
		void setMat3		(const String& name, const glm::mat3& val)				{ _setBuffer(name, &val, sizeof(float) * 3 * 3); }
		void setMat4		(const String& name, const glm::mat4& val)				{ _setBuffer(name, &val, sizeof(float) * 4 * 4); }
		void setFloatArr	(const String& name, const float* vals, int n)			{ _setBuffer(name, vals, sizeof(float) * n); }
		void setVec3Arr		(const String& name, const glm::vec3* vals, int n)		{ _setBuffer(name, vals, sizeof(float) * 3 * n); }
		void setBuffer		(const String& name, const void* data, uint64_t size)	{ _setBuffer(name, data, size); }

	private:
		void _setBuffer(const String& name, const void* data, uint64_t size)
		{
			if (m_constants.contains(name))
			{
				// we've already cached this parameter, we just have to update it
				mem::copy(m_constants[name].data, data, size);
			}
			else
			{
				// we haven't yet cached this parameter, add it to our list
				ShaderParameter p = {};
				p.size = size;
				p.offset = m_offsetAccumulator;
				p.data = malloc(size);

				mem::copy(p.data, data, size);

				m_constants.insert(name, p);
				m_offsetAccumulator += size;
			}

			// we've become dirty and will have to rebuild next time we ask for the data
			m_dirtyConstants = true;
		}

		void rebuildPackedConstantData();

		HashMap<String, ShaderParameter> m_constants;
		bool m_dirtyConstants;
		PackedData m_packedConstants;
		uint64_t m_offsetAccumulator = 0;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		void cleanUp();

		void loadFromSource(const char* source, uint64_t size);

		VkShaderModule getModule() const;
		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits type;
		ShaderParameters params;

	private:
		VkShaderModule m_shaderModule;
	};

	class ShaderEffect
	{
	public:
		ShaderEffect() = default;
		~ShaderEffect() = default;

		Vector<ShaderProgram*> stages;
	};
}

#endif // VK_SHADER_H_
