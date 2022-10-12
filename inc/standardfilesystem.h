#pragma once

#include "filesysteminterface.h"

/**
 * Default implementation for interfacing with the filesystem.
 */
class StandardFileSystem : public FileSystemInterface {
public:
	CStatInfo Stat(const fs::path& path) override;
	bool Exists(const fs::path& path) override;
	void VisitDirectoryContents(const std::function<void(const fs::path& path)>& visitFunc, const fs::path& path) override;
};
