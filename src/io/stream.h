#pragma once

#include "core/common.h"

namespace mgp
{
	class PlatformCore;

	class Stream
	{
	public:
		Stream(PlatformCore *platform);
		virtual ~Stream();

		virtual void read(void *buffer, uint64_t length) const;
		virtual void write(void *data, uint64_t length) const;

		virtual void seek(int64_t offset) const;

		virtual void close();

		virtual int64_t getPosition() const;
		virtual int64_t getSize() const;

		void *getStream();
		const void *getStream() const;

	protected:
		PlatformCore *p_platform;
		void *p_stream;
	};
}
