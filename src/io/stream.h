#ifndef STREAM_H_
#define STREAM_H_

#include "stream.h"
#include "../common.h"

namespace llt
{
	/**
	 * Representation of a generic stream of data.
	 */
	class Stream
	{
	public:
		Stream();
		virtual ~Stream();

		/*
		 * Read data at its current stream position.
		 */
		virtual void read(void* buffer, uint64_t length) const;

		/*
		 * Write data to its current stream position.
		 */
		virtual void write(void* data, uint64_t length) const;

		/*
		 * Goto a set stream position.
		 */
		virtual void seek(int64_t offset) const;

		/*
		 * Close the stream.
		 */
		virtual void close();

		/*
		 * Get the current stream position.
		 */
		virtual int64_t position() const;

		/*
		 * Get the size of the stream.
		 */
		virtual int64_t size() const;

		/*
		 * Get the actual memory pointer to the stream.
		 */
		void* getStream();
		const void* getStream() const;

	protected:
		void* p_stream;
	};
}

#endif // STREAM_H_
