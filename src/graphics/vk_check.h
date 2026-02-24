#pragma once

#define GFX_VK_CHECK(fn, msg) \
	do { \
		VkResult _gfx_vk_check_result = (fn); \
		if (_gfx_vk_check_result != VK_SUCCESS) \
			debug_log_crash(msg " (%d)", _gfx_vk_check_result); \
	} while (0)
