#ifndef LOG_H
#define LOG_H

#define LOG_MAX_CHANNELS        32
#define LOG_LINE_BUFFER_SIZE    4096
#define LOG_CHANNEL_COL_ALIGN   8

#define LOG_ANSI_RESET    "\x1b[0m"
#define LOG_ANSI_DIM      "\x1b[2m"

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

	LOG_Channel null_channel;
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

internal void LOG_Init     (LOG_Logger *logger, String8 sink);
internal void LOG_Shutdown (LOG_Logger *logger);

internal LOG_Channel LOG_OpenChannel  (LOG_Logger *logger, String8 name);
internal void        LOG_CloseChannel (LOG_Logger *logger, LOG_Channel channel);

internal void LOG_MakeDedupBody    (char *dst, i32 dst_size, const char *body, u32 count);
internal void LOG_FlushDedupToFile (LOG_Logger *logger, f32 elapsed);

internal i32 LOG_FormatLine(LOG_Logger *logger,
							char *dst, i32 dst_size,
							LOG_Level level, LOG_Channel channel,
							const char *file, i32 line, const char *fn,
							const char *body,
							b32 for_file, f32 elapsed,
							JOB_Context job_context);

internal void LOG_WriteV(LOG_Logger *logger,
						 LOG_Level level, LOG_Channel channel,
						 const char *file, i32 line, const char *fn,
						 const char *fmt, va_list args);

#endif // LOG_H
