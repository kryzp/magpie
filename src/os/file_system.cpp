#include "os/file_system.h"
#include "os/stream.h"
#include "core/log.h"
#include "core/str.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace stdfs = std::filesystem;

bool fs::Delete(const String &path)
{
	std::error_code ec;
	return stdfs::remove(path, ec);
}

bool fs::Exists(const String &path)
{
	std::error_code ec;
	return stdfs::exists(path, ec) && !stdfs::is_directory(path, ec);
}

i64 fs::GetLastWriteTime(const String &path)
{
	std::error_code ec;

	stdfs::file_time_type time = stdfs::last_write_time(path, ec);

	if (ec)
		return 0;

	return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

bool fs::DirectoryCreate(const String &path)
{
	std::error_code ec;
	return stdfs::create_directory(path, ec);
}

bool fs::DirectoryDelete(const String &path)
{
	std::error_code ec;
	return stdfs::remove(path, ec);
}

bool fs::DirectoryExists(const String &path)
{
	std::error_code ec;
	return stdfs::is_directory(path, ec);
}

Vector<String> fs::ListFileNames(const String &directory)
{
	Vector<String> result;
	std::error_code ec;

	for (const stdfs::directory_entry &entry : stdfs::directory_iterator(directory, ec)) {
		if (entry.is_regular_file())
			result.push_back(entry.path().filename().string());
	}

	return result;
}

Vector<String> fs::ListFilesWithExtension(const String &directory, const String &extension)
{
	Vector<String> names;

	for (const String &file : ListFileNames(directory)) {
		if (file.size() > extension.size() && str::EndsWith(file, extension)) {
			names.push_back(file.substr(0, file.size() - extension.size()));
		}
	}

	std::sort(names.begin(), names.end());

	return names;
}

Vector<String> fs::ListDirectories(const String &directory)
{
	Vector<String> result;
	std::error_code ec;

	for (const stdfs::directory_entry &entry : stdfs::directory_iterator(directory, ec)) {
		if (entry.is_directory())
			result.push_back(entry.path().filename().string());
	}

	return result;
}

String fs::AbsolutePath(const String &path)
{
	std::error_code ec;
	stdfs::path absolute = stdfs::absolute(path, ec);

	if (ec)
		return path;

	return absolute.lexically_normal().string();
}

bool fs::SetWorkingDirectory(const String &path)
{
	std::error_code ec;
	stdfs::current_path(path, ec);
	return !ec;
}

bool fs::ReadEntireFile(const String &path, Vector<u8> *out)
{
	os::Stream stream = os::Stream::FromFile(path, os::FileAccess_Read);

	if (!stream.IsOpen()) {
		DebugPrintE("Cannot open \"%s\".", path.c_str());
		return false;
	}

	i64 size = stream.Size();

	if (out) {
		out->resize(static_cast<u64>(size));

		i64 read = size > 0 ? stream.Read(out->data(), static_cast<u64>(size)) : 0;

		if (read != size) {
			DebugPrintE("Short read on \"%s\" (%lld of %lld).", path.c_str(), read, size);
			out->clear();
			return false;
		}
	}

	return true;
}

bool fs::ReadEntireTextFile(const String &path, String *out)
{
	Vector<u8> bytes;

	if (!ReadEntireFile(path, &bytes))
		return false;

	if (out)
		out->assign(reinterpret_cast<const char *>(bytes.data()), bytes.size());

	return true;
}

bool fs::WriteEntireFile(const String &path, const void *data, u64 size)
{
	os::Stream stream = os::Stream::FromFile(path, os::FilePresetCreate);

	if (!stream.IsOpen()) {
		DebugPrintE("Cannot open \"%s\" for writing.", path.c_str());
		return false;
	}

	return stream.Write(data, size) == static_cast<i64>(size);
}

bool fs::WriteEntireTextFile(const String &path, const String &text)
{
	return WriteEntireFile(path, text.data(), text.size());
}
