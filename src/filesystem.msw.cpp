
#if !!PLATFORM_MSW
#include <vector>
#include <string>
#include <cstdio>
#include <filesystem>

#include "fs.h"
#include "icy_log.h"

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
	auto ret = std::filesystem::exists(fspath.asLibcStr(), nothrow_please_kthx);
	return ret && !nothrow_please_kthx;
}

void remove(const path& fspath) {
	std::error_code nothrow_please_kthx;
	std::filesystem::remove(fspath.asLibcStr(), nothrow_please_kthx);
}

intmax_t file_size(const path& fspath) {
	std::error_code nothrow_please_kthx;
	auto ret = std::filesystem::file_size(fspath.asLibcStr(), nothrow_please_kthx);
	if (nothrow_please_kthx)
		return 0;

	return ret;
}

bool is_directory(const path& fspath) {
	std::error_code nothrow_please_kthx;
	auto ret = std::filesystem::is_directory(fspath.asLibcStr(), nothrow_please_kthx);
	return ret && !nothrow_please_kthx;
}

bool create_directory(const path& fspath) {
	// check if it's there already. On Windows, calling create_directory can be expensive, even if the dir already exists
	if (exists(fspath) && is_directory(fspath))
		return true;

	std::error_code nothrow_please_kthx;
	if (!std::filesystem::create_directories(fspath.asLibcStr(), nothrow_please_kthx)) {
		return (nothrow_please_kthx.value() == EEXIST || nothrow_please_kthx.value() == 183L); 	// return 1 for ERROR_ALREADY_EXISTS
	}
	return 1;
}

std::vector<path> directory_iterator(const path& fspath) {
	if (!fs::exists(fspath)) return {};
	std::vector<path> meh;
	for (const std::filesystem::path& item : std::filesystem::directory_iterator(fspath.asLibcStr())) {
		meh.push_back(item.u8string().c_str());
	}
	return meh;
}

void directory_iterator(const std::function<void (const fs::path& path)>& func, const path& fspath) {
	if (!fs::exists(fspath)) return;
	for (const std::filesystem::path& item : std::filesystem::directory_iterator(fspath.asLibcStr())) {
		func(item.u8string().c_str());
	}
}

const std::string& path::libc_path() const {
	return libc_path_;
}

void path::update_native_path() {
	libc_path_ = ConvertToMsw(uni_path_);
}

std::string absolute(const path& fspath) {
	return std::filesystem::absolute(fspath.asLibcStr()).lexically_normal().string();
}

bool stat(const path& fspath, struct stat& st) {
	return ::stat(fspath.asLibcStr().c_str(), &st) == 0;
}

} // namespace fs
#endif
