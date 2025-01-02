// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
#pragma once

#include "fs.h"

intptr_t posix_dlopen(const fs::path& origpath, uint32_t RTLD_flags);
bool	 posix_dlclose(intptr_t dlhandle);
void*    posix_dlsym   (intptr_t dlhandle, char const* name);
void*	 posix_dlsymreq(intptr_t dlhandle, char const* name);

#if PLATFORM_MSW || PLATFORM_SCE
#	define RTLD_LAZY    0
#	define RTLD_NOW     1
#	define RTLD_GLOBAL  0
#	define RTLD_LOCAL   2
#	define RTLD_NOLOAD  4
#else
	// assume POSIX compliant OS otherwise
#	include <dlfcn.h>
#endif
