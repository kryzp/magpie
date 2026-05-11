#ifndef OS_WIN32_LOG_H
#define OS_WIN32_LOG_H

#define OS_W32_LOG_MAX_CHANNELS        32
#define OS_W32_LOG_LINE_BUFFER_SIZE    4096
#define OS_W32_LOG_CHANNEL_COL_ALIGN   10

#define OS_W32_LOG_ANSI_RESET    "\x1b[0m"
#define OS_W32_LOG_ANSI_DIM      "\x1b[2m"

internal inline const char *
OS_W32_LOG_LevelToString(LOG_Level level)
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
OS_W32_LOG_LevelAnsi(LOG_Level level)
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

typedef struct OS_W32_LOG_ChannelEntry OS_W32_LOG_ChannelEntry;
struct OS_W32_LOG_ChannelEntry
{
	String8 name;
	b32 enabled;
};

typedef struct OS_W32_LOG_Logger OS_W32_LOG_Logger;
struct OS_W32_LOG_Logger
{
	Arena *arena;
	
	CH_Timer timer;
	
	OS_Handle mutex;
	
	OS_Handle file_stream;
	
	u32 channel_count;
	OS_W32_LOG_ChannelEntry channels[OS_W32_LOG_MAX_CHANNELS];

	LOG_Channel null_channel;
	LOG_Channel log_channel;

	// track last message and overwrite in-place if repeated.
	char               dedup_body[OS_W32_LOG_LINE_BUFFER_SIZE];
	LOG_Channel        dedup_channel;
	LOG_Level          dedup_level;
	u32                dedup_count;
	b32                dedup_active;
	OS_W32_JOB_Context dedup_job_context;
};

internal void OS_W32_LOG_Init     (OS_W32_LOG_Logger *logger, String8 sink);
internal void OS_W32_LOG_Shutdown (OS_W32_LOG_Logger *logger);

internal LOG_Channel OS_W32_LOG_OpenChannel  (OS_W32_LOG_Logger *logger, String8 name);
internal void        OS_W32_LOG_CloseChannel (OS_W32_LOG_Logger *logger, LOG_Channel channel);

internal void OS_W32_LOG_MakeDedupBody    (char *dst, i32 dst_size, const char *body, u32 count);
internal void OS_W32_LOG_FlushDedupToFile (OS_W32_LOG_Logger *logger, f32 elapsed);

internal i32 OS_W32_LOG_FormatLine(OS_W32_LOG_Logger *logger,
								   char *dst, i32 dst_size,
								   LOG_Level level, LOG_Channel channel,
								   const char *file, i32 line, const char *fn,
								   const char *body,
								   b32 for_file, f32 elapsed,
								   OS_W32_JOB_Context job_context);

internal void OS_W32_LOG_WriteV(OS_W32_LOG_Logger *logger,
								OS_W32_JOB_Context job_context,
								LOG_Level level, LOG_Channel channel,
								const char *file, i32 line, const char *fn,
								const char *fmt, va_list args);

#endif // OS_WIN32_LOG_H
