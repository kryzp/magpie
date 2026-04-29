#ifndef CORE_LOG_H
#define CORE_LOG_H

#define DebugLogEx(level, channel, ...) osapi->Log((level), (channel), __FILE__, __LINE__, __func__, __VA_ARGS__)

#define DebugLogT(channel, ...) DebugLogEx(LOG_Level_Trace, (channel), __VA_ARGS__)
#define DebugLogD(channel, ...) DebugLogEx(LOG_Level_Debug, (channel), __VA_ARGS__)
#define DebugLogI(channel, ...) DebugLogEx(LOG_Level_Info,  (channel), __VA_ARGS__)
#define DebugLogW(channel, ...) DebugLogEx(LOG_Level_Warn,  (channel), __VA_ARGS__)
#define DebugLogE(channel, ...) DebugLogEx(LOG_Level_Error, (channel), __VA_ARGS__)
#define DebugLogB(channel, ...) DebugLogEx(LOG_Level_Break, (channel), __VA_ARGS__)

#define DebugPrintT(...) DebugLogT(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintD(...) DebugLogD(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintI(...) DebugLogI(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintW(...) DebugLogW(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintE(...) DebugLogE(LOG_ChannelNull(), __VA_ARGS__)
#define DebugPrintB(...) DebugLogB(LOG_ChannelNull(), __VA_ARGS__)

#endif // CORE_LOG_H
