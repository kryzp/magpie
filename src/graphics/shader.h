#ifndef VK_SHADER_H_
#define VK_SHADER_H_

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "../container/vector.h"
#include "../container/string.h"
#include "../container/pair.h"

namespace llt
{
	class VulkanBackend;

	/**
	* Represents the possible parameters that can be passed into a shader.
	*/
	struct ShaderParameter
	{
		static constexpr uint64_t MAX_PARAMETER_SIZE = sizeof(float) * 64;

		/*
		* Different parameter types
		*/
		enum ParameterType
		{
			PARAM_TYPE_NONE,

			PARAM_TYPE_S8,
			PARAM_TYPE_S16,
			PARAM_TYPE_S32,
			PARAM_TYPE_S64,

			PARAM_TYPE_U8,
			PARAM_TYPE_U16,
			PARAM_TYPE_U32,
			PARAM_TYPE_U64,

			PARAM_TYPE_F32,
			PARAM_TYPE_F64,
			PARAM_TYPE_BOOL,

			PARAM_TYPE_VEC2F,
			PARAM_TYPE_VEC3F,
			PARAM_TYPE_VEC4F,

			PARAM_TYPE_MAT4X4F,

			PARAM_TYPE_MAX_ENUM
		};

		ParameterType type;
		byte data[MAX_PARAMETER_SIZE];

		/*
		* Each parameter has an alignment offset set by the vulkan standard
		*/
		constexpr uint64_t alignment_offset() const
		{
			switch (type)
			{
				case PARAM_TYPE_S8:			return 4;
				case PARAM_TYPE_S16:		return 4;
				case PARAM_TYPE_S32:		return 8;
				case PARAM_TYPE_S64:		return 16;

				case PARAM_TYPE_U8:			return 4;
				case PARAM_TYPE_U16:		return 4;
				case PARAM_TYPE_U32:		return 8;
				case PARAM_TYPE_U64:		return 16;

				case PARAM_TYPE_F32:		return 4;
				case PARAM_TYPE_F64:		return 8;

				case PARAM_TYPE_BOOL:		return 4;

				case PARAM_TYPE_VEC2F:		return 8;
				case PARAM_TYPE_VEC3F:		return 16;
				case PARAM_TYPE_VEC4F:		return 16;

				case PARAM_TYPE_MAT4X4F:	return 64;

				default: return 0;
			}
		}

		/*
		* Get the actual memory size of each parameter.
		*/
		constexpr uint64_t size() const
		{
			switch (type)
			{
				case PARAM_TYPE_S8:			return sizeof(int8_t);
				case PARAM_TYPE_S16:		return sizeof(int16_t);
				case PARAM_TYPE_S32:		return sizeof(int32_t);
				case PARAM_TYPE_S64:		return sizeof(int64_t);

				case PARAM_TYPE_U8:			return sizeof(uint8_t);
				case PARAM_TYPE_U16:		return sizeof(uint16_t);
				case PARAM_TYPE_U32:		return sizeof(uint32_t);
				case PARAM_TYPE_U64:		return sizeof(uint64_t);

				case PARAM_TYPE_F32:		return sizeof(float);
				case PARAM_TYPE_F64:		return sizeof(double);

				case PARAM_TYPE_BOOL:		return sizeof(bool);

				case PARAM_TYPE_VEC2F:		return sizeof(float) * 2;
				case PARAM_TYPE_VEC3F:		return sizeof(float) * 3;
				case PARAM_TYPE_VEC4F:		return sizeof(float) * 4;

				case PARAM_TYPE_MAT4X4F:	return sizeof(float) * 16;

				default: return 0;
			}
		}
	};

	/**
	* Represents a list of a shader program parameters
	*/
	class ShaderParameters
	{
	public:
		// the packed data is really just a list of bytes so this helps legibility
		using PackedData = Vector<byte>;

		ShaderParameters() = default;
		~ShaderParameters() = default;

		/*
		* Returns the packed data.
		*/
		const PackedData& getPackedConstants();

		/*
		* Completely resets the data.
		*/
		void reset()
		{
			m_constants.clear();
			m_packedConstants.clear();
			m_dirtyConstants = true;
		}

