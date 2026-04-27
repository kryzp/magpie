
global CoreFatalHandlerFn *core_FatalHandler = NULL;

internal void
CoreSetFatalHandler(CoreFatalHandlerFn *Handler)
{
	core_FatalHandler = Handler;
}

internal void
CoreFatal_(const char *file, i32 line, const char *fn, const char *fmt, ...)
{
	char msg[1024] = {0};

	va_list args;
	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);

	if (core_FatalHandler)
		core_FatalHandler(file, line, fn, msg);

	// just in case
	AssertTrue(false);
}
