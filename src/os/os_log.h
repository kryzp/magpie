#ifndef OS_LOG_H
#define OS_LOG_H

#ifndef LOG_COMPILE_MIN_FILTER
# define LOG_COMPILE_MIN_FILTER LOG_Level_Trace
#endif

typedef enum LOG_Level
{
	LOG_Level_Trace, // Granular Information (typically spammy)   (Arena allocations, Job's starting / stopping, Individual asset stages, ...)
	LOG_Level_Debug, // Sub-System Information                    (Swapchain created, Hot reloading assets, Shader compilation, ...)
	LOG_Level_Info,  // System Information                        (Graphics initialized, Assets shutdown, ...)
	LOG_Level_Warn,  // Warnings                                  (Asset not found - falling back, Channel pool full, ...)
	LOG_Level_Error, // Errors                                    (Shader compilation failed, Win32 object pool empty, Failed to open file, ...)
	LOG_Level_Break, // Fatal Break                               (Vulkan device lost, AppInit returned NULL, Render graph invalid, ...)
	LOG_Level_COUNT
}
LOG_Level;

typedef struct LOG_Channel LOG_Channel;
struct LOG_Channel
{
	u32 id;
};

internal inline b32
LOG_ChannelMatch(LOG_Channel a, LOG_Channel b)
{
	return a.id == b.id;
}

internal inline LOG_Channel
LOG_ChannelNull(void)
{
	LOG_Channel null_channel = {0};
	return null_channel;
}

#endif // OS_LOG_H
