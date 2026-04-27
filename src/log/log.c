
global LOG_Logger *log_selected = NULL;

internal void
LOG_InitAndSelect(LOG_Logger *logger, String8 sink)
{
	MemZeroStruct(logger);
	
	CH_TimerStart(&logger->timer);

	logger->mutex = osapi->MutexCreate();

	if (sink.len > 0)
		logger->file_stream = osapi->StreamFromFile(sink, OS_FILE_PRESET_CREATE);

	log_selected = logger;

	logger->null_channel = LOG_OpenChannel(String8Lit("--------"));
	logger->log_channel = LOG_OpenChannel(String8Lit("LOG"));

	if (!OS_HandleIsNull(logger->file_stream))
		DebugLogI(logger->log_channel, "Initialized (\"%.*s\" as file sink).", (i32)sink.len, sink.str);
	else
		DebugLogI(logger->log_channel, "Initialized (no file sink).");
}

internal void
LOG_Shutdown(void)
{
	AssertTrue(log_selected);

	if (log_selected->dedup_active)
	{
		f32 elapsed = CH_TimerElapsed(&log_selected->timer);
		LOG_FlushDedupToFile(elapsed);
		log_selected->dedup_active = false;
	}
	
	DebugLogI(log_selected->log_channel, "Shutting Down...");

	if (!OS_HandleIsNull(log_selected->file_stream))
	{
		osapi->StreamClose(log_selected->file_stream);
		log_selected->file_stream = OS_HandleNull();
	}

	osapi->MutexDestroy(log_selected->mutex);
	log_selected->mutex = OS_HandleNull();

	log_selected = NULL;
}

internal void
LOG_HotLoad(LOG_Logger *logger)
{
	log_selected = logger;
}

internal void
LOG_HotUnload(void)
{
	AssertTrue(log_selected);
}

internal LOG_Channel
LOG_OpenChannel(String8 name)
{
	AssertTrue(log_selected);
	AssertTrue(log_selected->channel_count < ArraySize(log_selected->channels));

	LOG_ChannelEntry *c = &log_selected->channels[log_selected->channel_count];
	c->name = name;
	c->enabled = true;

	LOG_Channel id = { log_selected->channel_count };

	log_selected->channel_count++;

	return id;
}

internal void
LOG_CloseChannel(LOG_Channel channel)
{
	AssertTrue(log_selected);
	
	log_selected->channels[channel.id].enabled = false;
}

