#ifndef GRAPHICS_VK_CHECK_H
#define GRAPHICS_VK_CHECK_H

// TODO: add a channel input so that logs are clearer
#define G_VK_CHECK(fn, ...) DebugPrintAssert((fn) == VK_SUCCESS, __VA_ARGS__)

#endif // GRAPHICS_VK_CHECK_H
