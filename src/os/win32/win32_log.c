
/*
 * due to the fact i suck at coding the logger is actually considered lower
 * than the job system and thus isn't allowed to use scratch arenas
 * note to future me NEVER USE SCRATCH ARENAS HERE.
 */

internal void
OS_W32_LOG_Init(OS_W32_LOG_Logger *logger, String8 sink)
{
	MemZeroStruct(logger);
	
	CH_TimerStart(&logger->timer);

	logger->mutex = osapi->MutexCreate();

	if (sink.len > 0)
		logger->file_stream = osapi->StreamFromFile(sink, OS_FILE_PRESET_CREATE);

	logger->null_channel = OS_W32_LOG_OpenChannel(logger, String8Lit("----------"));
	logger->log_channel  = OS_W32_LOG_OpenChannel(logger, String8Lit("WIN32/LOG"));

	if (!OS_HandleIsNull(logger->file_stream))
		DebugLogI(logger->log_channel, "Initialized (\"%.*s\" as file output).", String8VArg(sink));
	else
		DebugLogI(logger->log_channel, "Initialized (no file sink).");
}

internal void
OS_W32_LOG_Shutdown(OS_W32_LOG_Logger *logger)
{
	AssertTrue(logger);

	if (logger->dedup_active)
	{
		f32 elapsed = CH_TimerElapsed(&logger->timer);
		OS_W32_LOG_FlushDedupToFile(logger, elapsed);
		logger->dedup_active = false;
	}
	
	DebugLogI(logger->log_channel, "Shutting down...");

	if (!OS_HandleIsNull(logger->file_stream))
	{
		osapi->StreamClose(logger->file_stream);
		logger->file_stream = OS_HandleNull();
	}

	osapi->MutexDestroy(logger->mutex);
	logger->mutex = OS_HandleNull();
}

internal LOG_Channel
OS_W32_LOG_OpenChannel(OS_W32_LOG_Logger *logger, String8 name)
{
	AssertTrue(logger);
	AssertTrue(logger->channel_count < ArraySize(logger->channels));

	OS_W32_LOG_ChannelEntry *entry = &logger->channels[logger->channel_count];
	entry->name = name;
	entry->enabled = true;

	LOG_Channel channel = { logger->channel_count };

	logger->channel_count++;

	return channel;
}

internal LOG_Channel
OS_W32_LOG_OpenChannelFrom(OS_W32_LOG_Logger *logger, LOG_Channel parent, String8 name)
{
	AssertTrue(logger);
	AssertTrue(logger->channel_count < ArraySize(logger->channels));

	OS_W32_LOG_ChannelEntry *entry = &logger->channels[logger->channel_count];
	entry->name = name;
	entry->enabled = true;
	entry->parent = parent;

	LOG_Channel channel = { logger->channel_count };

	logger->channel_count++;

	return channel;
}

internal void
OS_W32_LOG_CloseChannel(OS_W32_LOG_Logger *logger, LOG_Channel channel)
{
	AssertTrue(logger);
	
	logger->channels[channel.id].enabled = false;
}

internal void
OS_W32_LOG_ChannelNameResolve(OS_W32_LOG_Logger *logger, LOG_Channel channel, char *dst, i32 dst_size)
{
    OS_W32_LOG_ChannelEntry *entry = &logger->channels[channel.id];

    if (entry->parent.id != 0)
    {
        char parent[64] = {0};
        OS_W32_LOG_ChannelNameResolve(logger, entry->parent, parent, sizeof(parent));
        snprintf(dst, (usize)dst_size, "%s/%.*s", parent, String8VArg(entry->name));
    }
    else
    {
        snprintf(dst, (usize)dst_size, "%.*s", String8VArg(entry->name));
    }
}

