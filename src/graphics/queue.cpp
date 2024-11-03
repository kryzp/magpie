#include "queue.h"

using namespace llt;

Queue::Queue()
	: m_queue()
	, m_family(QUEUE_FAMILY_NONE)
	, m_familyIdx(0)
	, m_frames()
	, m_currentFrameIdx(0)
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

void Queue::nextFrame()
{
	m_currentFrameIdx = (m_currentFrameIdx + 1) % mgc::FRAMES_IN_FLIGHT;
}

Queue::FrameData& Queue::getCurrentFrame()
{
	return m_frames[m_currentFrameIdx];
}

Queue::FrameData& Queue::getFrame(int idx)
{
	return m_frames[idx];
}

int Queue::getCurrentFrameIdx() const
{
	return m_currentFrameIdx;
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
