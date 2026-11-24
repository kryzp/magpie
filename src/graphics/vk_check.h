#ifndef GRAPHICS_VK_CHECK_H
#define GRAPHICS_VK_CHECK_H

#include "core/log.h"

#include <volk/volk.h>

// TODO: add a channel input so that logs are clearer
#define GRAPHICS_VK_CHECK(fn, ...) DebugPrintAssert((fn) == VK_SUCCESS, __VA_ARGS__)

#endif // GRAPHICS_VK_CHECK_H
