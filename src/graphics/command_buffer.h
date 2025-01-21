#ifndef COMMAND_BUFFER_H_
#define COMMAND_BUFFER_H_

#include "../third_party/volk.h"

namespace llt
{
	class CommandBuffer
	{
	public:
		CommandBuffer();
		~CommandBuffer();

	private:
		VkCommandBuffer m_commandBuffer;
	};
}

#endif // COMMAND_BUFFER_H_
