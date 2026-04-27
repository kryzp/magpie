#ifndef CORE_FATAL_H
#define CORE_FATAL_H

typedef void CoreFatalHandlerFn(const char *file, i32 line, const char *fn, const char *msg);

internal void CoreSetFatalHandler(CoreFatalHandlerFn *Handler);
internal void CoreFatal_(const char *file, i32 line, const char *fn, const char *fmt, ...);

#define CoreFatal(...) CoreFatal_(__FILE__, __LINE__, __func__, __VA_ARGS__)

#endif // CORE_FATAL_H
