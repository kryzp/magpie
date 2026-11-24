#ifndef OS_LOGGER_H
#define OS_LOGGER_H

#include "core/types.h"
#include "core/log.h"
#include "chrono/timer.h"
#include "os/job.h"
#include "os/spin_lock.h"
#include "os/stream.h"

#include <cstdarg>
#include "container/string.h"

#define LOGGER_MAX_CHANNELS            64
#define LOGGER_LINE_BUFFER_SIZE        4096
#define LOGGER_CHANNEL_COL_ALIGN       10

namespace os {

struct LogChannelEntry {
	String name;
	bool enabled = false;
	LogChannel parent;
};

class Logger {
public:
	explicit Logger(const String &sink);
	~Logger();

	Logger(const Logger &) = delete;
	Logger &operator=(const Logger &) = delete;

	LogChannel OpenChannel(LogChannel parent, const char *name);
	void CloseChannel(LogChannel channel);

	void WriteV(
		job::Context job_context,
		LogLevel level, LogChannel channel,
		const char *file, int line, const char *fn,
		const char *fmt, va_list args
	);

	LogChannel GetLogChannel() const
	{
		return log_channel;
	}

private:
	void ChannelNameResolve(LogChannel channel, char *dst, int dst_size) const;

	int FormatLine(
		char *dst, int dst_size,
		LogLevel level, LogChannel channel,
		const char *file, int line, const char *fn,
		const char *body,
		bool for_file, float elapsed,
		job::Context job_context
	) const;

	void WriteToFile(const char *line, int len);
	void FlushDedupToFile(float elapsed);

	Timer timer;

	job::FiberMutex mutex;
	Stream file_stream;

	SpinLock channel_lock;
	unsigned channel_count;
	LogChannelEntry channels[LOGGER_MAX_CHANNELS];

	LogChannel null_channel;
	LogChannel log_channel;

	// track last message and overwrite in-place if repeated.
	char dedup_body[LOGGER_LINE_BUFFER_SIZE];
	LogChannel dedup_channel;
	LogLevel dedup_level;
	unsigned dedup_count;
	bool dedup_active;
	job::Context dedup_job_context;
};

}

#endif // OS_LOGGER_H
