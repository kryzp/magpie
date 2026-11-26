#include "os/logger.h"

#include <cstdio>
#include <cstring>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#define LOGGER_ANSI_RESET "\x1b[0m"
#define LOGGER_ANSI_DIM   "\x1b[2m"

static os::Logger *logger = nullptr;

static const char *LogLevelToString(LogLevel level)
{
	switch (level) {
		case LogLevel_Trace:
			return "TRACE";
		case LogLevel_Debug:
			return "DEBUG";
		case LogLevel_Info:
			return "INFO ";
		case LogLevel_Warn:
			return "WARN ";
		case LogLevel_Error:
			return "ERROR";
		case LogLevel_Break:
			return "BREAK";
		default:
			break;
	}

	return "?????";
}

static const char *LogLevelAnsi(LogLevel level)
{
	switch (level) {
		case LogLevel_Trace:
			return "\x1b[90m";
		case LogLevel_Debug:
			return "\x1b[36m";
		case LogLevel_Info:
			return "\x1b[32m";
		case LogLevel_Warn:
			return "\x1b[33m";
		case LogLevel_Error:
			return "\x1b[31m";
		case LogLevel_Break:
			return "\x1b[41;30m";
		default:
			break;
	}

	return "";
}

static void MakeDedupBody(char *dst, int dst_size, const char *body, unsigned count)
{
	if (count > 1)
		snprintf(dst, static_cast<u64>(dst_size), "%s (%ux)", body, count);
	else
		snprintf(dst, static_cast<u64>(dst_size), "%s", body);
}

