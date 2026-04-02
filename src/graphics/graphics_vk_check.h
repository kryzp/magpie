#ifndef GRAPHICS_VK_CHECK
#define GRAPHICS_VK_CHECK

#define GFX_VK_CHECK(fn, msg)					\
	do											\
	{											\
		VkResult gfx_vk_check_result = (fn);	\
		if (gfx_vk_check_result != VK_SUCCESS)	\
			AssertTrue(false);					\
	} while (false);

#endif // GRAPHICS_VK_CHECK
