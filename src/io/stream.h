#pragma once

#include "core/types.h"
#include "container/string.h"

namespace io
{
	enum FileMode {
		FILE_MODE_OPEN,      // Open and append if exists.
		FILE_MODE_OPEN_RW,   // Open and read + append if exists.
		FILE_MODE_CREATE,    // Open and overwrite if exists.
		FILE_MODE_CREATE_RW  // Open and read + overwrite if exists.
	};

	inline const char *file_mode_to_string(FileMode mode)
	{
		switch (mode) {
			case FILE_MODE_OPEN:       return "a";
			case FILE_MODE_OPEN_RW:    return "ra";
			case FILE_MODE_CREATE:     return "w";
			case FILE_MODE_CREATE_RW:  return "rw";
		}

		return nullptr;
	}

	class Stream {
	public:
		Stream(void *buffer);
		~Stream();

		static Stream from_file(const char *path, FileMode mode);
		static Stream from_memory(void *memory, uint64_t length);
		static Stream from_const_memory(const void *memory, uint64_t length);

		void read(void *buffer, u64 length) const;
		void write(void *data, u64 length) const;
		void seek(s64 offset) const;
		void close();

		s64 position() const;
		s64 size() const;

		void *get_handle();
		const void *get_handle() const;
		
		bool get_line(String &str, int32_t &pointer) const;

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
		void *handle;
	};
}
