#ifndef VK_SHADER_H_
#define VK_SHADER_H_

#include "../third_party/volk.h"

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
			void *data;
			uint64_t size;
			uint64_t offset;
		};

	public:
		// the packed data is really just a list of bytes so this helps legibility
		using PackedData = Vector<byte>;

		ShaderParameters() = default;
		~ShaderParameters() = default;

		/*
		* Returns the packed data.
		*/
		const PackedData &getPackedConstants();

		/*
		* Completely resets the data.
		*/
		void cleanUp()
		{
			for (auto& [name, param] : m_constants)
			{
				free(param.data);
			}

			m_constants.clear();
			m_packedConstants.clear();
			m_dirtyConstants = true;
			m_size = 0;
		}

		void setBuffer(const String &name, const void *data, uint64_t size)	{ _setBuffer(name, data, size, 0); }

		template <typename T>
		void setValue(const String &name, const T &value) { _setBuffer(name, &value, sizeof(T), 0); }

		template <typename T>
		void setArray(const String &name, const T *values, uint64_t num) { _setBuffer(name, values, sizeof(T) * num, 0); }

		template <typename T>
		void setArrayValue(const String &name, const T &value, uint64_t index) { _setBuffer(name, &value, sizeof(T), index); }

	private:
		void _setBuffer(const String &name, const void *data, uint64_t size, uint64_t dstOffset)
		{
			if (m_constants.contains(name))
			{
				mem::copy((char *)m_constants[name].data + dstOffset*size, data, size);
			}
			else
			{
				ShaderParameter p = {};
				p.size = size;
				p.offset = m_size;
				p.data = malloc(size);

				mem::copy((char *)p.data + dstOffset*size, data, size);

				m_constants.insert(name, p);
				m_size += size;
			}

			m_dirtyConstants = true;
		}

		void rebuildPackedConstantData();

		HashMap<String, ShaderParameter> m_constants;
		bool m_dirtyConstants = false;
		PackedData m_packedConstants;
		uint64_t m_size = 0;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		void cleanUp();

		void loadFromSource(const char *source, uint64_t size);

		VkShaderModule getModule() const;
		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits type;

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