internal i32
OS_W32_LOG_FormatLine(OS_W32_LOG_Logger *logger,
					  char *dst, i32 dst_size,
					  LOG_Level level, LOG_Channel channel,
					  const char *file, i32 line, const char *fn,
					  const char *body,
					  b32 for_file, f32 elapsed,
					  OS_W32_JOB_Context job_context)
{
	const char *level_string = OS_W32_LOG_LevelToString (level);
	const char *level_ansi   = OS_W32_LOG_LevelAnsi     (level);
	
	i32 len = 0;
	b32 show_callsite = level >= LOG_Level_Error;

	// I just learned about this push/pop_macro stuff man this is so sick.
#pragma push_macro("Append")
#undef Append

#define Append(...)														\
	do																	\
	{																	\
		if (len < dst_size)												\
			len += snprintf(dst + len, (usize)(dst_size - len), __VA_ARGS__); \
	}																	\
	while (0)

	// Timestamp.
	if (for_file)
		Append("[  %7.3f  ] ", elapsed);
	else
		Append(OS_W32_LOG_ANSI_DIM "[  %7.3f  ]" OS_W32_LOG_ANSI_RESET " ", elapsed);

	// Job Context.
    {
        char worker_str[8] = {0};
        char fiber_str[8] = {0};

		// We could jsut use worker_id = 0 but
		// writing MT makes it more obvious so
		// I'm going with that.
        if (job_context.worker_id == 0)
            snprintf(worker_str, sizeof(worker_str), "MT ");
        else
            snprintf(worker_str, sizeof(worker_str), "%-3u", job_context.worker_id);

        if (job_context.fiber_id == -1)
            snprintf(fiber_str, sizeof(fiber_str), "---");
        else
            snprintf(fiber_str, sizeof(fiber_str), "%-3d", job_context.fiber_id);

        if (for_file)
            Append("[  W:%s F:%s  ] ", worker_str, fiber_str);
        else
            Append(OS_W32_LOG_ANSI_DIM "[  W:%s F:%s  ]" OS_W32_LOG_ANSI_RESET " ", worker_str, fiber_str);
    }

	// Level.
	if (for_file)
		Append("[  %s  ] ", level_string);

	// Channel.
	char channel_name[512] = {0};
	OS_W32_LOG_ChannelNameResolve(logger, channel, channel_name, sizeof(channel_name));

	if (for_file)
		Append("[  %-*s  ] ", OS_W32_LOG_CHANNEL_COL_ALIGN, channel_name);
	else
		Append("%s[  %-*s  ]" OS_W32_LOG_ANSI_RESET " ", level_ansi, OS_W32_LOG_CHANNEL_COL_ALIGN, channel_name);

	// Callsite.
	if (show_callsite)
	{
		const char *filename = file;
		for (const char *p = file; *p; p++)
			if (*p == '/' || *p == '\\')
				filename = p + 1;

		if (for_file)
			Append("%s:%d %s: ", filename, line, fn);
		else
			Append(OS_W32_LOG_ANSI_DIM "%s:%d %s:" OS_W32_LOG_ANSI_RESET " ", filename, line, fn);
	}

	Append("%s\n", body);

#pragma pop_macro("Append")
	
	if (len >= dst_size)
		len = dst_size - 1;

	return len;
}

internal void
OS_W32_LOG_MakeDedupBody(char *dst, i32 dst_size, const char *body, u32 count)
{
	if (count > 1)
		snprintf(dst, (usize)dst_size, "%s (%ux)", body, count);
	else
		snprintf(dst, (usize)dst_size, "%s", body);
}

