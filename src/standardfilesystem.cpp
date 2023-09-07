#include "standardfilesystem.h"

CStatInfo StandardFileSystem::Stat(const fs::path& path) {
	return posix_stat(path.asLibcStr().c_str());
}

bool StandardFileSystem::Exists(const fs::path& path) {
	return fs::exists(path);
}

void StandardFileSystem::VisitDirectoryContents(const std::function<void(const fs::path& path)>& visitFunc, const fs::path& path) {
	fs::directory_iterator(visitFunc, path);
}
