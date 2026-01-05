#include "filesystem.h"

#include "core/types.h"

#include <filesystem>

using namespace io;

String path::get_file_name(const String &path)
{
	std::filesystem::path file_path(path);
	std::string name = file_path.filename();
	return name;
}

String path::get_file_extension(const String &path)
{
	std::filesystem::path file_path(path);
	std::string ext = file_path.extension();
	return ext;
}

String path::get_file_directory(const String &path)
{
	std::filesystem::path file_path(path);
	std::string directory = file_path.parent_path().string();
	return directory;
}

String path::join(const String &path_a, String path_b)
{
	assert(false);
}

String path::normalize(const String &path)
{
	assert(false);
}
