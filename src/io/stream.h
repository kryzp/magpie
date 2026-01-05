#pragma once

#include "core/types.h"
#include "container/string.h"

namespace io
{

class Stream {
public:
	Stream();
	virtual ~Stream();

	virtual void read(void *buffer, u64 length) const;
	virtual void write(void *data, u64 length) const;
	virtual void seek(s64 offset) const;
	virtual void close();

	virtual s64 position() const;
	virtual s64 size() const;

	void *buffer();
	const void *buffer() const;

	template <typename T>
	T read() const
	{
		T var;
		read(&var, sizeof(T));
		return var;
	}

	template <typename T>
	void write(const T &var)
	{
		write(&var, sizeof(T));
	}

protected:
	void *buf;
};

enum FileMode {
	FILE_MODE_OPEN,      // Open and append if exists.
	FILE_MODE_OPEN_RW,   // Open and read + append if exists.
	FILE_MODE_CREATE,    // Open and overwrite if exists.
	FILE_MODE_CREATE_RW  // Open and read + overwrite if exists.
};

class FileStream : public Stream {
public:
	FileStream();
	FileStream(const char *path, FileMode mode);

	FileStream &open(const String &path, FileMode mode);

	bool get_line(String &str, int32_t &pointer) const;
};

class MemoryStream : public Stream {
public:
	MemoryStream();
	MemoryStream(void *memory, uint64_t length);

	MemoryStream &open(void *memory, uint64_t length);
};

class ConstMemoryStream : public Stream {
public:
	ConstMemoryStream();
	ConstMemoryStream(const void *memory, uint64_t length);

	ConstMemoryStream &open(const void *memory, uint64_t length);
};

}