os::Logger::Logger(const String &sink)
	: channel_count(0)
	, dedup_level(LogLevel_Trace)
	, dedup_count(0)
	, dedup_active(false)
{
	dedup_body[0] = '\0';

#if defined(_WIN32)
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD console_mode = 0;

	if (GetConsoleMode(console, &console_mode))
		SetConsoleMode(console, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

	timer.Start();

	logger = this;

	null_channel = OpenChannel(LogChannel(), "----------");
	log_channel = OpenChannel(LogChannel(), "PLATFORM/LOG"); // yes yes, i know. I'm faking the parent channel for aesthetics because I can't make the platform channel until the logging system is created!

	if (!sink.empty()) {
		file_stream = Stream::FromFile(sink, FilePresetCreate);
		DebugLogI(log_channel, "Initialized (\"%s\" as file output).", sink.c_str());
	} else {
		DebugLogI(log_channel, "Initialized (no file sink).");
	}
}

os::Logger::~Logger()
{
	if (dedup_active) {
		float elapsed = static_cast<float>(timer.Elapsed());
		FlushDedupToFile(elapsed);
		dedup_active = false;
	}

	DebugLogI(log_channel, "Shutting down...");

	file_stream.Close();

	logger = nullptr;
}

LogChannel os::Logger::OpenChannel(LogChannel parent, const char *name)
{
	channel_lock.Acquire();

	AssertTrue(channel_count < ArraySize(channels));

	LogChannelEntry *entry = &channels[channel_count];
	entry->name = name;
	entry->enabled = true;
	entry->parent = parent;

	LogChannel channel;
	channel.id = channel_count;

	channel_count++;

	channel_lock.Release();

	return channel;
}

void os::Logger::CloseChannel(LogChannel channel)
{
	channels[channel.id].enabled = false;
}

void os::Logger::ChannelNameResolve(LogChannel channel, char *dst, int dst_size) const
{
	const LogChannelEntry *entry = &channels[channel.id];

	if (entry->parent.id != 0) {
		char parent[64] = {};
		ChannelNameResolve(entry->parent, parent, sizeof(parent));
		snprintf(dst, static_cast<u64>(dst_size), "%s/%s", parent, entry->name.c_str());
	} else {
		snprintf(dst, static_cast<u64>(dst_size), "%s", entry->name.c_str());
	}
}

int os::Logger::FormatLine(
	char *dst, int dst_size,
	LogLevel level, LogChannel channel,
	const char *file, int line, const char *fn,
	const char *body,
	bool for_file, float elapsed,
	job::Context job_context
) const
{
	const char *level_string = LogLevelToString(level);
	const char *level_ansi = LogLevelAnsi(level);

	int len = 0;
	bool show_callsite = level >= LogLevel_Error;

	// I just learned about this push/pop_macro stuff man this is so sick.
#pragma push_macro("Append")
#undef Append

#define Append(...)                                                            \
	do {                                                                       \
		if (len < dst_size)                                                    \
			len += snprintf(dst + len, (u64)(dst_size - len), __VA_ARGS__); \
	} while (0)

	// Timestamp.
	if (for_file)
		Append("[  %7.3f  ] ", elapsed);
	else
		Append(LOGGER_ANSI_DIM "[  %7.3f  ]" LOGGER_ANSI_RESET " ", elapsed);

	// Job Context.
	{
		char worker_str[16] = {};
		char fiber_str[16] = {};

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
			Append(LOGGER_ANSI_DIM "[  W:%s F:%s  ]" LOGGER_ANSI_RESET " ", worker_str, fiber_str);
	}

	// Level.
	if (for_file)
		Append("[  %s  ] ", level_string);

	// Channel.
	char channel_name[512] = {};
	ChannelNameResolve(channel, channel_name, sizeof(channel_name));

	if (for_file)
		Append("[  %-*s  ] ", LOGGER_CHANNEL_COL_ALIGN, channel_name);
	else
		Append("%s[  %-*s  ]" LOGGER_ANSI_RESET " ", level_ansi, LOGGER_CHANNEL_COL_ALIGN, channel_name);

	// Callsite.
	if (show_callsite) {
		const char *filename = file;
		for (const char *p = file; *p; p++) {
			if (*p == '/' || *p == '\\')
				filename = p + 1;
		}

		if (for_file)
			Append("%s:%d %s: ", filename, line, fn);
		else
			Append(LOGGER_ANSI_DIM "%s:%d %s:" LOGGER_ANSI_RESET " ", filename, line, fn);
	}

	Append("%s\n", body);

#pragma pop_macro("Append")

	if (len >= dst_size)
		len = dst_size - 1;

	return len;
}

void os::Logger::WriteToFile(const char *line, int len)
{
	if (len <= 0 || !file_stream.IsOpen())
		return;

	file_stream.Write(line, static_cast<u64>(len));

	// necessary for linux since on linux it's buffered so after an assert
	// crash the last couple lines would be lost, unlike on windows.
	file_stream.Flush();
}

void os::Logger::FlushDedupToFile(float elapsed)
{
	if (!dedup_active || !file_stream.IsOpen())
		return;

	if (dedup_count <= 1)
		return;

	char body[LOGGER_LINE_BUFFER_SIZE] = {};
	MakeDedupBody(body, sizeof(body), dedup_body, dedup_count);

	char file_line[LOGGER_LINE_BUFFER_SIZE] = {};

	int file_len = FormatLine(
		file_line, sizeof(file_line),
		dedup_level,
		dedup_channel,
		"", 0, "",
		body,
		true, elapsed,
		dedup_job_context
	);

	WriteToFile(file_line, file_len);
}

void os::Logger::WriteV(
	job::Context job_context,
	LogLevel level, LogChannel channel,
	const char *file, int line, const char *fn,
	const char *fmt, va_list args
)
{
	if (level < LOG_COMPILE_MIN_FILTER)
		return;

	if (!channels[channel.id].enabled)
		return;

	float elapsed = static_cast<float>(timer.Elapsed());

	char body[LOGGER_LINE_BUFFER_SIZE] = {};
	vsnprintf(body, sizeof(body), fmt, args);

	mutex.Lock();
	{
		bool is_repeated_line =
			dedup_active &&
			dedup_level == level &&
			dedup_job_context.fiber_id == job_context.fiber_id &&
			dedup_channel == channel &&
			(strcmp(dedup_body, body) == 0);

		if (is_repeated_line) {
			dedup_count++;

			char dedup_line_body[LOGGER_LINE_BUFFER_SIZE] = {};
			MakeDedupBody(dedup_line_body, sizeof(dedup_line_body), body, dedup_count);

			char console_line[LOGGER_LINE_BUFFER_SIZE] = {};

			int console_len = FormatLine(
				console_line, sizeof(console_line),
				level, channel,
				file, line, fn,
				dedup_line_body,
				false, elapsed,
				dedup_job_context
			);

			if (console_len > 0) {
				int write_len = console_len;

				if (console_line[write_len - 1] == '\n')
					write_len--;

				// jump back to previous line to overwrite
				if (dedup_count == 2)
					fwrite("\x1b[1A\r", 1, 5, stdout);
				else
					fwrite("\r", 1, 1, stdout);

				fwrite(console_line, 1, static_cast<u64>(write_len), stdout);

				fflush(stdout);
			}
		} else {
			if (dedup_active) {
				FlushDedupToFile(elapsed);

				if (dedup_count > 1)
					fwrite("\n", 1, 1, stdout);
			}

			char console_line[LOGGER_LINE_BUFFER_SIZE] = {};

			int console_len = FormatLine(
				console_line, sizeof(console_line),
				level, channel,
				file, line, fn,
				body,
				false, elapsed,
				job_context
			);

			if (console_len > 0) {
				fwrite(console_line, 1, static_cast<u64>(console_len), stdout);

				if (level >= LogLevel_Warn)
					fflush(stdout);
			}

			// TODO
			// temporarily removed, writing *per line* is incredibly slow
			// figure out a better solution
			// idea: store the last idk 1024 lines, once it goes past that write them all then clear the buffer
			/*
			if (file_stream.IsOpen()) {
				char file_line[LOGGER_LINE_BUFFER_SIZE] = {};

				int file_len = FormatLine(
					file_line, sizeof(file_line),
					level, channel,
					file, line, fn,
					body,
					true, elapsed,
					job_context
				);

				WriteToFile(file_line, file_len);
			}
			*/

			snprintf(dedup_body, sizeof(dedup_body), "%s", body);
			dedup_level = level;
			dedup_channel = channel;
			dedup_count = 1;
			dedup_active = true;
			dedup_job_context = job_context;
		}
	}
	mutex.Unlock();

	if (level == LogLevel_Break) {
		fflush(stdout);
		file_stream.Close();
		AssertTrue(false);
	}
}

void logs::Write(LogLevel level, LogChannel channel, const char *file, int line, const char *fn, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	if (logger) {
		logger->WriteV(job::GetContext(), level, channel, file, line, fn, fmt, args);
	} else {
		vfprintf(stderr, fmt, args);
		fputc('\n', stderr);

		if (level == LogLevel_Break)
			AssertTrue(false);
	}

	va_end(args);
}

LogChannel logs::OpenChannel(const char *name)
{
	// 0 = null channel = no parent
	return OpenChannel(LogChannel(), name);
}

LogChannel logs::OpenChannel(LogChannel parent, const char *name)
{
	if (!logger)
		return LogChannel();

	return logger->OpenChannel(parent, name);
}

void logs::CloseChannel(LogChannel channel)
{
	if (logger)
		logger->CloseChannel(channel);
}
