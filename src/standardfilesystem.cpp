// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.

#include "standardfilesystem.h"

CStatInfo StandardFileSystem::Stat(const fs::path& path) {
	return posix_stat(path);
}

bool StandardFileSystem::Exists(const fs::path& path) {
	return fs::exists(path);
}

void StandardFileSystem::VisitDirectoryContents(const std::function<void(const fs::path& path)>& visitFunc, const fs::path& path) {
	fs::directory_iterator(visitFunc, path);
}
