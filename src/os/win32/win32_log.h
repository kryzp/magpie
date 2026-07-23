#ifndef OS_WIN32_LOG_H
#define OS_WIN32_LOG_H

#define LOG_W32_MAX_CHANNELS        32
#define LOG_W32_LINE_BUFFER_SIZE    4096
#define LOG_W32_CHANNEL_COL_ALIGN   10

#define LOG_W32_ANSI_RESET    "\x1b[0m"
#define LOG_W32_ANSI_DIM      "\x1b[2m"

internal inline const char *LOG_W32_LevelToString(LOG_Level level)
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

internal inline const char *LOG_W32_LevelAnsi(LOG_Level level)
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

typedef struct LOG_W32_ChannelEntry LOG_W32_ChannelEntry;
struct LOG_W32_ChannelEntry
{
	String8 name;
	b32 enabled;
	LOG_Channel parent;
};

typedef struct LOG_W32_Logger LOG_W32_Logger;
struct LOG_W32_Logger
{
	Arena *arena;
	
	CH_Timer timer;
	
	OS_Handle mutex;
	
	OS_Handle file_stream;
	
	u32 channel_count;
	LOG_W32_ChannelEntry channels[LOG_W32_MAX_CHANNELS];

	LOG_Channel null_channel;
	LOG_Channel log_channel;

	// track last message and overwrite in-place if repeated.
	char               dedup_body[LOG_W32_LINE_BUFFER_SIZE];
	LOG_Channel        dedup_channel;
	LOG_Level          dedup_level;
	u32                dedup_count;
	b32                dedup_active;
	J_W32_Context      dedup_job_context;
};

internal void LOG_W32_Init(LOG_W32_Logger *logger, String8 sink);
internal void LOG_W32_Shutdown(LOG_W32_Logger *logger);

internal LOG_Channel LOG_W32_OpenChannel(LOG_W32_Logger *logger, String8 name);
internal LOG_Channel LOG_W32_OpenChannelFrom(LOG_W32_Logger *logger, LOG_Channel parent, String8 name);
internal void LOG_W32_CloseChannel(LOG_W32_Logger *logger, LOG_Channel channel);

internal void LOG_W32_MakeDedupBody(char *dst, i32 dst_size, const char *body, u32 count);
internal void LOG_W32_FlushDedupToFile(LOG_W32_Logger *logger, f32 elapsed);

internal void LOG_W32_ChannelNameResolve(LOG_W32_Logger *logger, LOG_Channel channel, char *dst, i32 dst_size);

internal i32 LOG_W32_FormatLine(LOG_W32_Logger *logger,
								   char *dst, i32 dst_size,
								   LOG_Level level, LOG_Channel channel,
								   const char *file, i32 line, const char *fn,
								   const char *body,
								   b32 for_file, f32 elapsed,
								   J_W32_Context job_context);

internal void LOG_W32_WriteV(LOG_W32_Logger *logger,
								J_W32_Context job_context,
								LOG_Level level, LOG_Channel channel,
								const char *file, i32 line, const char *fn,
								const char *fmt, va_list args);

#endif // OS_WIN32_LOG_H
