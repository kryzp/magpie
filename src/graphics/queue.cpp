#include "queue.h"
#include "backend.h"

using namespace llt;

Queue::Queue()
	: m_queue()
	, m_family(QUEUE_FAMILY_MAX_ENUM)
	, m_familyIdx(0)
	, m_frames()
{
}

Queue::~Queue()
{
}

void Queue::init(VkQueue queue)
{
	m_queue = queue;
}

void Queue::setData(QueueFamily family, uint32_t familyIdx)
{
	m_family = family;
	m_familyIdx = familyIdx;
}

Queue::FrameData& Queue::getCurrentFrame()
{
	return m_frames[g_vulkanBackend->getCurrentFrameIdx()];
}

Queue::FrameData& Queue::getFrame(int idx)
{
	return m_frames[idx];
}

VkQueue Queue::getQueue() const
{
	return m_queue;
}

QueueFamily Queue::getFamily() const
{
	return m_family;
}

Optional<uint32_t> Queue::getFamilyIdx() const
{
	return m_familyIdx;
}
