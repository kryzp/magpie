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

		void setBuffer(const String& name, const void* data, uint64_t size)	{ _setBuffer(name, data, size); }

		template <typename T>
		void setValue(const String& name, const T& value) { _setBuffer(name, &value, sizeof(T)); }

		template <typename T>
		void setArray(const String& name, const T* values, uint64_t num) { _setBuffer(name, values, sizeof(T) * num); }

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
