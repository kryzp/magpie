#include "shader_buffer_mgr.h"
#include "../common.h"

llt::ShaderBufferMgr* llt::g_shaderBufferManager = nullptr;

using namespace llt;

ShaderBufferMgr::ShaderBufferMgr()
	: m_buffers()
{
}

ShaderBufferMgr::~ShaderBufferMgr()
{
	for (auto& buf : m_buffers) {
		buf->cleanUp();
	}
}

void ShaderBufferMgr::unbindAll()
{
	for (auto& buf : m_buffers) {
		buf->unbind();
	}
}

void ShaderBufferMgr::resetBufferUsageInFrame()
{
	for (auto& buf : m_buffers) {
		buf->resetBufferUsageInFrame();
	}
}

void ShaderBufferMgr::bindToDescriptorBuilder(DescriptorBuilder* builder, VkShaderStageFlagBits stage)
{
	for (auto& buf : m_buffers)
	{
		if (buf->isBound())
		{
			builder->bindBuffer(
				buf->getBoundIdx(),
				&buf->getDescriptor(),
				buf->getDescriptorType(),
				stage
			);
		}
	}
}

#include <algorithm>

Vector<uint32_t> ShaderBufferMgr::getDynamicOffsets()
{
	// dynamic offsets must follow binding order!!

	Vector<Pair<uint32_t, uint32_t>> boundOffsets;

	for (int i = 0; i < m_buffers.size(); i++)
	{
		auto& buf = m_buffers[i];

		if (buf->isBound())
		{
			boundOffsets.pushBack(Pair<uint32_t, uint32_t>(buf->getBoundIdx(), buf->getDynamicOffset()));
		}
	}

	sortBoundOffsets(boundOffsets, 0, boundOffsets.size() - 1);

	// ---

	Vector<uint32_t> result;

	for (int i = 0; i < boundOffsets.size(); i++)
	{
		result.pushBack(boundOffsets[i].second);
	}

	return result;
}

// quicksort
void ShaderBufferMgr::sortBoundOffsets(Vector<Pair<uint32_t, uint32_t>>& offsets, int lo, int hi)
{
	if (lo >= hi || lo < 0) {
		return;
	}

	int pivot = offsets[hi].first;
	int i = lo - 1;

	for (int j = lo; j < hi; j++)
	{
		if (offsets[j].first <= pivot)
		{
			i++;
			
			Pair<uint32_t, uint32_t> tmp = offsets[i];
			offsets[i] = offsets[j];
			offsets[j] = tmp;
		}
	}

	Pair<uint32_t, uint32_t> tmp = offsets[i + 1];
	offsets[i + 1] = offsets[hi];
	offsets[hi] = tmp;

	int partition = i + 1;

	sortBoundOffsets(offsets, lo, partition - 1);
	sortBoundOffsets(offsets, partition + 1, hi);
}

ShaderBuffer* ShaderBufferMgr::createUBO()
{
	ShaderBuffer* ubo = new ShaderBuffer();
	ubo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_UBO);
	m_buffers.pushBack(ubo);
	return m_buffers.back();
}

ShaderBuffer* ShaderBufferMgr::createSSBO()
{
	ShaderBuffer* ssbo = new ShaderBuffer();
	ssbo->init(INIT_BUFFER_SIZE, SHADER_BUFFER_SSBO);
	m_buffers.pushBack(ssbo);
	return m_buffers.back();
}
