// Copyright (c) 2021-2025, Implicit Conversions, Inc. Subject to the MIT License. See LICENSE file.
// Contents released under the The MIT License (MIT)

#pragma once

#if __has_include("fi-platform-defines-sce.h")
#	include "fi-platform-defines-sce.h"
#endif

#if __has_include("fi-platform-defines-nx.h")
#	include "fi-platform-defines-nx.h"
#endif

// Default selection of MSW platform is based on the _WIN32 define being provided by compiler.
// This is not a foolproof assumption, but a special environment can specify these defines
// explicitly via makefile.

// PLATFORM_MSW - (M)icro(S)oft (W)indows.  It's easier to grep-search for than 'win' or 'win32'
// This define describes the Windows Platform API and Operating System APIs only, and should be used sparingly.
// Most of the time it's more appropriate to use _MSC_VER for compiler-specific and mscrt features (the CRT
// is part of the compiler, not the OS).

#if !defined(PLATFORM_MSW)
#   if defined(_WIN32)
#       define PLATFORM_MSW     1
#   else
#       define PLATFORM_MSW     0
#   endif
#endif

#if !defined(PLATFORM_POSIX)
#	if PLATFORM_MSW
#		define PLATFORM_POSIX 	0
#	elif __has_include(<unistd.h>)
#		define PLATFORM_POSIX 	1
#	else
#		define PLATFORM_POSIX 	0
#	endif
#endif

#if !defined(PLATFORM_LINUX)
#   ifdef __linux__
#		define PLATFORM_LINUX 	1
#	else
#		define PLATFORM_LINUX 	0
#	endif
#endif

#if !defined(PLATFORM_MSCRT)
#   if defined(_WIN32)
#       define PLATFORM_MSCRT   1
#   else
#       define PLATFORM_MSCRT   0
#   endif
#endif

#define PLATFORM_HAS_GETENV (PLATFORM_MSW || PLATFORM_LINUX || PLATFORM_MAC)

