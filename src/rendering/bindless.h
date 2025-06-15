#pragma once

#include <inttypes.h>

#include <vector>
#include <unordered_map>

#include "math/calc.h"

#include "graphics/descriptor.h"

namespace mgp
{
	class Descriptor;
	class DescriptorLayout;
	class Sampler;
	class ImageView;
	class GraphicsCore;

	struct BindlessHandle
	{
		uint32_t id;
		BindlessHandle(uint32_t id) : id(id) { }
	};
	
	class BindlessResources
	{
		constexpr static uint32_t MAX_BUFFERS = 128;
		constexpr static uint32_t INVALID_HANDLE = CalcU::maxValue();

		constexpr static uint32_t SAMPLER_BINDING = 0;
		constexpr static uint32_t TEXTURE_2D_BINDING = 1;
		constexpr static uint32_t CUBEMAP_BINDING = 2;

	public:
		BindlessResources(GraphicsCore *gfx);
		~BindlessResources();
		
		BindlessHandle fromSampler(const Sampler *sampler);
		BindlessHandle fromTexture2D(const ImageView *view);
		BindlessHandle fromCubemap(const ImageView *view);

		Descriptor *getDescriptor();
		DescriptorLayout *getLayout();

	private:
		template <typename T>
		class BindlessFreeList
		{
		public:
			BindlessFreeList()
				: m_freeIndex(0)
				, m_freeIndices()
				, m_resourceToIndexMap()
			{
			}

			uint32_t registerResource(const T *t)
			{
				uint32_t index;

				if (!m_freeIndices.empty())
				{
					index = m_freeIndices.back();
					m_freeIndices.pop_back();
				}
				else
				{
					index = m_freeIndex;
					m_freeIndex++;
				}

				m_resourceToIndexMap[t] = index;

				return index;
			}

			void unregisterResource(const T *t)
			{
				uint32_t i = tryGetIndex(t);

				if (i != INVALID_HANDLE)
				{
					m_freeIndices.push_back(i);
					m_resourceToIndexMap.erase(t);
				}
			}

			uint32_t tryGetIndex(const T *t)
			{
				auto it = m_resourceToIndexMap.find(t);
				
				if (it != m_resourceToIndexMap.end())
					return it->second;

				return INVALID_HANDLE;
			}

		private:
			uint32_t m_freeIndex;
			std::vector<uint32_t> m_freeIndices;
			std::unordered_map<const T *, uint32_t> m_resourceToIndexMap;
		};

		uint32_t getMaxDescriptorSize(VkDescriptorType type);

		GraphicsCore *m_gfx;

		Descriptor *m_bindlessDesc;
		DescriptorLayout *m_bindlessLayout;
		DescriptorPoolStatic *m_bindlessPool;

		BindlessFreeList<Sampler> m_samplers;
		BindlessFreeList<ImageView> m_texture2Ds;
		BindlessFreeList<ImageView> m_cubemaps;
	};
}