internal void
OS_W32_LOG_FlushDedupToFile(OS_W32_LOG_Logger *logger, f32 elapsed)
{
	if (!logger->dedup_active || OS_HandleIsNull(logger->file_stream))
		return;

	if (logger->dedup_count <= 1)
		return;

	char body[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
	OS_W32_LOG_MakeDedupBody(body, sizeof(body), logger->dedup_body, logger->dedup_count);

	char file_line[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};

	i32 file_len = 0;

	if (!OS_HandleIsNull(logger->file_stream))
	{
		file_len = OS_W32_LOG_FormatLine(logger,
										 file_line, sizeof(file_line),
										 logger->dedup_level,
										 logger->dedup_channel,
										 "", 0, "",
										 body,
										 true, elapsed,
										 logger->dedup_job_context);
		
		if (file_len > 0)
			osapi->StreamWrite(logger->file_stream, file_line, (u64)file_len);
	}
}

internal void
OS_W32_LOG_WriteV(OS_W32_LOG_Logger *logger,
				  OS_W32_JOB_Context job_context,
				  LOG_Level level, LOG_Channel channel,
				  const char *file, i32 line, const char *fn,
				  const char *fmt, va_list args)
{
	AssertTrue(logger);

	if (level < LOG_COMPILE_MIN_FILTER)
		return;

	if (!logger->channels[channel.id].enabled)
		return;

	f32 elapsed = CH_TimerElapsed(&logger->timer);

	char body[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
	vsnprintf(body, sizeof(body), fmt, args);

	osapi->MutexLock(logger->mutex);
	{
		b32 is_repeated_line =
			logger->dedup_active &&
			logger->dedup_level == level &&
			logger->dedup_job_context.fiber_id == job_context.fiber_id &&
			LOG_ChannelMatch(logger->dedup_channel, channel) &&
			(CStrCompare(logger->dedup_body, body) == 0);
		
		if (is_repeated_line)
		{
			logger->dedup_count++;

			char dedup_body[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
			OS_W32_LOG_MakeDedupBody(dedup_body, sizeof(dedup_body), body, logger->dedup_count);
			
			char console_line[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = OS_W32_LOG_FormatLine(logger,
													console_line, sizeof(console_line),
													level, channel,
													file, line, fn,
													dedup_body,
													false, elapsed,
													logger->dedup_job_context);

			if (console_len > 0)
			{
				i32 write_len = console_len;

				if (console_line[write_len - 1] == '\n')
					write_len--;

				if (logger->dedup_count == 2) // jump back to previous line to overwrite
					fwrite("\x1b[1A\r", 1, 5, stdout);
				else
					fwrite("\r", 1, 1, stdout);

				fwrite(console_line, 1, (usize)write_len, stdout);

				fflush(stdout);
			}
		}
		else
		{
			if (logger->dedup_active)
			{
				OS_W32_LOG_FlushDedupToFile(logger, elapsed);

				if (logger->dedup_count > 1)
					fwrite("\n", 1, 1, stdout);
			}
			
			char console_line[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = OS_W32_LOG_FormatLine(logger,
													console_line, sizeof(console_line),
													level, channel,
													file, line, fn,
													body,
													false, elapsed,
													job_context);

			if (console_len > 0)
			{
				fwrite(console_line, 1, (usize)console_len, stdout);

				if (level >= LOG_Level_Warn)
					fflush(stdout);
			}
			
			if (!OS_HandleIsNull(logger->file_stream))
			{
				char file_line[OS_W32_LOG_LINE_BUFFER_SIZE] = {0};
				i32 file_len = 0;
			
				file_len = OS_W32_LOG_FormatLine(logger,
												 file_line, sizeof(file_line),
												 level, channel,
												 file, line, fn,
												 body,
												 true, elapsed,
												 job_context);
				
				if (file_len > 0)
					osapi->StreamWrite(logger->file_stream, file_line, (u64)file_len);
			}

			snprintf(logger->dedup_body, sizeof(logger->dedup_body), "%s", body);
			logger->dedup_level = level;
			logger->dedup_channel = channel;
			logger->dedup_count = 1;
			logger->dedup_active = true;
			logger->dedup_job_context = job_context;
		}
	}
	osapi->MutexUnlock(logger->mutex);

	if (level == LOG_Level_Break)
	{
		fflush(stdout);

		if (!OS_HandleIsNull(logger->file_stream))
		{
			osapi->StreamClose(logger->file_stream);
			logger->file_stream = OS_HandleNull();
		}

		AssertTrue(false);
	}
}
