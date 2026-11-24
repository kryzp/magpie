#ifndef CORE_LOG_H
#define CORE_LOG_H

#include "core/types.h"

#ifndef LOG_COMPILE_MIN_FILTER
#define LOG_COMPILE_MIN_FILTER LogLevel_Trace
#endif

enum LogLevel {
	LogLevel_Trace, // Granular Information (typically spammy)   (Allocations, Job's starting / stopping, Individual asset stages, ...)
	LogLevel_Debug, // Sub-System Information                    (Swapchain created, Reloading assets, Shader compilation, ...)
	LogLevel_Info,  // System Information                        (Graphics initialized, Assets shutdown, Important information, ...)
	LogLevel_Warn,  // Warnings                                  (Asset not found - falling back, Channel pool full, ...)
	LogLevel_Error, // Errors                                    (Shader compilation failed, Object pool empty, Failed to open file, ...)
	LogLevel_Break, // Break                                     (Vulkan device lost, App init failed, Render graph invalid, ...)
	//                 ^ called Break instead of Fatal because then the format would be DebugLogF which implies a formatted log instead of a fatal log.
	LogLevel_MaxEnum
};

struct LogChannel {
	static constexpr unsigned GLOBAL = 0;

	unsigned id = GLOBAL;

	bool operator==(const LogChannel &other) const
	{
		return id == other.id;
	}

	bool operator!=(const LogChannel &other) const
	{
		return id != other.id;
	}
};

namespace logs {

void Write(LogLevel level, LogChannel channel, const char *file, int line, const char *fn, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format(printf, 6, 7)))
#endif
	;

LogChannel OpenChannel(const char *name);
LogChannel OpenChannel(LogChannel parent, const char *name); // create a sub/child channel.
void CloseChannel(LogChannel channel);

}

#define AssertTrue(eq)                \
	do {                              \
		if (!(eq)) {                  \
			*((volatile int *)0) = 0; \
		}                             \
	} while (0)

#define DebugLogAt(level, channel, ...) logs::Write((level), (channel), __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DebugLogT(channel, ...) DebugLogAt(LogLevel_Trace, (channel), __VA_ARGS__)
#define DebugLogD(channel, ...) DebugLogAt(LogLevel_Debug, (channel), __VA_ARGS__)
#define DebugLogI(channel, ...) DebugLogAt(LogLevel_Info, (channel), __VA_ARGS__)
#define DebugLogW(channel, ...) DebugLogAt(LogLevel_Warn, (channel), __VA_ARGS__)
#define DebugLogE(channel, ...) DebugLogAt(LogLevel_Error, (channel), __VA_ARGS__)
#define DebugLogB(channel, ...) DebugLogAt(LogLevel_Break, (channel), __VA_ARGS__)

#define DebugPrintT(...) DebugLogT(LogChannel(), __VA_ARGS__)
#define DebugPrintD(...) DebugLogD(LogChannel(), __VA_ARGS__)
#define DebugPrintI(...) DebugLogI(LogChannel(), __VA_ARGS__)
#define DebugPrintW(...) DebugLogW(LogChannel(), __VA_ARGS__)
#define DebugPrintE(...) DebugLogE(LogChannel(), __VA_ARGS__)
#define DebugPrintB(...) DebugLogB(LogChannel(), __VA_ARGS__)

#define DebugLogAssert(channel, eq, ...)     \
	do {                                     \
		if (!(eq)) {                         \
			DebugLogB(channel, __VA_ARGS__); \
		}                                    \
	} while (0)

#define DebugPrintAssert(eq, ...)     \
	do {                              \
		if (!(eq)) {                  \
			DebugPrintB(__VA_ARGS__); \
		}                             \
	} while (0)

#endif // CORE_LOG_H
