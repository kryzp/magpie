#ifndef LOG_H
#define LOG_H

#define LOG_MAX_CHANNELS        32
#define LOG_LINE_BUFFER_SIZE    4096
#define LOG_CHANNEL_COL_ALIGN   8

#define LOG_ANSI_RESET    "\x1b[0m"
#define LOG_ANSI_DIM      "\x1b[2m"

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

internal inline const char *
LOG_LevelToString(LOG_Level level)
{
	switch (level)
	{
		case LOG_Level_Trace:  return "TRACE";
		case LOG_Level_Debug:  return "DEBUG";
		case LOG_Level_Info:   return "INFO ";
		case LOG_Level_Warn:   return "WARN ";
		case LOG_Level_Error:  return "ERROR";
		case LOG_Level_Break:  return "BREAK";
	}

	return "?????";
}

internal inline const char *
LOG_LevelAnsi(LOG_Level level)
{
	switch (level)
	{
		case LOG_Level_Trace:  return "\x1b[90m";
		case LOG_Level_Debug:  return "\x1b[36m";
		case LOG_Level_Info:   return "\x1b[32m";
		case LOG_Level_Warn:   return "\x1b[33m";
		case LOG_Level_Error:  return "\x1b[31m";
		case LOG_Level_Break:  return "\x1b[41;30m";
	}

	return "";
}

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

typedef struct LOG_ChannelEntry LOG_ChannelEntry;
struct LOG_ChannelEntry
{
	String8 name;
	b32 enabled;
};

typedef struct LOG_Logger LOG_Logger;
struct LOG_Logger
{
	Arena *arena;
	
	CH_Timer timer;
	
	OS_Handle mutex;
	
	OS_Handle file_stream;
	
	u32 channel_count;
	LOG_ChannelEntry channels[LOG_MAX_CHANNELS];

	LOG_Channel log_channel;

	// track last message and overwrite in-place if repeated.
	char        dedup_body[LOG_LINE_BUFFER_SIZE];
	LOG_Channel dedup_channel;
	LOG_Level   dedup_level;
	u32         dedup_count;
	b32         dedup_active;
	f32         dedup_start_elapsed;
	JOB_Context dedup_job_context;
};

internal void LOG_InitAndSelect(LOG_Logger *logger, String8 sink);
internal void LOG_Shutdown(void);
internal void LOG_HotLoad(LOG_Logger *logger);
internal void LOG_HotUnload(void);

internal LOG_Channel LOG_OpenChannel(String8 name);
internal void        LOG_CloseChannel(LOG_Channel channel);

internal void LOG_MakeDedupBody    (char *dst, i32 dst_size, const char *body, u32 count);
internal void LOG_FlushDedupToFile (f32 elapsed);

internal i32 LOG_FormatLine(char *dst, i32 dst_size,
							LOG_Level level, LOG_Channel channel,
							const char *file, i32 line, const char *fn,
							const char *body,
							b32 for_file, f32 elapsed,
							JOB_Context job_context);

internal void LOG_Write  (LOG_Level level, LOG_Channel channel, const char *file, i32 line, const char *fn, const char *fmt, ...);
internal void LOG_WriteV (LOG_Level level, LOG_Channel channel, const char *file, i32 line, const char *fn, const char *fmt, va_list args);

#define DebugLogEx(level, channel_id, ...) LOG_Write((level), (channel_id), __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DebugLogT(channel_id, ...) DebugLogEx(LOG_Level_Trace, (channel_id), __VA_ARGS__)
#define DebugLogD(channel_id, ...) DebugLogEx(LOG_Level_Debug, (channel_id), __VA_ARGS__)
#define DebugLogI(channel_id, ...) DebugLogEx(LOG_Level_Info,  (channel_id), __VA_ARGS__)
#define DebugLogW(channel_id, ...) DebugLogEx(LOG_Level_Warn,  (channel_id), __VA_ARGS__)
#define DebugLogE(channel_id, ...) DebugLogEx(LOG_Level_Error, (channel_id), __VA_ARGS__)
#define DebugLogB(channel_id, ...) DebugLogEx(LOG_Level_Break, (channel_id), __VA_ARGS__)

#endif // LOG_H