internal i32
LOG_FormatLine(char *dst, i32 dst_size,
			   LOG_Level level, LOG_Channel channel,
			   const char *file, i32 line, const char *fn,
			   const char *body,
			   b32 for_file, f32 elapsed, b32 remove_level_and_channel,
			   JOB_Context job_context)
{
	const char *level_string = LOG_LevelToString (level);
	const char *level_ansi   = LOG_LevelAnsi     (level);
	
	i32 len = 0;
	b32 show_callsite = level >= LOG_Level_Error;
	u8 *channel_name = log_selected->channels[channel.id].name.str;

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

	//if (remove_level_and_channel)
	{
		// Level.
		if (for_file)
			Append("[  %s  ] ", level_string);

		// Channel.
		if (for_file)
			Append("[  %-*s  ] ", LOG_CHANNEL_COL_ALIGN, channel_name);
		else
			Append("%s[  %-*s  ]" LOG_ANSI_RESET " ", level_ansi, LOG_CHANNEL_COL_ALIGN, channel_name);
	}
	/*
	else
	{
		// Level.
		if (for_file)
			Append("            ");

		// Channel.
		Append("   %-*s    ", LOG_CHANNEL_COL_ALIGN, "");
	}
	*/
	
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
LOG_FlushDedupToFile(f32 elapsed)
{
	if (!log_selected->dedup_active || OS_HandleIsNull(log_selected->file_stream))
		return;

	if (log_selected->dedup_count <= 1)
		return;

	char body[LOG_LINE_BUFFER_SIZE] = {0};
	LOG_MakeDedupBody(body, sizeof(body), log_selected->dedup_body, log_selected->dedup_count);

	char file_line[LOG_LINE_BUFFER_SIZE] = {0};

	i32 file_len = 0;

	if (!OS_HandleIsNull(log_selected->file_stream))
	{
		file_len = LOG_FormatLine(file_line, sizeof(file_line),
								  log_selected->dedup_level,
								  log_selected->dedup_channel,
								  "", 0, "",
								  body,
								  true, elapsed, true,
								  log_selected->dedup_job_context);
		
		if (file_len > 0)
			osapi->StreamWrite(log_selected->file_stream, file_line, (u64)file_len);
	}
}

internal void
LOG_Write(LOG_Level level, LOG_Channel channel,
		  const char *file, i32 line, const char *fn,
		  const char *fmt, ...)
{
	AssertTrue(log_selected);
	
	va_list args;
	va_start(args, fmt);
	LOG_WriteV(level, channel, file, line, fn, fmt, args);
	va_end(args);
}

internal void
LOG_WriteV(LOG_Level level, LOG_Channel channel,
		   const char *file, i32 line, const char *fn,
		   const char *fmt, va_list args)
{
	AssertTrue(log_selected);

	if (level < LOG_COMPILE_MIN_FILTER)
		return;

	if (!log_selected->channels[channel.id].enabled)
		return;

	f32 elapsed = CH_TimerElapsed(&log_selected->timer);

	JOB_Context job_context = osapi->JobGetContext();
	
	char body[LOG_LINE_BUFFER_SIZE] = {0};
	vsnprintf(body, sizeof(body), fmt, args);

	osapi->MutexLock(log_selected->mutex);
	{
		b32 is_repeated_line =
			log_selected->dedup_active &&
			log_selected->dedup_level == level &&
			log_selected->dedup_job_context.fiber_id == job_context.fiber_id &&
			LOG_ChannelMatch(log_selected->dedup_channel, channel) &&
			(CStrCompare(log_selected->dedup_body, body) == 0);

		b32 is_repeated_channel_and_level =
			log_selected->dedup_level == level &&
			LOG_ChannelMatch(log_selected->dedup_channel, channel);
		
		if (is_repeated_line)
		{
			log_selected->dedup_count++;

			char dedup_body[LOG_LINE_BUFFER_SIZE] = {0};
			LOG_MakeDedupBody(dedup_body, sizeof(dedup_body), body, log_selected->dedup_count);
			
			char console_line[LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = LOG_FormatLine(console_line, sizeof(console_line),
											 level, channel,
											 file, line, fn,
											 dedup_body,
											 false, elapsed, !is_repeated_channel_and_level,
											 log_selected->dedup_job_context);

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
			if (log_selected->dedup_active)
			{
				LOG_FlushDedupToFile(elapsed);

				if (log_selected->dedup_count > 1)
					fwrite("\n", 1, 1, stdout);
			}
			
			char console_line[LOG_LINE_BUFFER_SIZE] = {0};
	
			i32 console_len = LOG_FormatLine(console_line, sizeof(console_line),
											 level, channel,
											 file, line, fn,
											 body,
											 false, elapsed, !is_repeated_channel_and_level,
											 job_context);

			if (console_len > 0)
			{
				fwrite(console_line, 1, (usize)console_len, stdout);

				if (level >= LOG_Level_Warn)
					fflush(stdout);
			}
			
			if (!OS_HandleIsNull(log_selected->file_stream))
			{
				char file_line[LOG_LINE_BUFFER_SIZE] = {0};
				i32 file_len = 0;
			
				file_len = LOG_FormatLine(file_line, sizeof(file_line),
										  level, channel,
										  file, line, fn,
										  body,
										  true, elapsed, true,
										  job_context);
				
				if (file_len > 0)
					osapi->StreamWrite(log_selected->file_stream, file_line, (u64)file_len);
			}

			snprintf(log_selected->dedup_body, sizeof(log_selected->dedup_body), "%s", body);
			log_selected->dedup_level = level;
			log_selected->dedup_channel = channel;
			log_selected->dedup_count = 1;
			log_selected->dedup_active = true;
			log_selected->dedup_job_context = job_context;
		}
	}
	osapi->MutexUnlock(log_selected->mutex);

	if (level == LOG_Level_Break)
	{
		fflush(stdout);

		if (!OS_HandleIsNull(log_selected->file_stream))
		{
			osapi->StreamClose(log_selected->file_stream);
			log_selected->file_stream = OS_HandleNull();
		}

		AssertTrue(false);
	}
}