		/*
		* Functions for setting different variables in shaders
		*/
		void set(const String& name, int8_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_S8); }
		void set(const String& name, int16_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_S16); }
		void set(const String& name, int32_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_S32); }
		void set(const String& name, int64_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_S64); }
		void set(const String& name, uint8_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_U8); }
		void set(const String& name, uint16_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_U16); }
		void set(const String& name, uint32_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_U32); }
		void set(const String& name, uint64_t val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_U64); }
		void set(const String& name, float val)							{ _set(name, val, ShaderParameter::PARAM_TYPE_F32); }
		void set(const String& name, double val)						{ _set(name, val, ShaderParameter::PARAM_TYPE_F64); }
		void set(const String& name, bool val)							{ _set(name, val, ShaderParameter::PARAM_TYPE_BOOL); }
		void set(const String& name, const glm::vec2& val)				{ _set(name, val, ShaderParameter::PARAM_TYPE_VEC2F); }
		void set(const String& name, const glm::vec3& val)				{ _set(name, val, ShaderParameter::PARAM_TYPE_VEC3F); }
		void set(const String& name, const glm::vec4& val)				{ _set(name, val, ShaderParameter::PARAM_TYPE_VEC4F); }
		void set(const String& name, const glm::mat4& val)				{ _set(name, val, ShaderParameter::PARAM_TYPE_MAT4X4F); }
		void set(const String& name, const float* vals, int n)			{ _setArray(name, vals, n, ShaderParameter::PARAM_TYPE_F32); }
		void set(const String& name, const glm::vec3* vals, int n)		{ _setArray(name, vals, n, ShaderParameter::PARAM_TYPE_VEC3F); }

	private:
		template <typename T>
		void _setArray(const String& name, const T* vals, int n, ShaderParameter::ParameterType type)
		{
			// search for the parameter in our list
			int i = 0;
			for (; i <= m_constants.size(); i++) {
				if (i == m_constants.size()) {
					break;
				}
				if (m_constants[i].first == name) {
					break;
				}
			}

			// we've already cached this parameter, we just have to update it
			if (i < m_constants.size()) {
				for (int j = 0; j < n; j++) {
					mem::copy(m_constants[i].second.data, vals + j, sizeof(T));
				}
			}

			// we haven't yet cached this parameter, add it to our list
			else {
				for (int j = 0; j < n; j++) {
					ShaderParameter p = {};
					p.type = type;
					mem::copy(p.data, vals + j, sizeof(T));
					m_constants.pushBack(Pair(name, p));
				}
			}

			// we've become dirty and will have to rebuild next time we ask for the data
			m_dirtyConstants = true;
		}

		template <typename T>
		void _set(const String& name, const T& val, ShaderParameter::ParameterType type)
		{
			// search for the parameter in our list
			int i = 0;
			for (; i <= m_constants.size(); i++) {
				if (i == m_constants.size()) {
					break;
				}
				if (m_constants[i].first == name) {
					break;
				}
			}

			// we've already cached this parameter, we just have to update it
			if (i < m_constants.size()) {
				mem::copy(m_constants[i].second.data, &val, sizeof(T));
			}

			// we haven't yet cached this parameter, add it to our list
			else {
				ShaderParameter p = {};
				p.type = type;
				mem::copy(p.data, &val, sizeof(T));
				m_constants.pushBack(Pair(name, p));
			}

			// we've become dirty and will have to rebuild next time we ask for the data
			m_dirtyConstants = true;
		}

		/*
		* Rebuilds the constant data and packs it up.
		*/
		void rebuildPackedConstantData();

		Vector<Pair<String, ShaderParameter>> m_constants; // the reason we don't use a hashmap here is because we need to PRESERVE THE ORDER of the elements! the hashmap is inherently unordered.
		bool m_dirtyConstants;
		PackedData m_packedConstants;
	};

	class ShaderProgram
	{
	public:
		ShaderProgram();
		~ShaderProgram();

		void cleanUp();

		void loadFromSource(const char* source, uint64_t source_size);

		VkShaderModule getModule() const;
		VkPipelineShaderStageCreateInfo getShaderStageCreateInfo() const;

		VkShaderStageFlagBits type;
		ShaderParameters params;

	private:
		VkShaderModule m_shaderModule;
	};

	/**
	* Describes a collection of GPU shader programs, making up a full "effect", essentially
	* the pipeline of vertex -> ... -> fragment, all in stages.
	*/
	/*
	class ShaderEffect
	{
	public:
		ShaderEffect() = default;
		~ShaderEffect() = default;

		void addStage(ShaderProgram* shader)
		{
			stages.pushBack(shader);
		}

		Vector<ShaderProgram*> stages;
	};
	*/
}

#endif // VK_SHADER_H_
