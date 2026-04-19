#ifndef CORE_LOG_H
#define CORE_LOG_H

#define DebugLogF(message, ...) printf((message "\n"), ##__VA_ARGS__)

#endif // CORE_LOG_H
