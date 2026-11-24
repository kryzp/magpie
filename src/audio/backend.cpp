#include "audio/backend.h"

static aud::IAudioBackend *selected_backend = nullptr;

aud::IAudioBackend::IAudioBackend()
{
	selected_backend = this;
}

aud::IAudioBackend::~IAudioBackend()
{
	if (selected_backend == this)
		selected_backend = nullptr;
}

aud::IAudioBackend *aud::IAudioBackend::Get()
{
	return selected_backend;
}
