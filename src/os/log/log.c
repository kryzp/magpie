
internal void
LOG_Init(LOG_Logger *logger, String8 sink)
{
	MemZeroStruct(logger);
	
	CH_TimerStart(&logger->timer);

	logger->mutex = osapi->MutexCreate();

	if (sink.len > 0)
		logger->file_stream = osapi->StreamFromFile(sink, OS_FILE_PRESET_CREATE);

	logger->null_channel = LOG_OpenChannel(logger, String8Lit("--------"));
	logger->log_channel  = LOG_OpenChannel(logger, String8Lit("LOG"));

	if (!OS_HandleIsNull(logger->file_stream))
		DebugLogI(logger->log_channel, "Initialized (\"%.*s\" as file sink).", (i32)sink.len, sink.str);
	else
		DebugLogI(logger->log_channel, "Initialized (no file sink).");
}

internal void
LOG_Shutdown(LOG_Logger *logger)
{
	AssertTrue(logger);

	if (logger->dedup_active)
	{
		f32 elapsed = CH_TimerElapsed(&logger->timer);
		LOG_FlushDedupToFile(logger, elapsed);
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
LOG_OpenChannel(LOG_Logger *logger, String8 name)
{
	AssertTrue(logger);
	AssertTrue(logger->channel_count < ArraySize(logger->channels));

	LOG_ChannelEntry *c = &logger->channels[logger->channel_count];
	c->name = name;
	c->enabled = true;

	LOG_Channel id = { logger->channel_count };

	logger->channel_count++;

	return id;
}

internal void
LOG_CloseChannel(LOG_Logger *logger, LOG_Channel channel)
{
	AssertTrue(logger);
	
	logger->channels[channel.id].enabled = false;
}

internal i32
LOG_FormatLine(LOG_Logger *logger,
			   char *dst, i32 dst_size,
			   LOG_Level level, LOG_Channel channel,
			   const char *file, i32 line, const char *fn,
			   const char *body,
			   b32 for_file, f32 elapsed,
			   JOB_Context job_context)
{
	const char *level_string = LOG_LevelToString (level);
	const char *level_ansi   = LOG_LevelAnsi     (level);
	
	i32 len = 0;
	b32 show_callsite = level >= LOG_Level_Error;
	u8 *channel_name = logger->channels[channel.id].name.str;

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
		Append(LOG_ANSI_DIM "[  %7.3f  ]" LOG_ANSI_RESET " ", elapsed);

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
            Append(LOG_ANSI_DIM "[  W:%s F:%s  ]" LOG_ANSI_RESET " ", worker_str, fiber_str);
    }

	// Level.
	if (for_file)
		Append("[  %s  ] ", level_string);

	// Channel.
	if (for_file)
		Append("[  %-*s  ] ", LOG_CHANNEL_COL_ALIGN, channel_name);
	else
		Append("%s[  %-*s  ]" LOG_ANSI_RESET " ", level_ansi, LOG_CHANNEL_COL_ALIGN, channel_name);
	
	// Callsite.
	if (show_callsite)
	{
		ScratchArena scratch = ScratchBegin(NULL, 0);
		
		u8 *base = IO_PathGetFileNameExt(scratch.arena, String8FromCStr(file)).str;

		if (for_file)
			Append("%s:%d %s: ", base, line, fn);
		else
			Append(LOG_ANSI_DIM "%s:%d %s:" LOG_ANSI_RESET " ", base, line, fn);

		ScratchRelease(&scratch);
	}

	Append("%s\n", body);

#pragma pop_macro("Append")
	
	if (len >= dst_size)
		len = dst_size - 1;

	return len;
}

internal void
LOG_MakeDedupBody(char *dst, i32 dst_size, const char *body, u32 count)
{
	if (count > 1)
		snprintf(dst, (usize)dst_size, "%s (%ux)", body, count);
	else
		snprintf(dst, (usize)dst_size, "%s", body);
}

internal void
LOG_FlushDedupToFile(LOG_Logger *logger, f32 elapsed)
{
	if (!logger->dedup_active || OS_HandleIsNull(logger->file_stream))
		return;

	if (logger->dedup_count <= 1)
		return;

	char body[LOG_LINE_BUFFER_SIZE] = {0};
	LOG_MakeDedupBody(body, sizeof(body), logger->dedup_body, logger->dedup_count);

	char file_line[LOG_LINE_BUFFER_SIZE] = {0};

	i32 file_len = 0;

	if (!OS_HandleIsNull(logger->file_stream))
	{
		file_len = LOG_FormatLine(logger,
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
LOG_WriteV(LOG_Logger *logger,
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

	JOB_Context job_context = osapi->JobGetContext();
	
	char body[LOG_LINE_BUFFER_SIZE] = {0};
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

			char dedup_body[LOG_LINE_BUFFER_SIZE] = {0};
			LOG_MakeDedupBody(dedup_body, sizeof(dedup_body), body, logger->dedup_count);
			
			char console_line[LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = LOG_FormatLine(logger,
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
				
				fwrite("\r", 1, 1, stdout);
				fwrite(console_line, 1, (usize)write_len, stdout);

				fflush(stdout);
			}
		}
		else
		{
			if (logger->dedup_active)
			{
				LOG_FlushDedupToFile(logger, elapsed);

				if (logger->dedup_count > 1)
					fwrite("\n", 1, 1, stdout);
			}
			
			char console_line[LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = LOG_FormatLine(logger,
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
				char file_line[LOG_LINE_BUFFER_SIZE] = {0};
				i32 file_len = 0;
			
				file_len = LOG_FormatLine(logger,
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
