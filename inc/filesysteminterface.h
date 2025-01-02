// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#include "fs.h"
#include "posix_file.h"

#include <functional>

/**
 * Interface for interfacing with the file system abstractly.
 */
class FileSystemInterface {
public:
	virtual CStatInfo Stat(const fs::path& path) = 0;
	virtual bool Exists(const fs::path& path) = 0;
	virtual void VisitDirectoryContents(const std::function<void(const fs::path& path)>& visitFunc, const fs::path& path) = 0;
};
