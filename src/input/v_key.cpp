#include "v_key.h"
#include "input.h"

using namespace llt;

bool VirtualKey::isDown() const
{
	return g_inputState->isDown(*this);
}

bool VirtualKey::isPressed() const
{
	return g_inputState->isPressed(*this);
}

bool VirtualKey::isReleased() const
{
	return g_inputState->isReleased(*this);
}
