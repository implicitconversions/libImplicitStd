

#include "fs.h"
#include "icy_log.h"

#if USE_STD_FILESYSTEM

#include <vector>
#include <string>
#include <cstdio>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

namespace fs {

bool path::operator == (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) == 0; }
bool path::operator != (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) != 0; }
bool path::operator == (const char *s) const { return strcasecmp(uni_path_.c_str(), fs::PathFromString(s).c_str()) == 0; }
bool path::operator != (const char *s) const { return strcasecmp(uni_path_.c_str(), fs::PathFromString(s).c_str()) != 0; }

bool path::operator >  (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) >  0; }
bool path::operator >= (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) >= 0; }
bool path::operator <  (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) <  0; }
bool path::operator <= (const path& s) const { return strcasecmp(uni_path_.c_str(), s.uni_path_.c_str()) <= 0; }


// create a path from an incoming user-provided string.
// Performs santiy checks on input.
std::string PathFromString(const char* path)
{
	if (!path || !path[0])
		return {};

	std::string  path_;

	if (path[0] == '/') {
		// path starts with a forward slash, assume it's already normalized
		path_ = path;
	}
	else {
		// assume path is mixed forward/backslash, normalize to forward slash mode.
		path_ = fs::ConvertFromMsw(path);
	}

	if (path_.back() == '/') {
		path_.resize(path_.length() - 1);
	}
	return path_;
}

bool exists(const path& fspath) {
	if (fspath.empty()) return false;

	std::error_code nothrow_please_kthx;
	if (fspath.is_device()) {
		// /dev/null and /dev/tty (NUL/CON) always exist, contrary to what std::filesystem thinks... --jstine
		return true;
	}
	auto ret = std::filesystem::exists(fspath.native(), nothrow_please_kthx);
	return ret && !nothrow_please_kthx;
}

bool remove(const path& fspath) {
	std::error_code nothrow_please_kthx;
	return std::filesystem::remove(fspath.native(), nothrow_please_kthx);
}

bool remove_all(const path& fspath) {
	std::error_code nothrow_please_kthx;
	return std::filesystem::remove_all(fspath.native(), nothrow_please_kthx);
}

intmax_t file_size(const path& fspath) {
	std::error_code nothrow_please_kthx;
	auto ret = std::filesystem::file_size(fspath.native(), nothrow_please_kthx);
	if (nothrow_please_kthx)
		return 0;

	return ret;
}

bool is_directory(const path& fspath) {
	std::error_code nothrow_please_kthx;
	auto ret = std::filesystem::is_directory(fspath.native(), nothrow_please_kthx);
	return ret && !nothrow_please_kthx;
}

bool create_directory(const path& fspath) {
	// check if it's there already. On Windows, calling create_directory can be expensive, even if the dir already exists
	if (exists(fspath) && is_directory(fspath))
		return true;

	std::error_code nothrow_please_kthx;
	if (!std::filesystem::create_directories(fspath.native(), nothrow_please_kthx)) {
		return (nothrow_please_kthx.value() == EEXIST || nothrow_please_kthx.value() == 183L); 	// return 1 for ERROR_ALREADY_EXISTS
	}
	return 1;
}

std::vector<path> directory_iterator(const path& fspath) {
	if (!fs::exists(fspath)) return {};
	std::vector<path> meh;
	for (const std::filesystem::path& item : std::filesystem::directory_iterator(fspath.native())) {
		meh.push_back(item.string());
	}
	return meh;
}

void directory_iterator(const std::function<void (const fs::path& path)>& func, const path& fspath) {
	if (!fs::exists(fspath)) return;
	for (const std::filesystem::path& item : std::filesystem::directory_iterator(fspath.native())) {
		func(item.string());
	}
}

std::vector<path> recursive_directory_iterator(const path& fspath) {
	if (!fs::exists(fspath)) return {};
	std::vector<path> meh;
	for (const std::filesystem::path& item : std::filesystem::recursive_directory_iterator(fspath.native())) {
		meh.push_back(item.string());
	}
	return meh;
}

const std::string& path::libc_path() const {
#if FILESYSTEM_NEEDS_OS_PATH
	return libc_path_;
#else
	return uni_path_;
#endif
}

void path::update_native_path() {
#if FILESYSTEM_NEEDS_OS_PATH
	libc_path_ = ConvertToMsw(uni_path_);
#endif
}

bool stat(const path& fspath, struct ::stat& st) {
	return ::stat(fspath, &st) == 0;
}

} // namespace fs
#endif
