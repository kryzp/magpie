#include "stream.h"
#include "../system_backend.h"

using namespace llt;

Stream::Stream()
	: p_stream(nullptr)
{
}

Stream::~Stream()
{
	close(); // close automatically on destroy.
}

void Stream::read(void* buffer, uint64_t length) const
{
	if (p_stream) {
		g_systemBackend->streamRead(p_stream, buffer, length);
	}
}

void Stream::write(void* data, uint64_t length) const
{
	if (p_stream) {
		g_systemBackend->streamWrite(p_stream, data, length);
	}
}

void Stream::seek(int64_t offset) const
{
	if (p_stream) {
		g_systemBackend->streamSeek(p_stream, offset);
	}
}

void Stream::close()
{
	if (!p_stream) {
		return;
	}

	g_systemBackend->streamClose(p_stream);
	p_stream = nullptr;
}

int64_t Stream::position() const
{
	if (p_stream) {
		return g_systemBackend->streamPosition(p_stream);
	}

	return -1; // stream isnt open, return -1
}

int64_t Stream::size() const
{
	if (p_stream) {
		return g_systemBackend->streamSize(p_stream);
	}

	return -1; // stream isnt open, return -1
}

void* Stream::getStream()
{
	return p_stream;
}

const void* Stream::getStream() const
{
	return p_stream;
}
