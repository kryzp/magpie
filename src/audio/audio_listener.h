#ifndef AUDIO_LISTENER_H
#define AUDIO_LISTENER_H

typedef struct AUD_Listener AUD_Listener;
struct AUD_Listener
{
	v3 eye;
	v3 forward;
	v3 up;
};

#endif // AUDIO_LISTENER_H
